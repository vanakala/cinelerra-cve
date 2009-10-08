
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
#include "language.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


class YUVEffect;


class YUVConfig
{
public:
	YUVConfig();

	void copy_from(YUVConfig &src);
	int equivalent(YUVConfig &src);
	void interpolate(YUVConfig &prev, 
		YUVConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);

	float y, u, v;
};

class YUVLevel : public BC_FSlider
{
public:
	YUVLevel(YUVEffect *plugin, float *output, int x, int y);
	int handle_event();
	YUVEffect *plugin;
	float *output;
};

class YUVWindow : public BC_Window
{
public:
	YUVWindow(YUVEffect *plugin, int x, int y);
	void create_objects();
	int close_event();
	YUVLevel *y, *u, *v;
	YUVEffect *plugin;
};

PLUGIN_THREAD_HEADER(YUVEffect, YUVThread, YUVWindow)

class YUVEffect : public PluginVClient
{
public:
	YUVEffect(PluginServer *server);
	~YUVEffect();
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int show_gui();
	void raise_window();
	int set_string();
	int load_configuration();

	YUVConfig config;
	YUVThread *thread;
	BC_Hash *defaults;
};





REGISTER_PLUGIN(YUVEffect)







YUVConfig::YUVConfig()
{
	y = 0;
	u = 0;
	v = 0;
}

void YUVConfig::copy_from(YUVConfig &src)
{
	y = src.y;
	u = src.u;
	v = src.v;
}

int YUVConfig::equivalent(YUVConfig &src)
{
	return EQUIV(y, src.y) && EQUIV(u, src.u) && EQUIV(v, src.v);
}

void YUVConfig::interpolate(YUVConfig &prev, 
	YUVConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	y = prev.y * prev_scale + next.y * next_scale;
	u = prev.u * prev_scale + next.u * next_scale;
	v = prev.v * prev_scale + next.v * next_scale;
}






#define MAXVALUE 100

YUVLevel::YUVLevel(YUVEffect *plugin, float *output, int x, int y)
 : BC_FSlider(x, 
			y,
			0,
			200, 
			200, 
			-MAXVALUE, 
			MAXVALUE, 
			*output)
{
	this->plugin = plugin;
	this->output = output;
}

int YUVLevel::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


YUVWindow::YUVWindow(YUVEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	260, 
	100, 
	260, 
	100, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

void YUVWindow::create_objects()
{
	int x = 10, y = 10, x1 = 50;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	add_subwindow(this->y = new YUVLevel(plugin, &plugin->config.y, x1, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("U:")));
	add_subwindow(u = new YUVLevel(plugin, &plugin->config.u, x1, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("V:")));
	add_subwindow(v = new YUVLevel(plugin, &plugin->config.v, x1, y));

	show_window();
	flush();
}

int YUVWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}





PLUGIN_THREAD_OBJECT(YUVEffect, YUVThread, YUVWindow)






YUVEffect::YUVEffect(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}
YUVEffect::~YUVEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* YUVEffect::plugin_title() { return N_("YUV"); }
int YUVEffect::is_realtime() { return 1; } 


NEW_PICON_MACRO(YUVEffect)
SHOW_GUI_MACRO(YUVEffect, YUVThread)
RAISE_WINDOW_MACRO(YUVEffect)
SET_STRING_MACRO(YUVEffect)
LOAD_CONFIGURATION_MACRO(YUVEffect, YUVConfig)

void YUVEffect::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		load_configuration();
		thread->window->y->update(config.y);
		thread->window->u->update(config.u);
		thread->window->v->update(config.v);
		thread->window->unlock_window();
	}
}

int YUVEffect::load_defaults()
{
	char directory[BCTEXTLEN];
	sprintf(directory, "%syuv.rc", BCASTDIR);
	defaults = new BC_Hash(directory);
	defaults->load();
	config.y = defaults->get("Y", config.y);
	config.u = defaults->get("U", config.u);
	config.v = defaults->get("V", config.v);
	return 0;
}

int YUVEffect::save_defaults()
{
	defaults->update("Y", config.y);
	defaults->update("U", config.u);
	defaults->update("V", config.v);
	defaults->save();
	return 0;
}

void YUVEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("YUV");
	output.tag.set_property("Y", config.y);
	output.tag.set_property("U", config.u);
	output.tag.set_property("V", config.v);
	output.append_tag();
	output.tag.set_title("/YUV");
	output.append_tag();
	output.terminate_string();
}

void YUVEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	while(!input.read_tag())
	{
		if(input.tag.title_is("YUV"))
		{
			config.y = input.tag.get_property("Y", config.y);
			config.u = input.tag.get_property("U", config.u);
			config.v = input.tag.get_property("V", config.v);
		}
	}
}


