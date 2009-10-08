
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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




OverlayFrame::OverlayFrame(int cpus)
{
	temp_frame = 0;
	blend_engine = 0;
	scale_engine = 0;
	scaletranslate_engine = 0;
	translate_engine = 0;
	this->cpus = cpus;
}

OverlayFrame::~OverlayFrame()
{
	if(temp_frame) delete temp_frame;
	if(scale_engine) delete scale_engine;
	if(translate_engine) delete translate_engine;
	if(blend_engine) delete blend_engine;
	if(scaletranslate_engine) delete scaletranslate_engine;
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
/* if(mode != TRANSFER_NORMAL) printf("BLEND mode = %d\n", mode); */ \
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
			a = input4 > output4 ? input4 : output4; \
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
			a = input4 > output4 ? input4 : output4; \
			break; \
	} \
 \
 	if(sizeof(type) != 4) \
	{ \
		output[0] = (type)CLIP(r, 0, max); \
		output[1] = (type)CLIP(g, 0, max); \
		output[2] = (type)CLIP(b, 0, max); \
		output[3] = (type)a; \
	} \
	else \
	{ \
		output[0] = r; \
		output[1] = g; \
		output[2] = b; \
		output[3] = a; \
	} \
}



// Bicubic algorithm using multiprocessors
// input -> scale nearest integer boundaries -> temp -> translation -> blend -> output

// Nearest neighbor algorithm using multiprocessors for blending
// input -> scale + translate -> blend -> output


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
	float alpha,       // 0 - 1
	int mode,
	int interpolation_type)
{
	float w_scale = (out_x2 - out_x1) / (in_x2 - in_x1);
	float h_scale = (out_y2 - out_y1) / (in_y2 - in_y1);








	if(isnan(in_x1) ||
		isnan(in_y1) ||
		isnan(in_x2) ||
		isnan(in_y2) ||
		isnan(out_x1) ||
		isnan(out_y1) ||
		isnan(out_x2) ||
		isnan(out_y2)) return 1;
// printf("OverlayFrame::overlay 1 %f %f %f %f -> %f %f %f %f scale=%f %f\n", in_x1,
// in_y1,
// in_x2,
// in_y2,
// out_x1,
// out_y1,
// out_x2,
// out_y2,
// out_x2 - out_x1, 
// out_y2 - out_y1);

// Limit values
	if(in_x1 < 0)
	{
		out_x1 += -in_x1 * w_scale;
		in_x1 = 0;
	}
	else
	if(in_x1 >= input->get_w())
	{
		out_x1 -= (in_x1 - input->get_w()) * w_scale;
		in_x1 = input->get_w();
	}

	if(in_y1 < 0)
	{
		out_y1 += -in_y1 * h_scale;
		in_y1 = 0;
	}
	else
	if(in_y1 >= input->get_h())
	{
		out_y1 -= (in_y1 - input->get_h()) * h_scale;
		in_y1 = input->get_h();
	}

	if(in_x2 < 0)
	{
		out_x2 += -in_x2 * w_scale;
		in_x2 = 0;
	}
	else
	if(in_x2 >= input->get_w())
	{
		out_x2 -= (in_x2 - input->get_w()) * w_scale;
		in_x2 = input->get_w();
	}

	if(in_y2 < 0)
	{
		out_y2 += -in_y2 * h_scale;
		in_y2 = 0;
	}
	else
	if(in_y2 >= input->get_h())
	{
		out_y2 -= (in_y2 - input->get_h()) * h_scale;
		in_y2 = input->get_h();
	}

	if(out_x1 < 0)
	{
		in_x1 += -out_x1 / w_scale;
		out_x1 = 0;
	}
	else
	if(out_x1 >= output->get_w())
	{
		in_x1 -= (out_x1 - output->get_w()) / w_scale;
		out_x1 = output->get_w();
	}

	if(out_y1 < 0)
	{
		in_y1 += -out_y1 / h_scale;
		out_y1 = 0;
	}
	else
	if(out_y1 >= output->get_h())
	{
		in_y1 -= (out_y1 - output->get_h()) / h_scale;
		out_y1 = output->get_h();
	}

	if(out_x2 < 0)
	{
		in_x2 += -out_x2 / w_scale;
		out_x2 = 0;
	}
	else
	if(out_x2 >= output->get_w())
	{
		in_x2 -= (out_x2 - output->get_w()) / w_scale;
		out_x2 = output->get_w();
	}

	if(out_y2 < 0)
	{
		in_y2 += -out_y2 / h_scale;
		out_y2 = 0;
	}
	else
	if(out_y2 >= output->get_h())
	{
		in_y2 -= (out_y2 - output->get_h()) / h_scale;
		out_y2 = output->get_h();
	}










	float in_w = in_x2 - in_x1;
	float in_h = in_y2 - in_y1;
	float out_w = out_x2 - out_x1;
	float out_h = out_y2 - out_y1;
// Input for translation operation
	VFrame *translation_input = input;


	if(in_w <= 0 || in_h <= 0 || out_w <= 0 || out_h <= 0) return 0;


// printf("OverlayFrame::overlay 2 %f %f %f %f -> %f %f %f %f\n", in_x1,
// 			in_y1,
// 			in_x2,
// 			in_y2,
// 			out_x1,
// 			out_y1,
// 			out_x2,
// 			out_y2);





// ****************************************************************************
// Transfer to temp buffer by scaling nearest integer boundaries
// ****************************************************************************
	if(interpolation_type != NEAREST_NEIGHBOR &&
		(!EQUIV(w_scale, 1) || !EQUIV(h_scale, 1)))
	{
// Create integer boundaries for interpolation
		float in_x1_float = in_x1;
		float in_y1_float = in_y1;
		float in_x2_float = MIN(in_x2, input->get_w());
		float in_y2_float = MIN(in_y2, input->get_h());
		int out_x1_int = (int)out_x1;
		int out_y1_int = (int)out_y1;
		int out_x2_int = MIN((int)ceil(out_x2), output->get_w());
		int out_y2_int = MIN((int)ceil(out_y2), output->get_h());

// Dimensions of temp frame.  Integer boundaries scaled.
		int temp_w = (out_x2_int - out_x1_int);
		int temp_h = (out_y2_int - out_y1_int);
		VFrame *scale_output;



#define NO_TRANSLATION1 \
	(EQUIV(in_x1, 0) && \
	EQUIV(in_y1, 0) && \
	EQUIV(out_x1, 0) && \
	EQUIV(out_y1, 0) && \
	EQUIV(in_x2, in_x2_float) && \
	EQUIV(in_y2, in_y2_float) && \
	EQUIV(out_x2, temp_w) && \
	EQUIV(out_y2, temp_h))


#define NO_BLEND \
	(EQUIV(alpha, 1) && \
	(mode == TRANSFER_REPLACE || \
	(mode == TRANSFER_NORMAL && cmodel_components(input->get_color_model()) == 3)))





// Prepare destination for operation

// No translation and no blending.  The blending operation is built into the
// translation unit but not the scaling unit.
// input -> output
		if(NO_TRANSLATION1 &&
			NO_BLEND)
		{
// printf("OverlayFrame::overlay input -> output\n");

			scale_output = output;
			translation_input = 0;
		}
		else
// If translation or blending
// input -> nearest integer boundary temp
		{
			if(temp_frame && 
				(temp_frame->get_w() != temp_w ||
					temp_frame->get_h() != temp_h))
			{
				delete temp_frame;
				temp_frame = 0;
			}

			if(!temp_frame)
			{
				temp_frame = new VFrame(0,
					temp_w,
					temp_h,
					input->get_color_model(),
					-1);
			}
//printf("OverlayFrame::overlay input -> temp\n");


			temp_frame->clear_frame();

// printf("OverlayFrame::overlay 4 temp_w=%d temp_h=%d\n",
// 	temp_w, temp_h);
			scale_output = temp_frame;
			translation_input = scale_output;

// Adjust input coordinates to reflect new scaled coordinates.
			in_x1 = 0;
			in_y1 = 0;
			in_x2 = temp_w;
			in_y2 = temp_h;
		}



//printf("Overlay 1\n");

// Scale input -> scale_output
		if(!scale_engine) scale_engine = new ScaleEngine(this, cpus);
		scale_engine->scale_output = scale_output;
		scale_engine->scale_input = input;
		scale_engine->w_scale = w_scale;
		scale_engine->h_scale = h_scale;
		scale_engine->in_x1_float = in_x1_float;
		scale_engine->in_y1_float = in_y1_float;
		scale_engine->out_w_int = temp_w;
		scale_engine->out_h_int = temp_h;
		scale_engine->interpolation_type = interpolation_type;
//printf("Overlay 2\n");

//printf("OverlayFrame::overlay ScaleEngine 1 %d\n", out_h_int);
		scale_engine->process_packages();
//printf("OverlayFrame::overlay ScaleEngine 2\n");



	}

// printf("OverlayFrame::overlay 1  %.2f %.2f %.2f %.2f -> %.2f %.2f %.2f %.2f\n", 
// 	in_x1, 
// 	in_y1, 
// 	in_x2, 
// 	in_y2, 
// 	out_x1, 
// 	out_y1, 
// 	out_x2, 
// 	out_y2);





#define NO_TRANSLATION2 \
	(EQUIV(in_x1, 0) && \
	EQUIV(in_y1, 0) && \
	EQUIV(in_x2, translation_input->get_w()) && \
	EQUIV(in_y2, translation_input->get_h()) && \
	EQUIV(out_x1, 0) && \
	EQUIV(out_y1, 0) && \
	EQUIV(out_x2, output->get_w()) && \
	EQUIV(out_y2, output->get_h())) \

#define NO_SCALE \
	(EQUIV(out_x2 - out_x1, in_x2 - in_x1) && \
	EQUIV(out_y2 - out_y1, in_y2 - in_y1))

	


//printf("OverlayFrame::overlay 4 %d\n", mode);




	if(translation_input)
	{
// Direct copy
		if( NO_TRANSLATION2 &&
			NO_SCALE &&
			NO_BLEND)
		{
//printf("OverlayFrame::overlay direct copy\n");
			output->copy_from(translation_input);
		}
		else
// Blend only
		if( NO_TRANSLATION2 &&
			NO_SCALE)
		{
			if(!blend_engine) blend_engine = new BlendEngine(this, cpus);


			blend_engine->output = output;
			blend_engine->input = translation_input;
			blend_engine->alpha = alpha;
			blend_engine->mode = mode;

			blend_engine->process_packages();
		}
		else
// Scale and translate using nearest neighbor
// Translation is exactly on integer boundaries
		if(interpolation_type == NEAREST_NEIGHBOR ||
			EQUIV(in_x1, (int)in_x1) &&
			EQUIV(in_y1, (int)in_y1) &&
			EQUIV(in_x2, (int)in_x2) &&
			EQUIV(in_y2, (int)in_y2) &&

			EQUIV(out_x1, (int)out_x1) &&
			EQUIV(out_y1, (int)out_y1) &&
			EQUIV(out_x2, (int)out_x2) &&
			EQUIV(out_y2, (int)out_y2))
		{
//printf("OverlayFrame::overlay NEAREST_NEIGHBOR 1\n");
			if(!scaletranslate_engine) scaletranslate_engine = 
				new ScaleTranslateEngine(this, cpus);


			scaletranslate_engine->output = output;
			scaletranslate_engine->input = translation_input;
// Input for Scaletranslate is subpixel precise!
			scaletranslate_engine->in_x1 = in_x1;
			scaletranslate_engine->in_y1 = in_y1;
 			scaletranslate_engine->in_x2 = in_x2;
 			scaletranslate_engine->in_y2 = in_y2;
			scaletranslate_engine->out_x1 = (int)out_x1;
			scaletranslate_engine->out_y1 = (int)out_y1;
 			scaletranslate_engine->out_x2 = (int)out_x1 + (int)(out_x2 - out_x1);
 			scaletranslate_engine->out_y2 = (int)out_y1 + (int)(out_y2 - out_y1);
			scaletranslate_engine->alpha = alpha;
			scaletranslate_engine->mode = mode;

			scaletranslate_engine->process_packages();
		}
		else
// Fractional translation
		{
// Use fractional translation
// printf("OverlayFrame::overlay temp -> output  %.2f %.2f %.2f %.2f -> %.2f %.2f %.2f %.2f\n", 
// 	in_x1, 
// 	in_y1, 
// 	in_x2, 
// 	in_y2, 
// 	out_x1, 
// 	out_y1, 
// 	out_x2, 
// 	out_y2);

//printf("Overlay 3\n");
			if(!translate_engine) translate_engine = new TranslateEngine(this, cpus);
			translate_engine->translate_output = output;
			translate_engine->translate_input = translation_input;
			translate_engine->translate_in_x1 = in_x1;
			translate_engine->translate_in_y1 = in_y1;
			translate_engine->translate_in_x2 = in_x2;
			translate_engine->translate_in_y2 = in_y2;
			translate_engine->translate_out_x1 = out_x1;
			translate_engine->translate_out_y1 = out_y1;
			translate_engine->translate_out_x2 = out_x2;
			translate_engine->translate_out_y2 = out_y2;
			translate_engine->translate_alpha = alpha;
			translate_engine->translate_mode = mode;
//printf("Overlay 4\n");

//printf("OverlayFrame::overlay 5 %d\n", mode);
			translate_engine->process_packages();

		}
	}
//printf("OverlayFrame::overlay 2\n");

	return 0;
}







