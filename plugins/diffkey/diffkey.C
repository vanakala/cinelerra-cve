
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

#define GL_GLEXT_PROTOTYPES

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
 : PluginWindow(plugin->gui_string,
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

DiffKeyGUI::~DiffKeyGUI()
{
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

PLUGIN_CLASS_METHODS

void DiffKey::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sdiffkey.rc", BCASTDIR);

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

void DiffKey::process_frame(VFrame **frame)
{
	load_configuration();

// Don't process if only 1 layer.
	if(get_total_buffers() < 2) 
	{
		get_frame(frame[0], get_use_opengl());
		return;
	}

// Read frames from 2 layers
	get_frame(frame[0], get_use_opengl());
	get_frame(frame[1],get_use_opengl());

	top_frame = frame[0];
	bottom_frame = frame[1];

	if(get_use_opengl())
	{
		run_opengl();
		return;
	}

	if(!engine)
	{
		engine = new DiffKeyEngine(this);
	}

	engine->process_packages();
}

#define DIFFKEY_VARS(plugin) \
	float threshold = plugin->config.threshold / 100; \
	float pad = plugin->config.slope / 100; \
	float threshold_pad = threshold + pad; \


void DiffKey::handle_opengl()
{
#ifdef HAVE_GL
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

/* FIXIT
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

DiffKeyClient::~DiffKeyClient()
{
}

void DiffKeyClient::process_package(LoadPackage *ptr)
{
	DiffKeyPackage *pkg = (DiffKeyPackage*)ptr;
	DiffKey *plugin = engine->plugin;
	int w = plugin->top_frame->get_w();

#define RGB_TO_VALUE(r, g, b) \
((r) * R_TO_Y + (g) * G_TO_Y + (b) * B_TO_Y)

#define SQR(x) ((x) * (x))

#define DIFFKEY_MACRO(type, components, max, chroma_offset) \
{ \
 \
	for(int i = pkg->row1; i < pkg->row2; i++) \
	{ \
		type *top_row = (type*)plugin->top_frame->get_row_ptr(i); \
		type *bottom_row = (type*)plugin->bottom_frame->get_row_ptr(i); \
 \
		for(int j = 0; j < w; j++) \
		{ \
			float a = 1.0; \
 \
/* Test for value in range */ \
			if(plugin->config.do_value) \
			{ \
				float top_value; \
				float bottom_value; \
 \
/* Convert pixel data into floating point value */ \
				if(chroma_offset) \
				{ \
					float top_r = (float)top_row[0] / max; \
					float bottom_r = (float)bottom_row[0] / max; \
					top_value = top_r; \
					bottom_value = bottom_r; \
				} \
				else \
				{ \
					float top_r = (float)top_row[0] / max; \
					float top_g = (float)top_row[1] / max; \
					float top_b = (float)top_row[2] / max; \
					top_g -= (float)chroma_offset / max; \
					top_b -= (float)chroma_offset / max; \
 \
					float bottom_r = (float)bottom_row[0] / max; \
					float bottom_g = (float)bottom_row[1] / max; \
					float bottom_b = (float)bottom_row[2] / max; \
					bottom_g -= (float)chroma_offset / max; \
					bottom_b -= (float)chroma_offset / max; \
 \
					top_value = RGB_TO_VALUE(top_r, top_g, top_b); \
					bottom_value = RGB_TO_VALUE(bottom_r, bottom_g, bottom_b); \
				} \
 \
 				float min_v = bottom_value - threshold; \
				float max_v = bottom_value + threshold; \
 \
/* Full transparency if in range */ \
				if(top_value >= min_v && top_value < max_v) \
				{ \
					a = 0; \
				} \
				else \
/* Phased out if below or above range */ \
				if(top_value < min_v) \
				{ \
					if(min_v - top_value < pad) \
						a = (min_v - top_value) / pad; \
				} \
				else \
				if(top_value - max_v < pad) \
					a = (top_value - max_v) / pad; \
			} \
			else \
/* Use color cube */ \
			{ \
				float top_r = (float)top_row[0] / max; \
				float top_g = (float)top_row[1] / max; \
				float top_b = (float)top_row[2] / max; \
				top_g -= (float)chroma_offset / max; \
				top_b -= (float)chroma_offset / max; \
 \
				float bottom_r = (float)bottom_row[0] / max; \
				float bottom_g = (float)bottom_row[1] / max; \
				float bottom_b = (float)bottom_row[2] / max; \
				bottom_g -= (float)chroma_offset / max; \
				bottom_b -= (float)chroma_offset / max; \
 \
 \
				float difference = sqrt(SQR(top_r - bottom_r) +  \
					SQR(top_g - bottom_g) + \
					SQR(top_b - bottom_b)); \
 \
				if(difference < threshold) \
				{ \
					a = 0; \
				} \
				else \
				if(difference < threshold_pad) \
				{ \
					a = (difference - threshold) / pad; \
				} \
			} \
 \
/* multiply alpha */ \
			if(components == 4) \
			{ \
				top_row[3] = MIN((type)(a * max), top_row[3]); \
			} \
			else \
			{ \
				top_row[0] = (type)(a * top_row[0]); \
				top_row[1] = (type)(a * (top_row[1] - chroma_offset) + chroma_offset); \
				top_row[2] = (type)(a * (top_row[2] - chroma_offset) + chroma_offset); \
			} \
 \
			top_row += components; \
			bottom_row += components; \
		} \
	} \
}

	DIFFKEY_VARS(plugin)

	switch(plugin->top_frame->get_color_model())
	{
	case BC_RGB_FLOAT:
		DIFFKEY_MACRO(float, 3, 1.0, 0);
		break;
	case BC_RGBA_FLOAT:
		DIFFKEY_MACRO(float, 4, 1.0, 0);
		break;
	case BC_RGB888:
		DIFFKEY_MACRO(unsigned char, 3, 0xff, 0);
		break;
	case BC_RGBA8888:
		DIFFKEY_MACRO(unsigned char, 4, 0xff, 0);
		break;
	case BC_YUV888:
		DIFFKEY_MACRO(unsigned char, 3, 0xff, 0x80);
		break;
	case BC_YUVA8888:
		DIFFKEY_MACRO(unsigned char, 4, 0xff, 0x80);
		break;
	case BC_RGBA16161616:
		DIFFKEY_MACRO(uint16_t, 4, 0xffff, 0);
		break;
	case BC_AYUV16161616:
		for(int i = pkg->row1; i < pkg->row2; i++)
		{
			uint16_t *top_row = (uint16_t*)plugin->top_frame->get_row_ptr(i);
			uint16_t *bottom_row = (uint16_t*)plugin->bottom_frame->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				float a = 1.0;

				// Test for value in range
				if(plugin->config.do_value)
				{
					float top_value;
					float bottom_value;

					// Convert pixel data into floating point value
					top_value = (float)top_row[1] / 0xffff;
					bottom_value = (float)bottom_row[1] / 0xffff;

					float min_v = bottom_value - threshold;
					float max_v = bottom_value + threshold;

					// Full transparency if in range
					if(top_value >= min_v && top_value < max_v)
						a = 0;
					else
					// Phased out if below or above range
					if(top_value < min_v)
					{
						if(min_v - top_value < pad)
							a = (min_v - top_value) / pad;
					}
					else
					if(top_value - max_v < pad)
						a = (top_value - max_v) / pad;
				}
				else
				// Use color cube
				{
					float top_r = (float)top_row[1] / 0xffff;
					float top_g = (float)top_row[2] / 0xffff;
					float top_b = (float)top_row[3] / 0xffff;
					top_g -= (float)0x8000 / 0xffff;
					top_b -= (float)0x8000 / 0xffff;

					float bottom_r = (float)bottom_row[1] / 0xffff;
					float bottom_g = (float)bottom_row[2] / 0xffff;
					float bottom_b = (float)bottom_row[3] / 0xffff;
					bottom_g -= (float)0x8000 / 0xffff;
					bottom_b -= (float)0x8000 / 0xffff;

					float difference = sqrt(SQR(top_r - bottom_r) +
						SQR(top_g - bottom_g) +
						SQR(top_b - bottom_b));

					if(difference < threshold)
						a = 0;
					else
					if(difference < threshold_pad)
						a = (difference - threshold) / pad;
				}

				// multiply alpha
				top_row[0] = MIN((uint16_t)(a * 0xffff), top_row[0]);

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
