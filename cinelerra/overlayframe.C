// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2012 Monty <monty@xiph.org>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "bcsignals.h"
#include "clip.h"
#include "language.h"
#include "mutex.h"
#include "overlayframe.h"
#include "units.h"
#include "vframe.h"

struct transfers OverlayFrame::transfer_names[] =
{
	{ N_("Normal"), TRANSFER_NORMAL },
	{ N_("Replace"), TRANSFER_REPLACE },
	{ N_("Addition"), TRANSFER_ADDITION },
	{ N_("Subtract"), TRANSFER_SUBTRACT },
	{ N_("Multiply"), TRANSFER_MULTIPLY },
	{ N_("Divide"), TRANSFER_DIVIDE },
	{ N_("Max"), TRANSFER_MAX },
	{ 0, 0 }
};

// Easy abstraction of the float and int types.  Most of these are never used
// but GCC expects them.
static int my_abs(int64_t x)
{
	return llabs(x);
}

/*
 * New resampler code; replace the original somehwat blurry engine
 * with a fairly standard kernel resampling core.  This could be used
 * for full affine transformation but only implements scale/translate.
 * Mostly reuses the old blending macro code. 
 *
 * Pixel convention:
 *
 *  1) Pixels are points, not areas or squares.
 *
 *  2) To maintain the usual edge and scaling conventions, pixels are
 *     set inward from the image edge, eg, the left edge of an image is
 *     at pixel location x=-.5, not x=0.  Although pixels are not
 *     squares, the usual way of stating this is 'the pixel is located
 *     at the center of its square'.
 *
 *  3) Because of 1 and 2, we must truncate and weight the kernel
 *     convolution at the edge of the input area.  Otherwise, all
 *     resampled areas would be bordered by a transparency halo. E.g.
 *     in the old engine, upsampling HDV to 1920x1080 results in the
 *     left and right edges being partially transparent and underlying
 *     layers shining through.
 *
 *   4) The contribution of fractional pixels at the edges of input
 *     ranges are weighted according to the fraction.  Note that the
 *     kernel weighting is adjusted, not the opacity.  This is one
 *     exception to 'pixels have no area'.
 *
 *  5) The opacity of fractional pixels at the edges of the output
 *     range is adjusted according to the fraction. This is the other
 *     exception to 'pixels have no area'.
 *
 * Fractional alpha blending has been modified across the board from:
 *    output_alpha = input_alpha > output_alpha ? input_alpha : output_alpha;
 *  to:
 *    output_alpha = output_alpha + ((max - output_alpha) * input_alpha) / max;
 */

#define TRANSFORM_SPP    (4096)    /* number of data pts per unit x in lookup table */
#define INDEX_FRACTION   (8)       /* bits of fraction past TRANSFORM_SPP on kernel
                                      index accumulation */
#define TRANSFORM_MIN    (.5 / TRANSFORM_SPP)

/* Sinc needed for Lanczos kernel */
static double sinc(const double x)
{
	double y = x * M_PI;

	if(fabs(x) < TRANSFORM_MIN)
		return 1.0;

	return sin(y) / y;
}

/*
 * All resampling (except Nearest Neighbor) is performed via
 *   transformed 2D resampling kernels bult from 1D lookups.
 */
OverlayKernel::OverlayKernel(int interpolation_type)
{
	int i;
	this->type = interpolation_type;

	switch(interpolation_type)
	{
	case BILINEAR:
		width = 1;
		lookup = new double[(n = TRANSFORM_SPP) + 1];
		for (i = 0; i <= TRANSFORM_SPP; i++)
			lookup[i] = (double)(TRANSFORM_SPP - i) / TRANSFORM_SPP;
		break;

	/* Use a Catmull-Rom filter (not b-spline) */
	case BICUBIC:
		width = 2;
		lookup = new double[(n = 2 * TRANSFORM_SPP) + 1];
		for(i = 0; i <= TRANSFORM_SPP; i++)
		{
			double x = i / (double)TRANSFORM_SPP;
			lookup[i] = 1.0 - 2.5* x * x + 1.5 * x * x * x;
		}
		for(; i <= 2 * TRANSFORM_SPP; i++)
		{
			double x = i / (double)TRANSFORM_SPP;
			lookup[i] = 2.0 - 4.0 * x  + 2.5 * x * x - .5* x * x * x;
		}
		break;

	case LANCZOS:
		width = 3;
		lookup = new double[(n = 3 * TRANSFORM_SPP) + 1];
		for (i = 0; i <= 3 * TRANSFORM_SPP; i++)
			lookup[i] = sinc((double) i / TRANSFORM_SPP) * sinc((double) i / TRANSFORM_SPP / 3.0);
		break;

	default:
		width = 0;
		lookup = 0;
		n = 0;
		break;
	}
}

OverlayKernel::~OverlayKernel()
{
	delete[] lookup;
}

OverlayFrame::OverlayFrame(int cpus)
{
	direct_engine = 0;
	nn_engine = 0;
	sample_engine = 0;
	temp_frame = 0;
	memset(kernel, 0, sizeof(kernel));
	this->cpus = cpus;
}

OverlayFrame::~OverlayFrame()
{
	delete temp_frame;

	delete direct_engine;
	delete nn_engine;
	delete sample_engine;

	delete kernel[NEAREST_NEIGHBOR];
	delete kernel[BILINEAR];
	delete kernel[BICUBIC];
	delete kernel[LANCZOS];
}

static double epsilon_snap(double f)
{
	return rint(f * 1024) / 1024.;
}