ScalePackage::ScalePackage()
{
}




ScaleUnit::ScaleUnit(ScaleEngine *server, OverlayFrame *overlay)
 : LoadClient(server)
{
	this->overlay = overlay;
	this->engine = server;
}

ScaleUnit::~ScaleUnit()
{
}



void ScaleUnit::tabulate_reduction(bilinear_table_t* &table,
	float scale,
	int in_pixel1, 
	int out_total,
	int in_total)
{
	table = new bilinear_table_t[out_total];
	bzero(table, sizeof(bilinear_table_t) * out_total);
//printf("ScaleUnit::tabulate_reduction 1 %f %d %d %d\n", scale, in_pixel1, out_total, in_total);
	for(int i = 0; i < out_total; i++)
	{
		float out_start = i;
		float in_start = out_start * scale;
		float out_end = i + 1;
		float in_end = out_end * scale;
		bilinear_table_t *entry = table + i;
//printf("ScaleUnit::tabulate_reduction 1 %f %f %f %f\n", out_start, out_end, in_start, in_end);

// Store input fraction.  Using scale to normalize these didn't work.
		entry->input_fraction1 = (floor(in_start + 1) - in_start) /* / scale */;
		entry->input_fraction2 = 1.0 /* / scale */;
		entry->input_fraction3 = (in_end - floor(in_end)) /* / scale */;

		if(in_end >= in_total - in_pixel1)
		{
			in_end = in_total - in_pixel1 - 1;
			
			int difference = (int)in_end - (int)in_start - 1;
			if(difference < 0) difference = 0;
			entry->input_fraction3 = 1.0 - 
				entry->input_fraction1 - 
				entry->input_fraction2 * difference;
		}

// Store input pixels
		entry->input_pixel1 = (int)in_start;
		entry->input_pixel2 = (int)in_end;

// Normalize for middle pixels
		if(entry->input_pixel2 > entry->input_pixel1 + 1)
		{
			float total = entry->input_fraction1 + 
				entry->input_fraction2 * 
				(entry->input_pixel2 - entry->input_pixel1 - 1) + 
				entry->input_fraction3;
			entry->input_fraction1 /= total;
			entry->input_fraction2 /= total;
			entry->input_fraction3 /= total;
		}
		else
		{
			float total = entry->input_fraction1 +
				entry->input_fraction3;
			entry->input_fraction1 /= total;
			entry->input_fraction3 /= total;
		}

// printf("ScaleUnit::tabulate_reduction 1 %d %d %d %f %f %f %f\n", 
// i,
// entry->input_pixel1, 
// entry->input_pixel2,
// entry->input_fraction1,
// entry->input_fraction2,
// entry->input_fraction3,
// entry->input_fraction1 + 
// 	entry->input_fraction2 * 
// 	(entry->input_pixel2 - entry->input_pixel1 - 1) + 
// 	entry->input_fraction3);


// Sanity check
		if(entry->input_pixel1 > entry->input_pixel2)
		{
			entry->input_pixel1 = entry->input_pixel2;
			entry->input_fraction1 = 0;
		}

// Get total fraction of output pixel used
//		if(entry->input_pixel2 > entry->input_pixel1)
		entry->total_fraction = 
			entry->input_fraction1 +
			entry->input_fraction2 * (entry->input_pixel2 - entry->input_pixel1 - 1) +
			entry->input_fraction3;
		entry->input_pixel1 += in_pixel1;
		entry->input_pixel2 += in_pixel1;
	}
}

