
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
#include "colormodels.h"
#include "filexml.h"
#include "ivtc.h"
#include "ivtcwindow.h"
#include "language.h"

#include <stdio.h>
#include <string.h>


static char *pattern_text[] = 
{
	N_("A  B  BC  CD  D"),
	N_("AB  BC  CD  DE  EF"),
	N_("Automatic")
};


REGISTER_PLUGIN(IVTCMain)

IVTCConfig::IVTCConfig()
{
	frame_offset = 0;
	first_field = 0;
	automatic = 1;
	auto_threshold = 2;
	pattern = IVTCConfig::PULLDOWN32;
}

IVTCMain::IVTCMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	previous_min = 0x4000000000000000LL;
	previous_strategy = 0;
}

IVTCMain::~IVTCMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(engine)
	{
		if(temp_frame[0]) delete temp_frame[0];
		if(temp_frame[1]) delete temp_frame[1];
		temp_frame[0] = 0;
		temp_frame[1] = 0;
		delete engine;
	}
}

char* IVTCMain::plugin_title() { return N_("Inverse Telecine"); }
int IVTCMain::is_realtime() { return 1; }


int IVTCMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sivtc.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.frame_offset = defaults->get("FRAME_OFFSET", config.frame_offset);
	config.first_field = defaults->get("FIRST_FIELD", config.first_field);
	config.automatic = defaults->get("AUTOMATIC", config.automatic);
	config.auto_threshold = defaults->get("AUTO_THRESHOLD", config.auto_threshold);
	config.pattern = defaults->get("PATTERN", config.pattern);
	return 0;
}

int IVTCMain::save_defaults()
{
	defaults->update("FRAME_OFFSET", config.frame_offset);
	defaults->update("FIRST_FIELD", config.first_field);
	defaults->update("AUTOMATIC", config.automatic);
	defaults->update("AUTO_THRESHOLD", config.auto_threshold);
	defaults->update("PATTERN", config.pattern);
	defaults->save();
	return 0;
}

#include "picon_png.h"
NEW_PICON_MACRO(IVTCMain)
SHOW_GUI_MACRO(IVTCMain, IVTCThread)
SET_STRING_MACRO(IVTCMain)
RAISE_WINDOW_MACRO(IVTCMain)



int IVTCMain::load_configuration()
{
	KeyFrame *prev_keyframe;

	prev_keyframe = get_prev_keyframe(get_source_position());
// Must also switch between interpolation between keyframes and using first keyframe
	read_data(prev_keyframe);

	return 0;
}

void IVTCMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("IVTC");
	output.tag.set_property("FRAME_OFFSET", config.frame_offset);
	output.tag.set_property("FIRST_FIELD", config.first_field);
	output.tag.set_property("AUTOMATIC", config.automatic);
	output.tag.set_property("AUTO_THRESHOLD", config.auto_threshold);
	output.tag.set_property("PATTERN", config.pattern);
	output.append_tag();
	output.tag.set_title("/IVTC");
	output.append_tag();
	output.terminate_string();
}

void IVTCMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	float new_threshold;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("IVTC"))
			{
				config.frame_offset = input.tag.get_property("FRAME_OFFSET", config.frame_offset);
				config.first_field = input.tag.get_property("FIRST_FIELD", config.first_field);
				config.automatic = input.tag.get_property("AUTOMATIC", config.automatic);
				new_threshold = input.tag.get_property("AUTO_THRESHOLD", config.auto_threshold);
				config.pattern = input.tag.get_property("PATTERN", config.pattern);
			}
		}
	}
}



void IVTCMain::render_stop()
{
	previous_min = 0x4000000000000000LL;
}



// Pattern A B BC CD D
int IVTCMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();

	if(!engine)
	{
		temp_frame[0] = 0;
		temp_frame[1] = 0;
	
		engine = new IVTCEngine(this, smp + 1);
	}

// Determine position in pattern
	int pattern_position = (PluginClient::source_position +	config.frame_offset) % 5;

