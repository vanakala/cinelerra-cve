
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * Copyright (C) 2012 Monty <monty@xiph.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "clip.h"
#include "edl.inc"
#include "mutex.h"
#include "overlayframe.h"
#include "units.h"
#include "vframe.h"

// Easy abstraction of the float and int types.  Most of these are never used
// but GCC expects them.
static int my_abs(int32_t x)
{
	return abs(x);
}

static int my_abs(uint32_t x)
{
	return x;
}

static int my_abs(int64_t x)
{
	return llabs(x);
}

static int my_abs(uint64_t x)
{
	return x;
}

static float my_abs(float x)
{
	return fabsf(x);
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
#define TRANSFORM_MIN    (.5/TRANSFORM_SPP)

/* Sinc needed for Lanczos kernel */
static float sinc(const float x)
{
	float y = x * M_PI;

	if(fabsf(x) < TRANSFORM_MIN)
		return 1.0f;

	return sinf (y) / y;
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
		width = 1.f;
		lookup = new float[(n = TRANSFORM_SPP)+1];
		for (i = 0; i <= TRANSFORM_SPP; i++)
			lookup[i] = (float)(TRANSFORM_SPP-i)/TRANSFORM_SPP;
		break;

	/* Use a Catmull-Rom filter (not b-spline) */
	case BICUBIC:
		width = 2.;
		lookup = new float[(n = 2*TRANSFORM_SPP)+1];
		for(i = 0; i <= TRANSFORM_SPP; i++)
		{
			float x = i/(float)TRANSFORM_SPP;
			lookup[i] = 1.f - 2.5f*x*x + 1.5f*x*x*x;
		}
		for(; i <= 2*TRANSFORM_SPP; i++)
		{
			float x = i/(float)TRANSFORM_SPP;
			lookup[i] = 2.f - 4.f*x  + 2.5f*x*x - .5f*x*x*x;
		}
		break;

	case LANCZOS:
		width = 3.;
		lookup = new float[(n = 3*TRANSFORM_SPP)+1];
		for (i = 0; i <= 3*TRANSFORM_SPP; i++)
			lookup[i] = sinc((float)i/TRANSFORM_SPP) * sinc((float)i/TRANSFORM_SPP/3.0f);
		break;

	default:
		width = 0.;
		lookup = 0;
		n = 0;
		break;
	}
}

OverlayKernel::~OverlayKernel()
{
	if(lookup) delete[] lookup;
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
	if(temp_frame) delete temp_frame;

	if(direct_engine) delete direct_engine;
	if(nn_engine) delete nn_engine;
	if(sample_engine) delete sample_engine;

	if(kernel[NEAREST_NEIGHBOR]) delete kernel[NEAREST_NEIGHBOR];
	if(kernel[BILINEAR]) delete kernel[BILINEAR];
	if(kernel[BICUBIC]) delete kernel[BICUBIC];
	if(kernel[LANCZOS]) delete kernel[LANCZOS];
}

static float epsilon_snap(float f)
{
	return rintf(f * 1024) / 1024.;
}