void ScaleUnit::tabulate_enlarge(bilinear_table_t* &table,
	float scale,
	float in_pixel1, 
	int out_total,
	int in_total)
{
	table = new bilinear_table_t[out_total];
	bzero(table, sizeof(bilinear_table_t) * out_total);

	for(int i = 0; i < out_total; i++)
	{
		bilinear_table_t *entry = table + i;
		float in_pixel = i * scale + in_pixel1;
		entry->input_pixel1 = (int)floor(in_pixel);
		entry->input_pixel2 = entry->input_pixel1 + 1;

		if(in_pixel - in_pixel1 <= in_total)
		{
			entry->input_fraction3 = in_pixel - entry->input_pixel1;
		}
		else
		{
			entry->input_fraction3 = 0;
			entry->input_pixel2 = 0;
		}

		if(in_pixel - in_pixel1 >= 0)
		{
			entry->input_fraction1 = entry->input_pixel2 - in_pixel;
		}
		else
		{
			entry->input_fraction1 = 0;
			entry->input_pixel1 = (int)in_pixel1;
		}

		if(entry->input_pixel2 >= in_total)
		{
			entry->input_pixel2 = entry->input_pixel1;
			entry->input_fraction3 = 1.0 - entry->input_fraction1;
		}

		entry->total_fraction = 
			entry->input_fraction1 + 
			entry->input_fraction3;
// 
// printf("ScaleUnit::tabulate_enlarge %d %d %f %f %f\n",
// entry->input_pixel1,
// entry->input_pixel2,
// entry->input_fraction1,
// entry->input_fraction2,
// entry->input_fraction3);
	}
}

void ScaleUnit::dump_bilinear(bilinear_table_t *table, int total)
{
	printf("ScaleUnit::dump_bilinear\n");
	for(int i = 0; i < total; i++)
	{
		printf("out=%d inpixel1=%d inpixel2=%d infrac1=%f infrac2=%f infrac3=%f total=%f\n", 
			i,
			table[i].input_pixel1,
			table[i].input_pixel2,
			table[i].input_fraction1,
			table[i].input_fraction2,
			table[i].input_fraction3,
			table[i].total_fraction);
	}
}

#define PIXEL_REDUCE_MACRO(type, components, row) \
{ \
	type *input_row = &in_rows[row][x_entry->input_pixel1 * components]; \
	type *input_end = &in_rows[row][x_entry->input_pixel2 * components]; \
 \
/* Do first pixel */ \
	temp_f1 += input_scale1 * input_row[0]; \
	temp_f2 += input_scale1 * input_row[1]; \
	temp_f3 += input_scale1 * input_row[2]; \
	if(components == 4) temp_f4 += input_scale1 * input_row[3]; \
 \
/* Do last pixel */ \
/*	if(input_row < input_end) */\
	{ \
		temp_f1 += input_scale3 * input_end[0]; \
		temp_f2 += input_scale3 * input_end[1]; \
		temp_f3 += input_scale3 * input_end[2]; \
		if(components == 4) temp_f4 += input_scale3 * input_end[3]; \
	} \
 \
/* Do middle pixels */ \
	for(input_row += components; input_row < input_end; input_row += components) \
	{ \
		temp_f1 += input_scale2 * input_row[0]; \
		temp_f2 += input_scale2 * input_row[1]; \
		temp_f3 += input_scale2 * input_row[2]; \
		if(components == 4) temp_f4 += input_scale2 * input_row[3]; \
	} \
}

// Bilinear reduction and suboptimal enlargement.
// Very high quality.
#define BILINEAR_REDUCE(max, type, components) \
{ \
	bilinear_table_t *x_table, *y_table; \
	int out_h = pkg->out_row2 - pkg->out_row1; \
	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
 \
 	if(scale_w < 1) \
		tabulate_reduction(x_table, \
			1.0 / scale_w, \
			(int)in_x1_float, \
			out_w_int, \
			input->get_w()); \
	else \
		tabulate_enlarge(x_table, \
			1.0 / scale_w, \
			in_x1_float, \
			out_w_int, \
			input->get_w()); \
 \
 	if(scale_h < 1) \
		tabulate_reduction(y_table, \
			1.0 / scale_h, \
			(int)in_y1_float, \
			out_h_int, \
			input->get_h()); \
	else \
		tabulate_enlarge(y_table, \
			1.0 / scale_h, \
			in_y1_float, \
			out_h_int, \
			input->get_h()); \
/* dump_bilinear(y_table, out_h_int); */\
 \
 	for(int i = 0; i < out_h; i++) \
	{ \
		type *out_row = out_rows[i + pkg->out_row1]; \
		bilinear_table_t *y_entry = &y_table[i + pkg->out_row1]; \
/* printf("BILINEAR_REDUCE 2 %d %d %d %f %f %f\n", */ \
/* i, */ \
/* y_entry->input_pixel1, */ \
/* y_entry->input_pixel2, */ \
/* y_entry->input_fraction1, */ \
/* y_entry->input_fraction2, */ \
/* y_entry->input_fraction3); */ \
 \
		for(int j = 0; j < out_w_int; j++) \
		{ \
			bilinear_table_t *x_entry = &x_table[j]; \
/* Load rounding factors */ \
			float temp_f1; \
			float temp_f2; \
			float temp_f3; \
			float temp_f4; \
			if(sizeof(type) != 4) \
				temp_f1 = temp_f2 = temp_f3 = temp_f4 = .5; \
			else \
				temp_f1 = temp_f2 = temp_f3 = temp_f4 = 0; \
 \
/* First row */ \
			float input_scale1 = y_entry->input_fraction1 * x_entry->input_fraction1; \
			float input_scale2 = y_entry->input_fraction1 * x_entry->input_fraction2; \
			float input_scale3 = y_entry->input_fraction1 * x_entry->input_fraction3; \
			PIXEL_REDUCE_MACRO(type, components, y_entry->input_pixel1) \
 \
/* Last row */ \
			if(out_h) \
			{ \
				input_scale1 = y_entry->input_fraction3 * x_entry->input_fraction1; \
				input_scale2 = y_entry->input_fraction3 * x_entry->input_fraction2; \
				input_scale3 = y_entry->input_fraction3 * x_entry->input_fraction3; \
				PIXEL_REDUCE_MACRO(type, components, y_entry->input_pixel2) \
 \
/* Middle rows */ \
				if(out_h > 1) \
				{ \
					input_scale1 = y_entry->input_fraction2 * x_entry->input_fraction1; \
					input_scale2 = y_entry->input_fraction2 * x_entry->input_fraction2; \
					input_scale3 = y_entry->input_fraction2 * x_entry->input_fraction3; \
					for(int k = y_entry->input_pixel1 + 1; \
						k < y_entry->input_pixel2; \
						k++) \
					{ \
						PIXEL_REDUCE_MACRO(type, components, k) \
					} \
				} \
			} \
 \
 \
  			if(max != 1.0) \
			{ \
				if(temp_f1 > max) temp_f1 = max; \
				if(temp_f2 > max) temp_f2 = max; \
				if(temp_f3 > max) temp_f3 = max; \
				if(components == 4) if(temp_f4 > max) temp_f4 = max; \
			} \
 \
			out_row[j * components    ] = (type)temp_f1; \
			out_row[j * components + 1] = (type)temp_f2; \
			out_row[j * components + 2] = (type)temp_f3; \
			if(components == 4) out_row[j * components + 3] = (type)temp_f4; \
		} \
/*printf("BILINEAR_REDUCE 3 %d\n", i);*/ \
	} \
 \
	delete [] x_table; \
	delete [] y_table; \
}