//printf("IVTCMain::process_realtime %d %d\n", pattern_position, config.first_field);
	if(!temp_frame[0]) temp_frame[0] = new VFrame(0,
		input_ptr->get_w(),
		input_ptr->get_h(),
		input_ptr->get_color_model(),
		-1);
	if(!temp_frame[1]) temp_frame[1] = new VFrame(0,
		input_ptr->get_w(),
		input_ptr->get_h(),
		input_ptr->get_color_model(),
		-1);

	int row_size = VFrame::calculate_bytes_per_pixel(input_ptr->get_color_model()) * input_ptr->get_w();
	int64_t field1;
	int64_t field2;
	int64_t field1_sum;
	int64_t field2_sum;
	this->input = input_ptr;
	this->output = output_ptr;

// Determine pattern
	if(config.pattern == IVTCConfig::PULLDOWN32)
	{
		switch(pattern_position)
		{
// Direct copy
			case 0:
			case 4:
				if(input_ptr->get_rows()[0] != output_ptr->get_rows()[0])
					output_ptr->copy_from(input_ptr);
				break;

			case 1:
				temp_frame[0]->copy_from(input_ptr);
				if(input_ptr->get_rows()[0] != output_ptr->get_rows()[0])
					output_ptr->copy_from(input_ptr);
				break;

			case 2:
// Save one field for next frame.  Reuse previous frame.
				temp_frame[1]->copy_from(input_ptr);
				output_ptr->copy_from(temp_frame[0]);
				break;

			case 3:
// Combine previous field with current field.
				for(int i = 0; i < input_ptr->get_h(); i++)
				{
					if((i + config.first_field) & 1)
						memcpy(output_ptr->get_rows()[i], 
							input_ptr->get_rows()[i],
							row_size);
					else
						memcpy(output_ptr->get_rows()[i], 
							temp_frame[1]->get_rows()[i],
							row_size);
				}
				break;
		}
	}
	else
	if(config.pattern == IVTCConfig::SHIFTFIELD)
	{
		temp_frame[1]->copy_from(input_ptr);

// Recycle previous bottom or top
		for(int i = 0; i < input_ptr->get_h(); i++)
		{
			if((i + config.first_field) & 1)
				memcpy(output_ptr->get_rows()[i], 
					input_ptr->get_rows()[i],
					row_size);
			else
				memcpy(output_ptr->get_rows()[i],
					temp_frame[0]->get_rows()[i],
					row_size);
		}

// Swap temp frames
		VFrame *temp = temp_frame[0];
		temp_frame[0] = temp_frame[1];
		temp_frame[1] = temp;
	}
	else
	if(config.pattern == IVTCConfig::AUTOMATIC)
	{
// Compare averaged rows with original rows and 
// with previous rows.
// Take rows which are most similar to the averaged rows.
// Process frame.
		engine->process_packages();
// Copy current for future use
		temp_frame[1]->copy_from(input_ptr);

// Add results
		even_vs_current = 0;
		even_vs_prev = 0;
		odd_vs_current = 0;
		odd_vs_prev = 0;

		for(int i = 0; i < engine->get_total_clients(); i++)
		{
			IVTCUnit *unit = (IVTCUnit*)engine->get_client(i);
			even_vs_current += unit->even_vs_current;
			even_vs_prev += unit->even_vs_prev;
			odd_vs_current += unit->odd_vs_current;
			odd_vs_prev += unit->odd_vs_prev;
		}


		int64_t min;
		int strategy;


// First strategy.
// Even lines from previous frame are more similar to 
// averaged even lines in current frame.
// Take even lines from previous frame
		min = even_vs_prev;
		strategy = 0;

		if(even_vs_current < min)
		{
// Even lines from current frame are more similar to averaged
// even lines in current frame than previous combinations.
// Take all lines from current frame
			min = even_vs_current;
			strategy = 2;
		}

		if(min > odd_vs_prev)
		{
// Odd lines from previous frame are more similar to averaged
// odd lines in current frame than previous combinations.
// Take odd lines from previous frame
			min = odd_vs_prev;
			strategy = 1;
		}

		if(min > odd_vs_current)
		{
// Odd lines from current frame are more similar to averaged
// odd lines in current frame than previous combinations.
// Take odd lines from current frame.
			min = odd_vs_current;
			strategy = 2;
		}

		int confident = 1;
// Do something if not confident.
// Sometimes we never get the other field.
// Currently nothing is done because it doesn't fix the timing.
		if(min > previous_min * 4 && previous_strategy == 2)
		{
			confident = 0;
//			strategy = 3;
		}
// printf("IVTCMain::process_realtime 1: previous_min=%lld min=%lld strategy=%d confident=%d\n",
// previous_min,
// min,
// strategy,
// confident);



// printf("IVTCMain::process_realtime:\n    even_vs_current=%lld\n    even_vs_prev=%lld\n    odd_vs_current=%lld\n    odd_vs_prev=%lld\n    strategy=%d confident=%d\n",
// even_vs_current,
// even_vs_prev,
// odd_vs_current,
// odd_vs_prev,
// strategy,
// strategy == 2 && !use_direct_copy);

		switch(strategy)
		{
			case 0:
				for(int i = 0; i < input_ptr->get_h(); i++)
				{
					if(!(i & 1))
						memcpy(output_ptr->get_rows()[i], 
							temp_frame[0]->get_rows()[i],
							row_size);
					else
						memcpy(output_ptr->get_rows()[i],
							input_ptr->get_rows()[i],
							row_size);
				}
				break;
			case 1:
				for(int i = 0; i < input_ptr->get_h(); i++)
				{
					if(i & 1)
						memcpy(output_ptr->get_rows()[i], 
							temp_frame[0]->get_rows()[i],
							row_size);
					else
						memcpy(output_ptr->get_rows()[i],
							input_ptr->get_rows()[i],
							row_size);
				}
				break;
			case 2:
				output_ptr->copy_from(input_ptr);
				break;
			case 3:
//				output_ptr->copy_from(temp_frame[0]);
// Deinterlace
				for(int i = 0; i < input_ptr->get_h(); i++)
				{
					if(i & 1)
						memcpy(output_ptr->get_rows()[i], 
							input_ptr->get_rows()[i - 1],
							row_size);
					else
						memcpy(output_ptr->get_rows()[i],
							input_ptr->get_rows()[i],
							row_size);
				}
				break;
		}

		previous_min = min;
		previous_strategy = strategy;
		VFrame *temp = temp_frame[1];
		temp_frame[1] = temp_frame[0];
		temp_frame[0] = temp;
	}
	return 0;
}



void IVTCMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		if(config.pattern == IVTCConfig::AUTOMATIC)
		{
			thread->window->frame_offset->disable();
			thread->window->first_field->disable();
		}
		else
		{
			thread->window->frame_offset->enable();
			thread->window->first_field->enable();
		}
		thread->window->frame_offset->update((int64_t)config.frame_offset);
		thread->window->first_field->update(config.first_field);
//		thread->window->automatic->update(config.automatic);
		for(int i = 0; i < TOTAL_PATTERNS; i++)
		{
			thread->window->pattern[i]->update(config.pattern == i);
		}
		thread->window->unlock_window();
	}
}



// labs returns different values on x86_64 causing our accumulators to explode
#define ABS local_abs


#ifdef __x86_64__

static int local_abs(int value)
{
	return (value < 0 ? -value : value);
}

static float local_abs(float value)
{
	return (value < 0 ? -value : value);
}

#else

static int local_abs(int value)
{
	return abs(value);
}

static float local_abs(float value)
{
	return fabsf(value);
}


#endif




IVTCPackage::IVTCPackage()
 : LoadPackage()
{
}




IVTCUnit::IVTCUnit(IVTCEngine *server, IVTCMain *plugin)
 : LoadClient(server)
{
	this->server = server;
	this->plugin = plugin;
}

