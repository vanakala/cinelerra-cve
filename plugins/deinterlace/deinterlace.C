#include "clip.h"
#include "defaults.h"
#include "deinterlace.h"
#include "deinterwindow.h"
#include "filexml.h"
#include "keyframe.h"
#include "picon_png.h"
#include "vframe.h"








#include <stdint.h>
#include <string.h>


REGISTER_PLUGIN(DeInterlaceMain)




DeInterlaceConfig::DeInterlaceConfig()
{
	mode = DEINTERLACE_EVEN;
	adaptive = 1;
	threshold = 40;
}

int DeInterlaceConfig::equivalent(DeInterlaceConfig &that)
{
	return mode == that.mode &&
		adaptive == that.adaptive &&
		threshold == that.threshold;
}

void DeInterlaceConfig::copy_from(DeInterlaceConfig &that)
{
	mode = that.mode;
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
	temp = 0;
}

DeInterlaceMain::~DeInterlaceMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(temp) delete temp;
}

char* DeInterlaceMain::plugin_title() { return "Deinterlace"; }
int DeInterlaceMain::is_realtime() { return 1; }



#define DEINTERLACE_EVEN_MACRO(type, components, dominance, max) \
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

#define DEINTERLACE_AVG_EVEN_MACRO(type, components, dominance, max) \
{ \
	int w = input->get_w(); \
	int h = input->get_h(); \
	changed_rows = 0; \
 \
 	type **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)temp->get_rows(); \
	int max_h = h - 1; \
	int64_t abs_diff = 0, total = 0; \
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
		int64_t sum = 0; \
		int64_t accum_r, accum_b, accum_g, accum_a; \
 \
		memcpy(temp_row1, input_row1, w * components * sizeof(type)); \
		for(int j = 0; j < w; j++) \
		{ \
			accum_r = (*input_row1++) + (*input_row2++); \
			accum_g = (*input_row1++) + (*input_row2++); \
			accum_b = (*input_row1++) + (*input_row2++); \
			if(components == 4) \
				accum_a = (*input_row1++) + (*input_row2++); \
			accum_r >>= 1; \
			accum_g >>= 1; \
			accum_b >>= 1; \
			accum_a >>= 1; \
 \
 			total += *input_row3; \
			sum = ((int)*input_row3++) - accum_r; \
			abs_diff += (sum < 0 ? -sum : sum); \
			*temp_row2++ = accum_r; \
 \
 			total += *input_row3; \
			sum = ((int)*input_row3++) - accum_g; \
			abs_diff += (sum < 0 ? -sum : sum); \
			*temp_row2++ = accum_g; \
 \
 			total += *input_row3; \
			sum = ((int)*input_row3++) - accum_b; \
			abs_diff += (sum < 0 ? -sum : sum); \
			*temp_row2++ = accum_b; \
 \
			if(components == 4) \
			{ \
	 			total += *input_row3; \
				sum = ((int)*input_row3++) - accum_a; \
				abs_diff += (sum < 0 ? -sum : sum); \
				*temp_row2++ = accum_a; \
			} \
		} \
	} \
 \
	int64_t threshold = (int64_t)total * config.threshold / THRESHOLD_SCALAR; \
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

#define DEINTERLACE_AVG_MACRO(type, components) \
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
			result = ((uint64_t)input_row1[j] + input_row2[j]) >> 1; \
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


void DeInterlaceMain::deinterlace_even(VFrame *input, VFrame *output, int dominance)
{
	switch(input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DEINTERLACE_EVEN_MACRO(unsigned char, 3, dominance, 0xff);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DEINTERLACE_EVEN_MACRO(unsigned char, 4, dominance, 0xff);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DEINTERLACE_EVEN_MACRO(uint16_t, 3, dominance, 0xffff);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DEINTERLACE_EVEN_MACRO(uint16_t, 4, dominance, 0xffff);
			break;
	}
}

void DeInterlaceMain::deinterlace_avg_even(VFrame *input, VFrame *output, int dominance)
{
	switch(input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DEINTERLACE_AVG_EVEN_MACRO(unsigned char, 3, dominance, 0xff);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DEINTERLACE_AVG_EVEN_MACRO(unsigned char, 4, dominance, 0xff);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DEINTERLACE_AVG_EVEN_MACRO(uint16_t, 3, dominance, 0xffff);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DEINTERLACE_AVG_EVEN_MACRO(uint16_t, 4, dominance, 0xffff);
			break;
	}
}

void DeInterlaceMain::deinterlace_avg(VFrame *input, VFrame *output)
{
	switch(input->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DEINTERLACE_AVG_MACRO(unsigned char, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DEINTERLACE_AVG_MACRO(unsigned char, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DEINTERLACE_AVG_MACRO(uint16_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DEINTERLACE_AVG_MACRO(uint16_t, 4);
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
		case BC_RGBA8888:
		case BC_YUVA8888:
			DEINTERLACE_SWAP_MACRO(unsigned char, 4, dominance);
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


int DeInterlaceMain::process_realtime(VFrame *input, VFrame *output)
{
	changed_rows = input->get_h();
	load_configuration();
	if(!temp)
		temp = new VFrame(0,
			input->get_w(),
			input->get_h(),
			input->get_color_model());

	switch(config.mode)
	{
		case DEINTERLACE_NONE:
			output->copy_from(input);
			break;
		case DEINTERLACE_EVEN:
			deinterlace_even(input, output, 0);
			break;
		case DEINTERLACE_ODD:
			deinterlace_even(input, output, 1);
			break;
		case DEINTERLACE_AVG:
			deinterlace_avg(input, output);
			break;
		case DEINTERLACE_AVG_EVEN:
			deinterlace_avg_even(input, output, 0);
			break;
		case DEINTERLACE_AVG_ODD:
			deinterlace_avg_even(input, output, 1);
			break;
		case DEINTERLACE_SWAP_ODD:
			deinterlace_swap(input, output, 1);
			break;
		case DEINTERLACE_SWAP_EVEN:
			deinterlace_swap(input, output, 0);
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
	
	defaults = new Defaults(directory);
	defaults->load();
	config.mode = defaults->get("MODE", config.mode);
	config.adaptive = defaults->get("ADAPTIVE", config.adaptive);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	return 0;
}


int DeInterlaceMain::save_defaults()
{
	defaults->update("MODE", config.mode);
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
	output.tag.set_property("ADAPTIVE", config.adaptive);
	output.tag.set_property("THRESHOLD", config.threshold);
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
		thread->window->set_mode(config.mode, 0);
		thread->window->adaptive->update(config.adaptive);
		thread->window->threshold->update(config.threshold);
		thread->window->unlock_window();
	}
}