// Only 2 input pixels
#define BILINEAR_ENLARGE(max, type, components) \
{ \
/*printf("BILINEAR_ENLARGE 1\n");*/ \
	float k_y = 1.0 / scale_h; \
	float k_x = 1.0 / scale_w; \
	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
	int out_h = pkg->out_row2 - pkg->out_row1; \
	int in_h_int = input->get_h(); \
	int in_w_int = input->get_w(); \
	int *table_int_x1, *table_int_y1; \
	int *table_int_x2, *table_int_y2; \
	float *table_frac_x_f, *table_antifrac_x_f, *table_frac_y_f, *table_antifrac_y_f; \
	int *table_frac_x_i, *table_antifrac_x_i, *table_frac_y_i, *table_antifrac_y_i; \
 \
 	tabulate_blinear_f(table_int_x1,  \
		table_int_x2,  \
		table_frac_x_f,  \
		table_antifrac_x_f,  \
		k_x,  \
		0,  \
		out_w_int, \
		in_x1_float,  \
		in_w_int); \
 	tabulate_blinear_f(table_int_y1,  \
		table_int_y2,  \
		table_frac_y_f,  \
		table_antifrac_y_f,  \
		k_y,  \
		pkg->out_row1,  \
		pkg->out_row2,  \
		in_y1_float, \
		in_h_int); \
 \
	for(int i = 0; i < out_h; i++) \
	{ \
		int i_y1 = table_int_y1[i]; \
		int i_y2 = table_int_y2[i]; \
		float a_f; \
        float anti_a_f; \
		uint64_t a_i; \
        uint64_t anti_a_i; \
		a_f = table_frac_y_f[i]; \
        anti_a_f = table_antifrac_y_f[i]; \
		type *in_row1 = in_rows[i_y1]; \
		type *in_row2 = in_rows[i_y2]; \
		type *out_row = out_rows[i + pkg->out_row1]; \
 \
		for(int j = 0; j < out_w_int; j++) \
		{ \
			int i_x1 = table_int_x1[j]; \
			int i_x2 = table_int_x2[j]; \
			float output1r, output1g, output1b, output1a; \
			float output2r, output2g, output2b, output2a; \
			float output3r, output3g, output3b, output3a; \
			float output4r, output4g, output4b, output4a; \
			float b_f; \
			float anti_b_f; \
			b_f = table_frac_x_f[j]; \
			anti_b_f = table_antifrac_x_f[j]; \
\
    		output1r = in_row1[i_x1 * components]; \
    		output1g = in_row1[i_x1 * components + 1]; \
    		output1b = in_row1[i_x1 * components + 2]; \
    		if(components == 4) output1a = in_row1[i_x1 * components + 3]; \
\
    		output2r = in_row1[i_x2 * components]; \
    		output2g = in_row1[i_x2 * components + 1]; \
    		output2b = in_row1[i_x2 * components + 2]; \
    		if(components == 4) output2a = in_row1[i_x2 * components + 3]; \
\
    		output3r = in_row2[i_x1 * components]; \
    		output3g = in_row2[i_x1 * components + 1]; \
    		output3b = in_row2[i_x1 * components + 2]; \
    		if(components == 4) output3a = in_row2[i_x1 * components + 3]; \
\
    		output4r = in_row2[i_x2 * components]; \
    		output4g = in_row2[i_x2 * components + 1]; \
    		output4b = in_row2[i_x2 * components + 2]; \
    		if(components == 4) output4a = in_row2[i_x2 * components + 3]; \
\
			out_row[j * components] =  \
				(type)(anti_a_f * (anti_b_f * output1r +  \
				b_f * output2r) +  \
                a_f * (anti_b_f * output3r +  \
				b_f * output4r)); \
			out_row[j * components + 1] =   \
				(type)(anti_a_f * (anti_b_f * output1g +  \
				b_f * output2g) +  \
                a_f * ((anti_b_f * output3g) +  \
				b_f * output4g)); \
			out_row[j * components + 2] =   \
				(type)(anti_a_f * ((anti_b_f * output1b) +  \
				(b_f * output2b)) +  \
                a_f * ((anti_b_f * output3b) +  \
				b_f * output4b)); \
			if(components == 4) \
				out_row[j * components + 3] =   \
					(type)(anti_a_f * ((anti_b_f * output1a) +  \
					(b_f * output2a)) +  \
                	a_f * ((anti_b_f * output3a) +  \
					b_f * output4a)); \
		} \
	} \
 \
 \
	delete [] table_int_x1; \
	delete [] table_int_x2; \
	delete [] table_int_y1; \
	delete [] table_int_y2; \
	delete [] table_frac_x_f; \
	delete [] table_antifrac_x_f; \
	delete [] table_frac_y_f; \
	delete [] table_antifrac_y_f; \
 \
/*printf("BILINEAR_ENLARGE 2\n");*/ \
}


#define BICUBIC(max, type, components) \
{ \
	float k_y = 1.0 / scale_h; \
	float k_x = 1.0 / scale_w; \
	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
	float *bspline_x_f, *bspline_y_f; \
	int *bspline_x_i, *bspline_y_i; \
	int *in_x_table, *in_y_table; \
	int in_h_int = input->get_h(); \
	int in_w_int = input->get_w(); \
 \
	tabulate_bcubic_f(bspline_x_f,  \
		in_x_table, \
		k_x, \
		in_x1_float, \
		out_w_int, \
		in_w_int, \
		-1); \
 \
	tabulate_bcubic_f(bspline_y_f,  \
		in_y_table, \
		k_y, \
		in_y1_float, \
		out_h_int, \
		in_h_int, \
		1); \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		for(int j = 0; j < out_w_int; j++) \
		{ \
			int i_x = (int)(k_x * j); \
			float output1_f, output2_f, output3_f, output4_f; \
			uint64_t output1_i, output2_i, output3_i, output4_i; \
			output1_f = 0; \
			output2_f = 0; \
			output3_f = 0; \
			if(components == 4) \
				output4_f = 0; \
			int table_y = i * 4; \
 \
/* Kernel */ \
			for(int m = -1; m < 3; m++) \
			{ \
				float r1_f; \
				uint64_t r1_i; \
				r1_f = bspline_y_f[table_y]; \
				int y = in_y_table[table_y]; \
				int table_x = j * 4; \
 \
				for(int n = -1; n < 3; n++) \
				{ \
					float r2_f; \
					uint64_t r2_i; \
					r2_f = bspline_x_f[table_x]; \
					int x = in_x_table[table_x]; \
					float r_square_f; \
					uint64_t r_square_i; \
					r_square_f = r1_f * r2_f; \
					output1_f += r_square_f * in_rows[y][x * components]; \
					output2_f += r_square_f * in_rows[y][x * components + 1]; \
					output3_f += r_square_f * in_rows[y][x * components + 2]; \
					if(components == 4) \
						output4_f += r_square_f * in_rows[y][x * components + 3]; \
 \
					table_x++; \
				} \
				table_y++; \
			} \
 \
 \
			out_rows[i][j * components] = (type)output1_f; \
			out_rows[i][j * components + 1] = (type)output2_f; \
			out_rows[i][j * components + 2] = (type)output3_f; \
			if(components == 4) \
				out_rows[i][j * components + 3] = (type)output4_f; \
 \
		} \
	} \
 \
	delete [] bspline_x_f; \
	delete [] bspline_y_f; \
	delete [] in_x_table; \
	delete [] in_y_table; \
}




// Pow function is not thread safe in Compaqt C
#define CUBE(x) ((x) * (x) * (x))

float ScaleUnit::cubic_bspline(float x)
{
	float a, b, c, d;

	if((x + 2.0F) <= 0.0F) 
	{
    	a = 0.0F;
	}
	else 
	{
    	a = CUBE(x + 2.0F);
	}


	if((x + 1.0F) <= 0.0F) 
	{
    	b = 0.0F;
	}
	else 
	{
    	b = CUBE(x + 1.0F);
	}    

	if(x <= 0) 
	{
    	c = 0.0F;
	}
	else 
	{
    	c = CUBE(x);
	}  

	if((x - 1.0F) <= 0.0F) 
	{
    	d = 0.0F;
	}
	else 
	{
    	d = CUBE(x - 1.0F);
	}


	return (a - (4.0F * b) + (6.0F * c) - (4.0F * d)) / 6.0;
}


void ScaleUnit::tabulate_bcubic_f(float* &coef_table, 
	int* &coord_table,
	float scale,
	float start, 
	int pixels,
	int total_pixels,
	float coefficient)
{
	coef_table = new float[pixels * 4];
	coord_table = new int[pixels * 4];
	for(int i = 0, j = 0; i < pixels; i++)
	{
		float f_x = (float)i * scale + start;
		float a = f_x - floor(f_x);
		
		for(float m = -1; m < 3; m++)
		{
			coef_table[j] = cubic_bspline(coefficient * (m - a));
			coord_table[j] = (int)(f_x + m);
			CLAMP(coord_table[j], 0, total_pixels - 1);
			j++;
		}
		
	}
}