int OverlayFrame::overlay(VFrame *output,
	VFrame *input,
	double in_x1,
	double in_y1,
	double in_x2,
	double in_y2,
	double out_x1,
	double out_y1,
	double out_x2,
	double out_y2,
	double alpha,
	int mode,
	int interpolation_type)
{
	in_x1 = epsilon_snap(in_x1);
	in_x2 = epsilon_snap(in_x2);
	in_y1 = epsilon_snap(in_y1);
	in_y2 = epsilon_snap(in_y2);
	out_x1 = epsilon_snap(out_x1);
	out_x2 = epsilon_snap(out_x2);
	out_y1 = epsilon_snap(out_y1);
	out_y2 = epsilon_snap(out_y2);

	if(isnan(in_x1) ||
		isnan(in_x2) ||
		isnan(in_y1) ||
		isnan(in_y2) ||
		isnan(out_x1) ||
		isnan(out_x2) ||
		isnan(out_y1) ||
		isnan(out_y2)) return 1;

	if(in_x2 <= in_x1 || in_y2 <= in_y1 ||
			out_x2 <= out_x1 || out_y2 <= out_y1)
		return 1;

	double xscale = (out_x2 - out_x1) / (in_x2 - in_x1);
	double yscale = (out_y2 - out_y1) / (in_y2 - in_y1);

// Limit values
	if(in_x1 < 0)
	{
		out_x1 += -in_x1 * xscale;
		in_x1 = 0;
	}
	else
	if(in_x1 >= input->get_w())
	{
		out_x1 -= (in_x1 - input->get_w()) * xscale;
		in_x1 = input->get_w();
	}

	if(in_y1 < 0)
	{
		out_y1 += -in_y1 * yscale;
		in_y1 = 0;
	}
	else
	if(in_y1 >= input->get_h())
	{
		out_y1 -= (in_y1 - input->get_h()) * yscale;
		in_y1 = input->get_h();
	}

	if(in_x2 < 0)
	{
		out_x2 += -in_x2 * xscale;
		in_x2 = 0;
	}
	else
	if(in_x2 >= input->get_w())
	{
		out_x2 -= (in_x2 - input->get_w()) * xscale;
		in_x2 = input->get_w();
	}

	if(in_y2 < 0)
	{
		out_y2 += -in_y2 * yscale;
		in_y2 = 0;
	}
	else
	if(in_y2 >= input->get_h())
	{
		out_y2 -= (in_y2 - input->get_h()) * yscale;
		in_y2 = input->get_h();
	}

	if(out_x1 < 0)
	{
		in_x1 += -out_x1 / xscale;
		out_x1 = 0;
	}
	else
	if(out_x1 >= output->get_w())
	{
		in_x1 -= (out_x1 - output->get_w()) / xscale;
		out_x1 = output->get_w();
	}

	if(out_y1 < 0)
	{
		in_y1 += -out_y1 / yscale;
		out_y1 = 0;
	}
	else
	if(out_y1 >= output->get_h())
	{
		in_y1 -= (out_y1 - output->get_h()) / yscale;
		out_y1 = output->get_h();
	}

	if(out_x2 < 0)
	{
		in_x2 += -out_x2 / xscale;
		out_x2 = 0;
	}
	else
	if(out_x2 >= output->get_w())
	{
		in_x2 -= (out_x2 - output->get_w()) / xscale;
		out_x2 = output->get_w();
	}
	if(out_y2 < 0)
	{
		in_y2 += -out_y2 / yscale;
		out_y2 = 0;
	}
	else
	if(out_y2 >= output->get_h())
	{
		in_y2 -= (out_y2 - output->get_h()) / yscale;
		out_y2 = output->get_h();
	}

	if(in_x2 <= in_x1 || in_y2 <= in_y1 ||
			out_x2 <= out_x1 || out_y2 <= out_y1)
		return 0;

	/* don't interpolate integer translations, or scale no-ops */
	if(xscale == 1. && yscale == 1. &&
		(int)in_x1 == in_x1 && (int)in_x2 == in_x2 &&
		(int)in_y1 == in_y1 && (int)in_y2 == in_y2 &&
		(int)out_x1 == out_x1 && (int)out_x2 == out_x2 &&
		(int)out_y1 == out_y1 && (int)out_y2 == out_y2)
	{
		if(!direct_engine)
			direct_engine = new DirectEngine(cpus);

		direct_engine->output = output;
		direct_engine->input = input;
		direct_engine->in_x1 = in_x1;
		direct_engine->in_y1 = in_y1;
		direct_engine->out_x1 = out_x1;
		direct_engine->out_x2 = out_x2;
		direct_engine->out_y1 = out_y1;
		direct_engine->out_y2 = out_y2;
		direct_engine->alpha = alpha;
		direct_engine->mode = mode;
		direct_engine->process_packages();
	}
	else if(interpolation_type == NEAREST_NEIGHBOR)
	{
		if(!nn_engine)
			nn_engine = new NNEngine(cpus);

		nn_engine->output = output;
		nn_engine->input = input;
		nn_engine->in_x1 = in_x1;
		nn_engine->in_x2 = in_x2;
		nn_engine->in_y1 = in_y1;
		nn_engine->in_y2 = in_y2;
		nn_engine->out_x1 = out_x1;
		nn_engine->out_x2 = out_x2;
		nn_engine->out_y1 = out_y1;
		nn_engine->out_y2 = out_y2;
		nn_engine->alpha = alpha;
		nn_engine->mode = mode;
		nn_engine->process_packages();
	}
	else
	{
		int xtype = BILINEAR;
		int ytype = BILINEAR;

		switch(interpolation_type)
		{
		case CUBIC_CUBIC: // Bicubic enlargement and reduction
			xtype = BICUBIC;
			ytype = BICUBIC;
			break;
		case CUBIC_LINEAR: // Bicubic enlargement and bilinear reduction
			xtype = xscale > 1. ? BICUBIC : BILINEAR;
			ytype = yscale > 1. ? BICUBIC : BILINEAR;
			break;
		case LINEAR_LINEAR: // Bilinear enlargement and bilinear reduction
			xtype = BILINEAR;
			ytype = BILINEAR;
			break;
		case LANCZOS_LANCZOS: // Because we can
			xtype = LANCZOS;
			ytype = LANCZOS;
			break;
		}

		if(xscale == 1. && (int)in_x1 == in_x1 && (int)in_x2 == in_x2 &&
				(int)out_x1 == out_x1 && (int)out_x2 == out_x2)
			xtype = DIRECT_COPY;

		if(yscale == 1. && (int)in_y1 == in_y1 && (int)in_y2 == in_y2 &&
				(int)out_y1 == out_y1 && (int)out_y2 == out_y2)
			ytype = DIRECT_COPY;

		if(!kernel[xtype])
			kernel[xtype] = new OverlayKernel(xtype);
		if(!kernel[ytype])
			kernel[ytype] = new OverlayKernel(ytype);

// horizontal and vertical are separately resampled.  First we
// resample the input along X into a transposed, temporary frame,
// then resample/transpose the temporary space along X into the
// output.  Fractional pixels along the edge are handled in the X
// direction of each step

		// resampled dimension matches the transposed output space
		double temp_y1 = out_x1 - floor(out_x1);
		double temp_y2 = temp_y1 + (out_x2 - out_x1);
		int temp_h = ceil(temp_y2);

		// non-resampled dimension merely cropped
		double temp_x1 = in_y1 - floor(in_y1);
		double temp_x2 = temp_x1 + (in_y2-in_y1);
		int temp_w = ceil(temp_x2);

		if(temp_frame && (temp_frame->get_w() != temp_w ||
				temp_frame->get_h() != temp_h))
		{
			delete temp_frame;
			temp_frame = 0;
		}

		if(!temp_frame)
		{
			temp_frame = new VFrame(0,temp_w,temp_h,
				input->get_color_model(), -1);
		}

		temp_frame->clear_frame();

		if(!sample_engine)
			sample_engine = new SampleEngine(cpus);

		sample_engine->output = temp_frame;
		sample_engine->input = input;
		sample_engine->kernel = kernel[xtype];
		sample_engine->col_out1 = 0;
		sample_engine->col_out2 = temp_w;
		sample_engine->row_in = floor(in_y1);

		sample_engine->in1 = in_x1;
		sample_engine->in2 = in_x2;
		sample_engine->out1 = temp_y1;
		sample_engine->out2 = temp_y2;
		sample_engine->alpha = 1.;
		sample_engine->mode = TRANSFER_REPLACE;
		sample_engine->process_packages();

		sample_engine->output = output;
		sample_engine->input = temp_frame;
		sample_engine->kernel = kernel[ytype];
		sample_engine->col_out1 = floor(out_x1);
		sample_engine->col_out2 = ceil(out_x2);
		sample_engine->row_in = 0;

		sample_engine->in1 = temp_x1;
		sample_engine->in2 = temp_x2;
		sample_engine->out1 = out_y1;
		sample_engine->out2 = out_y2;
		sample_engine->alpha = alpha;
		sample_engine->mode = mode;
		sample_engine->process_packages();
	}
	return 0;
}

