// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "clip.h"
#include "filexml.h"
#include "brightness.h"
#include "colorspaces.h"
#include "bchash.h"
#include "language.h"
#include "picon_png.h"
#include "vframe.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN

BrightnessConfig::BrightnessConfig()
{
	brightness = 0;
	contrast = 0;
	luma = 1;
}

int BrightnessConfig::equivalent(BrightnessConfig &that)
{
	return (EQUIV(brightness, that.brightness) && 
		EQUIV(contrast, that.contrast) &&
		luma == that.luma);
}

void BrightnessConfig::copy_from(BrightnessConfig &that)
{
	brightness = that.brightness;
	contrast = that.contrast;
	luma = that.luma;
}

void BrightnessConfig::interpolate(BrightnessConfig &prev, 
	BrightnessConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->brightness = prev.brightness * prev_scale + next.brightness * next_scale;
	this->contrast = prev.contrast * prev_scale + next.contrast * next_scale;
	this->luma = prev.luma;
}


BrightnessMain::BrightnessMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

BrightnessMain::~BrightnessMain()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void BrightnessMain::reset_plugin()
{
	delete engine;
	engine = 0;
}

PLUGIN_CLASS_METHODS

VFrame *BrightnessMain::process_tmpframe(VFrame *frame)
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

	this->input = frame;

	if(!EQUIV(config.brightness, 0) || !EQUIV(config.contrast, 0))
	{
		if(!engine)
			engine = new BrightnessEngine(this, get_project_smp());
		engine->process_packages();
	}
	return frame;
}

