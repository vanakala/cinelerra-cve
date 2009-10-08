
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

REGISTER_PLUGIN(DeInterlaceMain)




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
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	copy_from(prev);
}




DeInterlaceMain::DeInterlaceMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
//	temp = 0;
	temp_prevframe=0;
}

DeInterlaceMain::~DeInterlaceMain()
{
	PLUGIN_DESTRUCTOR_MACRO
//	if(temp) delete temp;
	if(temp_prevframe) delete temp_prevframe;
}

char* DeInterlaceMain::plugin_title() { return N_("Deinterlace"); }
int DeInterlaceMain::is_realtime() { return 1; }



#define DEINTERLACE_TOP_MACRO(type, components, dominance) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = 0; i < h - 1; i += 2) \
	{ \
		type *input_row = (type*)input->get_rows()[dominance ? i + 1 : i]; \
		type *output_row1 = (type*)output->get_rows()[i]; \
		type *output_row2 = (type*)output->get_rows()[i + 1]; \
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
 	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)temp->get_rows(); \
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
		type *input_row1 = in_rows[in_number1]; \
		type *input_row2 = in_rows[in_number2]; \
		type *input_row3 = in_rows[out_number2]; \
		type *temp_row1 = out_rows[out_number1]; \
		type *temp_row2 = out_rows[out_number2]; \
		temp_type sum = 0; \
		temp_type accum_r, accum_b, accum_g, accum_a; \
 \
		memcpy(temp_row1, input_row1, w * components * sizeof(type)); \
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
			*temp_row2++ = accum_r; \
 \
 			total += *input_row3; \
			sum = ((temp_type)*input_row3++) - accum_g; \
			abs_diff += (sum < 0 ? -sum : sum); \
			*temp_row2++ = accum_g; \
 \
 			total += *input_row3; \
			sum = ((temp_type)*input_row3++) - accum_b; \
			abs_diff += (sum < 0 ? -sum : sum); \
			*temp_row2++ = accum_b; \
 \
			if(components == 4) \
			{ \
	 			total += *input_row3; \
				sum = ((temp_type)*input_row3++) - accum_a; \
				abs_diff += (sum < 0 ? -sum : sum); \
				*temp_row2++ = accum_a; \
			} \
		} \
	} \
 \
	temp_type threshold = (temp_type)total * config.threshold / THRESHOLD_SCALAR; \
/* printf("total=%lld threshold=%lld abs_diff=%lld\n", total, threshold, abs_diff); */ \
 	if(abs_diff > threshold || !config.adaptive) \
	{ \
		output->copy_from(temp); \
		changed_rows = 240; \
	} \
	else \
	{ \
		output->copy_from(input); \
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
		type *input_row1 = (type*)input->get_rows()[i]; \
		type *input_row2 = (type*)input->get_rows()[i + 1]; \
		type *output_row1 = (type*)output->get_rows()[i]; \
		type *output_row2 = (type*)output->get_rows()[i + 1]; \
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
		type *input_row1 = (type*)input->get_rows()[i]; \
		type *input_row2 = (type*)input->get_rows()[i + 1]; \
		type *output_row1 = (type*)output->get_rows()[i]; \
		type *output_row2 = (type*)output->get_rows()[i + 1]; \
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
		type *output_row1 = (type*)output->get_rows()[i]; \
		type *output_row2 = (type*)output->get_rows()[i + 1]; \
		type temp1, temp2; \
		\
		if (dominance) { \
			input_row1 = (type*)input->get_rows()[i]; \
			input_row2 = (type*)prevframe->get_rows()[i+1]; \
		} \
		else  {\
			input_row1 = (type*)prevframe->get_rows()[i]; \
			input_row2 = (type*)input->get_rows()[i+1]; \
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
	type *row_above=(type*)input->get_rows()[0]; \
	for(int i = dominance ?0:1; i < h - 1; i += 2) \
	{ \
		type *input_row;\
		type *input_row2; \
		type *old_row; \
		type *output_row1 = (type*)output->get_rows()[i]; \
		type *output_row2 = (type*)output->get_rows()[i + 1]; \
		temp_type pixel, below, old, above; \
		\
		input_row = (type*)input->get_rows()[i]; \
		input_row2 = (type*)input->get_rows()[i+1]; \
		old_row = (type*)prevframe->get_rows()[i]; \
\
		for(int j = 0; j < w * components; j++) \
		{ \
			pixel = input_row[j]; \
			below = input_row2[j]; \
			old = old_row[j]; \
			above = row_above[j]; \
\
			if  ( ( FABS(pixel-old) <= noise_threshold )  \
			|| ((pixel+old != 0) && (((FABS((double) pixel-old))/((double) pixel+old)) >= exp_threshold )) \
			|| ((above+below != 0) && (((FABS((double) pixel-old))/((double) above+below)) >= exp_threshold )) \
			) {\
				pixel=(above+below)/2 ;\
			}\
			output_row1[j] = pixel; \
			output_row2[j] = below; \
		} \
		row_above=input_row2; \
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
			DEINTERLACE_TOP_MACRO(uint16_t, 4, dominance);
			break;
	}
}

void DeInterlaceMain::deinterlace_avg_top(VFrame *input, VFrame *output, int dominance)
{
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
			DEINTERLACE_AVG_TOP_MACRO(uint16_t, int64_t, 4, dominance);
			break;
	}
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
			DEINTERLACE_BOBWEAVE_MACRO(uint16_t, uint64_t, 4, dominance, threshold, noise_threshold);
			break;
	}
}


int DeInterlaceMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	changed_rows = frame->get_h();
	load_configuration();


	read_frame(frame, 
		0, 
		start_position, 
		frame_rate);

// Temp was used for adaptive deinterlacing where it took deinterlacing
// an entire frame to decide if the deinterlaced output should be used.
	temp = frame;

// 	if(!temp)
// 		temp = new VFrame(0,
// 			frame->get_w(),
// 			frame->get_h(),
// 			frame->get_color_model());
	if(!temp_prevframe)
		temp_prevframe = new VFrame(0,
			frame->get_w(),
			frame->get_h(),
			frame->get_color_model());

	switch(config.mode)
	{
		case DEINTERLACE_NONE:
//			output->copy_from(input);
			break;
		case DEINTERLACE_KEEP:
			deinterlace_top(frame, frame, config.dominance);
			break;
		case DEINTERLACE_AVG:
			deinterlace_avg(frame, frame);
			break;
		case DEINTERLACE_AVG_1F:
			deinterlace_avg_top(frame, frame, config.dominance);
			break;
		case DEINTERLACE_SWAP:
			deinterlace_swap(frame, frame, config.dominance);
			break;
		case DEINTERLACE_BOBWEAVE:
			if (get_source_position()==0)
				read_frame(temp_prevframe,0, get_source_position(), get_framerate());
			else 
				read_frame(temp_prevframe,0, get_source_position()-1, get_framerate());
			deinterlace_bobweave(frame, temp_prevframe, frame, config.dominance);
			break;
		case DEINTERLACE_TEMPORALSWAP:
			if (get_source_position()==0)
				read_frame(temp_prevframe,0, get_source_position(), get_framerate());
			else 
				read_frame(temp_prevframe,0, get_source_position()-1, get_framerate());
			deinterlace_temporalswap(frame, temp_prevframe, frame, config.dominance);
			break; 
	}
	send_render_gui(&changed_rows);
	return 0;
}


void DeInterlaceMain::render_gui(void *data)
{
	if(thread)
	{
		thread->window->lock_window();
		char string[BCTEXTLEN];
		thread->window->get_status_string(string, *(int*)data);
		thread->window->status->update(string);
		thread->window->flush();
		thread->window->unlock_window();
	}
}

SHOW_GUI_MACRO(DeInterlaceMain, DeInterlaceThread)
RAISE_WINDOW_MACRO(DeInterlaceMain)
SET_STRING_MACRO(DeInterlaceMain)
NEW_PICON_MACRO(DeInterlaceMain)
LOAD_CONFIGURATION_MACRO(DeInterlaceMain, DeInterlaceConfig)


int DeInterlaceMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%sdeinterlace.rc", BCASTDIR);
	
	defaults = new BC_Hash(directory);
	defaults->load();
	config.mode = defaults->get("MODE", config.mode);
	config.dominance = defaults->get("DOMINANCE", config.dominance);
	config.adaptive = defaults->get("ADAPTIVE", config.adaptive);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	return 0;
}


int DeInterlaceMain::save_defaults()
{
	defaults->update("MODE", config.mode);
	defaults->update("DOMINANCE", config.dominance);
	defaults->update("ADAPTIVE", config.adaptive);
	defaults->update("THRESHOLD", config.threshold);
	defaults->save();
	return 0;
}

void DeInterlaceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("DEINTERLACE");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("DOMINANCE", config.dominance);
	output.tag.set_property("ADAPTIVE", config.adaptive);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.append_tag();
	output.tag.set_title("/DEINTERLACE");
	output.append_tag();
	output.terminate_string();
}

void DeInterlaceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

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

void DeInterlaceMain::update_gui()
{
	if(thread) 
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->set_mode(config.mode, 1);
		if (thread->window->dominance_top)
			thread->window->dominance_top->update(config.dominance?0:BC_Toggle::TOGGLE_CHECKED);
		if (thread->window->dominance_bottom)
			thread->window->dominance_bottom->update(config.dominance?BC_Toggle::TOGGLE_CHECKED:0);
		if (thread->window->adaptive)
			thread->window->adaptive->update(config.adaptive);
		if (thread->window->threshold)
			thread->window->threshold->update(config.threshold);
		thread->window->unlock_window();
	}
}