int OverlayFrame::overlay(VFrame *output,
	VFrame *input,
	float in_x1,
	float in_y1,
	float in_x2,
	float in_y2,
	float out_x1,
	float out_y1,
	float out_x2,
	float out_y2,
	float alpha,
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

	if (isnan(in_x1) ||
		isnan(in_x2) ||
		isnan(in_y1) ||
		isnan(in_y2) ||
		isnan(out_x1) ||
		isnan(out_x2) ||
		isnan(out_y1) ||
		isnan(out_y2)) return 1;

	float xscale = (out_x2 - out_x1) / (in_x2 - in_x1);
	float yscale = (out_y2 - out_y1) / (in_y2 - in_y1);

	/* don't interpolate integer translations, or scale no-ops */
	if(xscale == 1. && yscale == 1. &&
		(int)in_x1 == in_x1 && (int)in_x2 == in_x2 &&
		(int)in_y1 == in_y1 && (int)in_y2 == in_y2 &&
		(int)out_x1 == out_x1 && (int)out_x2 == out_x2 &&
		(int)out_y1 == out_y1 && (int)out_y2 == out_y2)
	{
		if(!direct_engine) direct_engine = new DirectEngine(cpus);

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
	else if (interpolation_type == NEAREST_NEIGHBOR)
	{
		if(!nn_engine) nn_engine = new NNEngine(cpus);
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

/*
 * horizontal and vertical are separately resampled.  First we
 * resample the input along X into a transposed, temporary frame,
 * then resample/transpose the temporary space along X into the
 * output.  Fractional pixels along the edge are handled in the X
 * direction of each step
 */
		// resampled dimension matches the transposed output space
		float temp_y1 = out_x1 - floor(out_x1);
		float temp_y2 = temp_y1 + (out_x2 - out_x1);
		int temp_h = ceil(temp_y2);

		// non-resampled dimension merely cropped
		float temp_x1 = in_y1 - floor(in_y1);
		float temp_x2 = temp_x1 + (in_y2-in_y1);
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

		if(!sample_engine) sample_engine = new SampleEngine(cpus);

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


// Verification:

// (255 * 255 + 0 * 0) / 255 = 255
// (255 * 127 + 255 * (255 - 127)) / 255 = 255

// (65535 * 65535 + 0 * 0) / 65535 = 65535
// (65535 * 32767 + 65535 * (65535 - 32767)) / 65535 = 65535


// Permutation 4 U

#define BLEND_3(max, temp_type, type, chroma_offset) \
{ \
	temp_type r, g, b; \
 \
	switch(mode) \
	{ \
		case TRANSFER_DIVIDE: \
			r = input1 ? (((temp_type)output[0] * max) / input1) : max; \
			if(chroma_offset) \
			{ \
				g = my_abs((temp_type)input2 - chroma_offset) > my_abs((temp_type)output[1] - chroma_offset) ? input2 : output[1]; \
				b = my_abs((temp_type)input3 - chroma_offset) > my_abs((temp_type)output[2] - chroma_offset) ? input3 : output[2]; \
			} \
			else \
			{ \
				g = input2 ? (temp_type)output[1] * max / (temp_type)input2 : max; \
				b = input3 ? (temp_type)output[2] * max / (temp_type)input3 : max; \
			} \
			r = (r * opacity + (temp_type)output[0] * transparency) / max; \
			g = (g * opacity + (temp_type)output[1] * transparency) / max; \
			b = (b * opacity + (temp_type)output[2] * transparency) / max; \
			break; \
		case TRANSFER_MULTIPLY: \
			r = ((temp_type)input1 * output[0]) / max; \
			if(chroma_offset) \
			{ \
				g = my_abs((temp_type)input2 - chroma_offset) > my_abs((temp_type)output[1] - chroma_offset) ? input2 : output[1]; \
				b = my_abs((temp_type)input3 - chroma_offset) > my_abs((temp_type)output[2] - chroma_offset) ? input3 : output[2]; \
			} \
			else \
			{ \
				g = (temp_type)input2 * (temp_type)output[1] / max; \
				b = (temp_type)input3 * (temp_type)output[2] / max; \
			} \
			r = (r * opacity + (temp_type)output[0] * transparency) / max; \
			g = (g * opacity + (temp_type)output[1] * transparency) / max; \
			b = (b * opacity + (temp_type)output[2] * transparency) / max; \
			break; \
		case TRANSFER_SUBTRACT: \
			r = (temp_type)output[0] - (temp_type)input1; \
			g = ((temp_type)output[1] - (temp_type)chroma_offset) - \
				((temp_type)input2 - (temp_type)chroma_offset) + \
				(temp_type)chroma_offset; \
			b = ((temp_type)output[2] - (temp_type)chroma_offset) - \
				((temp_type)input3 - (temp_type)chroma_offset) + \
				(temp_type)chroma_offset; \
			if(r < 0) r = 0; \
			if(g < 0) g = 0; \
			if(b < 0) b = 0; \
			r = (r * opacity + output[0] * transparency) / max; \
			g = (g * opacity + output[1] * transparency) / max; \
			b = (b * opacity + output[2] * transparency) / max; \
			break; \
		case TRANSFER_ADDITION: \
			r = (temp_type)input1 + output[0]; \
			g = ((temp_type)input2 - chroma_offset) + \
				((temp_type)output[1] - chroma_offset) + \
				(temp_type)chroma_offset; \
			b = ((temp_type)input3 - chroma_offset) + \
				((temp_type)output[2] - chroma_offset) + \
				(temp_type)chroma_offset; \
			r = (r * opacity + output[0] * transparency) / max; \
			g = (g * opacity + output[1] * transparency) / max; \
			b = (b * opacity + output[2] * transparency) / max; \
			break; \
		case TRANSFER_MAX: \
		{ \
			r = (temp_type)MAX(input1, output[0]); \
			temp_type g1 = ((temp_type)input2 - chroma_offset); \
			if(g1 < 0) g1 = -g1; \
			temp_type g2 = ((temp_type)output[1] - chroma_offset); \
			if(g2 < 0) g2 = -g2; \
			if(g1 > g2) \
				g = input2; \
			else \
				g = output[1]; \
			temp_type b1 = ((temp_type)input3 - chroma_offset); \
			if(b1 < 0) b1 = -b1; \
			temp_type b2 = ((temp_type)output[2] - chroma_offset); \
			if(b2 < 0) b2 = -b2; \
			if(b1 > b2) \
				b = input3; \
			else \
				b = output[2]; \
			r = (r * opacity + output[0] * transparency) / max; \
			g = (g * opacity + output[1] * transparency) / max; \
			b = (b * opacity + output[2] * transparency) / max; \
			break; \
		} \
		case TRANSFER_REPLACE: \
			r = input1; \
			g = input2; \
			b = input3; \
			break; \
		case TRANSFER_NORMAL: \
			r = ((temp_type)input1 * opacity + output[0] * transparency) / max; \
			g = ((temp_type)input2 * opacity + output[1] * transparency) / max; \
			b = ((temp_type)input3 * opacity + output[2] * transparency) / max; \
			break; \
	} \
 \
 	if(sizeof(type) != 4) \
	{ \
		output[0] = (type)CLIP(r, 0, max); \
		output[1] = (type)CLIP(g, 0, max); \
		output[2] = (type)CLIP(b, 0, max); \
	} \
	else \
	{ \
		output[0] = r; \
		output[1] = g; \
		output[2] = b; \
	} \
}

// Blending equations are drastically different for 3 and 4 components
#define BLEND_4(max, temp_type, type, chroma_offset) \
{ \
	temp_type r, g, b, a; \
	temp_type pixel_opacity, pixel_transparency; \
	temp_type output1 = output[0]; \
	temp_type output2 = output[1]; \
	temp_type output3 = output[2]; \
	temp_type output4 = output[3]; \
 \
	pixel_opacity = opacity * input4; \
	pixel_transparency = (temp_type)max * max - pixel_opacity; \
	a = output4 + ((max - output4) * input4) / max; \
 \
	switch(mode) \
	{ \
		case TRANSFER_DIVIDE: \
			r = input1 ? (((temp_type)output1 * max) / input1) : max; \
			if(chroma_offset) \
			{ \
				g = my_abs((temp_type)input2 - chroma_offset) > my_abs((temp_type)output2 - chroma_offset) ? input2 : output2; \
				b = my_abs((temp_type)input3 - chroma_offset) > my_abs((temp_type)output3 - chroma_offset) ? input3 : output3; \
			} \
			else \
			{ \
				g = input2 ? (temp_type)output2 * max / (temp_type)input2 : max; \
				b = input3 ? (temp_type)output3 * max / (temp_type)input3 : max; \
			} \
			r = (r * pixel_opacity + (temp_type)output1 * pixel_transparency) / max / max; \
			g = (g * pixel_opacity + (temp_type)output2 * pixel_transparency) / max / max; \
			b = (b * pixel_opacity + (temp_type)output3 * pixel_transparency) / max / max; \
			a = input4 > output4 ? input4 : output4; \
			break; \
		case TRANSFER_MULTIPLY: \
			r = ((temp_type)input1 * output1) / max; \
			if(chroma_offset) \
			{ \
				g = my_abs((temp_type)input2 - chroma_offset) > my_abs((temp_type)output2 - chroma_offset) ? input2 : output2; \
				b = my_abs((temp_type)input3 - chroma_offset) > my_abs((temp_type)output3 - chroma_offset) ? input3 : output3; \
			} \
			else \
			{ \
				g = (temp_type)input2 * (temp_type)output2 / max; \
				b = (temp_type)input3 * (temp_type)output3 / max; \
			} \
			r = (r * pixel_opacity + (temp_type)output1 * pixel_transparency) / max / max; \
			g = (g * pixel_opacity + (temp_type)output2 * pixel_transparency) / max / max; \
			b = (b * pixel_opacity + (temp_type)output3 * pixel_transparency) / max / max; \
			a = input4 > output4 ? input4 : output4; \
			break; \
		case TRANSFER_SUBTRACT: \
			r = (temp_type)output1 - input1; \
			g = ((temp_type)output2 - chroma_offset) - \
				((temp_type)input2 - (temp_type)chroma_offset) + \
				(temp_type)chroma_offset; \
			b = ((temp_type)output3 - chroma_offset) - \
				((temp_type)input3 - (temp_type)chroma_offset) + \
				(temp_type)chroma_offset; \
			if(r < 0) r = 0; \
			if(g < 0) g = 0; \
			if(b < 0) b = 0; \
			r = (r * pixel_opacity + output1 * pixel_transparency) / max / max; \
			g = (g * pixel_opacity + output2 * pixel_transparency) / max / max; \
			b = (b * pixel_opacity + output3 * pixel_transparency) / max / max; \
			a = input4 > output4 ? input4 : output4; \
			break; \
		case TRANSFER_ADDITION: \
			r = (temp_type)input1 + output1; \
			g = ((temp_type)input2 - chroma_offset) + \
				((temp_type)output2 - chroma_offset) + \
				chroma_offset; \
			b = ((temp_type)input3 - chroma_offset) + \
				((temp_type)output3 - chroma_offset) + \
				chroma_offset; \
			r = (r * pixel_opacity + output1 * pixel_transparency) / max / max; \
			g = (g * pixel_opacity + output2 * pixel_transparency) / max / max; \
			b = (b * pixel_opacity + output3 * pixel_transparency) / max / max; \
			a = input4 > output4 ? input4 : output4; \
			break; \
		case TRANSFER_MAX: \
		{ \
			r = (temp_type)MAX(input1, output1); \
			temp_type g1 = ((temp_type)input2 - chroma_offset); \
			if(g1 < 0) g1 = -g1; \
			temp_type g2 = ((temp_type)output2 - chroma_offset); \
			if(g2 < 0) g2 = -g2; \
			if(g1 > g2) \
				g = input2; \
			else \
				g = output2; \
			temp_type b1 = ((temp_type)input3 - chroma_offset); \
			if(b1 < 0) b1 = -b1; \
			temp_type b2 = ((temp_type)output3 - chroma_offset); \
			if(b2 < 0) b2 = -b2; \
			if(b1 > b2) \
				b = input3; \
			else \
				b = output3; \
			r = (r * pixel_opacity + output1 * pixel_transparency) / max / max; \
			g = (g * pixel_opacity + output2 * pixel_transparency) / max / max; \
			b = (b * pixel_opacity + output3 * pixel_transparency) / max / max; \
			break; \
		} \
		case TRANSFER_REPLACE: \
			r = input1; \
			g = input2; \
			b = input3; \
			a = input4; \
			break; \
		case TRANSFER_NORMAL: \
			r = (input1 * pixel_opacity + \
				output1 * pixel_transparency) / max / max; \
			g = ((input2 - chroma_offset) * pixel_opacity + \
				(output2 - chroma_offset) * pixel_transparency) \
				/ max / max + \
				chroma_offset; \
			b = ((input3 - chroma_offset) * pixel_opacity + \
				(output3 - chroma_offset) * pixel_transparency) \
				/ max / max + \
				chroma_offset; \
			break; \
	} \
 \
	if(sizeof(type) != 4) \
	{ \
		output[0] = (type)CLIP(r, 0, max); \
		output[1] = (type)CLIP(g, 0, max); \
		output[2] = (type)CLIP(b, 0, max); \
		output[3] = (type)CLIP(a, 0, max); \
	} \
	else \
	{ \
		output[0] = r; \
		output[1] = g; \
		output[2] = b; \
		output[3] = a; \
	} \
}

#define BLEND_ONLY(temp_type, type, max, components, chroma_offset) \
{ \
	temp_type opacity; \
	if(sizeof(type) != 4) \
		opacity = (temp_type)(alpha * max + 0.5); \
	else \
		opacity = (temp_type)(alpha * max); \
	temp_type transparency = max - opacity; \
 \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	ix *= components; \
	ox *= components; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		type* in_row = input_rows[i + iy] + ix; \
		type* output = output_rows[i] + ox; \
 \
		for(int j = 0; j < ow; j++) \
		{ \
			temp_type input1, input2, input3, input4; \
			input1 = in_row[0]; \
			input2 = in_row[1]; \
			input3 = in_row[2]; \
			if(components == 4) input4 = in_row[3]; \
 \
 \
			if(components == 3) \
			{ \
				BLEND_3(max, temp_type, type, chroma_offset); \
			} \
			else \
			{ \
				BLEND_4(max, temp_type, type, chroma_offset); \
			} \
 \
			in_row += components; \
			output += components; \
		} \
	} \
}


#define BLEND_ONLY_TRANSFER_REPLACE(type, components) \
{ \
 \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	int line_len = ow * sizeof(type) * components; \
	ix *= components; \
	ox *= components; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		memcpy(output_rows[i] + ox, input_rows[i + iy] + ix, line_len); \
	} \
}

// components is always 4
#define BLEND_ONLY_4_NORMAL(temp_type, type, max, chroma_offset) \
{ \
	temp_type opacity = (temp_type)(alpha * max + 0.5); \
	temp_type transparency = max - opacity; \
	temp_type max_squared = ((temp_type)max) * max; \
 \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	ix *= 4; \
	ox *= 4; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		type* in_row = input_rows[i + iy] + ix; \
		type* output = output_rows[i] + ox; \
 \
		for(int j = 0; j < ow; j++) \
		{ \
			temp_type pixel_opacity, pixel_transparency; \
			pixel_opacity = opacity * in_row[3]; \
			pixel_transparency = (temp_type)max_squared - pixel_opacity; \
 \
 \
			temp_type r,g,b; \
			output[0] = ((temp_type)in_row[0] * pixel_opacity + \
				(temp_type)output[0] * pixel_transparency) / max / max; \
			output[1] = (((temp_type)in_row[1] - chroma_offset) * pixel_opacity + \
				((temp_type)output[1] - chroma_offset) * pixel_transparency) \
				/ max / max + \
				chroma_offset; \
			output[2] = (((temp_type)in_row[2] - chroma_offset) * pixel_opacity + \
				((temp_type)output[2] - chroma_offset) * pixel_transparency) \
				/ max / max + \
				chroma_offset; \
			output[3] += ((temp_type)(max - output[3]) * in_row[3]) / max; \
 \
			in_row += 4; \
			output += 4; \
		} \
	} \
}


