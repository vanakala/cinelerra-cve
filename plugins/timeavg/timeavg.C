#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "timeavg.h"
#include "timeavgwindow.h"
#include "vframe.h"




#include <stdint.h>
#include <string.h>




REGISTER_PLUGIN(TimeAvgMain)





TimeAvgConfig::TimeAvgConfig()
{
	frames = 1;
	accumulate = 0;
	paranoid = 0;
}

void TimeAvgConfig::copy_from(TimeAvgConfig *src)
{
	this->frames = src->frames;
	this->accumulate = src->accumulate;
	this->paranoid = src->paranoid;
}

int TimeAvgConfig::equivalent(TimeAvgConfig *src)
{
	return frames == src->frames &&
		accumulate == src->accumulate &&
		paranoid == src->paranoid;
}













TimeAvgMain::TimeAvgMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	accumulation = 0;
	history = 0;
	history_size = 0;
	history_start = -0x7fffffff;
	history_frame = 0;
	history_valid = 0;
}

TimeAvgMain::~TimeAvgMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(accumulation) delete [] accumulation;
	if(history)
	{
		for(int i = 0; i < config.frames; i++)
			delete history[i];
		delete [] history;
	}
	if(history_frame) delete [] history_frame;
	if(history_valid) delete [] history_valid;
}

char* TimeAvgMain::plugin_title() { return N_("Time Average"); }
int TimeAvgMain::is_realtime() { return 1; }


NEW_PICON_MACRO(TimeAvgMain)

SHOW_GUI_MACRO(TimeAvgMain, TimeAvgThread)

SET_STRING_MACRO(TimeAvgMain)

RAISE_WINDOW_MACRO(TimeAvgMain);



int TimeAvgMain::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int h = frame->get_h();
	int w = frame->get_w();
	int color_model = frame->get_color_model();

	load_configuration();

// Allocate accumulation
	if(!accumulation)
	{
		accumulation = new unsigned char[w * 
			h * 
			cmodel_components(color_model) *
			MAX(sizeof(float), sizeof(int))];
		clear_accum(w, h, color_model);
	}

// Reallocate history
	if(history)
	{
		if(config.frames != history_size)
		{
			VFrame **history2;
			int64_t *history_frame2;
			int *history_valid2;
			history2 = new VFrame*[config.frames];
			history_frame2 = new int64_t[config.frames];
			history_valid2 = new int[config.frames];

// Copy existing frames over
			int i, j;
			for(i = 0, j = 0; i < config.frames && j < history_size; i++, j++)
			{
				history2[i] = history[j];
				history_frame2[i] = history_frame[i];
				history_valid2[i] = history_valid[i];
			}

// Delete extra previous frames and subtract from accumulation
			for( ; j < history_size; j++)
			{
				subtract_accum(history[j]);
				delete history[j];
			}
			delete [] history;
			delete [] history_frame;
			delete [] history_valid;


// Create new frames
			for( ; i < config.frames; i++)
			{
				history2[i] = new VFrame(0, w, h, color_model);
				history_frame2[i] = -0x7fffffff;
				history_valid2[i] = 0;
			}

			history = history2;
			history_frame = history_frame2;
			history_valid = history_valid2;

			history_size = config.frames;
		}
	}
	else
// Allocate history
	{
		history = new VFrame*[config.frames];
		for(int i = 0; i < config.frames; i++)
			history[i] = new VFrame(0, w, h, color_model);
		history_size = config.frames;
		history_frame = new int64_t[config.frames];
		bzero(history_frame, sizeof(int64_t) * config.frames);
		history_valid = new int[config.frames];
		bzero(history_valid, sizeof(int) * config.frames);
	}






// Create new history frames based on current frame
	int64_t *new_history_frames = new int64_t[history_size];
	for(int i = 0; i < history_size; i++)
	{
		new_history_frames[history_size - i - 1] = start_position - i;
	}

// Subtract old history frames which are not in the new vector
	int no_change = 1;
	for(int i = 0; i < history_size; i++)
	{
// Old frame is valid
		if(history_valid[i])
		{
			int got_it = 0;
			for(int j = 0; j < history_size; j++)
			{
// Old frame is equal to a new frame
				if(history_frame[i] == new_history_frames[j]) 
				{
					got_it = 1;
					break;
				}
			}

// Didn't find old frame in new frames
			if(!got_it)
			{
				subtract_accum(history[i]);
				history_valid[i] = 0;
				no_change = 0;
			}
		}
	}

// If all frames are still valid, assume tweek occurred upstream and reload.
	if(config.paranoid && no_change)
	{
		for(int i = 0; i < history_size; i++)
		{
			history_valid[i] = 0;
		}
		clear_accum(w, h, color_model);
	}