void ScaleUnit::tabulate_bcubic_i(int* &coef_table, 
	int* &coord_table,
	float scale,
	int start, 
	int pixels,
	int total_pixels,
	float coefficient)
{
	coef_table = new int[pixels * 4];
	coord_table = new int[pixels * 4];
	for(int i = 0, j = 0; i < pixels; i++)
	{
		float f_x = (float)i * scale + start;
		float a = f_x - floor(f_x);
		
		for(float m = -1; m < 3; m++)
		{
			coef_table[j] = (int)(cubic_bspline(coefficient * (m - a)) * 0x10000);
			coord_table[j] = (int)(f_x + m);
			CLAMP(coord_table[j], 0, total_pixels - 1);
			j++;
		}
		
	}
}

void ScaleUnit::tabulate_blinear_f(int* &table_int1,
		int* &table_int2,
		float* &table_frac,
		float* &table_antifrac,
		float scale,
		int pixel1,
		int pixel2,
		float start,
		int total_pixels)
{
	table_int1 = new int[pixel2 - pixel1];
	table_int2 = new int[pixel2 - pixel1];
	table_frac = new float[pixel2 - pixel1];
	table_antifrac = new float[pixel2 - pixel1];

	for(int i = pixel1, j = 0; i < pixel2; i++, j++)
	{
		float f_x = (float)i * scale + start;
		int i_x = (int)floor(f_x);
		float a = (f_x - floor(f_x));

		table_int1[j] = i_x;
		table_int2[j] = i_x + 1;
		CLAMP(table_int1[j], 0, total_pixels - 1);
		CLAMP(table_int2[j], 0, total_pixels - 1);
		table_frac[j] = a;
		table_antifrac[j] = 1.0F - a;
//printf("ScaleUnit::tabulate_blinear %d %d %d\n", j, table_int1[j], table_int2[j]);
	}
}

void ScaleUnit::tabulate_blinear_i(int* &table_int1,
		int* &table_int2,
		int* &table_frac,
		int* &table_antifrac,
		float scale,
		int pixel1,
		int pixel2,
		float start,
		int total_pixels)
{
	table_int1 = new int[pixel2 - pixel1];
	table_int2 = new int[pixel2 - pixel1];
	table_frac = new int[pixel2 - pixel1];
	table_antifrac = new int[pixel2 - pixel1];

	for(int i = pixel1, j = 0; i < pixel2; i++, j++)
	{
		double f_x = (float)i * scale + start;
		int i_x = (int)floor(f_x);
		float a = (f_x - floor(f_x));

		table_int1[j] = i_x;
		table_int2[j] = i_x + 1;
		CLAMP(table_int1[j], 0, total_pixels - 1);
		CLAMP(table_int2[j], 0, total_pixels - 1);
		table_frac[j] = (int)(a * 0xffff);
		table_antifrac[j] = (int)((1.0F - a) * 0x10000);
//printf("ScaleUnit::tabulate_blinear %d %d %d\n", j, table_int1[j], table_int2[j]);
	}
}

void ScaleUnit::process_package(LoadPackage *package)
{
	ScalePackage *pkg = (ScalePackage*)package;

//printf("ScaleUnit::process_package 1\n");
// Arguments for macros
	VFrame *output = engine->scale_output;
	VFrame *input = engine->scale_input;
	float scale_w = engine->w_scale;
	float scale_h = engine->h_scale;
	float in_x1_float = engine->in_x1_float;
	float in_y1_float = engine->in_y1_float;
	int out_h_int = engine->out_h_int;
	int out_w_int = engine->out_w_int;
	int do_yuv = 
		(input->get_color_model() == BC_YUV888 ||
		input->get_color_model() == BC_YUVA8888 ||
		input->get_color_model() == BC_YUV161616 ||
		input->get_color_model() == BC_YUVA16161616);

//printf("ScaleUnit::process_package 2 %f %f\n", engine->w_scale, engine->h_scale);
	if(engine->interpolation_type == CUBIC_CUBIC || 
		(engine->interpolation_type == CUBIC_LINEAR 
			&& engine->w_scale > 1 && 
			engine->h_scale > 1))
	{
		switch(engine->scale_input->get_color_model())
		{
			case BC_RGB_FLOAT:
				BICUBIC(1.0, float, 3);
				break;

			case BC_RGBA_FLOAT:
				BICUBIC(1.0, float, 4);
				break;

			case BC_RGB888:
			case BC_YUV888:
				BICUBIC(0xff, unsigned char, 3);
				break;

			case BC_RGBA8888:
			case BC_YUVA8888:
				BICUBIC(0xff, unsigned char, 4);
				break;

			case BC_RGB161616:
			case BC_YUV161616:
				BICUBIC(0xffff, uint16_t, 3);
				break;

			case BC_RGBA16161616:
			case BC_YUVA16161616:
				BICUBIC(0xffff, uint16_t, 4);
				break;
		}
	}
	else
// Perform bilinear scaling input -> scale_output
	if(engine->w_scale > 1 && 
		engine->h_scale > 1)
	{
		switch(engine->scale_input->get_color_model())
		{
			case BC_RGB_FLOAT:
				BILINEAR_ENLARGE(1.0, float, 3);
				break;

			case BC_RGBA_FLOAT:
				BILINEAR_ENLARGE(1.0, float, 4);
				break;

			case BC_RGB888:
			case BC_YUV888:
				BILINEAR_ENLARGE(0xff, unsigned char, 3);
				break;

			case BC_RGBA8888:
			case BC_YUVA8888:
				BILINEAR_ENLARGE(0xff, unsigned char, 4);
				break;

			case BC_RGB161616:
			case BC_YUV161616:
				BILINEAR_ENLARGE(0xffff, uint16_t, 3);
				break;

			case BC_RGBA16161616:
			case BC_YUVA16161616:
				BILINEAR_ENLARGE(0xffff, uint16_t, 4);
				break;
		}
	}
	else
// Bilinear reduction
	{
		switch(engine->scale_input->get_color_model())
		{
			case BC_RGB_FLOAT:
				BILINEAR_REDUCE(1.0, float, 3);
				break;
			case BC_RGBA_FLOAT:
				BILINEAR_REDUCE(1.0, float, 4);
				break;
			case BC_RGB888:
			case BC_YUV888:
				BILINEAR_REDUCE(0xff, unsigned char, 3);
				break;

			case BC_RGBA8888:
			case BC_YUVA8888:
				BILINEAR_REDUCE(0xff, unsigned char, 4);
				break;

			case BC_RGB161616:
			case BC_YUV161616:
				BILINEAR_REDUCE(0xffff, uint16_t, 3);
				break;

			case BC_RGBA16161616:
			case BC_YUVA16161616:
				BILINEAR_REDUCE(0xffff, uint16_t, 4);
				break;
		}
	}
//printf("ScaleUnit::process_package 3\n");

}













ScaleEngine::ScaleEngine(OverlayFrame *overlay, int cpus)
 : LoadServer(cpus, cpus)
{
	this->overlay = overlay;
}

ScaleEngine::~ScaleEngine()
{
}

void ScaleEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		ScalePackage *package = (ScalePackage*)get_package(i);
		package->out_row1 = out_h_int / get_total_packages() * i;
		package->out_row2 = package->out_row1 + out_h_int / get_total_packages();

		if(i >= get_total_packages() - 1)
			package->out_row2 = out_h_int;
	}
}

LoadClient* ScaleEngine::new_client()
{
	return new ScaleUnit(this, overlay);
}

LoadPackage* ScaleEngine::new_package()
{
	return new ScalePackage;
}













TranslatePackage::TranslatePackage()
{
}



TranslateUnit::TranslateUnit(TranslateEngine *server, OverlayFrame *overlay)
 : LoadClient(server)
{
	this->overlay = overlay;
	this->engine = server;
}

TranslateUnit::~TranslateUnit()
{
}



