// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "colorspaces.h"
#include "filexml.h"
#include "huesaturation.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

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
 : BC_FSlider(x, y, 0, w, w, MINHUE, MAXHUE, plugin->config.hue)
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
 : BC_FSlider(x, y, 0, w, w,
	MINSATURATION, MAXSATURATION,
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
	double fraction = (plugin->config.saturation - MINSATURATION) /
		MAXSATURATION;

	sprintf(string, "%0.4f", fraction);
	return string;
}

ValueSlider::ValueSlider(HueEffect *plugin, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, MINVALUE, MAXVALUE, plugin->config.value)
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
	double fraction = (plugin->config.value - MINVALUE) / MAXVALUE;

	sprintf(string, "%0.4f", fraction);
	return string;
}

PLUGIN_THREAD_METHODS

HueWindow::HueWindow(HueEffect *plugin, int x, int y)
 : PluginWindow(plugin,
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

void HueUnit::process_package(LoadPackage *package)
{
	HuePackage *pkg = (HuePackage*)package;
	int w = plugin->input->get_w();
	double h_offset = plugin->config.hue;
	double s_offset = (plugin->config.saturation -
		MINSATURATION) / MAXSATURATION;
	double v_offset = (plugin->config.value -
		MINVALUE) / MAXVALUE;
	float h, s, va;
	int y, u, v;
	float r, g, b;
	int r_i, g_i, b_i;

	switch(plugin->input->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = pkg->row1; i < pkg->row2; i++)
		{
			uint16_t* in_row = (uint16_t*)plugin->input->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				r = (float)in_row[0] / 0xffff;
				g = (float)in_row[1] / 0xffff;
				b = (float)in_row[2] / 0xffff;
				ColorSpaces::rgb_to_hsv(r, g, b, h, s, va);

				h += h_offset;
				s *= s_offset;
				va *= v_offset;

				if(h >= 360)
					h -= 360;
				if(h < 0)
					h += 360;

				if(s > 1)
					s = 1;
				if(va > 1)
					va = 1;
				if(s < 0)
					s = 0;
				if(va < 0)
					va = 0;

				ColorSpaces::hsv_to_rgb(r, g, b, h, s, va);
				r *= 0xffff;
				g *= 0xffff;
				b *= 0xffff;
				in_row[0] = CLIP(r, 0, 0xffff);
				in_row[1] = CLIP(g, 0, 0xffff);
				in_row[2] = CLIP(b, 0, 0xffff);

				in_row += 4;
			}
		}
		break;

	case BC_AYUV16161616:
		{
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				uint16_t* in_row = (uint16_t*)plugin->input->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{

					y = in_row[1];
					u = in_row[2];
					v = in_row[3];
					ColorSpaces::yuv_to_hsv(y, u, v, h, s, va, 0xffff);
					h += h_offset;
					s *= s_offset;
					va *= v_offset;

					if(h >= 360)
						h -= 360;
					if(h < 0)
						h += 360;
					if(s > 1)
						s = 1;
					if(va > 1)
						va = 1;
					if(s < 0)
						s = 0;
					if(va < 0)
						va = 0;

					ColorSpaces::hsv_to_yuv(y, u, v, h, s, va, 0xffff);
					in_row[1] = y;
					in_row[2] = u;
					in_row[3] = v;

					in_row += 4;
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
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void HueEffect::reset_plugin()
{
	delete engine;
	engine = 0;
}

PLUGIN_CLASS_METHODS

VFrame *HueEffect::process_tmpframe(VFrame *frame)
{
	int cmodel = frame->get_color_model();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(cmodel);
	return frame;
	}

	if(load_configuration())
		update_gui();

	input = frame;

	if(!EQUIV(config.hue, 0) || !EQUIV(config.saturation, 0) ||
		!EQUIV(config.value, 0))
	{
		if(!engine)
			engine = new HueEngine(this, get_project_smp());

		engine->process_packages();
	}
	return frame;
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

	output.tag.set_title("HUESATURATION");
	output.tag.set_property("HUE", config.hue);
	output.tag.set_property("SATURATION", config.saturation);
	output.tag.set_property("VALUE", config.value);
	output.append_tag();
	output.tag.set_title("/HUESATURATION");
	output.append_tag();
	keyframe->set_data(output.string);
}

void HueEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

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
/* FIXIT
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
