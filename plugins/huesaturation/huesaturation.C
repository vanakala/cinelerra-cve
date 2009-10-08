
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
#include "loadbalance.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "playback3d.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>


class HueEffect;

#define MINHUE -180
#define MAXHUE 180
#define MINSATURATION -100
#define MAXSATURATION 100
#define MINVALUE -100
#define MAXVALUE 100






class HueConfig
{
public:
	HueConfig();
	
	void copy_from(HueConfig &src);
	int equivalent(HueConfig &src);
	void interpolate(HueConfig &prev, 
		HueConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	float hue, saturation, value;
};

class HueSlider : public BC_FSlider
{
public:
	HueSlider(HueEffect *plugin, int x, int y, int w);
	int handle_event();
	HueEffect *plugin;
};

class SaturationSlider : public BC_FSlider
{
public:
	SaturationSlider(HueEffect *plugin, int x, int y, int w);
	int handle_event();
	char* get_caption();
	HueEffect *plugin;
	char string[BCTEXTLEN];
};

class ValueSlider : public BC_FSlider
{
public:
	ValueSlider(HueEffect *plugin, int x, int y, int w);
	int handle_event();
	char* get_caption();
	HueEffect *plugin;
	char string[BCTEXTLEN];
};

class HueWindow : public BC_Window
{
public:
	HueWindow(HueEffect *plugin, int x, int y);
	void create_objects();
	int close_event();
	HueEffect *plugin;
	HueSlider *hue;
	SaturationSlider *saturation;
	ValueSlider *value;
};

PLUGIN_THREAD_HEADER(HueEffect, HueThread, HueWindow)

class HueEngine : public LoadServer
{
public:
	HueEngine(HueEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	HueEffect *plugin;
};

class HuePackage : public LoadPackage
{
public:
	HuePackage();
	int row1, row2;
};

class HueUnit : public LoadClient
{
public:
	HueUnit(HueEffect *plugin, HueEngine *server);
	void process_package(LoadPackage *package);
	HueEffect *plugin;
	YUV yuv;
};

class HueEffect : public PluginVClient
{
public:
	HueEffect(PluginServer *server);
	~HueEffect();
	
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_configuration();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int show_gui();
	int set_string();
	void raise_window();
	void update_gui();
	int handle_opengl();