static YUV yuv_static;

#define YUV_MACRO(type, temp_type, max, components, use_yuv) \
{ \
	for(int i = 0; i < input->get_h(); i++) \
	{ \
		type *in_row = (type*)input->get_rows()[i]; \
		type *out_row = (type*)output->get_rows()[i]; \
		const float round = (sizeof(type) == 4) ? 0.0 : 0.5; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			if(use_yuv) \
			{ \
				int y = (int)((float)in_row[0] * y_scale + round); \
				int u = (int)((float)(in_row[1] - (max / 2 + 1)) * u_scale + round) + (max / 2 + 1); \
				int v = (int)((float)(in_row[2] - (max / 2 + 1)) * v_scale + round) + (max / 2 + 1); \
				out_row[0] = CLIP(y, 0, max); \
				out_row[1] = CLIP(u, 0, max); \
				out_row[2] = CLIP(v, 0, max); \
			} \
			else \
			{ \
				temp_type y, u, v, r, g, b; \
				if(sizeof(type) == 4) \
				{ \
					yuv_static.rgb_to_yuv_f(in_row[0], in_row[1], in_row[2], y, u, v); \
				} \
				else \
				if(sizeof(type) == 2) \
				{ \
					yuv_static.rgb_to_yuv_16(in_row[0], in_row[1], in_row[2], y, u, v); \
				} \
				else \
				{ \
					yuv_static.rgb_to_yuv_8(in_row[0], in_row[1], in_row[2], y, u, v); \
				} \
 \
 				if(sizeof(type) < 4) \
				{ \
					CLAMP(y, 0, max); \
					CLAMP(u, 0, max); \
					CLAMP(v, 0, max); \
 \
					y = temp_type((float)y * y_scale + round); \
					u = temp_type((float)(u - (max / 2 + 1)) * u_scale + round) + (max / 2 + 1); \
					v = temp_type((float)(v - (max / 2 + 1)) * v_scale + round) + (max / 2 + 1); \
				} \
				else \
				{ \
					y = temp_type((float)y * y_scale + round); \
					u = temp_type((float)u * u_scale + round); \
					v = temp_type((float)v * v_scale + round); \
				} \
 \
				if(sizeof(type) == 4) \
					yuv_static.yuv_to_rgb_f(r, g, b, y, u, v); \
				else \
				if(sizeof(type) == 2) \
					yuv_static.yuv_to_rgb_16(r, g, b, y, u, v); \
				else \
					yuv_static.yuv_to_rgb_8(r, g, b, y, u, v); \
 \
				out_row[0] = r; \
				out_row[1] = g; \
				out_row[2] = b; \
			} \
		 \
			in_row += components; \
			out_row += components; \
		} \
	} \
}

int YUVEffect::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();
	
	if(EQUIV(config.y, 0) && EQUIV(config.u, 0) && EQUIV(config.v, 0))
	{
		if(input->get_rows()[0] != output->get_rows()[0])
			output->copy_from(input);
	}
	else
	{
		int w = input->get_w();

		float y_scale = (float)(config.y + MAXVALUE) / MAXVALUE;
		float u_scale = (float)(config.u + MAXVALUE) / MAXVALUE;
		float v_scale = (float)(config.v + MAXVALUE) / MAXVALUE;

		if(u_scale > 1) u_scale = 1 + (u_scale - 1) * 4;
		if(v_scale > 1) v_scale = 1 + (v_scale - 1) * 4;

		switch(input->get_color_model())
		{
			case BC_RGB_FLOAT:
				YUV_MACRO(float, float, 1, 3, 0)
				break;

			case BC_RGB888:
				YUV_MACRO(unsigned char, int, 0xff, 3, 0)
				break;

			case BC_YUV888:
				YUV_MACRO(unsigned char, int, 0xff, 3, 1)
				break;

			case BC_RGB161616:
				YUV_MACRO(uint16_t, int, 0xffff, 3, 0)
				break;

			case BC_YUV161616:
				YUV_MACRO(uint16_t, int, 0xffff, 3, 1)
				break;

			case BC_RGBA_FLOAT:
				YUV_MACRO(float, float, 1, 4, 0)
				break;

			case BC_RGBA8888:
				YUV_MACRO(unsigned char, int, 0xff, 4, 0)
				break;

			case BC_YUVA8888:
				YUV_MACRO(unsigned char, int, 0xff, 4, 1)
				break;

			case BC_RGBA16161616:
				YUV_MACRO(uint16_t, int, 0xffff, 4, 0)
				break;

			case BC_YUVA16161616:
				YUV_MACRO(uint16_t, int, 0xffff, 4, 1)
				break;
		}



	}
	return 0;
}