void TranslateUnit::translation_array_f(transfer_table_f* &table, 
	float out_x1, 
	float out_x2,
	float in_x1,
	float in_x2,
	int in_total, 
	int out_total, 
	int &out_x1_int,
	int &out_x2_int)
{
	int out_w_int;
	float offset = out_x1 - in_x1;
//printf("OverlayFrame::translation_array_f 1 %f %f -> %f %f\n", in_x1, in_x2, out_x1, out_x2);

	out_x1_int = (int)out_x1;
	out_x2_int = MIN((int)ceil(out_x2), out_total);
	out_w_int = out_x2_int - out_x1_int;

	table = new transfer_table_f[out_w_int];
	bzero(table, sizeof(transfer_table_f) * out_w_int);


// printf("OverlayFrame::translation_array_f 2 %f %f -> %f %f scale=%f %f\n", 
// in_x1, 
// in_x2, 
// out_x1, 
// out_x2,
// in_x2 - in_x1,
// out_x2 - out_x1);
// 

	float in_x = in_x1;
	for(int out_x = out_x1_int; out_x < out_x2_int; out_x++)
	{
		transfer_table_f *entry = &table[out_x - out_x1_int];

		entry->in_x1 = (int)in_x;
		entry->in_x2 = (int)in_x + 1;

// Get fraction of output pixel to fill
		entry->output_fraction = 1;

		if(out_x1 > out_x)
		{
			entry->output_fraction -= out_x1 - out_x;
		}

		if(out_x2 < out_x + 1)
		{
			entry->output_fraction = (out_x2 - out_x);
		}

// Advance in_x until out_x_fraction is filled
		float out_x_fraction = entry->output_fraction;
		float in_x_fraction = floor(in_x + 1) - in_x;

		if(out_x_fraction <= in_x_fraction)
		{
			entry->in_fraction1 = out_x_fraction;
			entry->in_fraction2 = 0.0;
			in_x += out_x_fraction;
		}
		else
		{
			entry->in_fraction1 = in_x_fraction;
			in_x += out_x_fraction;
			entry->in_fraction2 = in_x - floor(in_x);
		}

// Clip in_x and zero out fraction.  This doesn't work for YUV.
		if(entry->in_x2 >= in_total)
		{
			entry->in_x2 = in_total - 1;
			entry->in_fraction2 = 0.0;
		}
		
		if(entry->in_x1 >= in_total)
		{
			entry->in_x1 = in_total - 1;
			entry->in_fraction1 = 0.0;
		}
// printf("OverlayFrame::translation_array_f 2 %d %d %d %f %f %f\n", 
// 	out_x, 
// 	entry->in_x1, 
// 	entry->in_x2, 
// 	entry->in_fraction1, 
// 	entry->in_fraction2, 
// 	entry->output_fraction);
 	}
}


void TranslateUnit::translation_array_i(transfer_table_i* &table, 
	float out_x1, 
	float out_x2,
	float in_x1,
	float in_x2,
	int in_total, 
	int out_total, 
	int &out_x1_int,
	int &out_x2_int)
{
	int out_w_int;
	float offset = out_x1 - in_x1;

	out_x1_int = (int)out_x1;
	out_x2_int = MIN((int)ceil(out_x2), out_total);
	out_w_int = out_x2_int - out_x1_int;

	table = new transfer_table_i[out_w_int];
	bzero(table, sizeof(transfer_table_i) * out_w_int);


//printf("OverlayFrame::translation_array_f 1 %f %f -> %f %f\n", in_x1, in_x2, out_x1, out_x2);

	float in_x = in_x1;
	for(int out_x = out_x1_int; out_x < out_x2_int; out_x++)
	{
		transfer_table_i *entry = &table[out_x - out_x1_int];

		entry->in_x1 = (int)in_x;
		entry->in_x2 = (int)in_x + 1;

// Get fraction of output pixel to fill
		entry->output_fraction = 0x10000;

		if(out_x1 > out_x)
		{
			entry->output_fraction -= (int)((out_x1 - out_x) * 0x10000);
		}

		if(out_x2 < out_x + 1)
		{
			entry->output_fraction = (int)((out_x2 - out_x) * 0x10000);
		}

// Advance in_x until out_x_fraction is filled
		int out_x_fraction = entry->output_fraction;
		int in_x_fraction = (int)((floor(in_x + 1) - in_x) * 0x10000);

		if(out_x_fraction <= in_x_fraction)
		{
			entry->in_fraction1 = out_x_fraction;
			entry->in_fraction2 = 0;
			in_x += (float)out_x_fraction / 0x10000;
		}
		else
		{
			entry->in_fraction1 = in_x_fraction;
			in_x += (float)out_x_fraction / 0x10000;
			entry->in_fraction2 = (int)((in_x - floor(in_x)) * 0x10000);
		}

// Clip in_x and zero out fraction.  This doesn't work for YUV.
		if(entry->in_x2 >= in_total)
		{
			entry->in_x2 = in_total - 1;
			entry->in_fraction2 = 0;
		}

		if(entry->in_x1 >= in_total)
		{
			entry->in_x1 = in_total - 1;
			entry->in_fraction1 = 0;
		}
// printf("OverlayFrame::translation_array_f 2 %d %d %d %f %f %f\n", 
// 	out_x, 
// 	entry->in_x1, 
// 	entry->in_x2, 
// 	entry->in_fraction1, 
// 	entry->in_fraction2, 
// 	entry->output_fraction);
 	}
}


































#define TRANSLATE(max, temp_type, type, components, chroma_offset) \
{ \
 \
	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
 \
 \
	temp_type master_opacity; \
	if(sizeof(type) != 4) \
		master_opacity = (temp_type)(alpha * max + 0.5); \
	else \
		master_opacity = (temp_type)(alpha * max); \
	temp_type master_transparency = max - master_opacity; \
	float round = 0.0; \
	if(sizeof(type) != 4) \
		round = 0.5; \
 \
 \
	for(int i = row1; i < row2; i++) \
	{ \
		int in_y1; \
		int in_y2; \
		float y_fraction1_f; \
		float y_fraction2_f; \
		float y_output_fraction_f; \
		in_y1 = y_table_f[i - out_y1_int].in_x1; \
		in_y2 = y_table_f[i - out_y1_int].in_x2; \
		y_fraction1_f = y_table_f[i - out_y1_int].in_fraction1; \
		y_fraction2_f = y_table_f[i - out_y1_int].in_fraction2; \
		y_output_fraction_f = y_table_f[i - out_y1_int].output_fraction; \
		type *in_row1 = in_rows[(in_y1)]; \
		type *in_row2 = in_rows[(in_y2)]; \
		type *out_row = out_rows[i]; \
 \
		for(int j = out_x1_int; j < out_x2_int; j++) \
		{ \
			int in_x1; \
			int in_x2; \
			float x_fraction1_f; \
			float x_fraction2_f; \
			float x_output_fraction_f; \
			in_x1 = x_table_f[j - out_x1_int].in_x1; \
			in_x2 = x_table_f[j - out_x1_int].in_x2; \
			x_fraction1_f = x_table_f[j - out_x1_int].in_fraction1; \
			x_fraction2_f = x_table_f[j - out_x1_int].in_fraction2; \
			x_output_fraction_f = x_table_f[j - out_x1_int].output_fraction; \
			type *output = &out_row[j * components]; \
			temp_type input1, input2, input3, input4; \
 \
			float fraction1 = x_fraction1_f * y_fraction1_f; \
			float fraction2 = x_fraction2_f * y_fraction1_f; \
			float fraction3 = x_fraction1_f * y_fraction2_f; \
			float fraction4 = x_fraction2_f * y_fraction2_f; \
 \
			input1 = (type)(in_row1[in_x1 * components] * fraction1 +  \
				in_row1[in_x2 * components] * fraction2 +  \
				in_row2[in_x1 * components] * fraction3 +  \
				in_row2[in_x2 * components] * fraction4 + round); \
 \
/* Add chroma to fractional pixels */ \
 			if(chroma_offset) \
			{ \
				float extra_chroma = (1.0F - \
					fraction1 - \
					fraction2 - \
					fraction3 - \
					fraction4) * chroma_offset; \
				input2 = (type)(in_row1[in_x1 * components + 1] * fraction1 +  \
					in_row1[in_x2 * components + 1] * fraction2 +  \
					in_row2[in_x1 * components + 1] * fraction3 +  \
					in_row2[in_x2 * components + 1] * fraction4 + \
					extra_chroma + round); \
				input3 = (type)(in_row1[in_x1 * components + 2] * fraction1 +  \
					in_row1[in_x2 * components + 2] * fraction2 +  \
					in_row2[in_x1 * components + 2] * fraction3 +  \
					in_row2[in_x2 * components + 2] * fraction4 +  \
					extra_chroma + round); \
			} \
			else \
			{ \
				input2 = (type)(in_row1[in_x1 * components + 1] * fraction1 +  \
					in_row1[in_x2 * components + 1] * fraction2 +  \
					in_row2[in_x1 * components + 1] * fraction3 +  \
					in_row2[in_x2 * components + 1] * fraction4 + round); \
				input3 = (type)(in_row1[in_x1 * components + 2] * fraction1 +  \
					in_row1[in_x2 * components + 2] * fraction2 +  \
					in_row2[in_x1 * components + 2] * fraction3 +  \
					in_row2[in_x2 * components + 2] * fraction4 + round); \
			} \
 \
			if(components == 4) \
				input4 = (type)(in_row1[in_x1 * components + 3] * fraction1 +  \
					in_row1[in_x2 * components + 3] * fraction2 +  \
					in_row2[in_x1 * components + 3] * fraction3 +  \
					in_row2[in_x2 * components + 3] * fraction4 + round); \
 \
			temp_type opacity; \
			if(sizeof(type) != 4) \
				opacity = (temp_type)(master_opacity *  \
					y_output_fraction_f *  \
					x_output_fraction_f + 0.5); \
			else \
				opacity = (temp_type)(master_opacity *  \
					y_output_fraction_f *  \
					x_output_fraction_f); \
			temp_type transparency = max - opacity; \
 \
/* printf("TRANSLATE 2 %x %d %d\n", opacity, j, i); */ \
 \
			if(components == 3) \
			{ \
				BLEND_3(max, temp_type, type, chroma_offset); \
			} \
			else \
			{ \
				BLEND_4(max, temp_type, type, chroma_offset); \
			} \
		} \
	} \
}

