
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


#define TOP_FIELD_FIRST 0
#define BOTTOM_FIELD_FIRST 1
#define TOTAL_FRAMES 10

class Decimate;
class DecimateWindow;


class DecimateConfig
{
public:
	DecimateConfig();
	void copy_from(DecimateConfig *config);
	int equivalent(DecimateConfig *config);

	double input_rate;
// Averaged frames is useless.  Some of the frames are permanently
// destroyed in conversion from PAL to NTSC.
	int averaged_frames;
	int least_difference;
};




class DecimateRate : public BC_TextBox
{
public:
	DecimateRate(Decimate *plugin, 
		DecimateWindow *gui, 
		int x, 
		int y);
	int handle_event();
	Decimate *plugin;
	DecimateWindow *gui;
};

class DecimateRateMenu : public BC_ListBox
{
public:
	DecimateRateMenu(Decimate *plugin, 
		DecimateWindow *gui, 
		int x, 
		int y);
	int handle_event();
	Decimate *plugin;
	DecimateWindow *gui;
};

class DecimateDifference : public BC_CheckBox
{
public:
	DecimateDifference(Decimate *plugin,
		int x, 
		int y);
	int handle_event();
	Decimate *plugin;
};

class DecimateAvgDifference : public BC_CheckBox
{
public:
	DecimateAvgDifference(Decimate *plugin,
		int x, 
		int y);
	int handle_event();
	Decimate *plugin;
};


class DecimateWindow : public BC_Window
{
public:
	DecimateWindow(Decimate *plugin, int x, int y);
	~DecimateWindow();

	void create_objects();
	int close_event();

	ArrayList<BC_ListBoxItem*> frame_rates;
	Decimate *plugin;
	DecimateRate *rate;
	DecimateRateMenu *rate_menu;
	BC_Title *last_dropped;
//	DecimateDifference *difference;
//	DecimateAvgDifference *avg_difference;
};


PLUGIN_THREAD_HEADER(Decimate, DecimateThread, DecimateWindow)



class Decimate : public PluginVClient
{
public:
	Decimate(PluginServer *server);
	~Decimate();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int show_gui();
	int load_configuration();
	int set_string();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void raise_window();
	void update_gui();
	void render_gui(void *data);

	int64_t calculate_difference(VFrame *frame1, VFrame *frame2);
	void fill_lookahead(double frame_rate,
		int64_t start_position);
	void decimate_frame();
	void init_fdct();
	void fdct(uint16_t *block);
	int64_t calculate_fdct(VFrame *frame);

// fdct coefficients
	double c[8][8];
	int fdct_ready;

// each difference is the difference between the previous frame and the 
// subscripted frame
	int64_t differences[TOTAL_FRAMES];

// read ahead number of frames
	VFrame *frames[TOTAL_FRAMES];
// Number of frames in the lookahead buffer
	int lookahead_size;
// Next position beyond end of lookahead buffer relative to input rate
	int64_t lookahead_end;
// Framerate of lookahead buffer
	double lookahead_rate;
// Last requested position
	int64_t last_position;

	DecimateThread *thread;
	DecimateConfig config;
	BC_Hash *defaults;
};












DecimateConfig::DecimateConfig()
{
	input_rate = (double)30000 / 1001;
	least_difference = 1;
	averaged_frames = 0;
}

void DecimateConfig::copy_from(DecimateConfig *config)
{
	this->input_rate = config->input_rate;
	this->least_difference = config->least_difference;
	this->averaged_frames = config->averaged_frames;
}

int DecimateConfig::equivalent(DecimateConfig *config)
{
	return EQUIV(this->input_rate, config->input_rate);
}