#define IVTC_MACRO(type, temp_type, components, is_yuv) \
{ \
 	type **curr_rows = (type**)plugin->input->get_rows(); \
 	type **prev_rows = (type**)plugin->temp_frame[0]->get_rows(); \
/* Components to skip for YUV */ \
	int skip = components - 1; \
 \
	for(int i = ptr->y1; i < ptr->y2; i++) \
	{ \
/* Rows to average in the input frame */ \
		int input_row1_number = i - 1; \
		int input_row2_number = i + 1; \
		input_row1_number = MAX(0, input_row1_number); \
		input_row2_number = MIN(h - 1, input_row2_number); \
		type *input_row1 = curr_rows[input_row1_number]; \
		type *input_row2 = curr_rows[input_row2_number]; \
 \
/* Rows to compare the averaged rows to */ \
		type *current_row = curr_rows[i]; \
		type *prev_row = prev_rows[i]; \
 \
		temp_type current_difference = 0; \
		temp_type prev_difference = 0; \
		for(int j = 0; j < w; j++) \
		{ \
/* This only compares luminance */ \
/* Get average of current rows */ \
			temp_type average = ((temp_type)*input_row1 + *input_row2) / 2; \
/* Difference between averaged current rows and original inbetween row */ \
			current_difference += ABS(average - *current_row); \
/* Difference between averaged current rows and previous inbetween row */ \
			prev_difference += ABS(average - *prev_row); \
 \
/* Do RGB channels */ \
			if(!is_yuv) \
			{ \
				average = ((temp_type)input_row1[1] + input_row2[1]) / 2; \
				current_difference += ABS(average - current_row[1]); \
				prev_difference += ABS(average - prev_row[1]); \
				average = ((temp_type)input_row1[2] + input_row2[2]) / 2; \
				current_difference += ABS(average - current_row[2]); \
				prev_difference += ABS(average - prev_row[2]); \
			} \
 \
/* Add to row accumulators */ \
			current_row += components; \
			prev_row += components; \
			input_row1 += components; \
			input_row2 += components; \
		} \
 \
/* Store row differences in even or odd variables */ \
		if(sizeof(type) == 4) \
		{ \
			if(i % 2) \
			{ \
				odd_vs_current += (int64_t)(current_difference * 0xffff); \
				odd_vs_prev += (int64_t)(prev_difference); \
			} \
			else \
			{ \
				even_vs_current += (int64_t)(current_difference); \
				even_vs_prev += (int64_t)(prev_difference); \
			} \
		} \
		else \
		{ \
			if(i % 2) \
			{ \
				odd_vs_current += (int64_t)current_difference; \
				odd_vs_prev += (int64_t)prev_difference; \
			} \
			else \
			{ \
				even_vs_current += (int64_t)current_difference; \
				even_vs_prev += (int64_t)prev_difference; \
			} \
		} \
	} \
}

void IVTCUnit::clear_totals()
{
	even_vs_current = 0;
	even_vs_prev = 0;
	odd_vs_current = 0;
	odd_vs_prev = 0;
}

void IVTCUnit::process_package(LoadPackage *package)
{
	IVTCPackage *ptr = (IVTCPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();

	switch(plugin->input->get_color_model())
	{
		case BC_RGB_FLOAT:
			IVTC_MACRO(float, float, 3, 0);
			break;
		case BC_RGB888:
			IVTC_MACRO(unsigned char, int, 3, 0);
			break;
		case BC_YUV888:
			IVTC_MACRO(unsigned char, int, 3, 1);
			break;
		case BC_RGBA_FLOAT:
			IVTC_MACRO(float, float, 4, 0);
			break;
		case BC_RGBA8888:
			IVTC_MACRO(unsigned char, int, 4, 0);
			break;
		case BC_YUVA8888:
			IVTC_MACRO(unsigned char, int, 4, 1);
			break;
		case BC_RGB161616:
			IVTC_MACRO(uint16_t, int, 3, 0);
			break;
		case BC_YUV161616:
			IVTC_MACRO(uint16_t, int, 3, 1);
			break;
		case BC_RGBA16161616:
			IVTC_MACRO(uint16_t, int, 4, 0);
			break;
		case BC_YUVA16161616:
			IVTC_MACRO(uint16_t, int, 4, 1);
			break;
	}
	
}





IVTCEngine::IVTCEngine(IVTCMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

IVTCEngine::~IVTCEngine()
{
}

void IVTCEngine::init_packages()
{
	int increment = plugin->input->get_h() / get_total_packages();
	increment /= 2;
	increment *= 2;
	if(!increment) increment = 2;
	int y1 = 0;
	for(int i = 0; i < get_total_packages(); i++)
	{
		IVTCPackage *package = (IVTCPackage*)get_package(i);
		package->y1 = y1;
		y1 += increment;
		if(y1 > plugin->input->get_h()) y1 = plugin->input->get_h();
		package->y2 = y1;
	}
	for(int i = 0; i < get_total_clients(); i++)
	{
		IVTCUnit *unit = (IVTCUnit*)get_client(i);
		unit->clear_totals();
	}
}

LoadClient* IVTCEngine::new_client()
{
	return new IVTCUnit(this, plugin);
}

LoadPackage* IVTCEngine::new_package()
{
	return new IVTCPackage;
}