// Add new history frames which are not in the old vector
	for(int i = 0; i < history_size; i++)
	{
// Find new frame in old vector
		int got_it = 0;
		for(int j = 0; j < history_size; j++)
		{
			if(history_valid[j] && history_frame[j] == new_history_frames[i])
			{
				got_it = 1;
				break;
			}
		}

// Didn't find new frame in old vector
		if(!got_it)
		{
// Get first unused entry
			for(int j = 0; j < history_size; j++)
			{
				if(!history_valid[j])
				{
// Load new frame into it
					history_frame[j] = new_history_frames[i];
					history_valid[j] = 1;
					read_frame(history[j],
						0,
						history_frame[j],
						frame_rate);
					add_accum(history[j]);
					break;
				}
			}
		}
	}

	delete [] new_history_frames;




// Transfer accumulation to output with division if average is desired.
	transfer_accum(frame);



	return 0;
}










// Reset accumulation
#define CLEAR_ACCUM(type, components, chroma) \
{ \
	type *row = (type*)accumulation; \
	if(chroma) \
	{ \
		for(int i = 0; i < w * h; i++) \
		{ \
			*row++ = 0x0; \
			*row++ = chroma; \
			*row++ = chroma; \
			if(components == 4) *row++ = 0x0; \
		} \
	} \
	else \
	{ \
		bzero(row, w * h * sizeof(type) * components); \
	} \
}


void TimeAvgMain::clear_accum(int w, int h, int color_model)
{
	switch(color_model)
	{
		case BC_RGB888:
			CLEAR_ACCUM(int, 3, 0x0)
			break;
		case BC_RGB_FLOAT:
			CLEAR_ACCUM(float, 3, 0x0)
			break;
		case BC_RGBA8888:
			CLEAR_ACCUM(int, 4, 0x0)
			break;
		case BC_RGBA_FLOAT:
			CLEAR_ACCUM(float, 4, 0x0)
			break;
		case BC_YUV888:
			CLEAR_ACCUM(int, 3, 0x80)
			break;
		case BC_YUVA8888:
			CLEAR_ACCUM(int, 4, 0x80)
			break;
		case BC_YUV161616:
			CLEAR_ACCUM(int, 3, 0x8000)
			break;
		case BC_YUVA16161616:
			CLEAR_ACCUM(int, 4, 0x8000)
			break;
	}
}


#define SUBTRACT_ACCUM(type, accum_type, components, chroma) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		accum_type *accum_row = (accum_type*)accumulation + \
			i * w * components; \
		type *frame_row = (type*)frame->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			*accum_row++ -= *frame_row++; \
			*accum_row++ -= (accum_type)*frame_row++ - chroma; \
			*accum_row++ -= (accum_type)*frame_row++ - chroma; \
			if(components == 4) *accum_row++ -= *frame_row++; \
		} \
	} \
}


void TimeAvgMain::subtract_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB888:
			SUBTRACT_ACCUM(unsigned char, int, 3, 0x0)
			break;
		case BC_RGB_FLOAT:
			SUBTRACT_ACCUM(float, float, 3, 0x0)
			break;
		case BC_RGBA8888:
			SUBTRACT_ACCUM(unsigned char, int, 4, 0x0)
			break;
		case BC_RGBA_FLOAT:
			SUBTRACT_ACCUM(float, float, 4, 0x0)
			break;
		case BC_YUV888:
			SUBTRACT_ACCUM(unsigned char, int, 3, 0x80)
			break;
		case BC_YUVA8888:
			SUBTRACT_ACCUM(unsigned char, int, 4, 0x80)
			break;
		case BC_YUV161616:
			SUBTRACT_ACCUM(uint16_t, int, 3, 0x8000)
			break;
		case BC_YUVA16161616:
			SUBTRACT_ACCUM(uint16_t, int, 4, 0x8000)
			break;
	}
}


#define ADD_ACCUM(type, accum_type, components, chroma) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		accum_type *accum_row = (accum_type*)accumulation + \
			i * w * components; \
		type *frame_row = (type*)frame->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			*accum_row++ += *frame_row++; \
			*accum_row++ += (accum_type)*frame_row++ - chroma; \
			*accum_row++ += (accum_type)*frame_row++ - chroma; \
			if(components == 4) *accum_row++ += *frame_row++; \
		} \
	} \
}


void TimeAvgMain::add_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB888:
			ADD_ACCUM(unsigned char, int, 3, 0x0)
			break;
		case BC_RGB_FLOAT:
			ADD_ACCUM(float, float, 3, 0x0)
			break;
		case BC_RGBA8888:
			ADD_ACCUM(unsigned char, int, 4, 0x0)
			break;
		case BC_RGBA_FLOAT:
			ADD_ACCUM(float, float, 4, 0x0)
			break;
		case BC_YUV888:
			ADD_ACCUM(unsigned char, int, 3, 0x80)
			break;
		case BC_YUVA8888:
			ADD_ACCUM(unsigned char, int, 4, 0x80)
			break;
		case BC_YUV161616:
			ADD_ACCUM(uint16_t, int, 3, 0x8000)
			break;
		case BC_YUVA16161616:
			ADD_ACCUM(uint16_t, int, 4, 0x8000)
			break;
	}
}

