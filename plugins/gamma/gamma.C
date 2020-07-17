// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "colorspaces.h"
#include "filexml.h"
#include "gamma.h"
#include "bchash.h"
#include "language.h"
#include "picon_png.h"

#include <stdio.h>
#include <string.h>

#include "aggregated.h"


REGISTER_PLUGIN


GammaConfig::GammaConfig()
{
	max = 1;
	gamma = 0.6;
	automatic = 1;
	plot = 1;
}

int GammaConfig::equivalent(GammaConfig &that)
{
	return EQUIV(max, that.max) &&
		EQUIV(gamma, that.gamma) &&
		automatic == that.automatic &&
		plot == that.plot;
}

void GammaConfig::copy_from(GammaConfig &that)
{
	max = that.max;
	gamma = that.gamma;
	automatic = that.automatic;
	plot = that.plot;
}

void GammaConfig::interpolate(GammaConfig &prev, 
	GammaConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->max = prev.max * prev_scale + next.max * next_scale;
	this->gamma = prev.gamma * prev_scale + next.gamma * next_scale;
	this->automatic = prev.automatic;
	this->plot = prev.plot;
}

GammaPackage::GammaPackage()
 : LoadPackage()
{
	start = end = 0;
}

GammaUnit::GammaUnit(GammaMain *plugin)
{
	this->plugin = plugin;
}

void GammaUnit::process_package(LoadPackage *package)
{
	GammaPackage *pkg = (GammaPackage*)package;
	GammaEngine *engine = (GammaEngine*)get_server();
	VFrame *data = engine->data;
	int w = data->get_w();
	int ir, ig, ib, iy, iu, iv;
	int slot;

// The same algorithm used by dcraw
	if(engine->operation == GammaEngine::HISTOGRAM)
	{
		switch(data->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = pkg->start; i < pkg->end; i++)
			{
				uint16_t *row = (uint16_t*)data->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					ir = row[0];
					ig = row[1];
					ib = row[2];
					slot = ir * HISTOGRAM_SIZE / 0xffff;
					accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++;
					slot = ig * HISTOGRAM_SIZE / 0xffff;
					accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++;
					slot = ib * HISTOGRAM_SIZE / 0xffff;
					accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++;
					row += 4;
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = pkg->start; i < pkg->end; i++)
			{
				uint16_t *row = (uint16_t*)data->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					iy = row[1];
					iu = row[2];
					iv = row[3];
					ColorSpaces::yuv_to_rgb_16(ir, ig, ib, iy, iu, iv);
					slot = ir * HISTOGRAM_SIZE / 0xffff;
					accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++;
					slot = ig * HISTOGRAM_SIZE / 0xffff;
					accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++;
					slot = ib * HISTOGRAM_SIZE / 0xffff;
					accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++;
					row += 4;
				}
			}
			break;
		}
	}
	else
	{
		double max = plugin->config.max;
		double scale = 1.0 / max;
		double gamma = plugin->config.gamma - 1.0;

		switch(data->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = pkg->start; i < pkg->end; i++)
			{
				uint16_t *row = (uint16_t*)data->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					ir = row[0];
					ig = row[1];
					ib = row[2];
					ir *= scale * pow(2.0 * ir / 0xffff, gamma);
					ig *= scale * pow(2.0 * ig / 0xffff, gamma);
					ib *= scale * pow(2.0 * ib / 0xffff, gamma);
					row[0] = CLIP(ir, 0, 0xffff);
					row[1] = CLIP(ig, 0, 0xffff);
					row[2] = CLIP(ib, 0, 0xffff);
					row += 4;
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = pkg->start; i < pkg->end; i++)
			{
				uint16_t *row = (uint16_t*)data->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					iy = row[1];
					iu = row[2];
					iv = row[3];
					ColorSpaces::yuv_to_rgb_16(ir, ig, ib, iy, iu, iv);
					ir *= scale * pow(2.0 * ir / 0xffff, gamma);
					ig *= scale * pow(2.0 * ig / 0xffff, gamma);
					ib *= scale * pow(2.0 * ib / 0xffff, gamma);
					CLAMP(ir, 0, 0xffff);
					CLAMP(ig, 0, 0xffff);
					CLAMP(ib, 0, 0xffff);
					ColorSpaces::rgb_to_yuv_16(ir, ig, ib, iy, iu, iv);
					row[1] = iy;
					row[2] = iu;
					row[3] = iv;
					row += 4;
				}
			}
			break;
		}
	}
}

