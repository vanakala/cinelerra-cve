// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bchash.h"
#include "bcslider.h"
#include "bctitle.h"
#include "bctoggle.h"
#include "clip.h"
#include "colorspaces.h"
#include "diffkey.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"

#include <string.h>

REGISTER_PLUGIN


DiffKeyConfig::DiffKeyConfig()
{
	threshold = 0.1;
	slope = 0;
	do_value = 0;
}

void DiffKeyConfig::copy_from(DiffKeyConfig &src)
{
	this->threshold = src.threshold;
	this->slope = src.slope;
	this->do_value = src.do_value;
}


int DiffKeyConfig::equivalent(DiffKeyConfig &src)
{
	return EQUIV(threshold, src.threshold) &&
		EQUIV(slope, src.slope) &&
		do_value == src.do_value;
}

void DiffKeyConfig::interpolate(DiffKeyConfig &prev, 
	DiffKeyConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->threshold = prev.threshold * prev_scale + next.threshold * next_scale;
	this->slope = prev.slope * prev_scale + next.slope * next_scale;
	this->do_value = prev.do_value;
}


DiffKeyThreshold::DiffKeyThreshold(DiffKey *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0, 100, plugin->config.threshold)
{
	this->plugin = plugin;
}

int DiffKeyThreshold::handle_event()
{
	plugin->config.threshold = get_value();
	plugin->send_configure_change();
	return 1;
}


DiffKeySlope::DiffKeySlope(DiffKey *plugin, int x, int y)
 : BC_FSlider(x, y, 0, 200, 200, 0, 100, plugin->config.slope)
{
	this->plugin = plugin;
}

int DiffKeySlope::handle_event()
{
	plugin->config.slope = get_value();
	plugin->send_configure_change();
	return 1;
}


DiffKeyDoValue::DiffKeyDoValue(DiffKey *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.do_value, _("Use Value"))
{
	this->plugin = plugin;
}

int DiffKeyDoValue::handle_event()
{
	plugin->config.do_value = get_value();
	plugin->send_configure_change();
	return 1;
}


DiffKeyGUI::DiffKeyGUI(DiffKey *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	320,
	100)
{
	BC_Title *title;
	int x2;

	x = y = 10;

	add_subwindow(title = new BC_Title(x, y, _("Threshold:")));
	x += title->get_w() + 10;
	add_subwindow(threshold = new DiffKeyThreshold(plugin, x, y));
	x = 10;
	y += threshold->get_h() + 10;
	add_subwindow(title = new BC_Title(x, y, _("Slope:")));
	x += title->get_w() + 10;
	add_subwindow(slope = new DiffKeySlope(plugin, x, y));
	x = 10;
	y += slope->get_h() + 10;
	add_subwindow(do_value = new DiffKeyDoValue(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DiffKeyGUI::update()
{
	threshold->update(plugin->config.threshold);
	slope->update(plugin->config.slope);
	do_value->update(plugin->config.do_value);
}

PLUGIN_THREAD_METHODS


DiffKey::DiffKey(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

DiffKey::~DiffKey()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void DiffKey::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
	}
}

PLUGIN_CLASS_METHODS

void DiffKey::load_defaults()
{
// load the defaults
	defaults = load_defaults_file("diffkey.rc");

	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.slope = defaults->get("SLOPE", config.slope);
	config.do_value = defaults->get("DO_VALUE", config.do_value);
}

void DiffKey::save_defaults()
{
	defaults->update("THRESHOLD", config.threshold);
	defaults->update("SLOPE", config.slope);
	defaults->update("DO_VALUE", config.do_value);
	defaults->save();
}

void DiffKey::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DIFFKEY");
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("SLOPE", config.slope);
	output.tag.set_property("DO_VALUE", config.do_value);
	output.append_tag();
	output.tag.set_title("/DIFFKEY");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DiffKey::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("DIFFKEY"))
		{
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.slope = input.tag.get_property("SLOPE", config.slope);
			config.do_value = input.tag.get_property("DO_VALUE", config.do_value);
		}
	}
}

void DiffKey::process_tmpframes(VFrame **frame)
{
	int cmodel = frame[0]->get_color_model();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(cmodel);
		return;
	}

	if(load_configuration())
		update_gui();