DecimateWindow::DecimateWindow(Decimate *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	210, 
	160, 
	200, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

DecimateWindow::~DecimateWindow()
{
	frame_rates.remove_all_objects();
}

void DecimateWindow::create_objects()
{
	int x = 10, y = 10;

	frame_rates.append(new BC_ListBoxItem("1"));
	frame_rates.append(new BC_ListBoxItem("5"));
	frame_rates.append(new BC_ListBoxItem("10"));
	frame_rates.append(new BC_ListBoxItem("12"));
	frame_rates.append(new BC_ListBoxItem("15"));
	frame_rates.append(new BC_ListBoxItem("23.97"));
	frame_rates.append(new BC_ListBoxItem("24"));
	frame_rates.append(new BC_ListBoxItem("25"));
	frame_rates.append(new BC_ListBoxItem("29.97"));
	frame_rates.append(new BC_ListBoxItem("30"));
	frame_rates.append(new BC_ListBoxItem("50"));
	frame_rates.append(new BC_ListBoxItem("59.94"));
	frame_rates.append(new BC_ListBoxItem("60"));

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Input frames per second:")));
	y += 30;
	add_subwindow(rate = new DecimateRate(plugin, 
		this, 
		x, 
		y));
	add_subwindow(rate_menu = new DecimateRateMenu(plugin, 
		this, 
		x + rate->get_w() + 5, 
		y));
	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Last frame dropped: ")));
	add_subwindow(last_dropped = new BC_Title(x + title->get_w() + 5, y, ""));

// 	y += 30;
// 	add_subwindow(difference = new DecimateDifference(plugin,
// 		x, 
// 		y));
// 	y += 30;
// 	add_subwindow(avg_difference = new DecimateAvgDifference(plugin,
// 		x, 
// 		y));
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(DecimateWindow)












DecimateRate::DecimateRate(Decimate *plugin, 
	DecimateWindow *gui, 
	int x, 
	int y)
 : BC_TextBox(x, 
	y, 
	90,
	1,
	(float)plugin->config.input_rate)
{
	this->plugin = plugin;
	this->gui = gui;
}

int DecimateRate::handle_event()
{
	plugin->config.input_rate = Units::atoframerate(get_text());
	plugin->send_configure_change();
	return 1;
}



// DecimateDifference::DecimateDifference(Decimate *plugin,
// 	int x, 
// 	int y)
//  : BC_CheckBox(x, y, plugin->config.least_difference, "Drop least difference")
// {
// 	this->plugin = plugin;
// }
// int DecimateDifference::handle_event()
// {
// 	plugin->config.least_difference = get_value();
// 	plugin->send_configure_change();
// 	return 1;
// }
// 
// 
// 
// 
// DecimateAvgDifference::DecimateAvgDifference(Decimate *plugin,
// 	int x, 
// 	int y)
//  : BC_CheckBox(x, y, plugin->config.averaged_frames, "Drop averaged frames")
// {
// 	this->plugin = plugin;
// }
// 
// int DecimateAvgDifference::handle_event()
// {
// 	plugin->config.averaged_frames = get_value();
// 	plugin->send_configure_change();
// 	return 1;
// }
// 



DecimateRateMenu::DecimateRateMenu(Decimate *plugin, 
	DecimateWindow *gui, 
	int x, 
	int y)
 : BC_ListBox(x,
 	y,
	100,
	200,
	LISTBOX_TEXT,
	&gui->frame_rates,
	0,
	0,
	1,
	0,
	1)
{
	this->plugin = plugin;
	this->gui = gui;
}

int DecimateRateMenu::handle_event()
{
	char *text = get_selection(0, 0)->get_text();
	plugin->config.input_rate = atof(text);
	gui->rate->update(text);
	plugin->send_configure_change();
	return 1;
}











PLUGIN_THREAD_OBJECT(Decimate, DecimateThread, DecimateWindow)










REGISTER_PLUGIN(Decimate)






Decimate::Decimate(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	bzero(frames, sizeof(VFrame*) * TOTAL_FRAMES);
	for(int i = 0; i < TOTAL_FRAMES; i++)
		differences[i] = -1;
	lookahead_size = 0;
	lookahead_end = -1;
	last_position = -1;
	fdct_ready = 0;
}


Decimate::~Decimate()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(frames[0])
	{
		for(int i = 0; i < TOTAL_FRAMES; i++)
		{
			delete frames[i];
		}
	}
}

#define DIFFERENCE_MACRO(type, temp_type, components) \
{ \
	temp_type result2 = 0; \
	for(int i = 0; i < h; i++) \
	{ \
		type *row1 = (type*)frame1->get_rows()[i]; \
		type *row2 = (type*)frame2->get_rows()[i]; \
		for(int j = 0; j < w * components; j++) \
		{ \
			temp_type temp = *row1 - *row2; \
			result2 += (temp > 0 ? temp : -temp); \
			row1++; \
			row2++; \
		} \
	} \
	result = (int64_t)result2; \
}

int64_t Decimate::calculate_difference(VFrame *frame1, VFrame *frame2)
{
	int w = frame1->get_w();
	int h = frame1->get_h();
	int64_t result = 0;
	switch(frame1->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			DIFFERENCE_MACRO(unsigned char, int64_t, 3);
			break;
		case BC_RGB_FLOAT:
			DIFFERENCE_MACRO(float, double, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			DIFFERENCE_MACRO(unsigned char, int64_t, 4);
			break;
		case BC_RGBA_FLOAT:
			DIFFERENCE_MACRO(float, double, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			DIFFERENCE_MACRO(uint16_t, int64_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			DIFFERENCE_MACRO(uint16_t, int64_t, 4);
			break;
	}
	return result;
}

void Decimate::init_fdct()
{
  int i, j;
  double s;

  for (i=0; i<8; i++)
  {
    s = (i==0) ? sqrt(0.125) : 0.5;

    for (j=0; j<8; j++)
      c[i][j] = s * cos((M_PI/8.0)*i*(j+0.5));
  }
}

void Decimate::fdct(uint16_t *block)
{
	int i, j;
	double s;
	double tmp[64];

	for(i = 0; i < 8; i++)
    	for(j = 0; j < 8; j++)
    	{
    		s = 0.0;

/*
 *     		for(k = 0; k < 8; k++)
 *         		s += c[j][k] * block[8 * i + k];
 */
        	s += c[j][0] * block[8 * i + 0];
        	s += c[j][1] * block[8 * i + 1];
        	s += c[j][2] * block[8 * i + 2];
        	s += c[j][3] * block[8 * i + 3];
        	s += c[j][4] * block[8 * i + 4];
        	s += c[j][5] * block[8 * i + 5];
        	s += c[j][6] * block[8 * i + 6];
        	s += c[j][7] * block[8 * i + 7];

    		tmp[8 * i + j] = s;
    	}

	for(j = 0; j < 8; j++)
    	for(i = 0; i < 8; i++)
    	{
    		s = 0.0;

/*
 *     	  	for(k = 0; k < 8; k++)
 *        	    s += c[i][k] * tmp[8 * k + j];
 */
        	s += c[i][0] * tmp[8 * 0 + j];
        	s += c[i][1] * tmp[8 * 1 + j];
        	s += c[i][2] * tmp[8 * 2 + j];
        	s += c[i][3] * tmp[8 * 3 + j];
        	s += c[i][4] * tmp[8 * 4 + j];
        	s += c[i][5] * tmp[8 * 5 + j];
        	s += c[i][6] * tmp[8 * 6 + j];
        	s += c[i][7] * tmp[8 * 7 + j];

    		block[8 * i + j] = (int)floor(s + 0.499999);
/*
 * reason for adding 0.499999 instead of 0.5:
 * s is quite often x.5 (at least for i and/or j = 0 or 4)
 * and setting the rounding threshold exactly to 0.5 leads to an
 * extremely high arithmetic implementation dependency of the result;
 * s being between x.5 and x.500001 (which is now incorrectly rounded
 * downwards instead of upwards) is assumed to occur less often
 * (if at all)
 */
      }
}


#define CALCULATE_DCT(type, components) \
{ \
	uint16_t *output = temp; \
	for(int k = 0; k < 8; k++) \
	{ \
		type *input = (type*)frame->get_rows()[i + k] + j * components; \
		for(int l = 0; l < 8; l++) \
		{ \
			*output = (*input << 8) | *input; \
			output++; \
			input += components; \
		} \
	} \
	fdct(temp); \
}

int64_t Decimate::calculate_fdct(VFrame *frame)
{
	if(!fdct_ready)
	{
		init_fdct();
		fdct_ready = 1;
	}

	uint16_t temp[64];
	uint64_t result[64];
	bzero(result, sizeof(int64_t) * 64);
	int w = frame->get_w();
	int h = frame->get_h();


	for(int i = 0; i < h - 8; i += 8)
	{
		for(int j = 0; j < w - 8; j += 8)
		{
			CALCULATE_DCT(unsigned char, 3)
// Add result to accumulation of transforms
			for(int k = 0; k < 64; k++)
			{
				result[k] += temp[k];
			}
		}
	}

	uint64_t max_result = 0;
	int highest = 0;
	for(int i = 0; i < 64; i++)
	{
		if(result[i] > max_result)
		{
			max_result = result[i];
			highest = i;
		}
	}

	return highest;
}

void Decimate::decimate_frame()
{
	int64_t min_difference = 0x7fffffffffffffffLL;
	int64_t max_difference2 = 0x0;
	int result = -1;

	if(!lookahead_size) return;

	for(int i = 0; i < lookahead_size; i++)
	{
// Drop least different frame from sequence
		if(config.least_difference && 
			differences[i] >= 0 &&
			differences[i] < min_difference)
		{
			min_difference = differences[i];
			result = i;
		}
	}

// If all the frames had differences of 0, like a pure black screen, delete
// the first frame.
	if(result < 0) result = 0;

	VFrame *temp = frames[result];
	for(int i = result; i < lookahead_size - 1; i++)
	{
		frames[i] = frames[i + 1];
		differences[i] = differences[i + 1];
	}


	frames[lookahead_size - 1] = temp;
	lookahead_size--;
	send_render_gui(&result);
}

void Decimate::fill_lookahead(double frame_rate,
	int64_t start_position)
{
// Lookahead rate changed
	if(!EQUIV(config.input_rate, lookahead_rate))
	{
		lookahead_size = 0;
	}

	lookahead_rate = config.input_rate;

// Start position is not contiguous with last request
	if(last_position + 1 != start_position)
	{
		lookahead_size = 0;
	}

	last_position = start_position;

// Normalize requested position to input rate
	if(!lookahead_size)
	{
		lookahead_end = (int64_t)((double)start_position * 
			config.input_rate / 
			frame_rate);
	}

	while(lookahead_size < TOTAL_FRAMES)
	{
// Import frame into next lookahead slot
		read_frame(frames[lookahead_size], 
			0, 
			lookahead_end, 
			config.input_rate);
// Fill difference buffer
		if(lookahead_size > 0)
			differences[lookahead_size] = 
				calculate_difference(frames[lookahead_size - 1], 
					frames[lookahead_size]);

// Increase counters relative to input rate
		lookahead_size++;
		lookahead_end++;

// Decimate one if last frame in buffer and lookahead_end is behind predicted
// end.
		int64_t decimated_end = (int64_t)((double)(start_position + TOTAL_FRAMES) *
			config.input_rate / 
			frame_rate);
		if(lookahead_size >= TOTAL_FRAMES &&
			lookahead_end < decimated_end)
		{
			decimate_frame();
		}
	}
}


int Decimate::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{

//printf("Decimate::process_buffer 1 %lld %f\n", start_position, frame_rate);
	load_configuration();

	if(!frames[0])
	{
		for(int i = 0; i < TOTAL_FRAMES; i++)
		{
			frames[i] = new VFrame(0,
				frame->get_w(),
				frame->get_h(),
				frame->get_color_model(),
				-1);
		}
	}


// Fill lookahead buffer at input rate with decimation
	fill_lookahead(frame_rate, start_position);

// printf("Decimate::process_buffer");
// for(int i = 0; i < TOTAL_FRAMES; i++)
// printf(" %lld", differences[i]);
// printf("\n");


// Pull first frame off lookahead
	frame->copy_from(frames[0]);
	VFrame *temp = frames[0];
	for(int i = 0; i < TOTAL_FRAMES - 1; i++)
	{
		frames[i] = frames[i + 1];
		differences[i] = differences[i + 1];
	}
	frames[TOTAL_FRAMES - 1] = temp;
	lookahead_size--;
	return 0;
}



char* Decimate::plugin_title() { return N_("Decimate"); }
int Decimate::is_realtime() { return 1; }

NEW_PICON_MACRO(Decimate) 

SHOW_GUI_MACRO(Decimate, DecimateThread)

RAISE_WINDOW_MACRO(Decimate)

SET_STRING_MACRO(Decimate);

int Decimate::load_configuration()
{
	KeyFrame *prev_keyframe;
	DecimateConfig old_config;
	old_config.copy_from(&config);
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return !old_config.equivalent(&config);
}

int Decimate::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sdecimate.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.input_rate = defaults->get("INPUT_RATE", config.input_rate);
//	config.averaged_frames = defaults->get("AVERAGED_FRAMES", config.averaged_frames);
//	config.least_difference = defaults->get("LEAST_DIFFERENCE", config.least_difference);
	config.input_rate = Units::fix_framerate(config.input_rate);
	return 0;
}

int Decimate::save_defaults()
{
	defaults->update("INPUT_RATE", config.input_rate);
//	defaults->update("AVERAGED_FRAMES", config.averaged_frames);
//	defaults->update("LEAST_DIFFERENCE", config.least_difference);
	defaults->save();
	return 0;
}

void Decimate::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("DECIMATE");
	output.tag.set_property("INPUT_RATE", config.input_rate);
//	output.tag.set_property("AVERAGED_FRAMES", config.averaged_frames);
//	output.tag.set_property("LEAST_DIFFERENCE", config.least_difference);
	output.append_tag();
	output.tag.set_title("/DECIMATE");
	output.append_tag();
	output.terminate_string();
}

void Decimate::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("DECIMATE"))
		{
			config.input_rate = input.tag.get_property("INPUT_RATE", config.input_rate);
//			config.averaged_frames = input.tag.get_property("AVERAGED_FRAMES", config.averaged_frames);
//			config.least_difference = input.tag.get_property("LEAST_DIFFERENCE", config.least_difference);
			config.input_rate = Units::fix_framerate(config.input_rate);
		}
	}
}

void Decimate::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("Decimate::update_gui");
			thread->window->rate->update((float)config.input_rate);
//  		thread->window->difference->update(config.least_difference);
//  		thread->window->avg_difference->update(config.averaged_frames);
			thread->window->unlock_window();
		}
	}
}

void Decimate::render_gui(void *data)
{
	if(thread)
	{
		thread->window->lock_window("Decimate::render_gui");

		int dropped = *(int*)data;
		char string[BCTEXTLEN];

		sprintf(string, "%d", dropped);
		thread->window->last_dropped->update(string);

		thread->window->unlock_window();
	}
}



