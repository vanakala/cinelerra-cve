
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

#define PLUGIN_TITLE N_("Hue saturation")
#define PLUGIN_CLASS HueEffect
#define PLUGIN_CONFIG_CLASS HueConfig
#define PLUGIN_THREAD_CLASS HueThread
#define PLUGIN_GUI_CLASS HueWindow

#define GL_GLEXT_PROTOTYPES

#include "pluginmacros.h"

#include "bchash.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

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
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	float hue, saturation, value;
	PLUGIN_CONFIG_CLASS_MEMBERS
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

class HueWindow : public PluginWindow
{
public:
	HueWindow(HueEffect *plugin, int x, int y);

	void update();

	HueSlider *hue;
	SaturationSlider *saturation;
	ValueSlider *value;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

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

	PLUGIN_CLASS_MEMBERS

	void process_frame(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	VFrame *input, *output;
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
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

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

PLUGIN_THREAD_METHODS

HueWindow::HueWindow(HueEffect *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
			x,
			y,
			345,
			100)
{
	int x1 = 125;
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Hue:")));
	add_subwindow(hue = new HueSlider(plugin, x1, y, 200));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Saturation:")));
	add_subwindow(saturation = new SaturationSlider(plugin, x1, y, 200));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Value:")));
	add_subwindow(value = new ValueSlider(plugin, x1, y, 200));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void HueWindow::update()
{
	hue->update(plugin->config.hue);
	saturation->update(plugin->config.saturation);
	value->update(plugin->config.value);
}


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
		type* in_row = (type*)plugin->input->get_row_ptr(i); \
		type* out_row = (type*)plugin->output->get_row_ptr(i); \
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

	case BC_AYUV16161616:
		{
			float h_offset = plugin->config.hue;
			float s_offset = ((float)plugin->config.saturation -
				MINSATURATION) / MAXSATURATION;
			float v_offset = ((float)plugin->config.value -
				MINVALUE) / MAXVALUE;
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				uint16_t* in_row = (uint16_t*)plugin->input->get_row_ptr(i);
				uint16_t* out_row = (uint16_t*)plugin->output->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					float h, s, va;
					int y, u, v;
					float r, g, b;
					int r_i, g_i, b_i;

					y = (int)in_row[1];
					u = (int)in_row[2];
					v = (int)in_row[3];
					yuv.yuv_to_rgb_16(r_i, g_i, b_i, y, u, v);
					HSV::rgb_to_hsv((float)r_i / 0xffff,
						(float)g_i / 0xffff,
						(float)b_i / 0xffff,
						h, s, va);

					h += h_offset;
					s *= s_offset;
					va *= v_offset;

					if(h >= 360) h -= 360;
					if(h < 0) h += 360;
					if(s > 1) s = 1;
					if(va > 1) va = 1;
					if(s < 0) s = 0;
					if(va < 0) va = 0;
					HSV::hsv_to_yuv(y, u, v, h, s, va, 0xffff);
					out_row[1] = y;
					out_row[2] = u;
					out_row[3] = v;

					in_row += 4;
					out_row += 4;
				}
			}
		}
		break;
	}
}


REGISTER_PLUGIN


HueEffect::HueEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

HueEffect::~HueEffect()
{
	if(engine) delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void HueEffect::process_frame(VFrame *frame)
{
	load_configuration();

	get_frame(frame, get_use_opengl());

	this->input = frame;
	this->output = frame;
	if(EQUIV(config.hue, 0) && EQUIV(config.saturation, 0) && EQUIV(config.value, 0))
	{
		return;
	}
	else
	{
		if(get_use_opengl())
		{
			run_opengl();
			return;
		}

		if(!engine) engine = new HueEngine(this, PluginClient::smp + 1);

		engine->process_packages();
	}
}

void HueEffect::load_defaults()
{
	defaults = load_defaults_file("huesaturation.rc");
	config.hue = defaults->get("HUE", config.hue);
	config.saturation = defaults->get("SATURATION", config.saturation);
	config.value = defaults->get("VALUE", config.value);
}

void HueEffect::save_defaults()
{
	defaults->update("HUE", config.hue);
	defaults->update("SATURATION", config.saturation);
	defaults->update("VALUE", config.value);
	defaults->save();
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

void HueEffect::handle_opengl()
{
#ifdef HAVE_GL
	static const char *yuv_saturation_frag = 
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
/* FIXIT
	static const char *yuv_frag = 
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

	static const char *rgb_frag = 
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
	*/
/* FIXIT
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
	*/
#endif
}