const char *OverlayFrame::transfer_name(int transfer)
{
	for(int i = 0; transfer_names[i].text; i++)
		if(transfer_names[i].value == transfer)
			return transfer_names[i].text;

	return transfer_names[0].text;
}

int OverlayFrame::transfer_mode(const char *text)
{
	for(int i = 0; transfer_names[i].text; i++)
		if(strcmp(_(transfer_names[i].text), text) == 0)
			return transfer_names[i].value;

	return TRANSFER_NORMAL;
}

// Verification:

// (255 * 255 + 0 * 0) / 255 = 255
// (255 * 127 + 255 * (255 - 127)) / 255 = 255

// (65535 * 65535 + 0 * 0) / 65535 = 65535
// (65535 * 32767 + 65535 * (65535 - 32767)) / 65535 = 65535


// Permutation 4 U
// Blending equations are drastically different for 3 and 4 components
#define BLEND(chroma_offset) \
{ \
	int64_t r, g, b, a; \
	int64_t g1, g2, b1, b2; \
	int64_t pixel_opacity, pixel_transparency; \
	int64_t output1; \
	int64_t output2; \
	int64_t output3; \
	int64_t output4; \
 \
	if(!chroma_offset) \
	{ \
		output1 = out_row[0]; \
		output2 = out_row[1]; \
		output3 = out_row[2]; \
		output4 = out_row[3]; \
	} \
	else \
	{ \
		output4 = out_row[0]; \
		output1 = out_row[1]; \
		output2 = out_row[2]; \
		output3 = out_row[3]; \
	} \
 \
	pixel_opacity = opacity * input4; \
	pixel_transparency = (int64_t)0xffff * 0xffff - pixel_opacity; \
	a = output4 + ((0xffff - output4) * input4) / 0xffff; \
 \
	switch(mode) \
	{ \
	case TRANSFER_DIVIDE: \
		r = input1 ? ((output1 * 0xffff) / input1) : 0xffff; \
		if(chroma_offset) \
		{ \
			g = my_abs(input2 - chroma_offset) > my_abs(output2 - chroma_offset) ? input2 : output2; \
			b = my_abs(input3 - chroma_offset) > my_abs(output3 - chroma_offset) ? input3 : output3; \
		} \
		else \
		{ \
			g = input2 ? output2 * 0xffff / input2 : 0xffff; \
			b = input3 ? output3 * 0xffff / input3 : 0xffff; \
		} \
		r = (r * pixel_opacity + output1 * pixel_transparency) / 0xffff / 0xffff; \
		g = (g * pixel_opacity + output2 * pixel_transparency) / 0xffff / 0xffff; \
		b = (b * pixel_opacity + output3 * pixel_transparency) / 0xffff / 0xffff; \
		break; \
	case TRANSFER_MULTIPLY: \
		r = (input1 * output1) / 0xffff; \
		if(chroma_offset) \
		{ \
			g = my_abs(input2 - chroma_offset) > my_abs(output2 - chroma_offset) ? input2 : output2; \
			b = my_abs(input3 - chroma_offset) > my_abs(output3 - chroma_offset) ? input3 : output3; \
		} \
		else \
		{ \
			g = input2 * output2 / 0xffff; \
			b = input3 * output3 / 0xffff; \
		} \
		r = (r * pixel_opacity + output1 * pixel_transparency) / 0xffff / 0xffff; \
		g = (g * pixel_opacity + output2 * pixel_transparency) / 0xffff / 0xffff; \
		b = (b * pixel_opacity + output3 * pixel_transparency) / 0xffff / 0xffff; \
		break; \
	case TRANSFER_SUBTRACT: \
		r = output1 - input1; \
		g = (output2 - chroma_offset) - \
			(input2 - chroma_offset) + \
			chroma_offset; \
		b = (output3 - chroma_offset) - \
			(input3 - chroma_offset) + \
			chroma_offset; \
		if(r < 0) r = 0; \
		if(g < 0) g = 0; \
		if(b < 0) b = 0; \
		r = (r * pixel_opacity + output1 * pixel_transparency) / 0xffff / 0xffff; \
		g = (g * pixel_opacity + output2 * pixel_transparency) / 0xffff / 0xffff; \
		b = (b * pixel_opacity + output3 * pixel_transparency) / 0xffff / 0xffff; \
		break; \
	case TRANSFER_ADDITION: \
		r = input1 + output1; \
		g = (input2 - chroma_offset) + \
			(output2 - chroma_offset) + \
			chroma_offset; \
		b = (input3 - chroma_offset) + \
			(output3 - chroma_offset) + \
			chroma_offset; \
		r = (r * pixel_opacity + output1 * pixel_transparency) / 0xffff / 0xffff; \
		g = (g * pixel_opacity + output2 * pixel_transparency) / 0xffff / 0xffff; \
		b = (b * pixel_opacity + output3 * pixel_transparency) / 0xffff / 0xffff; \
		break; \
	case TRANSFER_MAX: \
		r = MAX(input1, output1); \
		g1 = input2 - chroma_offset; \
		if(g1 < 0) \
			g1 = -g1; \
		g2 = output2 - chroma_offset; \
		if(g2 < 0) \
			g2 = -g2; \
		if(g1 > g2) \
			g = input2; \
		else \
			g = output2; \
		b1 = input3 - chroma_offset; \
		if(b1 < 0) \
			b1 = -b1; \
		b2 = output3 - chroma_offset; \
		if(b2 < 0) \
			b2 = -b2; \
		if(b1 > b2) \
			b = input3; \
		else \
			b = output3; \
		r = (r * pixel_opacity + output1 * pixel_transparency) / 0xffff / 0xffff; \
		g = (g * pixel_opacity + output2 * pixel_transparency) / 0xffff / 0xffff; \
		b = (b * pixel_opacity + output3 * pixel_transparency) / 0xffff / 0xffff; \
		break; \
	case TRANSFER_REPLACE: \
		r = input1; \
		g = input2; \
		b = input3; \
		a = input4; \
		break; \
	case TRANSFER_NORMAL: \
		r = (input1 * pixel_opacity + \
			output1 * pixel_transparency) / 0xffff / 0xffff; \
		g = ((input2 - chroma_offset) * pixel_opacity + \
			(output2 - chroma_offset) * pixel_transparency) / \
			0xffff / 0xffff + chroma_offset; \
		b = ((input3 - chroma_offset) * pixel_opacity + \
			(output3 - chroma_offset) * pixel_transparency) / \
			0xffff / 0xffff + chroma_offset; \
		break; \
	} \
 \
	if(!chroma_offset) \
	{ \
		out_row[0] = CLIP(r, 0, 0xffff); \
		out_row[1] = CLIP(g, 0, 0xffff); \
		out_row[2] = CLIP(b, 0, 0xffff); \
		out_row[3] = CLIP(a, 0, 0xffff); \
	} \
	else \
	{ \
		out_row[0] = CLIP(a, 0, 0xffff); \
		out_row[1] = CLIP(r, 0, 0xffff); \
		out_row[2] = CLIP(g, 0, 0xffff); \
		out_row[3] = CLIP(b, 0, 0xffff); \
	} \
}