// components is always 3
#define BLEND_ONLY_3_NORMAL(temp_type, type, max, chroma_offset) \
{ \
	const int bits = sizeof(type) * 8; \
	temp_type opacity = (temp_type)(alpha * ((temp_type)1 << bits) + 0.5); \
	temp_type transparency = ((temp_type)1 << bits) - opacity; \
 \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	ix *= 3; \
	ox *= 3; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		type* in_row = input_rows[i + iy] + ix; \
		type* output = output_rows[i] + ox; \
 \
		for(int j = 0; j < ow * 3; j++) \
		{ \
			*output = ((temp_type)*in_row * opacity + *output * transparency) >> bits; \
			in_row ++; \
			output ++; \
		} \
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
	float alpha = engine->alpha;
	int mode = engine->mode;

	int ix = engine->in_x1;
	int ox = engine->out_x1;
	int ow = engine->out_x2 - ox;
	int iy = engine->in_y1 - engine->out_y1;

	if (mode == TRANSFER_REPLACE)
	{
		switch(input->get_color_model())
		{
		case BC_RGB_FLOAT:
			BLEND_ONLY_TRANSFER_REPLACE(float, 3);
			break;
		case BC_RGBA_FLOAT:
			BLEND_ONLY_TRANSFER_REPLACE(float, 4);
			break;
		case BC_RGB888:
		case BC_YUV888:
			BLEND_ONLY_TRANSFER_REPLACE(unsigned char, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			BLEND_ONLY_TRANSFER_REPLACE(unsigned char, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			BLEND_ONLY_TRANSFER_REPLACE(uint16_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			BLEND_ONLY_TRANSFER_REPLACE(uint16_t, 4);
			break;
		}
	}
	else if(mode == TRANSFER_NORMAL)
	{
		switch(input->get_color_model()){
		case BC_RGB_FLOAT:
		{
			float opacity = alpha;
			float transparency = 1.0 - alpha;

			float** output_rows = (float**)output->get_rows();
			float** input_rows = (float**)input->get_rows();

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				float* in_row = input_rows[i + iy] + ix;
				float* output = output_rows[i] + ox;

				for(int j = 0; j < ow * 3; j++)
				{
					*output = *in_row * opacity + *output * transparency;
					in_row++;
					output++;
				}
			}
			break;
		}

		case BC_RGBA_FLOAT:
		{
			float opacity = alpha;
			float transparency = 1.0 - alpha;

			float** output_rows = (float**)output->get_rows();
			float** input_rows = (float**)input->get_rows();

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				float* in_row = input_rows[i + iy] + ix;
				float* output = output_rows[i] + ox;

				for(int j = 0; j < ow; j++)
				{
					float pixel_opacity, pixel_transparency;
					pixel_opacity = opacity * in_row[3];
					pixel_transparency = 1.0 - pixel_opacity;

					output[0] = in_row[0] * pixel_opacity +
					output[0] * pixel_transparency;
					output[1] = in_row[1] * pixel_opacity +
						output[1] * pixel_transparency;
					output[2] = in_row[2] * pixel_opacity +
						output[2] * pixel_transparency;
					output[3] += (1. - output[3]) * in_row[3];

					in_row += 4;
					output += 4;
				}
			}
			break;
		}

		case BC_RGB888:
			BLEND_ONLY_3_NORMAL(uint32_t, unsigned char, 0xff, 0);
			break;
		case BC_YUV888:
			BLEND_ONLY_3_NORMAL(int32_t, unsigned char, 0xff, 0x80);
			break;
		case BC_RGBA8888:
			BLEND_ONLY_4_NORMAL(uint32_t, unsigned char, 0xff, 0);
			break;
		case BC_YUVA8888:
			BLEND_ONLY_4_NORMAL(int32_t, unsigned char, 0xff, 0x80);
			break;
		case BC_RGB161616:
			BLEND_ONLY_3_NORMAL(uint64_t, uint16_t, 0xffff, 0);
			break;
		case BC_YUV161616:
			BLEND_ONLY_3_NORMAL(int64_t, uint16_t, 0xffff, 0x8000);
			break;
		case BC_RGBA16161616:
			BLEND_ONLY_4_NORMAL(uint64_t, uint16_t, 0xffff, 0);
			break;
		case BC_YUVA16161616:
			BLEND_ONLY_4_NORMAL(int64_t, uint16_t, 0xffff, 0x8000);
			break;
		}
	}
	else
		switch(input->get_color_model())
		{
		case BC_RGB_FLOAT:
			BLEND_ONLY(float, float, 1.0, 3, 0);
			break;
		case BC_RGBA_FLOAT:
			BLEND_ONLY(float, float, 1.0, 4, 0);
			break;
		case BC_RGB888:
			BLEND_ONLY(int32_t, unsigned char, 0xff, 3, 0);
			break;
		case BC_YUV888:
			BLEND_ONLY(int32_t, unsigned char, 0xff, 3, 0x80);
			break;
		case BC_RGBA8888:
			BLEND_ONLY(int32_t, unsigned char, 0xff, 4, 0);
			break;
		case BC_YUVA8888:
			BLEND_ONLY(int32_t, unsigned char, 0xff, 4, 0x80);
			break;
		case BC_RGB161616:
			BLEND_ONLY(int64_t, uint16_t, 0xffff, 3, 0);
			break;
		case BC_YUV161616:
			BLEND_ONLY(int64_t, uint16_t, 0xffff, 3, 0x8000);
			break;
		case BC_RGBA16161616:
			BLEND_ONLY(int64_t, uint16_t, 0xffff, 4, 0);
			break;
		case BC_YUVA16161616:
			BLEND_ONLY(int64_t, uint16_t, 0xffff, 4, 0x8000);
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

#define BLEND_NN(temp_type, type, max, components, chroma_offset) \
{ \
	temp_type opacity; \
 \
	if(sizeof(type) != 4) \
		opacity = (temp_type)(alpha * max + 0.5); \
	else \
		opacity = (temp_type)(alpha * max); \
	temp_type transparency = max - opacity; \
 \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	ox *= components; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		int *lx = engine->in_lookup_x; \
		type* in_row = input_rows[*ly++]; \
		type* output = output_rows[i] + ox; \
 \
		for(int j = 0; j < ow; j++) \
		{ \
			temp_type input1, input2, input3, input4; \
			in_row += *lx++; \
			input1 = in_row[0]; \
			input2 = in_row[1]; \
			input3 = in_row[2]; \
 \
			if(components == 4) \
				input4 = in_row[3]; \
 \
			if(components == 3) \
			{ \
				BLEND_3(max, temp_type, type, chroma_offset); \
			} \
			else \
			{ \
				BLEND_4(max, temp_type, type, chroma_offset); \
			} \
 \
				output += components; \
		} \
	} \
}

#define BLEND_NN_TRANSFER_REPLACE(type, components) \
{ \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	ox *= components; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		int *lx = engine->in_lookup_x; \
		type* in_row = input_rows[*ly++]; \
		type* output = output_rows[i] + ox; \
 \
		for(int j = 0; j < ow; j++) \
		{ \
			in_row += *lx++; \
			*output++ = in_row[0]; \
			*output++ = in_row[1]; \
			*output++ = in_row[2]; \
 \
			if(components==4) \
				*output++ = in_row[3]; \
		} \
	} \
}

#define BLEND_NN_4_NORMAL(temp_type, type, max, chroma_offset) \
{ \
	temp_type opacity = (temp_type)(alpha * max + 0.5); \
	temp_type transparency = max - opacity; \
	temp_type max_squared = ((temp_type)max) * max; \
 \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	ox *= 4; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		int *lx = engine->in_lookup_x; \
		type* in_row = input_rows[*ly++]; \
		type* output = output_rows[i] + ox; \
 \
		for(int j = 0; j < ow; j++) \
		{ \
			temp_type pixel_o, pixel_t, r, g, b; \
 \
			in_row += *lx++; \
			pixel_o = opacity * in_row[3]; \
			pixel_t = (temp_type)max_squared - pixel_o; \
			output[0] = ((temp_type)in_row[0] * pixel_o + \
				(temp_type)output[0] * pixel_t) / max / max; \
			output[1] = (((temp_type)in_row[1] - chroma_offset) * pixel_o + \
				((temp_type)output[1] - chroma_offset) * pixel_t)  \
				/ max / max + chroma_offset; \
			output[2] = (((temp_type)in_row[2] - chroma_offset) * pixel_o + \
				((temp_type)output[2] - chroma_offset) * pixel_t) \
				/ max / max + chroma_offset; \
			output[3] += ((temp_type)(max - output[3]) * in_row[3]) / max; \
 \
			output += 4; \
		} \
	} \
}