	HueConfig config;
	VFrame *input, *output;
	BC_Hash *defaults;
	HueThread *thread;
	HueEngine *engine;
};




















HueConfig::HueConfig()
{
	hue = saturation = value = 0;
}
	
void HueConfig::copy_from(HueConfig &src)
{
	hue = src.hue;
	saturation = src.saturation;
	value = src.value;
}
int HueConfig::equivalent(HueConfig &src)
{
	return EQUIV(hue, src.hue) && 
		EQUIV(saturation, src.saturation) && 
		EQUIV(value, src.value);
}
void HueConfig::interpolate(HueConfig &prev, 
	HueConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->hue = prev.hue * prev_scale + next.hue * next_scale;
	this->saturation = prev.saturation * prev_scale + next.saturation * next_scale;
	this->value = prev.value * prev_scale + next.value * next_scale;
}








HueSlider::HueSlider(HueEffect *plugin, int x, int y, int w)
 : BC_FSlider(x, 
			y,
			0,
			w, 
			w, 
			(float)MINHUE, 
			(float)MAXHUE, 
			plugin->config.hue)
{
	this->plugin = plugin;
}
int HueSlider::handle_event()
{
	plugin->config.hue = get_value();
	plugin->send_configure_change();
	return 1;
}







SaturationSlider::SaturationSlider(HueEffect *plugin, int x, int y, int w)
 : BC_FSlider(x, 
			y,
			0,
			w, 
			w, 
			(float)MINSATURATION, 
			(float)MAXSATURATION, 
			plugin->config.saturation)
{
	this->plugin = plugin;
}
int SaturationSlider::handle_event()
{
	plugin->config.saturation = get_value();
	plugin->send_configure_change();
	return 1;
}

char* SaturationSlider::get_caption()
{
	float fraction = ((float)plugin->config.saturation - MINSATURATION) / 
		MAXSATURATION;;
	sprintf(string, "%0.4f", fraction);
	return string;
}







ValueSlider::ValueSlider(HueEffect *plugin, int x, int y, int w)
 : BC_FSlider(x, 
			y,
			0,
			w, 
			w, 
			(float)MINVALUE, 
			(float)MAXVALUE, 
			plugin->config.value)
{
	this->plugin = plugin;
}
int ValueSlider::handle_event()
{
	plugin->config.value = get_value();
	plugin->send_configure_change();
	return 1;
}

char* ValueSlider::get_caption()
{
	float fraction = ((float)plugin->config.value - MINVALUE) / MAXVALUE;
	sprintf(string, "%0.4f", fraction);
	return string;
}







HueWindow::HueWindow(HueEffect *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
			x,
			y,
			310, 
			100, 
			310, 
			100, 
			0,
			0, 
			1)
{
	this->plugin = plugin;
}
void HueWindow::create_objects()
{
	int x = 10, y = 10, x1 = 100;
	add_subwindow(new BC_Title(x, y, _("Hue:")));
	add_subwindow(hue = new HueSlider(plugin, x1, y, 200));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Saturation:")));
	add_subwindow(saturation = new SaturationSlider(plugin, x1, y, 200));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Value:")));
	add_subwindow(value = new ValueSlider(plugin, x1, y, 200));
	show_window();
	flush();
}


WINDOW_CLOSE_EVENT(HueWindow)








PLUGIN_THREAD_OBJECT(HueEffect, HueThread, HueWindow)

HueEngine::HueEngine(HueEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}
void HueEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		HuePackage *pkg = (HuePackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}
LoadClient* HueEngine::new_client()
{
	return new HueUnit(plugin, this);
}
LoadPackage* HueEngine::new_package()
{
	return new HuePackage;
}








HuePackage::HuePackage()
 : LoadPackage()
{
}

HueUnit::HueUnit(HueEffect *plugin, HueEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}







#define HUESATURATION(type, max, components, use_yuv) \
{ \
	float h_offset = plugin->config.hue; \
	float s_offset = ((float)plugin->config.saturation - MINSATURATION) / MAXSATURATION; \
	float v_offset = ((float)plugin->config.value - MINVALUE) / MAXVALUE; \
	for(int i = pkg->row1; i < pkg->row2; i++) \
	{ \
		type* in_row = (type*)plugin->input->get_rows()[i]; \
		type* out_row = (type*)plugin->output->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			float h, s, va; \
			int y, u, v; \
			float r, g, b; \
			int r_i, g_i, b_i; \
 \
			if(use_yuv) \
			{ \
				y = (int)in_row[0]; \
				u = (int)in_row[1]; \
				v = (int)in_row[2]; \
				if(max == 0xffff) \
					yuv.yuv_to_rgb_16(r_i, g_i, b_i, y, u, v); \
				else \
					yuv.yuv_to_rgb_8(r_i, g_i, b_i, y, u, v); \
				HSV::rgb_to_hsv((float)r_i / max, \
					(float)g_i / max, \
					(float)b_i / max, \
					h, \
					s, \
					va); \
			} \
			else \
			{ \
				r = (float)in_row[0] / max; \
				g = (float)in_row[1] / max; \
				b = (float)in_row[2] / max; \
				HSV::rgb_to_hsv(r, g, b, h, s, va); \
			} \
 \
 \
			h += h_offset; \
			s *= s_offset; \
			va *= v_offset; \
 \
			if(h >= 360) h -= 360; \
			if(h < 0) h += 360; \
			if(sizeof(type) < 4) \
			{ \
				if(s > 1) s = 1; \
				if(va > 1) va = 1; \
				if(s < 0) s = 0; \
				if(va < 0) va = 0; \
			} \
 \
			if(use_yuv) \
			{ \
				HSV::hsv_to_yuv(y, u, v, h, s, va, max); \
				out_row[0] = y; \
				out_row[1] = u; \
				out_row[2] = v; \
			} \
			else \
			{ \
				HSV::hsv_to_rgb(r, g, b, h, s, va); \
				if(sizeof(type) < 4) \
				{ \
					r *= max; \
					g *= max; \
					b *= max; \
					out_row[0] = (type)CLIP(r, 0, max); \
					out_row[1] = (type)CLIP(g, 0, max); \
					out_row[2] = (type)CLIP(b, 0, max); \
				} \
				else \
				{ \
					out_row[0] = (type)r; \
					out_row[1] = (type)g; \
					out_row[2] = (type)b; \
				} \
			} \
 \
			in_row += components; \
			out_row += components; \
		} \
	} \
}


void HueUnit::process_package(LoadPackage *package)
{
	HuePackage *pkg = (HuePackage*)package;
	int w = plugin->input->get_w();

	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			HUESATURATION(unsigned char, 0xff, 3, 0)
			break;

		case BC_RGB_FLOAT:
			HUESATURATION(float, 1, 3, 0)
			break;

		case BC_YUV888:
			HUESATURATION(unsigned char, 0xff, 3, 1)
			break;

		case BC_RGB161616:
			HUESATURATION(uint16_t, 0xffff, 3, 0)
			break;

		case BC_YUV161616:
			HUESATURATION(uint16_t, 0xffff, 3, 1)
			break;

		case BC_RGBA_FLOAT:
			HUESATURATION(float, 1, 4, 0)
			break;

		case BC_RGBA8888:
			HUESATURATION(unsigned char, 0xff, 4, 0)
			break;

		case BC_YUVA8888:
			HUESATURATION(unsigned char, 0xff, 4, 1)
			break;

		case BC_RGBA16161616:
			HUESATURATION(uint16_t, 0xffff, 4, 0)
			break;

		case BC_YUVA16161616:
			HUESATURATION(uint16_t, 0xffff, 4, 1)
			break;

	}
}