/* Direct translate / blend **********************************************/

DirectPackage::DirectPackage()
{
}

DirectUnit::DirectUnit(DirectEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

DirectUnit::~DirectUnit()
{
}

void DirectUnit::process_package(LoadPackage *package)
{
	DirectPackage *pkg = (DirectPackage*)package;

	VFrame *output = engine->output;
	VFrame *input = engine->input;
	double alpha = engine->alpha;
	int mode = engine->mode;

	int ix = engine->in_x1;
	int ox = engine->out_x1;
	int ow = engine->out_x2 - ox;
	int iy = engine->in_y1 - engine->out_y1;

	int64_t opacity = alpha * 0xffff + 0.5;
	int64_t transparency = 0xffff - opacity;
	int64_t max_squared = ((uint64_t)0xffff) * 0xffff;

	if(mode == TRANSFER_REPLACE)
	{
		switch(input->get_color_model())
		{
		case BC_RGBA16161616:
		case BC_AYUV16161616:
			int line_len = ow * sizeof(uint16_t) * 4;

			ix *= 4;
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				memcpy((uint16_t*)output->get_row_ptr(i) + ox,
					(uint16_t*)input->get_row_ptr(i + iy) + ix, line_len);
			}
			break;
		}
	}
	else if(mode == TRANSFER_NORMAL)
	{
		switch(input->get_color_model())
		{
		case BC_RGBA16161616:
			ix *= 4;
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(i + iy) + ix;
				uint16_t *out_row = (uint16_t*)output->get_row_ptr(i) + ox;

				for(int j = 0; j < ow; j++)
				{
					uint64_t pixel_opacity, pixel_transparency;

					pixel_opacity = opacity * in_row[3];
					pixel_transparency = max_squared - pixel_opacity;

					out_row[0] = ((int64_t)in_row[0] * pixel_opacity +
						(int64_t)out_row[0] * pixel_transparency) /
						0xffff / 0xffff;
					out_row[1] = ((int64_t)in_row[1] * pixel_opacity +
						(int64_t)out_row[1] * pixel_transparency) /
						0xffff / 0xffff;
					out_row[2] = ((int64_t)in_row[2] * pixel_opacity +
						(uint64_t)out_row[2] * pixel_transparency) /
						0xffff / 0xffff;
					out_row[3] += ((int64_t)(0xffff - out_row[3]) *
						in_row[3]) / 0xffff;

					in_row += 4;
					out_row += 4;
				}
			}
			break;
		case BC_AYUV16161616:
			ix *= 4;
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(i + iy) + ix;
				uint16_t *out_row = (uint16_t*)output->get_row_ptr(i) + ox;

				for(int j = 0; j < ow; j++)
				{
					int64_t pixel_opacity, pixel_transparency;

					pixel_opacity = opacity * in_row[0];
					pixel_transparency = max_squared - pixel_opacity;

					out_row[0] += ((int64_t)(0xffff - out_row[0]) *
						in_row[0]) / 0xffff;
					out_row[1] = ((int64_t)in_row[1] * pixel_opacity +
						(int64_t)out_row[1] * pixel_transparency) /
						0xffff / 0xffff;
					out_row[2] = (((int64_t)in_row[2] - 0x8000) * pixel_opacity +
						((int64_t)out_row[2] - 0x8000) * pixel_transparency) /
						0xffff / 0xffff + 0x8000;
					out_row[3] = (((int64_t)in_row[3] - 0x8000) * pixel_opacity +
						((int64_t)out_row[3] - 0x8000) * pixel_transparency) /
						0xffff / 0xffff + 0x8000;

						in_row += 4;
						out_row += 4;
				}
			}
			break;
		}
	}
	else
		switch(input->get_color_model())
		{
		case BC_RGBA16161616:
			ix *= 4;
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(i + iy) + ix;
				uint16_t *out_row = (uint16_t*)output->get_row_ptr(i) + ox;

				for(int j = 0; j < ow; j++)
				{
					int64_t input1, input2, input3, input4;
					input1 = in_row[0];
					input2 = in_row[1];
					input3 = in_row[2];
					input4 = in_row[3];
					BLEND(0);

					in_row += 4;
					out_row += 4;
				}
			}
			break;
		case BC_AYUV16161616:
			ix *= 4;
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(i + iy) + ix;
				uint16_t* out_row = (uint16_t*)output->get_row_ptr(i) + ox;

				for(int j = 0; j < ow; j++)
				{
					int64_t input1, input2, input3, input4;

					input4 = in_row[0];
					input1 = in_row[1];
					input2 = in_row[2];
					input3 = in_row[3];
					BLEND(0x8000);

					in_row += 4;
					out_row += 4;
				}
			}
			break;
		}
}

