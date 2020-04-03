
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

#include "clip.h"
#include "bchash.h"
#include "deinterlace.h"
#include "deinterwindow.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

REGISTER_PLUGIN


DeInterlaceConfig::DeInterlaceConfig()
{
	mode = DEINTERLACE_AVG;
	dominance = 0;
	adaptive = 1;
	threshold = 40;
}

int DeInterlaceConfig::equivalent(DeInterlaceConfig &that)
{
	return mode == that.mode &&
		dominance == that.dominance &&
		adaptive == that.adaptive &&
		threshold == that.threshold;
}

void DeInterlaceConfig::copy_from(DeInterlaceConfig &that)
{
	mode = that.mode;
	dominance = that.dominance;
	adaptive = that.adaptive;
	threshold = that.threshold;
}

void DeInterlaceConfig::interpolate(DeInterlaceConfig &prev,
	DeInterlaceConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	copy_from(prev);
}


DeInterlaceMain::DeInterlaceMain(PluginServer *server)
 : PluginVClient(server)
{
	temp_prevframe = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

DeInterlaceMain::~DeInterlaceMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

#define DEINTERLACE_TOP_MACRO(type, components, dominance) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = 0; i < h - 1; i += 2) \
	{ \
		type *input_row = (type*)input->get_row_ptr(dominance ? i + 1 : i); \
		type *output_row1 = (type*)output->get_row_ptr(i); \
		type *output_row2 = (type*)output->get_row_ptr(i + 1); \
		memcpy(output_row1, input_row, w * components * sizeof(type)); \
		memcpy(output_row2, input_row, w * components * sizeof(type)); \
	} \
}

#define DEINTERLACE_AVG_TOP_MACRO(type, temp_type, components, dominance) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
	changed_rows = 0; \
 \
	int max_h = h - 1; \
	temp_type abs_diff = 0, total = 0; \
 \
	for(int i = 0; i < max_h; i += 2) \
	{ \
		int in_number1 = dominance ? i - 1 : i + 0; \
		int in_number2 = dominance ? i + 1 : i + 2; \
		int out_number1 = dominance ? i - 1 : i; \
		int out_number2 = dominance ? i : i + 1; \
		in_number1 = MAX(in_number1, 0); \
		in_number2 = MIN(in_number2, max_h); \
		out_number1 = MAX(out_number1, 0); \
		out_number2 = MIN(out_number2, max_h); \
 \
		type *input_row1 = (type*)input->get_row_ptr(in_number1); \
		type *input_row2 = (type*)input->get_row_ptr(in_number2); \
		type *input_row3 = (type*)input->get_row_ptr(out_number2); \
		type *out_row1 = (type*)output->get_row_ptr(out_number1); \
		type *out_row2 = (type*)output->get_row_ptr(out_number2); \
		temp_type sum = 0; \
		temp_type accum_r, accum_b, accum_g, accum_a; \
 \
		memcpy(out_row1, input_row1, w * components * sizeof(type)); \
		for(int j = 0; j < w; j++) \
		{ \
			accum_r = (*input_row1++) + (*input_row2++); \
			accum_g = (*input_row1++) + (*input_row2++); \
			accum_b = (*input_row1++) + (*input_row2++); \
			if(components == 4) \
				accum_a = (*input_row1++) + (*input_row2++); \
			accum_r /= 2; \
			accum_g /= 2; \
			accum_b /= 2; \
			accum_a /= 2; \
 \
			total += *input_row3; \
			sum = ((temp_type)*input_row3++) - accum_r; \
			abs_diff += (sum < 0 ? -sum : sum); \
			*out_row2++ = accum_r; \
 \
			total += *input_row3; \
			sum = ((temp_type)*input_row3++) - accum_g; \
			abs_diff += (sum < 0 ? -sum : sum); \
			*out_row2++ = accum_g; \
 \
			total += *input_row3; \
			sum = ((temp_type)*input_row3++) - accum_b; \
			abs_diff += (sum < 0 ? -sum : sum); \
			*out_row2++ = accum_b; \
 \
			if(components == 4) \
			{ \
	 			total += *input_row3; \
				sum = ((temp_type)*input_row3++) - accum_a; \
				abs_diff += (sum < 0 ? -sum : sum); \
				*out_row2++ = accum_a; \
			} \
		} \
	} \
 \
	temp_type threshold = (temp_type)total * config.threshold / THRESHOLD_SCALAR; \
	if(abs_diff > threshold || !config.adaptive) \
	{ \
		result = output; \
		changed_rows = 240; \
	} \
	else \
	{ \
		result = input; \
		changed_rows = 0; \
	} \
 \
}