// Don't process if only 1 layer.
	if(get_total_buffers() > 1)
	{
		top_frame = frame[0];
		bottom_frame = frame[1];

		if(top_frame->get_w() != bottom_frame->get_w() ||
			top_frame->get_h() != bottom_frame->get_h())
		{
			abort_plugin(_("Tracks must have the same dimensions"));
			return;
		}

		top_frame->set_transparent();

		if(!engine)
			engine = new DiffKeyEngine(this);

		engine->process_packages();
	}
}

/* FIXIT
#define DIFFKEY_VARS(plugin) \
	float threshold = plugin->config.threshold / 100; \
	float pad = plugin->config.slope / 100; \
	float threshold_pad = threshold + pad; \
	*/

void DiffKey::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	static const char *diffkey_head = 
		"uniform sampler2D tex_bg;\n"
		"uniform sampler2D tex_fg;\n"
		"uniform float threshold;\n"
		"uniform float pad;\n"
		"uniform float threshold_pad;\n"
		"void main()\n"
		"{\n"
		"	vec4 foreground = texture2D(tex_fg, gl_TexCoord[0].st);\n"
		"	vec4 background = texture2D(tex_bg, gl_TexCoord[0].st);\n";

	static const char *colorcube = 
		"	float difference = length(foreground.rgb - background.rgb);\n";

	static const char *yuv_value = 
		"	float difference = abs(foreground.r - background.r);\n";

	static const char *rgb_value = 
		"	float difference = abs(dot(foreground.rgb, vec3(0.29900, 0.58700, 0.11400)) - \n"
		"						dot(background.rgb, vec3(0.29900, 0.58700, 0.11400)));\n";

	static const char *diffkey_tail = 
		"	vec4 result;\n"
		"	if(difference < threshold)\n"
		"		result.a = 0.0;\n"
		"	else\n"
		"	if(difference < threshold_pad)\n"
		"		result.a = (difference - threshold) / pad;\n"
		"	else\n"
		"		result.a = 1.0;\n"
		"	result.rgb = foreground.rgb;\n"
		"	gl_FragColor = result;\n"
		"}\n";

	top_frame->enable_opengl();
	top_frame->init_screen();

	top_frame->to_texture();
	bottom_frame->to_texture();

	top_frame->enable_opengl();
	top_frame->init_screen();

	unsigned int shader_id = 0;
	if(config.do_value)
	{
		if(ColorModels::is_yuv(top_frame->get_color_model()))
			shader_id = VFrame::make_shader(0, 
				diffkey_head,
				yuv_value,
				diffkey_tail,
				0);
		else
			shader_id = VFrame::make_shader(0, 
				diffkey_head,
				rgb_value,
				diffkey_tail,
				0);
	}
	else
	{
			shader_id = VFrame::make_shader(0, 
				diffkey_head,
				colorcube,
				diffkey_tail,
				0);
	}

	DIFFKEY_VARS(this)

	bottom_frame->bind_texture(1);
	top_frame->bind_texture(0);

	if(shader_id > 0)
	{
		glUseProgram(shader_id);
		glUniform1i(glGetUniformLocation(shader_id, "tex_fg"), 0);
		glUniform1i(glGetUniformLocation(shader_id, "tex_bg"), 1);
		glUniform1f(glGetUniformLocation(shader_id, "threshold"), threshold);
		glUniform1f(glGetUniformLocation(shader_id, "pad"), pad);
		glUniform1f(glGetUniformLocation(shader_id, "threshold_pad"), threshold_pad);
	}

	if(ColorModels::components(get_output()->get_color_model()) == 3)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		top_frame->clear_pbuffer();
	}

	top_frame->draw_texture();
	glUseProgram(0);
	top_frame->set_opengl_state(VFrame::SCREEN);
// Fastest way to discard output
	bottom_frame->set_opengl_state(VFrame::TEXTURE);
	glDisable(GL_BLEND);
	*/
#endif
}


DiffKeyEngine::DiffKeyEngine(DiffKey *plugin) 
 : LoadServer(plugin->get_project_smp() + 1, plugin->get_project_smp() + 1)
{
	this->plugin = plugin;
}

