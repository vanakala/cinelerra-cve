#include "colormodels.h"
#include "filexml.h"
#include "ivtc.h"
#include "ivtcwindow.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN(IVTCMain)

IVTCConfig::IVTCConfig()
{
	frame_offset = 0;
	first_field = 0;
	automatic = 1;
	auto_threshold = 2;
	pattern = 0;
}

IVTCMain::IVTCMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
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
		for(int i = 0; i < (smp + 1); i++)
			delete engine[i];
		delete [] engine;
	}
}

char* IVTCMain::plugin_title() { return "Inverse Telecine"; }

int IVTCMain::is_realtime() { return 1; }


int IVTCMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sivtc.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
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


void IVTCMain::compare_fields(VFrame *frame1, 
	VFrame *frame2, 
	int64_t &field1,
	int64_t &field2)
{
	field1 = field2 = 0;
	for(int i = 0; i < (smp + 1); i++)
	{
		engine[i]->start_process_frame(frame1, frame2);
	}
	
	for(int i = 0; i < (smp + 1); i++)
	{
		engine[i]->wait_process_frame();
		field1 += engine[i]->field1;
		field2 += engine[i]->field2;
	}
}

#define ABS(x) ((x) > 0 ? (x) : -(x))

// Pattern A B BC CD D
int IVTCMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();

//printf("IVTCMain::process_realtime 1\n");
	if(!engine)
	{
		temp_frame[0] = 0;
		temp_frame[1] = 0;
		state = 0;
		new_field = 0;
		average = 0;
		total_average = (int64_t)(project_frame_rate + 0.5);
	
		int y1, y2, y_increment;
		y_increment = input_ptr->get_h() / (smp + 1);
		y1 = 0;

		engine = new IVTCEngine*[(smp + 1)];
		for(int i = 0; i < (smp + 1); i++)
		{
			y2 = y1 + y_increment;
			if(i == (PluginClient::smp + 1) - 1 && 
				y2 < input_ptr->get_h() - 1) 
				y2 = input_ptr->get_h() - 1;
			engine[i] = new IVTCEngine(this, y1, y2);
			engine[i]->start();
			y1 += y_increment;
		}
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

// Determine pattern
	if(config.pattern == 1)
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
// Determine where in the pattern we are
	if(config.automatic)
	{
		int64_t field1;
		int64_t field2;

		compare_fields(temp_frame[0], 
			input_ptr, 
			field1,
			field2);

		int64_t field_difference = field2 - field1;
		int64_t threshold;


		temp_frame[1]->copy_from(input_ptr);

// Automatically generate field difference threshold using weighted average of previous frames
		if(average > 0)
			threshold = average;
		else
			threshold = ABS(field_difference);

//printf("IVTCMain::process_realtime 1 %d %lld %lld %lld\n", state, average, threshold, field_difference);
// CD
		if(state == 3)
		{
			state = 4;

// Compute new threshold for next time
			average = (int64_t)(average * total_average + 
				ABS(field_difference)) / (total_average + 1);

			for(int i = 0; i < input_ptr->get_h(); i++)
			{
				if((i + new_field) & 1)
					memcpy(output_ptr->get_rows()[i], 
						input_ptr->get_rows()[i],
						row_size);
				else
					memcpy(output_ptr->get_rows()[i],
						temp_frame[0]->get_rows()[i],
						row_size);
			}
		}
		else
// Neither field changed enough
// A or B or D
		if(ABS(field_difference) <= threshold ||
			state == 4)
		{
			state = 0;

// Compute new threshold for next time
			average = (int64_t)(average * total_average + 
				ABS(field_difference)) / (total_average + 1);

			if(input_ptr->get_rows()[0] != output_ptr->get_rows()[0])
				output_ptr->copy_from(input_ptr);
		}
		else
// Field 2 changed more than field 1
		if(field_difference > threshold)
		{
// BC bottom field new
			state = 3;
			new_field = 1;

// Compute new threshold for next time
			average = (int64_t)(average * total_average + 
				ABS(field_difference)) / (total_average + 1);

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
		}
		else
// Field 1 changed more than field 2
		if(field_difference < -threshold)
		{
// BC top field new
			state = 3;
			new_field = 0;

// Compute new threshold for next time
			average = (int64_t)(average * total_average + 
				ABS(field_difference)) / (total_average + 1);

			for(int i = 0; i < input_ptr->get_h(); i++)
			{
				if(i & 1)
					memcpy(output_ptr->get_rows()[i],
						input_ptr->get_rows()[i],
						row_size);
				else
					memcpy(output_ptr->get_rows()[i], 
						temp_frame[0]->get_rows()[i],
						row_size);
			}
		}

// Swap temp frames
		VFrame *temp = temp_frame[0];
		temp_frame[0] = temp_frame[1];
		temp_frame[1] = temp;
	}
	else
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
//printf("IVTCMain::process_realtime 2\n");

	return 0;
}

void IVTCMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->frame_offset->update((int64_t)config.frame_offset);
		thread->window->first_field->update(config.first_field);
		thread->window->automatic->update(config.automatic);
		thread->window->pattern[0]->update(config.pattern == 0);
		thread->window->pattern[1]->update(config.pattern == 1);
		thread->window->unlock_window();
	}
}




IVTCEngine::IVTCEngine(IVTCMain *plugin, int start_y, int end_y)
 : Thread()
{
	this->plugin = plugin;
	set_synchronous(1);
	this->start_y = start_y;
	this->end_y = end_y;
	input_lock.lock();
	output_lock.lock();
	last_frame = 0;
}

IVTCEngine::~IVTCEngine()
{
}


// Use all channels to get more info
#define COMPARE_ROWS(result, row1, row2, type, width, components) \
{ \
	for(int i = 0; i < width * components; i++) \
	{ \
		result += labs(((type*)row1)[i] - ((type*)row2)[i]); \
	} \
}

#define COMPARE_FIELDS(rows1, rows2, type, width, height, components) \
{ \
	int w = width * components; \
	int h = height; \
	 \
	for(int i = 0; i < h; i++) \
	{ \
		type *row1 = (type*)(rows1)[i]; \
		type *row2 = (type*)(rows2)[i]; \
 \
		if(i & 1) \
			for(int j = 0; j < w; j++) \
			{ \
				field2 += labs(row1[j] - row2[j]); \
			} \
		else \
			for(int j = 0; j < w; j++) \
			{ \
				field1 += labs(row1[j] - row2[j]); \
			} \
	} \
}

#define COMPARE_FIELDS_YUV(rows1, rows2, type, width, height, components) \
{ \
	int w = width * components; \
	int h = height; \
	 \
	for(int i = 0; i < h; i++) \
	{ \
		type *row1 = (type*)(rows1)[i]; \
		type *row2 = (type*)(rows2)[i]; \
 \
		if(i & 1) \
			for(int j = 0; j < w; j += components) \
			{ \
				field2 += labs(row1[j] - row2[j]); \
			} \
		else \
			for(int j = 0; j < w; j += components) \
			{ \
				field1 += labs(row1[j] - row2[j]); \
			} \
	} \
}


void IVTCEngine::run()
{
	while(1)
	{
		input_lock.lock();
		if(last_frame)
		{
			output_lock.unlock();
			return;
		}
		
		field1 = 0;
		field2 = 0;
		switch(input->get_color_model())
		{
			case BC_RGB888:
				COMPARE_FIELDS(input->get_rows() + start_y, 
					output->get_rows() + start_y, 
					unsigned char, 
					input->get_w(),
					end_y - start_y, 
					3);
				break;
			case BC_YUV888:
				COMPARE_FIELDS_YUV(input->get_rows() + start_y, 
					output->get_rows() + start_y, 
					unsigned char, 
					input->get_w(),
					end_y - start_y, 
					3);
				break;
			case BC_RGBA8888:
				COMPARE_FIELDS(input->get_rows() + start_y, 
					output->get_rows() + start_y, 
					unsigned char, 
					input->get_w(),
					end_y - start_y, 
					4);
					break;
			case BC_YUVA8888:
				COMPARE_FIELDS_YUV(input->get_rows() + start_y, 
					output->get_rows() + start_y, 
					unsigned char, 
					input->get_w(),
					end_y - start_y, 
					4);
					break;
			case BC_RGB161616:
				COMPARE_FIELDS(input->get_rows() + start_y, 
					output->get_rows() + start_y, 
					u_int16_t, 
					input->get_w(),
					end_y - start_y, 
					3);
				break;
			case BC_YUV161616:
				COMPARE_FIELDS_YUV(input->get_rows() + start_y, 
					output->get_rows() + start_y, 
					u_int16_t, 
					input->get_w(),
					end_y - start_y, 
					3);
				break;
			case BC_RGBA16161616:
				COMPARE_FIELDS(input->get_rows() + start_y, 
					output->get_rows() + start_y, 
					u_int16_t, 
					input->get_w(),
					end_y - start_y, 
					4);
				break;
			case BC_YUVA16161616:
				COMPARE_FIELDS_YUV(input->get_rows() + start_y, 
					output->get_rows() + start_y, 
					u_int16_t, 
					input->get_w(),
					end_y - start_y, 
					4);
				break;
		}
		output_lock.unlock();
	}
}


int IVTCEngine::start_process_frame(VFrame *output, VFrame *input)
{
	this->output = output;
	this->input = input;
	input_lock.unlock();
	return 0;
}

int IVTCEngine::wait_process_frame()
{
	output_lock.lock();
	return 0;
}