#define DEINTERLACE_AVG_MACRO(type, temp_type, components) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = 0; i < h - 1; i += 2) \
	{ \
		type *input_row1 = (type*)input->get_row_ptr(i); \
		type *input_row2 = (type*)input->get_row_ptr(i + 1); \
		type *output_row1 = (type*)output->get_row_ptr(i); \
		type *output_row2 = (type*)output->get_row_ptr(i + 1); \
		type result; \
 \
		for(int j = 0; j < w * components; j++) \
		{ \
			result = ((temp_type)input_row1[j] + input_row2[j]) / 2; \
			output_row1[j] = result; \
			output_row2[j] = result; \
		} \
	} \
}

#define DEINTERLACE_SWAP_MACRO(type, components, dominance) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = dominance; i < h - 1; i += 2) \
	{ \
		type *input_row1 = (type*)input->get_row_ptr(i); \
		type *input_row2 = (type*)input->get_row_ptr(i + 1); \
		type *output_row1 = (type*)output->get_row_ptr(i); \
		type *output_row2 = (type*)output->get_row_ptr(i + 1); \
		type temp1, temp2; \
 \
		for(int j = 0; j < w * components; j++) \
		{ \
			temp1 = input_row1[j]; \
			temp2 = input_row2[j]; \
			output_row1[j] = temp2; \
			output_row2[j] = temp1; \
		} \
	} \
}


#define DEINTERLACE_TEMPORALSWAP_MACRO(type, components, dominance) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = 0; i < h - 1; i += 2) \
	{ \
		type *input_row1;\
		type *input_row2; \
		type *output_row1 = (type*)output->get_row_ptr(i); \
		type *output_row2 = (type*)output->get_row_ptr(i + 1); \
		type temp1, temp2; \
		\
		if (dominance) { \
			input_row1 = (type*)input->get_row_ptr(i); \
			input_row2 = (type*)prevframe->get_row_ptr(i + 1); \
		} \
		else \
		{ \
			input_row1 = (type*)prevframe->get_row_ptr(i); \
			input_row2 = (type*)input->get_row_ptr(i + 1); \
		} \
 \
		for(int j = 0; j < w * components; j++) \
		{ \
			temp1 = input_row1[j]; \
			temp2 = input_row2[j]; \
			output_row1[j] = temp1; \
			output_row2[j] = temp2; \
		} \
	} \
}


/* Bob & Weave deinterlacer:

For each pixel, 
	if it's similar to the previous frame 
	then keep it
	else average with line above and below

Similar is defined as in abs(difference)/(sum) < threshold
*/
#define FABS(a) (((a)<0)?(0-(a)):(a))
#define FMAX(a,b) (((a)>(b))?(a):(b))
#define FMIN(a,b) (((a)<(b))?(a):(b))

#define SQ(a) ((a)*(a))
// threshold < 100 -> a-b/a+b <
 

#define DEINTERLACE_BOBWEAVE_MACRO(type, temp_type, components, dominance, threshold, noise_threshold) \
{ \
	/* Ooooohh, I like fudge factors */ \
	double exp_threshold=exp(((double)threshold - 50 )/2);\
	int w = input->get_w(); \
	int h = input->get_h(); \
	type *row_above=(type*)input->get_row_ptr(0); \
	for(int i = dominance ?0:1; i < h - 1; i += 2) \
	{ \
		type *input_row;\
		type *input_row2; \
		type *old_row; \
		type *output_row1 = (type*)output->get_row_ptr(i); \
		type *output_row2 = (type*)output->get_row_ptr(i + 1); \
		temp_type pixel, below, old, above; \
		\
		input_row = (type*)input->get_row_ptr(i); \
		input_row2 = (type*)input->get_row_ptr(i + 1); \
		old_row = (type*)prevframe->get_row_ptr(i); \
\
		for(int j = 0; j < w * components; j++) \
		{ \
			pixel = input_row[j]; \
			below = input_row2[j]; \
			old = old_row[j]; \
			above = row_above[j]; \
\
			if((FABS(pixel - old) <= noise_threshold) || \
				((pixel + old != 0) && \
				(((FABS((double)pixel - old)) / ((double)pixel + old)) \
					>= exp_threshold )) || \
				((above + below != 0) && \
				(((FABS((double)pixel - old)) / ((double)above + below)) \
					>= exp_threshold ))) \
			{ \
				pixel = (above + below) / 2; \
			} \
			output_row1[j] = pixel; \
			output_row2[j] = below; \
		} \
		row_above = input_row2; \
	} \
}