REGISTER_PLUGIN(HueEffect)


HueEffect::HueEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}
HueEffect::~HueEffect()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(engine) delete engine;
}

int HueEffect::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		get_use_opengl());
	

	this->input = frame;
	this->output = frame;
	if(EQUIV(config.hue, 0) && EQUIV(config.saturation, 0) && EQUIV(config.value, 0))
	{
		return 0;
	}
	else
	{
		if(get_use_opengl())
		{
			run_opengl();
			return 0;
		}

		if(!engine) engine = new HueEngine(this, PluginClient::smp + 1);
		
		engine->process_packages();
	}
	return 0;
}

char* HueEffect::plugin_title() { return N_("Hue saturation"); }
int HueEffect::is_realtime() { return 1; }

NEW_PICON_MACRO(HueEffect)
SHOW_GUI_MACRO(HueEffect, HueThread)
SET_STRING_MACRO(HueEffect)
RAISE_WINDOW_MACRO(HueEffect)
LOAD_CONFIGURATION_MACRO(HueEffect, HueConfig)

int HueEffect::load_defaults()
{
	char directory[BCTEXTLEN];
	sprintf(directory, "%shuesaturation.rc", BCASTDIR);
	defaults = new BC_Hash(directory);
	defaults->load();
	config.hue = defaults->get("HUE", config.hue);
	config.saturation = defaults->get("SATURATION", config.saturation);
	config.value = defaults->get("VALUE", config.value);
	return 0;
}
int HueEffect::save_defaults()
{
	defaults->update("HUE", config.hue);
	defaults->update("SATURATION", config.saturation);
	defaults->update("VALUE", config.value);
	defaults->save();
	return 0;
}
void HueEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("HUESATURATION");
	output.tag.set_property("HUE", config.hue);
	output.tag.set_property("SATURATION", config.saturation);
	output.tag.set_property("VALUE", config.value);
	output.append_tag();
	output.tag.set_title("/HUESATURATION");
	output.append_tag();
	output.terminate_string();
}
void HueEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	while(!input.read_tag())
	{
		if(input.tag.title_is("HUESATURATION"))
		{
			config.hue = input.tag.get_property("HUE", config.hue);
			config.saturation = input.tag.get_property("SATURATION", config.saturation);
			config.value = input.tag.get_property("VALUE", config.value);
		}
	}
}
void HueEffect::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		load_configuration();
		thread->window->hue->update(config.hue);
		thread->window->saturation->update(config.saturation);
		thread->window->value->update(config.value);
		thread->window->unlock_window();
	}
}