DirectEngine::DirectEngine(int cpus)
 : LoadServer(cpus, cpus)
{
}

DirectEngine::~DirectEngine()
{
}

void DirectEngine::init_packages()
{
	int out_h;

	if(in_x1 < 0)
	{
		out_x1 -= in_x1;
		in_x1 = 0;
	}
	if(in_y1 < 0)
	{
		out_y1 -= in_y1;
		in_y1 = 0;
	}
	if(out_x1 < 0)
	{
		in_x1 -= out_x1;
		out_x1 = 0;
	}
	if(out_y1 < 0)
	{
		in_y1 -= out_y1;
		out_y1 = 0;
	}

	if(out_x2 > output->get_w())
	{
		out_x2 = output->get_w();
	}
	if(out_y2 > output->get_h())
	{
		out_y2 = output->get_h();
	}

	out_h = out_y2-out_y1;

	for(int i = 0; i < get_total_packages(); i++)
	{
		DirectPackage *package = (DirectPackage*)get_package(i);

		package->out_row1 = out_h * i / get_total_packages() + out_y1;
		package->out_row2 = out_h * (i + 1) / get_total_packages() + out_y1;
	}
}

LoadClient* DirectEngine::new_client()
{
	return new DirectUnit(this);
}

LoadPackage* DirectEngine::new_package()
{
	return new DirectPackage;
}

/* Nearest Neighbor scale / translate / blend ********************/
NNPackage::NNPackage()
{
}