void DeInterlaceMain::deinterlace_top(VFrame *input, VFrame *output, int dominance)
{
	switch(input->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
		DEINTERLACE_TOP_MACRO(unsigned char, 3, dominance);
		break;
	case BC_RGB_FLOAT:
		DEINTERLACE_TOP_MACRO(float, 3, dominance);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		DEINTERLACE_TOP_MACRO(unsigned char, 4, dominance);
		break;
	case BC_RGBA_FLOAT:
		DEINTERLACE_TOP_MACRO(float, 4, dominance);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		DEINTERLACE_TOP_MACRO(uint16_t, 3, dominance);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		DEINTERLACE_TOP_MACRO(uint16_t, 4, dominance);
		break;
	}
}

VFrame *DeInterlaceMain::deinterlace_avg_top(VFrame *input, VFrame *output, int dominance)
{
	VFrame *result;

	switch(input->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
		DEINTERLACE_AVG_TOP_MACRO(unsigned char, int64_t, 3, dominance);
		break;
	case BC_RGB_FLOAT:
		DEINTERLACE_AVG_TOP_MACRO(float, double, 3, dominance);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		DEINTERLACE_AVG_TOP_MACRO(unsigned char, int64_t, 4, dominance);
		break;
	case BC_RGBA_FLOAT:
		DEINTERLACE_AVG_TOP_MACRO(float, double, 4, dominance);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		DEINTERLACE_AVG_TOP_MACRO(uint16_t, int64_t, 3, dominance);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		DEINTERLACE_AVG_TOP_MACRO(uint16_t, int64_t, 4, dominance);
		break;
	}
	return result;
}

void DeInterlaceMain::deinterlace_avg(VFrame *input, VFrame *output)
{
	switch(input->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
		DEINTERLACE_AVG_MACRO(unsigned char, uint64_t, 3);
		break;
	case BC_RGB_FLOAT:
		DEINTERLACE_AVG_MACRO(float, double, 3);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		DEINTERLACE_AVG_MACRO(unsigned char, uint64_t, 4);
		break;
	case BC_RGBA_FLOAT:
		DEINTERLACE_AVG_MACRO(float, double, 4);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		DEINTERLACE_AVG_MACRO(uint16_t, uint64_t, 3);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		DEINTERLACE_AVG_MACRO(uint16_t, uint64_t, 4);
		break;
	}
}

void DeInterlaceMain::deinterlace_swap(VFrame *input, VFrame *output, int dominance)
{
	switch(input->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
		DEINTERLACE_SWAP_MACRO(unsigned char, 3, dominance);
		break;
	case BC_RGB_FLOAT:
		DEINTERLACE_SWAP_MACRO(float, 3, dominance);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		DEINTERLACE_SWAP_MACRO(unsigned char, 4, dominance);
		break;
	case BC_RGBA_FLOAT:
		DEINTERLACE_SWAP_MACRO(float, 4, dominance);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		DEINTERLACE_SWAP_MACRO(uint16_t, 3, dominance);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		DEINTERLACE_SWAP_MACRO(uint16_t, 4, dominance);
		break;
	}
}

void DeInterlaceMain::deinterlace_temporalswap(VFrame *input, VFrame *prevframe, VFrame *output, int dominance)
{
	switch(input->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
		DEINTERLACE_TEMPORALSWAP_MACRO(unsigned char, 3, dominance);
		break;
	case BC_RGB_FLOAT:
		DEINTERLACE_TEMPORALSWAP_MACRO(float, 3, dominance);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		DEINTERLACE_TEMPORALSWAP_MACRO(unsigned char, 4, dominance);
		break;
	case BC_RGBA_FLOAT:
		DEINTERLACE_TEMPORALSWAP_MACRO(float, 4, dominance);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		DEINTERLACE_TEMPORALSWAP_MACRO(uint16_t, 3, dominance);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		DEINTERLACE_TEMPORALSWAP_MACRO(uint16_t, 4, dominance);
		break;
	}
}