#define BLEND_NN_3_NORMAL(temp_type, type, max, chroma_offset) \
{ \
	const int bits = sizeof(type) * 8; \
	temp_type opacity = (temp_type)(alpha*((temp_type)1 << bits) + 0.5); \
	temp_type transparency = ((temp_type)1 << bits) - opacity; \
	ox *= 3; \
 \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		int *lx = engine->in_lookup_x; \
		type* in_row = input_rows[*ly++]; \
		type* output = output_rows[i]+ox; \
 \
		for(int j = 0; j < ow; j++) \
		{ \
			in_row += *lx++; \
			output[0] = ((temp_type)in_row[0] * opacity + \
				output[0] * transparency) >> bits; \
			output[1] = ((temp_type)in_row[1] * opacity + \
				output[1] * transparency) >> bits; \
			output[2] = ((temp_type)in_row[2] * opacity + \
				output[2] * transparency) >> bits; \
 \
			output += 3; \
		} \
	} \
}

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
	float alpha = engine->alpha;
	int mode = engine->mode;

	int ox = engine->out_x1i;
	int ow = engine->out_x2i - ox;
	int *ly = engine->in_lookup_y + pkg->out_row1;

	if (mode == TRANSFER_REPLACE)
	{
		switch(input->get_color_model())
		{
		case BC_RGB_FLOAT:
			BLEND_NN_TRANSFER_REPLACE(float, 3);
			break;
		case BC_RGBA_FLOAT:
			BLEND_NN_TRANSFER_REPLACE(float, 4);
			break;
		case BC_RGB888:
		case BC_YUV888:
			BLEND_NN_TRANSFER_REPLACE(unsigned char, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			BLEND_NN_TRANSFER_REPLACE(unsigned char, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			BLEND_NN_TRANSFER_REPLACE(uint16_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			BLEND_NN_TRANSFER_REPLACE(uint16_t, 4);
			break;
		}
	}
	else if (mode == TRANSFER_NORMAL)
	{
		switch(input->get_color_model())
		{
		case BC_RGB_FLOAT:
		{
			float opacity = alpha;
			float transparency = 1.0 - alpha;

			float** output_rows = (float**)output->get_rows();
			float** input_rows = (float**)input->get_rows();
			ox *= 3;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				int *lx = engine->in_lookup_x;
				float *in_row = input_rows[*ly++];
				float* output = output_rows[i] + ox;

				for(int j = 0; j < ow; j++)
				{
					in_row += *lx++;
					output[0] = in_row[0] * opacity + output[0] * transparency;
					output[1] = in_row[1] * opacity + output[1] * transparency;
					output[2] = in_row[2] * opacity + output[2] * transparency;
					output += 3;
				}
			}
			break;
		}
		case BC_RGBA_FLOAT:
		{
			float opacity = alpha;
			float transparency = 1.0 - alpha;

			float** output_rows = (float**)output->get_rows();
			float** input_rows = (float**)input->get_rows();
			ox *= 4;

			for(int i = pkg->out_row1; i < pkg->out_row2; i++)
			{
				int *lx = engine->in_lookup_x;
				float *in_row = input_rows[*ly++];
				float* output = output_rows[i] + ox;

				for(int j = 0; j < ow; j++)
				{
					float pixel_opacity, pixel_transparency;

					in_row += *lx++;
					pixel_opacity = opacity * in_row[3];
					pixel_transparency = 1.0 - pixel_opacity;

					output[0] = in_row[0] * pixel_opacity +
						output[0] * pixel_transparency;
					output[1] = in_row[1] * pixel_opacity +
						output[1] * pixel_transparency;
					output[2] = in_row[2] * pixel_opacity +
						output[2] * pixel_transparency;
					output[3] += (1. - output[3]) * in_row[3];

					output += 4;
				}
			}
			break;
		}
		case BC_RGB888:
			BLEND_NN_3_NORMAL(uint32_t, unsigned char, 0xff, 0);
			break;
		case BC_YUV888:
			BLEND_NN_3_NORMAL(int32_t, unsigned char, 0xff, 0x80);
			break;
		case BC_RGBA8888:
			BLEND_NN_4_NORMAL(uint32_t, unsigned char, 0xff, 0);
			break;
		case BC_YUVA8888:
			BLEND_NN_4_NORMAL(int32_t, unsigned char, 0xff, 0x80);
			break;
		case BC_RGB161616:
			BLEND_NN_3_NORMAL(uint64_t, uint16_t, 0xffff, 0);
			break;
		case BC_YUV161616:
			BLEND_NN_3_NORMAL(int64_t, uint16_t, 0xffff, 0x8000);
			break;
		case BC_RGBA16161616:
			BLEND_NN_4_NORMAL(uint64_t, uint16_t, 0xffff, 0);
			break;
		case BC_YUVA16161616:
			BLEND_NN_4_NORMAL(int64_t, uint16_t, 0xffff, 0x8000);
			break;
		}
	}
	else
		switch(input->get_color_model())
		{
		case BC_RGB_FLOAT:
			BLEND_NN(float, float, 1.0, 3, 0);
			break;
		case BC_RGBA_FLOAT:
			BLEND_NN(float, float, 1.0, 4, 0);
			break;
		case BC_RGB888:
			BLEND_NN(int32_t, unsigned char, 0xff, 3, 0);
			break;
		case BC_YUV888:
			BLEND_NN(int32_t, unsigned char, 0xff, 3, 0x80);
			break;
		case BC_RGBA8888:
			BLEND_NN(int32_t, unsigned char, 0xff, 4, 0);
			break;
		case BC_YUVA8888:
			BLEND_NN(int32_t, unsigned char, 0xff, 4, 0x80);
			break;
		case BC_RGB161616:
			BLEND_NN(int64_t, uint16_t, 0xffff, 3, 0);
			break;
		case BC_YUV161616:
			BLEND_NN(int64_t, uint16_t, 0xffff, 3, 0x8000);
			break;
		case BC_RGBA16161616:
			BLEND_NN(int64_t, uint16_t, 0xffff, 4, 0);
			break;
		case BC_YUVA16161616:
			BLEND_NN(int64_t, uint16_t, 0xffff, 4, 0x8000);
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
	if(in_lookup_x)
		delete[] in_lookup_x;
	if(in_lookup_y)
		delete[] in_lookup_y;
}

void NNEngine::init_packages()
{
	int in_w = input->get_w();
	int in_h = input->get_h();
	int out_w = output->get_w();
	int out_h = output->get_h();

	float in_subw = in_x2 - in_x1;
	float in_subh = in_y2 - in_y1;
	float out_subw = out_x2 - out_x1;
	float out_subh = out_y2 - out_y1;
	int first, last, count, i;
	int components;

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
	in_lookup_x = new int[out_x2i-out_x1i];

	if(in_lookup_y)
		delete[] in_lookup_y;
	in_lookup_y = new int[out_h];

	switch(input->get_color_model())
	{
	case BC_RGB_FLOAT:
	case BC_RGB888:
	case BC_YUV888:
	case BC_RGB161616:
	case BC_YUV161616:
	case BC_YUVA16161616:
		components = 3;
		break;

	case BC_RGBA_FLOAT:
	case BC_RGBA8888:
	case BC_YUVA8888:
	case BC_RGBA16161616:
		components = 4;
		break;
	}

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
				in_lookup_x[0] = in * components;
			}
			else
			{
				in_lookup_x[count] = (in-last)*components;
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
		int in = (i - out_y1+.5) * in_subh / out_subh + in_y1;
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

#define SAMPLE_3(max, temp_type, type, chroma_offset, round) \
{ \
	float temp[oh*3]; \
	type **output_rows = (type**)voutput->get_rows() + o1i; \
	type **input_rows = (type**)vinput->get_rows(); \
	temp_type opacity = (alpha * max + round); \
	temp_type transparency = max - opacity; \
 \
	for(int i = pkg->out_col1; i < pkg->out_col2; i++) \
	{ \
 \
		if(opacity == 0) \
		{ \
			/* don't bother resampling if the frame is invisible */ \
			temp_type input1 = 0; \
			temp_type input2 = chroma_offset; \
			temp_type input3 = chroma_offset; \
			temp_type input4 = 0; \
			for(int j = 0; j < oh; j++) \
			{ \
				type *output = output_rows[j] + i * 4; \
				BLEND_4(max, temp_type, type, chroma_offset); \
			} \
		} \
		else \
		{ \
			type *input = input_rows[i - engine->col_out1 + engine->row_in]; \
			float *tempp = temp; \
 \
			if(!k) \
			{ \
				/* direct copy case */ \
				type *ip = input + i1i * 3; \
				for(int j = 0; j < oh; j++) \
				{ \
					*tempp++ = *ip++; \
					*tempp++ = (*ip++) - chroma_offset; \
					*tempp++ = (*ip++) - chroma_offset; \
				} \
			} \
			else \
			{ \
				/* resample */ \
				for(int j = 0; j < oh; j++) \
				{ \
					float racc=0.f, gacc=0.f, bacc=0.f; \
					int ki = lookup_sk[j]; \
					int x = lookup_sx0[j]; \
					type *ip = input+x * 3; \
					float wacc = 0; \
					while(x++ < lookup_sx1[j]) \
					{ \
						float kv = k[abs(ki >> INDEX_FRACTION)]; \
 \
						/* handle fractional pixels on edges of input */ \
						if(x == i1i) kv *= i1f; \
						if(x + 1 == i2i) kv *= i2f; \
 \
						wacc += kv; \
						racc += kv * *ip++; \
						gacc += kv * ((*ip++) - chroma_offset); \
						bacc += kv * ((*ip++) - chroma_offset); \
						ki+=kd; \
					} \
					if(wacc > 0.) \
						wacc = 1. / wacc; \
					*tempp++ = racc * wacc; \
					*tempp++ = gacc * wacc; \
					*tempp++ = bacc * wacc; \
				} \
			} \
 \
			/* handle fractional pixels on edges of output */ \
			temp[0] *= o1f; \
			temp[1] *= o1f; \
			temp[2] *= o1f; \
			temp[oh * 3 - 3] *= o2f; \
			temp[oh * 3 - 2] *= o2f; \
			temp[oh * 3 - 1] *= o2f; \
			tempp=temp; \
 \
			/* blend output */ \
			for(int j = 0; j < oh; j++) \
			{ \
				type *output = output_rows[j] + i * 3; \
				temp_type input1 = *tempp++ + round; \
				temp_type input2 = (*tempp++) + chroma_offset + round; \
				temp_type input3 = (*tempp++) + chroma_offset + round; \
				BLEND_3(max, temp_type, type, chroma_offset); \
			} \
		} \
	} \
}

#define SAMPLE_4(max, temp_type, type, chroma_offset, round) \
{ \
	float temp[oh*4]; \
	type **output_rows = (type**)voutput->get_rows() + o1i; \
	type **input_rows = (type**)vinput->get_rows(); \
	temp_type opacity = (alpha * max + round); \
	temp_type transparency = max - opacity; \
 \
	for(int i = pkg->out_col1; i < pkg->out_col2; i++) \
	{ \
 \
		if(opacity == 0) \
		{ \
 \
			/* don't bother resampling if the frame is invisible */ \
			temp_type input1 = 0; \
			temp_type input2 = chroma_offset; \
			temp_type input3 = chroma_offset; \
			temp_type input4 = 0; \
			for(int j = 0; j < oh; j++) \
			{ \
				type *output = output_rows[j] + i * 4; \
				BLEND_4(max, temp_type, type, chroma_offset); \
			} \
		} \
		else \
		{ \
			type *input = input_rows[i - engine->col_out1 + engine->row_in]; \
			float *tempp = temp; \
 \
			if(!k) \
			{ \
				/* direct copy case */ \
				type *ip = input + i1i * 4; \
				for(int j = 0; j < oh; j++) \
				{ \
					*tempp++ = *ip++; \
					*tempp++ = (*ip++) - chroma_offset; \
					*tempp++ = (*ip++) - chroma_offset; \
					*tempp++ = *ip++; \
				} \
			} \
			else \
			{ \
				/* resample */ \
				for(int j = 0; j < oh; j++) \
				{ \
					float racc=0.f, gacc=0.f, bacc=0.f, aacc=0.f; \
					int ki = lookup_sk[j]; \
					int x = lookup_sx0[j]; \
					type *ip = input + x * 4; \
					float wacc = 0; \
					float awacc = 0; \
					while(x++ < lookup_sx1[j]) \
					{ \
						float kv = k[abs(ki >> INDEX_FRACTION)]; \
 \
						/* handle fractional pixels on edges of input */ \
						if(x == i1i) kv *= i1f; \
						if(x + 1 == i2i) kv *= i2f; \
 \
						float a = ip[3] * kv; \
						awacc += kv; \
						kv = a; \
						wacc += kv; \
 \
						racc += kv * *ip++; \
						gacc += kv * (*ip++ - chroma_offset); \
						bacc += kv * (*ip++ - chroma_offset); \
						aacc += kv; ip++; \
						ki+=kd; \
 \
					} \
					if(wacc > 0) wacc = 1. / wacc; \
					if(awacc > 0) awacc = 1. / awacc; \
					*tempp++ = racc * wacc; \
					*tempp++ = gacc * wacc; \
					*tempp++ = bacc * wacc; \
					*tempp++ = aacc * awacc; \
				} \
			} \
 \
			/* handle fractional pixels on edges of output */ \
			temp[0] *= o1f; \
			temp[1] *= o1f; \
			temp[2] *= o1f; \
			temp[3] *= o1f; \
			temp[oh * 4 - 4] *= o2f; \
			temp[oh * 4 - 3] *= o2f; \
			temp[oh * 4 - 2] *= o2f; \
			temp[oh * 4 - 1] *= o2f; \
			tempp=temp; \
 \
			/* blend output */ \
			for(int j = 0; j < oh; j++) \
			{ \
				type *output = output_rows[j] + i * 4; \
				temp_type input1 = *tempp++ + round; \
				temp_type input2 = (*tempp++) + chroma_offset + round; \
				temp_type input3 = (*tempp++) + chroma_offset + round; \
				temp_type input4 = *tempp++ + round; \
				BLEND_4(max, temp_type, type, chroma_offset); \
			} \
		} \
	} \
}

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

	float i1  = engine->in1;
	float i2  = engine->in2;
	float o1  = engine->out1;
	float o2  = engine->out2;

	if(i2 - i1 <= 0 || o2 - o1 <= 0)
		return;

	VFrame *voutput = engine->output;
	VFrame *vinput = engine->input;
	float alpha = engine->alpha;
	int mode = engine->mode;
	float opacity = alpha;
	float transparency = 1.0 - alpha;

	int   iw  = vinput->get_w();
	int   i1i = floor(i1);
	int   i2i = ceil(i2);
	float i1f = 1.f - i1 + i1i;
	float i2f = 1.f - i2i + i2;

	int   o1i = floor(o1);
	int   o2i = ceil(o2);
	float o1f = 1.f - o1 + o1i;
	float o2f = 1.f - o2i + o2;
	int   oh  = o2i - o1i;

	float *k  = engine->kernel->lookup;
	float kw  = engine->kernel->width;
	int   kn  = engine->kernel->n;
	int   kd = engine->kd;

	int *lookup_sx0 = engine->lookup_sx0;
	int *lookup_sx1 = engine->lookup_sx1;
	int *lookup_sk = engine->lookup_sk;
	float *lookup_wacc = engine->lookup_wacc;

	/* resample into a temporary row vector, then blend */
	switch(vinput->get_color_model())
	{
	case BC_RGB_FLOAT:
		SAMPLE_3(1.f, float, float, 0.f, 0.f);
		break;
	case BC_RGBA_FLOAT:
		SAMPLE_4(1.f, float, float, 0.f, 0.f);
		break;
	case BC_RGB888:
		SAMPLE_3(255, int32_t, unsigned char, 0.f, .5f);
		break;
	case BC_YUV888:
		SAMPLE_3(255, int32_t, unsigned char, 128.f, .5f);
		break;
	case BC_RGBA8888:
		SAMPLE_4(255, int32_t, unsigned char, 0.f, .5f);
		break;
	case BC_YUVA8888:
		SAMPLE_4(255, int32_t, unsigned char, 128.f, .5f);
		break;
	case BC_RGB161616:
		SAMPLE_3(0xffff, int64_t, uint16_t, 0.f, .5f);
		break;
	case BC_YUV161616:
		SAMPLE_3(0xffff, int64_t, uint16_t, 32768.f, .5f);
		break;
	case BC_RGBA16161616:
		SAMPLE_4(0xffff, int64_t, uint16_t, 0.f, .5f);
		break;
	case BC_YUVA16161616:
		SAMPLE_4(0xffff, int64_t, uint16_t, 32768.f, .5f);
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
	if(lookup_sx0) delete [] lookup_sx0;
	if(lookup_sx1) delete [] lookup_sx1;
	if(lookup_sk) delete [] lookup_sk;
	if(lookup_wacc) delete [] lookup_wacc;
}

/*
 * unlike the Direct and NN engines, the Sample engine works across
 * output columns (it makes for more economical memory addressing
 * during convolution)
 */
void SampleEngine::init_packages()
{
	int   iw  = input->get_w();
	int   i1i = floor(in1);
	int   i2i = ceil(in2);
	float i1f = 1.f - in1 + i1i;
	float i2f = 1.f - i2i + in2;

	int   oy  = floor(out1);
	float oyf = out1 - oy;
	int   oh  = ceil(out2) - oy;

	float *k  = kernel->lookup;
	float kw  = kernel->width;
	int   kn  = kernel->n;

	if(in2 - in1 <= 0 || out2 - out1 <= 0)
		return;

	/* determine kernel spatial coverage */
	float scale = (out2 - out1) / (in2 - in1);
	float iscale = (in2 - in1) / (out2 - out1);
	float coverage = fabs(1.f / scale);
	float bound = (coverage < 1.f ? kw : kw * coverage) - (.5f / TRANSFORM_SPP);
	float coeff = (coverage < 1.f ? 1.f : scale) * TRANSFORM_SPP;

	if(lookup_sx0) delete [] lookup_sx0;
	if(lookup_sx1) delete [] lookup_sx1;
	if(lookup_sk) delete [] lookup_sk;
	if(lookup_wacc) delete [] lookup_wacc;

	lookup_sx0 = new int[oh];
	lookup_sx1 = new int[oh];
	lookup_sk = new int[oh];
	lookup_wacc = new float[oh];

	kd = (double)coeff * (1 << INDEX_FRACTION) + .5;

	/* precompute kernel values and weight sums */
	for(int i = 0; i < oh; i++)
	{
		/* map destination back to source */
		double sx = (i - oyf + .5) * iscale + in1 - .5;

		/*
		 * clip iteration to source area but not source plane. Points
		 * outside the source plane count as transparent. Points outside
		 * the source area don't count at all.  The actual convolution
		 * later will be clipped to both, but we need to compute
		 * weights.
		 */
		int sx0 = MAX((int)floor(sx - bound) + 1, i1i);
		int sx1 = MIN((int)ceil(sx + bound), i2i);
		int ki = (double)(sx0 - sx) * coeff * (1 << INDEX_FRACTION)
				+ (1 << (INDEX_FRACTION - 1)) + .5;
		float wacc=0.;

		lookup_sx0[i] = -1;
		lookup_sx1[i] = -1;

		for(int j= sx0; j < sx1; j++)
		{
			int kv = (ki >> INDEX_FRACTION);
			if(kv > kn) break;
			if(kv >= -kn)
			{
				/*
				 * the contribution of the first and last input pixel (if
				 * fractional) are linearly weighted by the fraction
				 */
				if(j == i1i)
				{
					wacc += k[abs(kv)] * i1f;
				}
				else if(j + 1 == i2i)
				{
					wacc += k[abs(kv)] * i2f;
				}
				else
					wacc += k[abs(kv)];

				/* this is where we clip the kernel convolution to the source plane */
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