NNUnit::NNUnit(NNEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

NNUnit::~NNUnit()
{
}

void NNUnit::process_package(LoadPackage *package)
{
	NNPackage *pkg = (NNPackage*)package;
	VFrame *output = engine->output;
	VFrame *input = engine->input;
	double alpha = engine->alpha;
	int mode = engine->mode;

	int ox = engine->out_x1i;
	int ow = engine->out_x2i - ox;
	int *ly = engine->in_lookup_y + pkg->out_row1;

	int64_t opacity = alpha * 0xffff + 0.5;
	int64_t transparency = 0xffff - opacity;
	int64_t max_squared = (uint64_t)0xffff * 0xffff;

	if(mode == TRANSFER_REPLACE)
	{
		switch(input->get_color_model())
		{
		case BC_RGBA16161616:
		case BC_AYUV16161616:
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				int *lx = engine->in_lookup_x;
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(*ly++);
				uint16_t* out_row = (uint16_t*)output->get_row_ptr(i) + ox;

				for(int j = 0; j < ow; j++)
				{
					in_row += *lx++;
					*out_row++ = in_row[0];
					*out_row++ = in_row[1];
					*out_row++ = in_row[2];
					*out_row++ = in_row[3];
				}
			}
			break;
		}
	}
	else if(mode == TRANSFER_NORMAL)
	{
		switch(input->get_color_model())
		{
		case BC_RGBA16161616:
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				int *lx = engine->in_lookup_x;
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(*ly++);
				uint16_t *out_row = (uint16_t*)output->get_row_ptr(i) + ox;

				for(int j = 0; j < ow; j++)
				{
					int64_t pixel_o, pixel_t, r, g, b;

					in_row += *lx++;
					pixel_o = opacity * in_row[3];
					pixel_t = (int64_t)max_squared - pixel_o;

					out_row[0] = ((int64_t)in_row[0] * pixel_o +
						(int64_t)out_row[0] * pixel_t) /
						0xffff / 0xffff;
					out_row[1] = ((int64_t)in_row[1] * pixel_o +
						(int64_t)out_row[1] * pixel_t) /
						0xffff / 0xffff;
					out_row[2] = ((int64_t)in_row[2] * pixel_o +
						(int64_t)out_row[2] * pixel_t) /
						0xffff / 0xffff;
					out_row[3] += ((int64_t)(0xffff - out_row[3]) *
						in_row[3]) / 0xffff;

					out_row += 4;
				}
			}
			break;
		case BC_AYUV16161616:
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				int *lx = engine->in_lookup_x;
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(*ly++);
				uint16_t *out_row = (uint16_t*)output->get_row_ptr(i) + ox;

				for(int j = 0; j < ow; j++)
				{
					int64_t pixel_o, pixel_t, r, g, b;

					in_row += *lx++;
					pixel_o = opacity * in_row[0];
					pixel_t = max_squared - pixel_o;
					out_row[0] += ((int64_t)(0xffff - out_row[0]) *
						in_row[0]) / 0xffff;
					out_row[1] = ((int64_t)in_row[1] * pixel_o +
						(int64_t)out_row[1] * pixel_t) / 0xffff / 0xffff;
					out_row[2] = (((int64_t)in_row[2] - 0x8000) * pixel_o +
						((int64_t)out_row[2] - 0x8000) * pixel_t) /
						0xffff / 0xffff + 0x8000;
					out_row[3] = (((int64_t)in_row[3] - 0x8000) * pixel_o +
						((int64_t)out_row[3] - 0x8000) * pixel_t) /
						0xffff / 0xffff + 0x8000;

					out_row += 4;
				}
			}
			break;
		}
	}
	else
		switch(input->get_color_model())
		{
		case BC_RGBA16161616:
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				int *lx = engine->in_lookup_x;
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(*ly++);
				uint16_t *out_row = (uint16_t*)output->get_row_ptr(i) + ox;

				for(int j = 0; j < ow; j++)
				{
					int64_t input1, input2, input3, input4;
					in_row += *lx++;

					input1 = in_row[0];
					input2 = in_row[1];
					input3 = in_row[2];
					input4 = in_row[3];
					BLEND(0x8000);
					out_row += 4;
				}
			}
			break;
		case BC_AYUV16161616:
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				int *lx = engine->in_lookup_x;
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(*ly++);
				uint16_t* out_row = (uint16_t*)output->get_row_ptr(i) + ox;

				for(int j = 0; j < ow; j++)
				{
					int64_t input1, input2, input3, input4;

					in_row += *lx++;
					input1 = in_row[1];
					input2 = in_row[2];
					input3 = in_row[3];
					input4 = in_row[0];
					BLEND(0x8000);
					out_row += 4;
				}
			}
			break;
		}
}

NNEngine::NNEngine(int cpus)
 : LoadServer(cpus, cpus)
{
	in_lookup_x = 0;
	in_lookup_y = 0;
}

NNEngine::~NNEngine()
{
	delete[] in_lookup_x;
	delete[] in_lookup_y;
}

void NNEngine::init_packages()
{
	int in_w = input->get_w();
	int in_h = input->get_h();
	int out_w = output->get_w();
	int out_h = output->get_h();

	double in_subw = in_x2 - in_x1;
	double in_subh = in_y2 - in_y1;
	double out_subw = out_x2 - out_x1;
	double out_subh = out_y2 - out_y1;
	int first, last, count, i;

	out_x1i = rint(out_x1);
	out_x2i = rint(out_x2);
	if(out_x1i < 0)
		out_x1i = 0;
	if(out_x1i > out_w)
		out_x1i = out_w;
	if(out_x2i < 0)
		out_x2i = 0;
	if(out_x2i > out_w)
		out_x2i = out_w;

	if(in_lookup_x)
		delete[] in_lookup_x;
	in_lookup_x = new int[out_x2i - out_x1i];

	if(in_lookup_y)
		delete[] in_lookup_y;
	in_lookup_y = new int[out_h];

	first = count = 0;

	for(i = out_x1i; i < out_x2i; i++)
	{
		int in = (i - out_x1 + .5) * in_subw / out_subw + in_x1;
		if(in < in_x1)
			in = in_x1;
		if(in > in_x2)
			in = in_x2;

		if(in >= 0 && in < in_w && in >= in_x1 && i >= 0 && i < out_w)
		{
			if(count == 0)
			{
				first = i;
				in_lookup_x[0] = in * 4;
			}
			else
			{
				in_lookup_x[count] = (in-last) * 4;
			}
			last = in;
			count++;
		}
		else
			if(count)
				break;
	}
	out_x1i = first;
	out_x2i = first + count;
	first = count = 0;

	for(i = out_y1; i < out_y2; i++)
	{
		int in = (i - out_y1 + .5) * in_subh / out_subh + in_y1;
		if(in < in_y1)
			in = in_y1;
		if(in > in_y2)
			in = in_y2;
		if(in >= 0 && in < in_h && i >= 0 && i < out_h)
		{
			if(count == 0)
				first = i;
			in_lookup_y[i] = in;
			count++;
		}
		else
			if(count)
				break;
	}
	out_y1 = first;
	out_y2 = first + count;

	for(int i = 0; i < get_total_packages(); i++)
	{
		NNPackage *package = (NNPackage*)get_package(i);
		package->out_row1 = count * i / get_total_packages() + out_y1;
		package->out_row2 = count * (i + 1) / get_total_packages() + out_y1;
	}
}

LoadClient* NNEngine::new_client()
{
	return new NNUnit(this);
}

LoadPackage* NNEngine::new_package()
{
	return new NNPackage;
}

/* Fully resampled scale / translate / blend ******************************/
SamplePackage::SamplePackage()
{
}