void TranslateUnit::process_package(LoadPackage *package)
{
	TranslatePackage *pkg = (TranslatePackage*)package;
	int out_y1_int; 
	int out_y2_int; 
	int out_x1_int; 
	int out_x2_int; 


// Variables for TRANSLATE
	VFrame *input = engine->translate_input;
	VFrame *output = engine->translate_output;
	float in_x1 = engine->translate_in_x1;
	float in_y1 = engine->translate_in_y1;
	float in_x2 = engine->translate_in_x2;
	float in_y2 = engine->translate_in_y2;
	float out_x1 = engine->translate_out_x1;
	float out_y1 = engine->translate_out_y1;
	float out_x2 = engine->translate_out_x2;
	float out_y2 = engine->translate_out_y2;
	float alpha = engine->translate_alpha;
	int row1 = pkg->out_row1;
	int row2 = pkg->out_row2;
	int mode = engine->translate_mode;
	int in_total_x = input->get_w();
	int in_total_y = input->get_h();
	int do_yuv = 
		(engine->translate_input->get_color_model() == BC_YUV888 ||
		engine->translate_input->get_color_model() == BC_YUVA8888 ||
		engine->translate_input->get_color_model() == BC_YUV161616 ||
		engine->translate_input->get_color_model() == BC_YUVA16161616);

	transfer_table_f *x_table_f; 
	transfer_table_f *y_table_f; 
	transfer_table_i *x_table_i; 
	transfer_table_i *y_table_i; 

	translation_array_f(x_table_f,  
		out_x1,  
		out_x2, 
		in_x1, 
		in_x2, 
		in_total_x,  
		output->get_w(),  
		out_x1_int, 
		out_x2_int); 
	translation_array_f(y_table_f,  
		out_y1,  
		out_y2, 
		in_y1, 
		in_y2, 
		in_total_y,  
		output->get_h(),  
		out_y1_int, 
		out_y2_int); 
//	printf("TranslateUnit::process_package 1 %d\n", mode);
//	Timer a;
//	a.update();

	switch(engine->translate_input->get_color_model())
	{
		case BC_RGB888:
			TRANSLATE(0xff, uint32_t, unsigned char, 3, 0);
			break;

		case BC_RGBA8888:
			TRANSLATE(0xff, uint32_t, unsigned char, 4, 0);
			break;

		case BC_RGB_FLOAT:
			TRANSLATE(1.0, float, float, 3, 0);
			break;

		case BC_RGBA_FLOAT:
			TRANSLATE(1.0, float, float, 4, 0);
			break;

		case BC_RGB161616:
			TRANSLATE(0xffff, uint64_t, uint16_t, 3, 0);
			break;

		case BC_RGBA16161616:
			TRANSLATE(0xffff, uint64_t, uint16_t, 4, 0);
			break;

		case BC_YUV888:
			TRANSLATE(0xff, int32_t, unsigned char, 3, 0x80);
			break;

		case BC_YUVA8888:
			TRANSLATE(0xff, int32_t, unsigned char, 4, 0x80);
			break;

		case BC_YUV161616:
			TRANSLATE(0xffff, int64_t, uint16_t, 3, 0x8000);
			break;

		case BC_YUVA16161616:
			TRANSLATE(0xffff, int64_t, uint16_t, 4, 0x8000);
			break;
	}
//	printf("blend mode %i, took %li ms\n", mode, a.get_difference());

	delete [] x_table_f; 
	delete [] y_table_f; 
}










TranslateEngine::TranslateEngine(OverlayFrame *overlay, int cpus)
 : LoadServer(cpus, cpus)
{
	this->overlay = overlay;
}

TranslateEngine::~TranslateEngine()
{
}

void TranslateEngine::init_packages()
{
	int out_y1_int = (int)translate_out_y1;
	int out_y2_int = MIN((int)ceil(translate_out_y2), translate_output->get_h());
	int out_h = out_y2_int - out_y1_int;

	for(int i = 0; i < get_total_packages(); i++)
	{
		TranslatePackage *package = (TranslatePackage*)get_package(i);
		package->out_row1 = (int)(out_y1_int + out_h / 
			get_total_packages() * 
			i);
		package->out_row2 = (int)((float)package->out_row1 + 
			out_h / 
			get_total_packages());
		if(i >= get_total_packages() - 1)
			package->out_row2 = out_y2_int;
	}
}

LoadClient* TranslateEngine::new_client()
{
	return new TranslateUnit(this, overlay);
}

LoadPackage* TranslateEngine::new_package()
{
	return new TranslatePackage;
}








#define SCALE_TRANSLATE(max, temp_type, type, components, chroma_offset) \
{ \
	temp_type opacity; \
	if(sizeof(type) != 4) \
		opacity = (temp_type)(alpha * max + 0.5); \
	else \
		opacity = (temp_type)(alpha * max); \
	temp_type transparency = max - opacity; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		int in_y = y_table[i - out_y1]; \
		type *in_row = (type*)in_rows[in_y]; \
		type *output = (type*)out_rows[i] + out_x1 * components; \
 \
/* X direction is scaled and requires a table lookup */ \
 		if(out_w != in_x2 - in_x1) \
		{ \
			for(int j = 0; j < out_w; j++) \
			{ \
				type *in_row_plus_x = in_row + x_table[j] * components; \
				temp_type input1, input2, input3, input4; \
	 \
 				input1 = in_row_plus_x[0]; \
				input2 = in_row_plus_x[1]; \
				input3 = in_row_plus_x[2]; \
				if(components == 4) \
					input4 = in_row_plus_x[3]; \
	 \
				if(components == 3) \
				{ \
					BLEND_3(max, temp_type, type, chroma_offset); \
				} \
				else \
				{ \
					BLEND_4(max, temp_type, type, chroma_offset); \
				} \
				output += components; \
			} \
		} \
		else \
/* X direction is not scaled */ \
		{ \
			in_row += in_x1 * components; \
			for(int j = 0; j < out_w; j++) \
			{ \
				temp_type input1, input2, input3, input4; \
	 \
 				input1 = in_row[0]; \
				input2 = in_row[1]; \
				input3 = in_row[2]; \
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
				in_row += components; \
				output += components; \
			} \
		} \
	} \
}



ScaleTranslateUnit::ScaleTranslateUnit(ScaleTranslateEngine *server, OverlayFrame *overlay)
 : LoadClient(server)
{
	this->overlay = overlay;
	this->scale_translate = server;
}

ScaleTranslateUnit::~ScaleTranslateUnit()
{
}