void BrightnessMain::handle_opengl()
{
#ifdef HAVE_GL
/* To be fixed
	static const char *brightness_yuvluma_frag = 
		"uniform sampler2D tex;\n"
		"uniform float brightness;\n"
		"uniform float contrast;\n"
		"uniform float offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 yuva = texture2D(tex, gl_TexCoord[0].st);\n"
		"	yuva.r += brightness;\n"
		"	yuva.r = yuva.r * contrast + offset;\n"
		"	gl_FragColor = yuva;\n"
		"}\n";

	static const char *brightness_yuv_frag = 
		"uniform sampler2D tex;\n"
		"uniform float brightness;\n"
		"uniform float contrast;\n"
		"uniform float offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 yuva = texture2D(tex, gl_TexCoord[0].st);\n"
		"	yuva.r += brightness;\n"
		"	yuva.rgb *= vec3(contrast, contrast, contrast);\n"
		"	yuva.rgb += vec3(offset, offset, offset);\n"
		"	gl_FragColor = yuva;\n"
		"}\n";

	static const char *brightness_rgb_frag =
		"uniform sampler2D tex;\n"
		"uniform float brightness;\n"
		"uniform float contrast;\n"
		"uniform float offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 rgba = texture2D(tex, gl_TexCoord[0].st);\n"
		"	rgba.rgb += vec3(brightness, brightness, brightness);\n"
		"	rgba.rgb *= vec3(contrast, contrast, contrast);\n"
		"	rgba.rgb += vec3(offset, offset, offset);\n"
		"	gl_FragColor = rgba;\n"
		"}\n";

	static const char *brightness_rgbluma_frag =
		"uniform sampler2D tex;\n"
		"uniform float brightness;\n"
		"uniform float contrast;\n"
		"uniform float offset;\n"
		"void main()\n"
		"{\n"
		"	const mat3 yuv_to_rgb_matrix = mat3(\n"
		"		1,       1,        1, \n"
		"		0,       -0.34414, 1.77200, \n"
		"		1.40200, -0.71414, 0);\n"
		"	const mat3 rgb_to_yuv_matrix = mat3(\n"
		"		0.29900, -0.16874, 0.50000, \n"
		"		0.58700, -0.33126, -0.41869, \n"
		"		0.11400, 0.50000,  -0.08131);\n"
		"	vec4 rgba = texture2D(tex, gl_TexCoord[0].st);\n"
		"	rgba.rgb = rgb_to_yuv_matrix * rgba.rgb;\n"
		"	rgba.r += brightness;\n"
		"	rgba.r = rgba.r * contrast + offset;\n"
		"	rgba.rgb = yuv_to_rgb_matrix * rgba.rgb;\n"
		"	gl_FragColor = rgba;\n"
		"}\n";
	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int shader_id = 0;
	switch(get_output()->get_color_model())
	{
	case BC_YUV888:
	case BC_YUVA8888:
		if(config.luma)
			shader_id = VFrame::make_shader(0,
				brightness_yuvluma_frag,
				0);
		else
			shader_id = VFrame::make_shader(0,
				brightness_yuv_frag,
				0);
		break;
	default:
		if(config.luma)
			shader_id = VFrame::make_shader(0,
				brightness_rgbluma_frag,
				0);
		else
			shader_id = VFrame::make_shader(0,
				brightness_rgb_frag,
				0);
		break;
	}

	if(shader_id > 0) 
	{
		glUseProgram(shader_id);
		glUniform1i(glGetUniformLocation(shader_id, "tex"), 0);
		glUniform1f(glGetUniformLocation(shader_id, "brightness"), config.brightness / 100);
		float contrast = (config.contrast < 0) ? 
			(config.contrast + 100) / 100 : 
			(config.contrast + 25) / 25;
		glUniform1f(glGetUniformLocation(shader_id, "contrast"), contrast);
		float offset = 0.5 - contrast / 2;
		glUniform1f(glGetUniformLocation(shader_id, "offset"), offset);
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);

	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}

void BrightnessMain::load_defaults()
{
	defaults = load_defaults_file("brightness.rc");

	config.brightness = defaults->get("BRIGHTNESS", config.brightness);
	config.contrast = defaults->get("CONTRAST", config.contrast);
	config.luma = defaults->get("LUMA", config.luma);
}

void BrightnessMain::save_defaults()
{
	defaults->update("BRIGHTNESS", config.brightness);
	defaults->update("CONTRAST", config.contrast);
	defaults->update("LUMA", config.luma);
	defaults->save();
}

void BrightnessMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("BRIGHTNESS");
	output.tag.set_property("BRIGHTNESS", config.brightness);
	output.tag.set_property("CONTRAST",  config.contrast);
	output.tag.set_property("LUMA",  config.luma);

	output.append_tag();
	output.tag.set_title("/BRIGHTNESS");
	output.append_tag();
	keyframe->set_data(output.string);
}

void BrightnessMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("BRIGHTNESS"))
		{
			config.brightness = input.tag.get_property("BRIGHTNESS", config.brightness);
			config.contrast = input.tag.get_property("CONTRAST", config.contrast);
			config.luma = input.tag.get_property("LUMA", config.luma);
		}
	}
}


BrightnessPackage::BrightnessPackage()
 : LoadPackage()
{
}

BrightnessUnit::BrightnessUnit(BrightnessEngine *server, BrightnessMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
}

void BrightnessUnit::process_package(LoadPackage *package)
{
	BrightnessPackage *pkg = (BrightnessPackage*)package;
	VFrame *input = plugin->input;
	int row1 = pkg->row1;
	int row2 = pkg->row2;
	int width = input->get_w();
	int r, g, b;

	switch(input->get_color_model())
	{
	case BC_RGBA16161616:
		if(!EQUIV(plugin->config.brightness, 0))
		{
			int offset = round(plugin->config.brightness / 100.0 * 0xffff);

			for(int i = row1; i < row2; i++)
			{
				uint16_t *input_row = (uint16_t*)input->get_row_ptr(i);

				for(int j = 0; j < width; j++)
				{
					r = input_row[j * 4] + offset;
					g = input_row[j * 4 + 1] + offset;
					b = input_row[j * 4 + 2] + offset;

					input_row[j * 4] = CLIP(r, 0, 0xffff);
					input_row[j * 4 + 1] = CLIP(g, 0, 0xffff);
					input_row[j * 4 + 2] = CLIP(r, 0, 0xffff);
				}
			}
		}

		if(!EQUIV(plugin->config.contrast, 0))
		{
			double contrast = (plugin->config.contrast < 0) ?
				(plugin->config.contrast + 100.0) / 100.0 :
				(plugin->config.contrast + 25.0) / 25.0;

			int scalar = round(contrast * 0x100);
			int offset = (0xffff << 8) / 2 - 0xffff * scalar / 2;
			int y, u, v;

			for(int i = row1; i < row2; i++)
			{
				uint16_t *input_row = (uint16_t*)input->get_row_ptr(i);

				if(plugin->config.luma)
				{
					for(int j = 0; j < width; j++)
					{
						r = input_row[j * 4];
						g = input_row[j * 4 + 1];
						b = input_row[j * 4 + 2];

						ColorSpaces::rgb_to_yuv_16(
							r, g, b, y, u, v);

						y = (y * scalar + offset) >> 8;
						CLAMP(y, 0, 0xffff);

						ColorSpaces::yuv_to_rgb_16(
							r, g, b, y, u, v);

						input_row[j * 4] = r;
						input_row[j * 4 + 1] = g;
						input_row[j * 4 + 2] = b;
					}
				}
				else
				{
					for(int j = 0; j < width; j++)
					{
						r = input_row[j * 4];
						g = input_row[j * 4 + 1];
						b = input_row[j * 4 + 2];

						r = (r * scalar + offset) >> 8;
						g = (g * scalar + offset) >> 8;
						b = (b * scalar + offset) >> 8;

						input_row[j * 4] = CLIP(r, 0, 0xffff);
						input_row[j * 4 + 1] = CLIP(g, 0, 0xffff);
						input_row[j * 4 + 2] = CLIP(b, 0, 0xffff);
					}
				}
			}
		}
		break;

	case BC_AYUV16161616:
		if(!EQUIV(plugin->config.brightness, 0))
		{
			int offset = plugin->config.brightness / 100 * 0xffff;

			for(int i = row1; i < row2; i++)
			{
				uint16_t *input_row = (uint16_t*)input->get_row_ptr(i);

				for(int j = 0; j < width; j++)
				{
					int y = input_row[j * 4 + 1] + offset;
					input_row[j * 4 + 1] = CLIP(y, 0, 0xffff);
				}
			}
		}

		if(!EQUIV(plugin->config.contrast, 0))
		{
			double contrast = (plugin->config.contrast < 0) ?
				(plugin->config.contrast + 100.0) / 100 :
				(plugin->config.contrast + 25.0) / 25.0;

			int scalar = round(contrast * 0x100);
			int offset = (0xffff << 8) / 2 - 0xffff * scalar / 2;
			int y;

			for(int i = row1; i < row2; i++)
			{
				uint16_t *input_row = (uint16_t*)input->get_row_ptr(i);

				for(int j = 0; j < width; j++) \
				{
					y = input_row[j * 4 + 1];
					y = (y * scalar + offset) >> 8;
					input_row[j * 4 + 1] = CLIP(y, 0, 0xffff);
				}
			}
		}
		break;
	}
}


BrightnessEngine::BrightnessEngine(BrightnessMain *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void BrightnessEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		BrightnessPackage *package = (BrightnessPackage*)LoadServer::get_package(i);
		package->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		package->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* BrightnessEngine::new_client()
{
	return new BrightnessUnit(this, plugin);
}

LoadPackage* BrightnessEngine::new_package()
{
	return new BrightnessPackage;
}