int HueEffect::handle_opengl()
{
#ifdef HAVE_GL
	static char *yuv_saturation_frag = 
		"uniform sampler2D tex;\n"
		"uniform float s_offset;\n"
		"uniform float v_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
		"	pixel.r *= v_offset;\n"
		"	pixel.gb -= vec2(0.5, 0.5);\n"
		"	pixel.g *= s_offset;\n"
		"	pixel.b *= s_offset;\n"
		"	pixel.gb += vec2(0.5, 0.5);\n"
		"	gl_FragColor = pixel;\n"
		"}\n";


	static char *yuv_frag = 
		"uniform sampler2D tex;\n"
		"uniform float h_offset;\n"
		"uniform float s_offset;\n"
		"uniform float v_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
			YUV_TO_RGB_FRAG("pixel")
			RGB_TO_HSV_FRAG("pixel")
		"	pixel.r += h_offset;\n"
		"	pixel.g *= s_offset;\n"
		"	pixel.b *= v_offset;\n"
		"	if(pixel.r >= 360.0) pixel.r -= 360.0;\n"
		"	if(pixel.r < 0.0) pixel.r += 360.0;\n"
			HSV_TO_RGB_FRAG("pixel")
			RGB_TO_YUV_FRAG("pixel")
		"	gl_FragColor = pixel;\n"
		"}\n";

	static char *rgb_frag = 
		"uniform sampler2D tex;\n"
		"uniform float h_offset;\n"
		"uniform float s_offset;\n"
		"uniform float v_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
			RGB_TO_HSV_FRAG("pixel")
		"	pixel.r += h_offset;\n"
		"	pixel.g *= s_offset;\n"
		"	pixel.b *= v_offset;\n"
		"	if(pixel.r >= 360.0) pixel.r -= 360.0;\n"
		"	if(pixel.r < 0.0) pixel.r += 360.0;\n"
			HSV_TO_RGB_FRAG("pixel")
		"	gl_FragColor = pixel;\n"
		"}\n";


	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int frag_shader = 0;
	switch(get_output()->get_color_model())
	{
		case BC_YUV888:
		case BC_YUVA8888:
// This is a lousy approximation but good enough for the masker.
			if(EQUIV(config.hue, 0))
				frag_shader = VFrame::make_shader(0,
					yuv_saturation_frag,
					0);
			else
				frag_shader = VFrame::make_shader(0,
					yuv_frag,
					0);
			break;
		default:
			frag_shader = VFrame::make_shader(0,
				rgb_frag,
				0);
			break;
	}


	if(frag_shader > 0) 
	{
		glUseProgram(frag_shader);
		glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
		glUniform1f(glGetUniformLocation(frag_shader, "h_offset"), config.hue);
		glUniform1f(glGetUniformLocation(frag_shader, "s_offset"), 
			((float)config.saturation - MINSATURATION) / MAXSATURATION);
		glUniform1f(glGetUniformLocation(frag_shader, "v_offset"), 
			((float)config.value - MINVALUE) / MAXVALUE);
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
}