SampleUnit::SampleUnit(SampleEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

SampleUnit::~SampleUnit()
{
}

void SampleUnit::process_package(LoadPackage *package)
{
	SamplePackage *pkg = (SamplePackage*)package;

	double i1  = engine->in1;
	double i2  = engine->in2;
	double o1  = engine->out1;
	double o2  = engine->out2;

	if(i2 - i1 <= 0 || o2 - o1 <= 0)
		return;

	VFrame *voutput = engine->output;
	VFrame *vinput = engine->input;
	double alpha = engine->alpha;
	int mode = engine->mode;

	int   iw  = vinput->get_w();
	int   i1i = floor(i1);
	int   i2i = ceil(i2);
	double i1f = 1.0 - i1 + i1i;
	double i2f = 1.0 - i2i + i2;

	int   o1i = floor(o1);
	int   o2i = ceil(o2);
	double o1f = 1.f - o1 + o1i;
	double o2f = 1.f - o2i + o2;
	int   oh  = o2i - o1i;

	double *k  = engine->kernel->lookup;
	double kw  = engine->kernel->width;
	int   kn  = engine->kernel->n;
	int   kd = engine->kd;

	int *lookup_sx0 = engine->lookup_sx0;
	int *lookup_sx1 = engine->lookup_sx1;
	int *lookup_sk = engine->lookup_sk;
	double *lookup_wacc = engine->lookup_wacc;

	double temp[oh * 4];
	int64_t opacity = alpha * 0xffff + .5;
	int64_t transparency = 0xffff - opacity;

	// resample into a temporary row vector, then blend
	switch(vinput->get_color_model())
	{
	case BC_RGBA16161616:

		for(int i = pkg->out_col1; i < pkg->out_col2; i++)
		{
			if(opacity == 0)
			{
				// don't bother resampling if the frame is invisible
				int64_t input1 = 0;
				int64_t input2 = 0;
				int64_t input3 = 0;
				int64_t input4 = 0;

				for(int j = 0; j < oh; j++)
				{
					uint16_t *out_row = (uint16_t*)voutput->get_row_ptr(o1i + j) + i * 4;
					BLEND(0);
				}
			}
			else
			{
				uint16_t *input = (uint16_t*)vinput->get_row_ptr(i -
					engine->col_out1 + engine->row_in);
				double *tempp = temp;

				if(!k)
				{
					// direct copy case
					uint16_t *ip = input + i1i * 4;

					for(int j = 0; j < oh; j++)
					{
						*tempp++ = *ip++;
						*tempp++ = *ip++;
						*tempp++ = *ip++;
						*tempp++ = *ip++;
					}
				}
				else
				{
					// resample
					for(int j = 0; j < oh; j++)
					{
						double racc=0., gacc=0., bacc=0., aacc=0.;
						int ki = lookup_sk[j];
						int x = lookup_sx0[j];
						uint16_t *ip = input + x * 4;
						double wacc = 0;
						double awacc = 0;

						while(x++ < lookup_sx1[j])
						{
							double kv = k[abs(ki >> INDEX_FRACTION)];

							// handle fractional pixels on edges of input
							if(x == i1i)
								kv *= i1f;
							if(x + 1 == i2i)
								kv *= i2f;

							double a = ip[3] * kv;
							awacc += kv;
							kv = a;
							wacc += kv;

							racc += kv * *ip++;
							gacc += kv * *ip++;
							bacc += kv * *ip++;
							aacc += kv; ip++;
							ki += kd;

						}
						if(wacc > 0)
							wacc = 1. / wacc;
						if(awacc > 0)
							awacc = 1. / awacc;
						*tempp++ = racc * wacc;
						*tempp++ = gacc * wacc;
						*tempp++ = bacc * wacc;
						*tempp++ = aacc * awacc;
					}
				}

				// handle fractional pixels on edges of output
				temp[0] *= o1f;
				temp[1] *= o1f;
				temp[2] *= o1f;
				temp[3] *= o1f;
				temp[oh * 4 - 4] *= o2f;
				temp[oh * 4 - 3] *= o2f;
				temp[oh * 4 - 2] *= o2f;
				temp[oh * 4 - 1] *= o2f;
				tempp = temp;

				// blend output
				for(int j = 0; j < oh; j++)
				{
					uint16_t *out_row = (uint16_t*)voutput->get_row_ptr(o1i + j) + i * 4;
					int64_t input1 = *tempp++ + 0.5;
					int64_t input2 = *tempp++ + 0.5;
					int64_t input3 = *tempp++ + 0.5;
					int64_t input4 = *tempp++ + 0.5;
					BLEND(0);
				}
			}
		}
		break;
	case BC_AYUV16161616:
		for(int i = pkg->out_col1; i < pkg->out_col2; i++)
		{
			if(opacity == 0)
			{

				// don't bother resampling if the frame is invisible
				int64_t input1 = 0;
				int64_t input2 = 0x8000;
				int64_t input3 = 0x8000;
				int64_t input4 = 0;

				for(int j = 0; j < oh; j++)
				{
					uint16_t *out_row = (uint16_t*)voutput->get_row_ptr(o1i + j) + i * 4;
					BLEND(0x8000);
				}
			}
			else
			{
				uint16_t *input = (uint16_t*)vinput->get_row_ptr(i - engine->col_out1 + engine->row_in);
				double *tempp = temp;

				if(!k)
				{
					// direct copy case
					uint16_t *ip = input + i1i * 4;

					for(int j = 0; j < oh; j++)
					{
						*tempp++ = *ip++;
						*tempp++ = *ip++;
						*tempp++ = (*ip++) - 0x8000;
						*tempp++ = (*ip++) - 0x8000;
					}
				}
				else
				{
					// resample
					for(int j = 0; j < oh; j++)
					{
						double racc=0, gacc=0, bacc=0, aacc=0;
						int ki = lookup_sk[j];
						int x = lookup_sx0[j];
						uint16_t *ip = input + x * 4;
						double wacc = 0;
						double awacc = 0;

						while(x++ < lookup_sx1[j])
						{
							double kv = k[abs(ki >> INDEX_FRACTION)];

							// handle fractional pixels on edges of input
							if(x == i1i)
								kv *= i1f;
							if(x + 1 == i2i)
								kv *= i2f;

							double a = ip[0] * kv;
							awacc += kv;
							kv = a;
							wacc += kv;

							aacc += kv; ip++;
							racc += kv * *ip++;
							gacc += kv * (*ip++ - 0x8000);
							bacc += kv * (*ip++ - 0x8000);
							ki += kd;
						}

						if(wacc > 0)
							wacc = 1. / wacc;
						if(awacc > 0)
							awacc = 1. / awacc;
						*tempp++ = aacc * awacc;
						*tempp++ = racc * wacc;
						*tempp++ = gacc * wacc;
						*tempp++ = bacc * wacc;
					}
				}

				// handle fractional pixels on edges of output
				temp[0] *= o1f;
				temp[1] *= o1f;
				temp[2] *= o1f;
				temp[3] *= o1f;
				temp[oh * 4 - 4] *= o2f;
				temp[oh * 4 - 3] *= o2f;
				temp[oh * 4 - 2] *= o2f;
				temp[oh * 4 - 1] *= o2f;
				tempp = temp;

				// blend output
				for(int j = 0; j < oh; j++)
				{
					uint16_t *out_row = (uint16_t*)voutput->get_row_ptr(o1i + j) + i * 4;
					int64_t input4 = *tempp++ + 0.5;
					int64_t input1 = *tempp++ + 0.5;
					int64_t input2 = (*tempp++) + 0x8000 + 0.5;
					int64_t input3 = (*tempp++) + 0x8000 + 0.5;
					BLEND(0x8000);
				}
			}
		}
		break;
	}
}


SampleEngine::SampleEngine(int cpus)
 : LoadServer(cpus, cpus)
{
	lookup_sx0 = 0;
	lookup_sx1 = 0;
	lookup_sk = 0;
	lookup_wacc = 0;
	kd = 0;
}

SampleEngine::~SampleEngine()
{
	delete [] lookup_sx0;
	delete [] lookup_sx1;
	delete [] lookup_sk;
	delete [] lookup_wacc;
}

// unlike the Direct and NN engines, the Sample engine works across
// output columns (it makes for more economical memory addressing
// during convolution)
void SampleEngine::init_packages()
{
	int   iw  = input->get_w();
	int   i1i = floor(in1);
	int   i2i = ceil(in2);
	double i1f = 1.f - in1 + i1i;
	double i2f = 1.f - i2i + in2;

	int    oy  = floor(out1);
	double oyf = out1 - oy;
	int    oh  = ceil(out2) - oy;

	double *k  = kernel->lookup;
	double kw  = kernel->width;
	int   kn  = kernel->n;

	if(in2 - in1 <= 0 || out2 - out1 <= 0)
		return;

	/* determine kernel spatial coverage */
	double scale = (out2 - out1) / (in2 - in1);
	double iscale = (in2 - in1) / (out2 - out1);
	double coverage = fabs(1.f / scale);
	double bound = (coverage < 1 ? kw : kw * coverage) - (.5 / TRANSFORM_SPP);
	double coeff = (coverage < 1 ? 1 : scale) * TRANSFORM_SPP;

	delete [] lookup_sx0;
	delete [] lookup_sx1;
	delete [] lookup_sk;
	delete [] lookup_wacc;

	lookup_sx0 = new int[oh];
	lookup_sx1 = new int[oh];
	lookup_sk = new int[oh];
	lookup_wacc = new double[oh];

	kd = coeff * (1 << INDEX_FRACTION) + .5;

	// precompute kernel values and weight sums
	for(int i = 0; i < oh; i++)
	{
		// map destination back to source
		double sx = (i - oyf + .5) * iscale + in1 - .5;

		// clip iteration to source area but not source plane. Points
		// outside the source plane count as transparent. Points outside
		// the source area don't count at all.  The actual convolution
		// later will be clipped to both, but we need to compute
		// weights.

		int sx0 = MAX((int)floor(sx - bound) + 1, i1i);
		int sx1 = MIN((int)ceil(sx + bound), i2i);
		int ki = (double)(sx0 - sx) * coeff * (1 << INDEX_FRACTION)
				+ (1 << (INDEX_FRACTION - 1)) + .5;
		double wacc = 0.;

		lookup_sx0[i] = -1;
		lookup_sx1[i] = -1;

		for(int j= sx0; j < sx1; j++)
		{
			int kv = (ki >> INDEX_FRACTION);

			if(kv > kn)
				break;
			if(kv >= -kn)
			{
				// the contribution of the first and last input pixel (if
				// fractional) are linearly weighted by the fraction
				if(j == i1i)
					wacc += k[abs(kv)] * i1f;
				else if(j + 1 == i2i)
					wacc += k[abs(kv)] * i2f;
				else
					wacc += k[abs(kv)];

				// this is where we clip the kernel convolution to the source plane
				if(j >= 0 && j < iw)
				{
					if(lookup_sx0[i] == -1)
					{
						lookup_sx0[i] = j;
						lookup_sk[i] = ki;
					}
					lookup_sx1[i] = j+1;
				}
			}
			ki += kd;
		}
		lookup_wacc[i] = wacc > 0. ? 1. / wacc : 0.;
	}
	for(int i = 0; i < get_total_packages(); i++)
	{
		SamplePackage *package = (SamplePackage*)get_package(i);

		package->out_col1 = (col_out2 - col_out1) * i / get_total_packages() + col_out1;
		package->out_col2 = (col_out2 - col_out1) * (i + 1) / get_total_packages() + col_out1;
	}
}

LoadClient* SampleEngine::new_client()
{
	return new SampleUnit(this);
}

LoadPackage* SampleEngine::new_package()
{
	return new SamplePackage;
}