GammaEngine::GammaEngine(GammaMain *plugin)
 : LoadServer(plugin->get_project_smp(),
	plugin->get_project_smp())
{
	this->plugin = plugin;
}

void GammaEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		GammaPackage *package = (GammaPackage*)get_package(i);
		package->start = data->get_h() * i / get_total_packages();
		package->end = data->get_h() * (i + 1) / get_total_packages();
	}

// Initialize clients here in case some don't get run.
	for(int i = 0; i < get_total_clients(); i++)
	{
		GammaUnit *unit = (GammaUnit*)get_client(i);
		memset(unit->accum, 0, sizeof(int) * HISTOGRAM_SIZE);
	}
	memset(accum, 0, sizeof(int) * HISTOGRAM_SIZE);
}

LoadClient* GammaEngine::new_client()
{
	return new GammaUnit(plugin);
}

LoadPackage* GammaEngine::new_package()
{
	return new GammaPackage;
}

void GammaEngine::process_packages(int operation, VFrame *data)
{
	this->data = data;
	this->operation = operation;

	LoadServer::process_packages();

	for(int i = 0; i < get_total_clients(); i++)
	{
		GammaUnit *unit = (GammaUnit*)get_client(i);
		for(int j = 0; j < HISTOGRAM_SIZE; j++)
		{
			accum[j] += unit->accum[j];
		}
	}
}


GammaMain::GammaMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

GammaMain::~GammaMain()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void GammaMain::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
	}
}

PLUGIN_CLASS_METHODS

VFrame *GammaMain::process_tmpframe(VFrame *frame)
{
	int color_model = frame->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return frame;
	}

	load_configuration();

	if(!engine)
		engine = new GammaEngine(this);

	if(config.automatic)
	{
		calculate_max(frame);
// Always plot to set the slider
		render_gui(this);
	}
	else
	if(config.plot)
	{
		engine->process_packages(GammaEngine::HISTOGRAM, frame);
		render_gui(this);
	}
	engine->process_packages(GammaEngine::APPLY, frame);
	return frame;
}

void GammaMain::calculate_max(VFrame *frame)
{
	int total_pixels = frame->get_w() * frame->get_h() * 3;
	int max_fraction = (int)((int64_t)total_pixels * 99 / 100);
	int current = 0;

	engine->process_packages(GammaEngine::HISTOGRAM, frame);
	config.max = 1;

	for(int i = 0; i < HISTOGRAM_SIZE; i++)
	{
		current += engine->accum[i];
		if(current > max_fraction)
		{
			config.max = (float)i / HISTOGRAM_SIZE;
			break;
		}
	}
}

void GammaMain::render_gui(void *data)
{
	if(thread)
	{
		if(config.automatic)
			thread->window->update();
		else
			thread->window->update_histogram();
	}
}

void GammaMain::load_defaults()
{
	defaults = load_defaults_file("gamma.rc");

	config.max = defaults->get("MAX", config.max);
	config.gamma = defaults->get("GAMMA", config.gamma);
	config.automatic = defaults->get("AUTOMATIC", config.automatic);
	config.plot = defaults->get("PLOT", config.plot);
}

void GammaMain::save_defaults()
{
	defaults->update("MAX", config.max);
	defaults->update("GAMMA", config.gamma);
	defaults->update("AUTOMATIC", config.automatic);
	defaults->update("PLOT", config.plot);
	defaults->save();
}

void GammaMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("GAMMA");
	output.tag.set_property("MAX", config.max);
	output.tag.set_property("GAMMA", config.gamma);
	output.tag.set_property("AUTOMATIC",  config.automatic);
	output.tag.set_property("PLOT",  config.plot);
	output.append_tag();
	output.tag.set_title("/GAMMA");
	output.append_tag();
	keyframe->set_data(output.string);
}

void GammaMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("GAMMA"))
		{
			config.max = input.tag.get_property("MAX", config.max);
			config.gamma = input.tag.get_property("GAMMA", config.gamma);
			config.automatic = input.tag.get_property("AUTOMATIC", config.automatic);
			config.plot = input.tag.get_property("PLOT", config.plot);
		}
	}
}

void GammaMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	get_output()->to_texture();
	get_output()->enable_opengl();

	const char *shader_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int current_shader = 0;

	GAMMA_COMPILE(shader_stack, current_shader, 0);

	unsigned int shader = VFrame::make_shader(0,
				shader_stack[0],
				shader_stack[1],
				shader_stack[2],
				shader_stack[3],
				shader_stack[4],
				shader_stack[5],
				shader_stack[6],
				shader_stack[7],
				0);

	if(shader > 0) 
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		GAMMA_UNIFORMS(shader)
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}