void ScaleTranslateUnit::scale_array_f(int* &table, 
	int out_x1, 
	int out_x2,
	float in_x1,
	float in_x2)
{
	float scale = (float)(out_x2 - out_x1) / (in_x2 - in_x1);

	table = new int[(int)out_x2 - out_x1];
	
	for(int i = 0; i < out_x2 - out_x1; i++)
		table[i] = (int)((float)i / scale + in_x1);
}

void ScaleTranslateUnit::process_package(LoadPackage *package)
{
	ScaleTranslatePackage *pkg = (ScaleTranslatePackage*)package;

// Args for NEAREST_NEIGHBOR_MACRO
	VFrame *output = scale_translate->output;
	VFrame *input = scale_translate->input;
	int in_x1 = (int)scale_translate->in_x1;
	int in_y1 = (int)scale_translate->in_y1;
	int in_x2 = (int)scale_translate->in_x2;
	int in_y2 = (int)scale_translate->in_y2;
	int out_x1 = scale_translate->out_x1;
	int out_y1 = scale_translate->out_y1;
	int out_x2 = scale_translate->out_x2;
	int out_y2 = scale_translate->out_y2;
	float alpha = scale_translate->alpha;
	int mode = scale_translate->mode;
	int out_w = out_x2 - out_x1;

	int *x_table = 0;
	int *y_table;
	unsigned char **in_rows = input->get_rows();
	unsigned char **out_rows = output->get_rows();

//	Timer a;
//	a.update();
//printf("ScaleTranslateUnit::process_package 1 %d\n", mode);
	if(out_w != in_x2 - in_x1)
	{
		scale_array_f(x_table, 
			out_x1, 
			out_x2,
			scale_translate->in_x1,
			scale_translate->in_x2);
	}
	scale_array_f(y_table, 
		out_y1, 
		out_y2,
		scale_translate->in_y1,
		scale_translate->in_y2);


 	if (mode == TRANSFER_REPLACE && (out_w == in_x2 - in_x1)) 
	{
// if we have transfer replace and x direction is not scaled, PARTY!
 		char bytes_per_pixel = input->calculate_bytes_per_pixel(input->get_color_model());
 		int line_len = out_w * bytes_per_pixel;
 		int in_start_byte = in_x1 * bytes_per_pixel;
 		int out_start_byte = out_x1 * bytes_per_pixel;
 		for(int i = pkg->out_row1; i < pkg->out_row2; i++) 
 		{
 			memcpy (out_rows[i] + out_start_byte, 
 				in_rows[y_table[i - out_y1]] + in_start_byte , 
 				line_len);
 		}

 	} 
	else
	switch(input->get_color_model())
	{
		case BC_RGB888:
			SCALE_TRANSLATE(0xff, uint32_t, uint8_t, 3, 0);
			break;

		case BC_RGB_FLOAT:
			SCALE_TRANSLATE(1.0, float, float, 3, 0);
			break;

		case BC_YUV888:
			SCALE_TRANSLATE(0xff, int32_t, uint8_t, 3, 0x80);
			break;

		case BC_RGBA8888:
			SCALE_TRANSLATE(0xff, uint32_t, uint8_t, 4, 0);
			break;

		case BC_RGBA_FLOAT:
			SCALE_TRANSLATE(1.0, float, float, 4, 0);
			break;

		case BC_YUVA8888:
			SCALE_TRANSLATE(0xff, int32_t, uint8_t, 4, 0x80);
			break;


		case BC_RGB161616:
			SCALE_TRANSLATE(0xffff, uint64_t, uint16_t, 3, 0);
			break;

		case BC_YUV161616:
			SCALE_TRANSLATE(0xffff, int64_t, uint16_t, 3, 0x8000);
			break;

		case BC_RGBA16161616:
			SCALE_TRANSLATE(0xffff, uint64_t, uint16_t, 4, 0);
			break;

		case BC_YUVA16161616:
			SCALE_TRANSLATE(0xffff, int64_t, uint16_t, 4, 0x8000);
			break;
	}
	
//printf("blend mode %i, took %li ms\n", mode, a.get_difference());
	if(x_table)
		delete [] x_table;
	delete [] y_table;

};









ScaleTranslateEngine::ScaleTranslateEngine(OverlayFrame *overlay, int cpus)
 : LoadServer(cpus, cpus)
{
	this->overlay = overlay;
}

ScaleTranslateEngine::~ScaleTranslateEngine()
{
}

void ScaleTranslateEngine::init_packages()
{
	int out_h = out_y2 - out_y1;

	for(int i = 0; i < get_total_packages(); i++)
	{
		ScaleTranslatePackage *package = (ScaleTranslatePackage*)get_package(i);
		package->out_row1 = (int)(out_y1 + out_h / 
			get_total_packages() * 
			i);
		package->out_row2 = (int)((float)package->out_row1 + 
			out_h / 
			get_total_packages());
		if(i >= get_total_packages() - 1)
			package->out_row2 = out_y2;
	}
}

LoadClient* ScaleTranslateEngine::new_client()
{
	return new ScaleTranslateUnit(this, overlay);
}

LoadPackage* ScaleTranslateEngine::new_package()
{
	return new ScaleTranslatePackage;
}


ScaleTranslatePackage::ScaleTranslatePackage()
{
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
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		type* in_row = input_rows[i]; \
		type* output = output_rows[i]; \
 \
		for(int j = 0; j < w; j++) \
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
	int w = input->get_w(); \
	int h = input->get_h(); \
	int line_len = w * sizeof(type) * components; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		memcpy(output_rows[i], input_rows[i], line_len); \
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
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		type* in_row = input_rows[i]; \
		type* output = output_rows[i]; \
 \
		for(int j = 0; j < w; j++) \
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
			output[3] = (type)(in_row[3] > output[3] ? in_row[3] : output[3]); \
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
	int w = input->get_w() * 3; \
	int h = input->get_h(); \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) \
	{ \
		type* in_row = input_rows[i]; \
		type* output = output_rows[i]; \
 \
		for(int j = 0; j < w; j++) /* w = 3x width! */ \
		{ \
			*output = ((temp_type)*in_row * opacity + *output * transparency) >> bits; \
			in_row ++; \
			output ++; \
		} \
	} \
}



BlendUnit::BlendUnit(BlendEngine *server, OverlayFrame *overlay)
 : LoadClient(server)
{
	this->overlay = overlay;
	this->blend_engine = server;
}

BlendUnit::~BlendUnit()
{
}

void BlendUnit::process_package(LoadPackage *package)
{
	BlendPackage *pkg = (BlendPackage*)package;


	VFrame *output = blend_engine->output;
	VFrame *input = blend_engine->input;
	float alpha = blend_engine->alpha;
	int mode = blend_engine->mode;

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
	else
	if (mode == TRANSFER_NORMAL) 
	{
		switch(input->get_color_model())
		{
			case BC_RGB_FLOAT:
			{
				float opacity = alpha;
				float transparency = 1.0 - alpha;

				float** output_rows = (float**)output->get_rows();
				float** input_rows = (float**)input->get_rows();
				int w = input->get_w() * 3;
				int h = input->get_h();

				for(int i = pkg->out_row1; i < pkg->out_row2; i++)
				{
					float* in_row = input_rows[i];
					float* output = output_rows[i];
/* w = 3x width! */
					for(int j = 0; j < w; j++) 
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
				int w = input->get_w();
				int h = input->get_h();
			
				for(int i = pkg->out_row1; i < pkg->out_row2; i++)
				{
					float* in_row = input_rows[i];
					float* output = output_rows[i];
			
					for(int j = 0; j < w; j++)
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
						output[3] = in_row[3] > output[3] ? in_row[3] : output[3];

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



BlendEngine::BlendEngine(OverlayFrame *overlay, int cpus)
 : LoadServer(cpus, cpus)
{
	this->overlay = overlay;
}

BlendEngine::~BlendEngine()
{
}

void BlendEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		BlendPackage *package = (BlendPackage*)get_package(i);
		package->out_row1 = (int)(input->get_h() / 
			get_total_packages() * 
			i);
		package->out_row2 = (int)((float)package->out_row1 +
			input->get_h() / 
			get_total_packages());

		if(i >= get_total_packages() - 1)
			package->out_row2 = input->get_h();
	}
}

LoadClient* BlendEngine::new_client()
{
	return new BlendUnit(this, overlay);
}

LoadPackage* BlendEngine::new_package()
{
	return new BlendPackage;
}


BlendPackage::BlendPackage()
{
}