#define TRANSFER_ACCUM(type, accum_type, components, chroma, max) \
{ \
	if(!config.accumulate) \
	{ \
		accum_type denominator = history_size; \
		for(int i = 0; i < h; i++) \
		{ \
			accum_type *accum_row = (accum_type*)accumulation + \
				i * w * components; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
				*frame_row++ = *accum_row++ / denominator; \
				*frame_row++ = (*accum_row++ - chroma) / denominator + chroma; \
				*frame_row++ = (*accum_row++ - chroma) / denominator + chroma; \
				if(components == 4) *frame_row++ = *accum_row++ / denominator; \
			} \
		} \
	} \
	else \
	{ \
		for(int i = 0; i < h; i++) \
		{ \
			accum_type *accum_row = (accum_type*)accumulation + \
				i * w * components; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
				if(sizeof(type) < 4) \
				{ \
					accum_type r = *accum_row++; \
					accum_type g = *accum_row++ + chroma; \
					accum_type b = *accum_row++ + chroma; \
					*frame_row++ = CLIP(r, 0, max); \
					*frame_row++ = CLIP(g, 0, max); \
					*frame_row++ = CLIP(b, 0, max); \
					if(components == 4) \
					{ \
						accum_type a = *accum_row++; \
						*frame_row++ = CLIP(a, 0, max); \
					} \
				} \
				else \
				{ \
					*frame_row++ = *accum_row++; \
					*frame_row++ = *accum_row++ + chroma; \
					*frame_row++ = *accum_row++ + chroma; \
					if(components == 4) \
					{ \
						*frame_row++ = *accum_row++; \
					} \
				} \
			} \
		} \
	} \
}


void TimeAvgMain::transfer_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB888:
			TRANSFER_ACCUM(unsigned char, int, 3, 0x0, 0xff)
			break;
		case BC_RGB_FLOAT:
			TRANSFER_ACCUM(float, float, 3, 0x0, 1)
			break;
		case BC_RGBA8888:
			TRANSFER_ACCUM(unsigned char, int, 4, 0x0, 0xff)
			break;
		case BC_RGBA_FLOAT:
			TRANSFER_ACCUM(float, float, 4, 0x0, 1)
			break;
		case BC_YUV888:
			TRANSFER_ACCUM(unsigned char, int, 3, 0x80, 0xff)
			break;
		case BC_YUVA8888:
			TRANSFER_ACCUM(unsigned char, int, 4, 0x80, 0xff)
			break;
		case BC_YUV161616:
			TRANSFER_ACCUM(uint16_t, int, 3, 0x8000, 0xffff)
			break;
		case BC_YUVA16161616:
			TRANSFER_ACCUM(uint16_t, int, 4, 0x8000, 0xffff)
			break;
	}
}


int TimeAvgMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%stimeavg.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.frames = defaults->get("FRAMES", config.frames);
	config.accumulate = defaults->get("ACCUMULATE", config.accumulate);
	config.paranoid = defaults->get("PARANOID", config.paranoid);
	return 0;
}

int TimeAvgMain::save_defaults()
{
	defaults->update("FRAMES", config.frames);
	defaults->update("ACCUMULATE", config.accumulate);
	defaults->update("PARANOID", config.paranoid);
	defaults->save();
	return 0;
}

int TimeAvgMain::load_configuration()
{
	KeyFrame *prev_keyframe;
	TimeAvgConfig old_config;
	old_config.copy_from(&config);

	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return !old_config.equivalent(&config);
}

void TimeAvgMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("TIME_AVERAGE");
	output.tag.set_property("FRAMES", config.frames);
	output.tag.set_property("ACCUMULATE", config.accumulate);
	output.tag.set_property("PARANOID", config.paranoid);
	output.append_tag();
	output.terminate_string();
}

void TimeAvgMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("TIME_AVERAGE"))
		{
			config.frames = input.tag.get_property("FRAMES", config.frames);
			config.accumulate = input.tag.get_property("ACCUMULATE", config.accumulate);
			config.paranoid = input.tag.get_property("PARANOID", config.paranoid);
		}
	}
}


void TimeAvgMain::update_gui()
{
	if(thread) 
	{
		if(load_configuration())
		{
			thread->window->lock_window("TimeAvgMain::update_gui");
			thread->window->total_frames->update(config.frames);
			thread->window->accum->update(config.accumulate);
			thread->window->avg->update(!config.accumulate);
			thread->window->paranoid->update(config.paranoid);
			thread->window->unlock_window();
		}
	}
}