void DiffKeyEngine::init_packages()
{
	int increment = plugin->top_frame->get_h() / get_total_packages() + 1;
	int y = 0;
	for(int i = 0; i < get_total_packages(); i++)
	{
		DiffKeyPackage *pkg = (DiffKeyPackage*)get_package(i);
		pkg->row1 = y;
		pkg->row2 = MIN(y + increment, plugin->top_frame->get_h()); 
		y  += increment;
	}
}

LoadClient* DiffKeyEngine::new_client()
{
	return new DiffKeyClient(this);
}

LoadPackage* DiffKeyEngine::new_package()
{
	return new DiffKeyPackage;
}


DiffKeyClient::DiffKeyClient(DiffKeyEngine *engine)
 : LoadClient(engine)
{
	this->engine = engine;
}


void DiffKeyClient::process_package(LoadPackage *ptr)
{
	DiffKeyPackage *pkg = (DiffKeyPackage*)ptr;
	DiffKey *plugin = engine->plugin;
	int w = plugin->top_frame->get_w();
	int threshold = round(plugin->config.threshold * 0xffff / 100);
	int pad = round(plugin->config.slope * 0xffff / 100);
	int threshold_pad = threshold + pad;

	switch(plugin->top_frame->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = pkg->row1; i < pkg->row2; i++)
		{
			uint16_t *top_row = (uint16_t*)plugin->top_frame->get_row_ptr(i);
			uint16_t *bottom_row = (uint16_t*)plugin->bottom_frame->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				int alpha = 0xfffff;

				// Test for value in range
				if(plugin->config.do_value)
				{
					int top_value = ColorSpaces::rgb_to_y_16(
						top_row[0], top_row[1], top_row[2]);
					int bottom_value = ColorSpaces::rgb_to_y_16(
						bottom_row[0], bottom_row[1], bottom_row[2]);

					int min_v = bottom_value - threshold;
					int max_v = bottom_value + threshold;

					// Full transparency if in range
					if(top_value >= min_v && top_value < max_v)
						alpha = 0;
					else
					// Phased out if below or above range
					if(top_value < min_v)
					{
						if(min_v - top_value < pad)
							alpha = (min_v - top_value) / pad;
					}
					else
					if(top_value - max_v < pad)
						alpha = (top_value - max_v) / pad;
				}
				else
				// Use color cube
				{
					double dr = top_row[0] - bottom_row[0];
					double dg = top_row[1] - bottom_row[1];
					double db = top_row[2] - bottom_row[2];

					int difference = round(sqrt(SQR(dr) +
						SQR(dg) + SQR(db)));

					if(difference < threshold)
						alpha = 0;
					else
					if(difference < threshold_pad)
						alpha = (difference - threshold) / pad;
				}

				// multiply alpha
				top_row[3] = MIN(alpha, top_row[3]);

				top_row += 4;
				bottom_row += 4;
			}
		}
		break;
	case BC_AYUV16161616:
		for(int i = pkg->row1; i < pkg->row2; i++)
		{
			uint16_t *top_row = (uint16_t*)plugin->top_frame->get_row_ptr(i);
			uint16_t *bottom_row = (uint16_t*)plugin->bottom_frame->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				int alpha = 0xffff;

				// Test for value in range
				if(plugin->config.do_value)
				{
					int top_value = top_row[1];
					int bottom_value = bottom_row[1];

					int min_v = bottom_value - threshold;
					int max_v = bottom_value + threshold;

					// Full transparency if in range
					if(top_value >= min_v && top_value < max_v)
						alpha = 0;
					else
					// Phased out if below or above range
					if(top_value < min_v)
					{
						if(min_v - top_value < pad)
							alpha = (min_v - top_value) / pad;
					}
					else
					if(top_value - max_v < pad)
						alpha = (top_value - max_v) / pad;
				}
				else
				// Use color cube
				{
					double dy = top_row[1] - bottom_row[1];
					double du = top_row[2] - bottom_row[2];
					double dv = top_row[3] - bottom_row[3];

					int difference = round(sqrt(SQR(dy) +
						SQR(du) + SQR(dv)));

					if(difference < threshold)
						alpha = 0;
					else
					if(difference < threshold_pad)
						alpha = (difference - threshold) / pad;
				}

				// multiply alpha
				top_row[0] = MIN(alpha, top_row[0]);

				top_row += 4;
				bottom_row += 4;
			}
		}
		break;
	}
}


DiffKeyPackage::DiffKeyPackage()
 : LoadPackage()
{
}
