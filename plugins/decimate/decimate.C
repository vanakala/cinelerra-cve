
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Decimate")
#define PLUGIN_CLASS Decimate
#define PLUGIN_CONFIG_CLASS DecimateConfig
#define PLUGIN_THREAD_CLASS DecimateThread
#define PLUGIN_GUI_CLASS DecimateWindow

#include "pluginmacros.h"

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
	PLUGIN_CONFIG_CLASS_MEMBERS
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

class DecimateWindow : public BC_Window
{
public:
	DecimateWindow(Decimate *plugin, int x, int y);
	~DecimateWindow();

	void update();

	ArrayList<BC_ListBoxItem*> frame_rates;
	DecimateRate *rate;
	DecimateRateMenu *rate_menu;
	BC_Title *last_dropped;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class Decimate : public PluginVClient
{
public:
	Decimate(PluginServer *server);
	~Decimate();

	void process_frame(VFrame *frame);

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void render_gui(void *data);

	int64_t calculate_difference(VFrame *frame1, VFrame *frame2);
	void fill_lookahead(ptstime start_pts);
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
	ptstime lookahead_end_pts;
// Framerate of lookahead buffer
	double lookahead_rate;
// Last requested position
	ptstime last_pts;
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
	x = y = 10;

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
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

DecimateWindow::~DecimateWindow()
{
	frame_rates.remove_all_objects();
}

void DecimateWindow::update()
{
	rate->update((float)plugin->config.input_rate);
}


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


DecimateRateMenu::DecimateRateMenu(Decimate *plugin, 
	DecimateWindow *gui, 
	int x, 
	int y)
 : BC_ListBox(x,
	y,
	100,
	200,
	&gui->frame_rates,
	LISTBOX_POPUP)
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


PLUGIN_THREAD_METHODS
REGISTER_PLUGIN


Decimate::Decimate(PluginServer *server)
 : PluginVClient(server)
{
	memset(frames, 0, sizeof(VFrame*) * TOTAL_FRAMES);
	for(int i = 0; i < TOTAL_FRAMES; i++)
		differences[i] = -1;
	lookahead_size = 0;
	lookahead_end_pts = -1;
	last_pts = -1;
	fdct_ready = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

Decimate::~Decimate()
{
	if(frames[0])
	{
		for(int i = 0; i < TOTAL_FRAMES; i++)
		{
			delete frames[i];
		}
	}
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

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
	memset(result, 0, sizeof(int64_t) * 64);
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
	for(int i = 0; i < result; i++)
	{
		frames[i]->copy_pts(frames[i + 1]);
	}
	for(int i = result; i < lookahead_size - 1; i++)
	{
		frames[i] = frames[i + 1];
		differences[i] = differences[i + 1];
	}

	frames[lookahead_size - 1] = temp;
	lookahead_size--;
	send_render_gui(&result);
}

void Decimate::fill_lookahead(ptstime start_pts)
{
	VFrame *frp;
	ptstime coef;

// Lookahead rate changed
	if(!EQUIV(config.input_rate, lookahead_rate))
	{
		lookahead_size = 0;
	}

	lookahead_rate = config.input_rate;
	coef = lookahead_rate / project_frame_rate;

// Start position is not contiguous with last request
	if(!PTSEQU(last_pts, start_pts))
	{
		lookahead_size = 0;
	}

	last_pts = start_pts + 1.0 / project_frame_rate;

// Calculate start of replacement
	if(!lookahead_size)
	{
		lookahead_end_pts = (start_pts - source_start_pts) * 
			coef + source_start_pts;
	}

	while(lookahead_size < TOTAL_FRAMES)
	{
// Import frame into next lookahead slot
		frp = frames[lookahead_size];
		frp->set_pts(lookahead_end_pts);
		get_frame(frp);

		lookahead_end_pts = frp->next_pts();

// Calculete dst pts and duration
		frp->set_pts((frp->get_pts() - source_start_pts) / coef + source_start_pts);
		frp->set_duration(frp->get_duration() / coef);

// Fill difference buffer
		if(lookahead_size > 0)
		{
			differences[lookahead_size] = 
				calculate_difference(frames[lookahead_size - 1], 
					frames[lookahead_size]);
		}
		lookahead_size++;

		if(lookahead_size >= TOTAL_FRAMES &&
				frames[0]->get_pts() < start_pts &&
				!frames[0]->pts_in_frame(start_pts))
		{
			decimate_frame();
		}
	}
}

void Decimate::process_frame(VFrame *frame)
{
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
	fill_lookahead(frame->get_pts());

// Pull first frame off lookahead
	frame->copy_from(frames[0]);

	frame->set_duration(1 / project_frame_rate);
	frame->set_pts(source_pts);

	VFrame *temp = frames[0];
	for(int i = 0; i < TOTAL_FRAMES - 1; i++)
	{
		frames[i] = frames[i + 1];
		differences[i] = differences[i + 1];
	}
	frames[TOTAL_FRAMES - 1] = temp;
	lookahead_size--;
}

int Decimate::load_configuration()
{
	KeyFrame *prev_keyframe;
	DecimateConfig old_config;
	old_config.copy_from(&config);
	prev_keyframe = prev_keyframe_pts(source_pts);
	read_data(prev_keyframe);
	return !old_config.equivalent(&config);
}

void Decimate::load_defaults()
{
	defaults = load_defaults_file("decimate.rc");

	config.input_rate = defaults->get("INPUT_RATE", config.input_rate);
	config.input_rate = Units::fix_framerate(config.input_rate);
}

void Decimate::save_defaults()
{
	defaults->update("INPUT_RATE", config.input_rate);
	defaults->save();
}

void Decimate::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("DECIMATE");
	output.tag.set_property("INPUT_RATE", config.input_rate);
	output.append_tag();
	output.tag.set_title("/DECIMATE");
	output.append_tag();
	output.terminate_string();
}

void Decimate::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("DECIMATE"))
		{
			config.input_rate = input.tag.get_property("INPUT_RATE", config.input_rate);
			config.input_rate = Units::fix_framerate(config.input_rate);
		}
	}
}

void Decimate::render_gui(void *data)
{
	if(thread)
	{
		int dropped = *(int*)data;
		char string[BCTEXTLEN];

		sprintf(string, "%d", dropped);
		thread->window->last_dropped->update(string);
	}
}