void DeInterlaceMain::deinterlace_bobweave(VFrame *input, VFrame *prevframe, VFrame *output, int dominance)
{
	int threshold=config.threshold;
	int noise_threshold=0;

	switch(input->get_color_model())
	{
	case BC_RGB888:
	case BC_YUV888:
		DEINTERLACE_BOBWEAVE_MACRO(unsigned char, uint64_t, 3, dominance, threshold, noise_threshold);
		break;
	case BC_RGB_FLOAT:
		DEINTERLACE_BOBWEAVE_MACRO(float, double, 3, dominance, threshold, noise_threshold);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		DEINTERLACE_BOBWEAVE_MACRO(unsigned char, uint64_t, 4, dominance, threshold, noise_threshold);
		break;
	case BC_RGBA_FLOAT:
		DEINTERLACE_BOBWEAVE_MACRO(float, double, 4, dominance, threshold, noise_threshold);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		DEINTERLACE_BOBWEAVE_MACRO(uint16_t, uint64_t, 3, dominance, threshold, noise_threshold);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		DEINTERLACE_BOBWEAVE_MACRO(uint16_t, uint64_t, 4, dominance, threshold, noise_threshold);
		break;
	}
}

VFrame *DeInterlaceMain::process_tmpframe(VFrame *frame)
{
	VFrame *output = 0;
	VFrame *res;

	changed_rows = frame->get_h();
	load_configuration();

	switch(config.mode)
	{
	case DEINTERLACE_NONE:
		break;
	case DEINTERLACE_KEEP:
		output = clone_vframe(frame);
		deinterlace_top(frame, output, config.dominance);
		break;
	case DEINTERLACE_AVG:
		output = clone_vframe(frame);
		deinterlace_avg(frame, output);
		break;
	case DEINTERLACE_AVG_1F:
		output = clone_vframe(frame);
		res = deinterlace_avg_top(frame, output, config.dominance);
		if(res == frame)
		{
			frame = output;
			output = res;
		}
		break;
	case DEINTERLACE_SWAP:
		output = clone_vframe(frame);
		deinterlace_swap(frame, output, config.dominance);
		break;
	case DEINTERLACE_BOBWEAVE:
		output = clone_vframe(frame);
		temp_prevframe = clone_vframe(frame);
		temp_prevframe->set_pts(frame->get_pts() - frame->get_duration());
		get_frame(temp_prevframe);
		deinterlace_bobweave(frame, temp_prevframe, output, config.dominance);
		break;
	case DEINTERLACE_TEMPORALSWAP:
		output = clone_vframe(frame);
		temp_prevframe = clone_vframe(frame);
		temp_prevframe->set_pts(frame->get_pts() - frame->get_duration());
		get_frame(temp_prevframe);
		deinterlace_temporalswap(frame, temp_prevframe, output, config.dominance);
		break;
	}
	release_vframe(temp_prevframe);
	if(output)
	{
		release_vframe(frame);
		frame = output;
	}
	render_gui(&changed_rows);
	return frame;
}

void DeInterlaceMain::render_gui(void *data)
{
	if(thread)
	{
		char string[BCTEXTLEN];
		thread->window->get_status_string(string, changed_rows);
		thread->window->status->update(string);
		thread->window->flush();
	}
}

void DeInterlaceMain::load_defaults()
{
	defaults = load_defaults_file("deinterlace.rc");

	config.mode = defaults->get("MODE", config.mode);
	config.dominance = defaults->get("DOMINANCE", config.dominance);
	config.adaptive = defaults->get("ADAPTIVE", config.adaptive);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
}

void DeInterlaceMain::save_defaults()
{
	defaults->update("MODE", config.mode);
	defaults->update("DOMINANCE", config.dominance);
	defaults->update("ADAPTIVE", config.adaptive);
	defaults->update("THRESHOLD", config.threshold);
	defaults->save();
}

void DeInterlaceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DEINTERLACE");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("DOMINANCE", config.dominance);
	output.tag.set_property("ADAPTIVE", config.adaptive);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.append_tag();
	output.tag.set_title("/DEINTERLACE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DeInterlaceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("DEINTERLACE"))
		{
			config.mode = input.tag.get_property("MODE", config.mode);
			config.dominance = input.tag.get_property("DOMINANCE", config.dominance);
			config.adaptive = input.tag.get_property("ADAPTIVE", config.adaptive);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
		}
	}
}
