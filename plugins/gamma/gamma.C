
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

#include "bcsignals.h"
#include "colorspaces.h"
#include "filexml.h"
#include "gamma.h"
#include "bchash.h"
#include "language.h"
#include "picon_png.h"
#include "workarounds.h"


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
	return (EQUIV(max, that.max) && 
		EQUIV(gamma, that.gamma) &&
		automatic == that.automatic) &&
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
	float r, g, b, y, u, v;

// The same algorithm used by dcraw
	if(engine->operation == GammaEngine::HISTOGRAM)
	{
#define HISTOGRAM_HEAD(type) \
		for(int i = pkg->start; i < pkg->end; i++) \
		{ \
			type *row = (type*)data->get_row_ptr(i); \
			for(int j = 0; j < w; j++) \
			{

#define HISTOGRAM_TAIL(components) \
				int slot; \
				slot = (int)(r * HISTOGRAM_SIZE); \
				accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++; \
				slot = (int)(g * HISTOGRAM_SIZE); \
				accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++; \
				slot = (int)(b * HISTOGRAM_SIZE); \
				accum[CLIP(slot, 0, HISTOGRAM_SIZE - 1)]++; \
				row += components; \
			} \
		}

		switch(data->get_color_model())
		{
		case BC_RGB888:
			HISTOGRAM_HEAD(unsigned char)
			r = (float)row[0] / 0xff;
			g = (float)row[1] / 0xff;
			b = (float)row[2] / 0xff;
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGBA8888:
			HISTOGRAM_HEAD(unsigned char)
			r = (float)row[0] / 0xff;
			g = (float)row[1] / 0xff;
			b = (float)row[2] / 0xff;
			HISTOGRAM_TAIL(4)
			break;
		case BC_RGB_FLOAT:
			HISTOGRAM_HEAD(float)
			r = row[0];
			g = row[1];
			b = row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGBA_FLOAT:
			HISTOGRAM_HEAD(float)
			r = row[0];
			g = row[1];
			b = row[2];
			HISTOGRAM_TAIL(4)
			break;
		case BC_YUV888:
			HISTOGRAM_HEAD(unsigned char)
			y = row[0];
			u = row[1];
			v = row[2];
			y /= 0xff;
			u = (float)((u - 0x80) / 0xff);
			v = (float)((v - 0x80) / 0xff);
			ColorSpaces::yuv_to_rgb_f(r, g, b, y, u, v);
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUVA8888:
			HISTOGRAM_HEAD(unsigned char)
			y = row[0];
			u = row[1];
			v = row[2];
			y /= 0xff;
			u = (float)((u - 0x80) / 0xff);
			v = (float)((v - 0x80) / 0xff);
			ColorSpaces::yuv_to_rgb_f(r, g, b, y, u, v);
			HISTOGRAM_TAIL(4)
			break;
		case BC_RGBA16161616:
			HISTOGRAM_HEAD(uint16_t)
			r = (float)row[0] / 0xffff;
			g = (float)row[1] / 0xffff;
			b = (float)row[2] / 0xffff;
			HISTOGRAM_TAIL(4)
			break;
		case BC_YUVA16161616:
			HISTOGRAM_HEAD(uint16_t)
			y = row[1];
			u = row[2];
			v = row[3];
			y /= 0xffff;
			u = (float)((u - 0x8000) / 0xffff);
			v = (float)((v - 0x8000) / 0xffff);
			ColorSpaces::yuv_to_rgb_f(r, g, b, y, u, v);
			HISTOGRAM_TAIL(4)
			break;
		}
	}
	else
	{
		float max = plugin->config.max;
		float scale = 1.0 / max;
		float gamma = plugin->config.gamma - 1.0;

#define GAMMA_HEAD(type) \
		for(int i = pkg->start; i < pkg->end; i++) \
		{ \
			type *row = (type*)data->get_row_ptr(i); \
			for(int j = 0; j < w; j++) \
			{

// powf errors don't show up until later in the pipeline, which makes
// this very hard to isolate.
#define MY_POW(x, y) ((x > 0.0) ? powf(x * 2 / max, y) : 0.0)

#define GAMMA_MID \
				r = r * scale * MY_POW(r, gamma); \
				g = g * scale * MY_POW(g, gamma); \
				b = b * scale * MY_POW(b, gamma); \

#define GAMMA_TAIL(components) \
				row += components; \
			} \
		}

		switch(data->get_color_model())
		{
		case BC_RGB888:
			GAMMA_HEAD(unsigned char)
			r = (float)row[0] / 0xff;
			g = (float)row[1] / 0xff;
			b = (float)row[2] / 0xff;
			GAMMA_MID
			row[0] = (int)CLIP(r * 0xff, 0, 0xff);
			row[1] = (int)CLIP(g * 0xff, 0, 0xff);
			row[2] = (int)CLIP(b * 0xff, 0, 0xff);
			GAMMA_TAIL(3)
			break;
		case BC_RGBA8888:
			GAMMA_HEAD(unsigned char)
			r = (float)row[0] / 0xff;
			g = (float)row[1] / 0xff;
			b = (float)row[2] / 0xff;
			GAMMA_MID
			row[0] = (int)CLIP(r * 0xff, 0, 0xff);
			row[1] = (int)CLIP(g * 0xff, 0, 0xff);
			row[2] = (int)CLIP(b * 0xff, 0, 0xff);
			GAMMA_TAIL(4)
			break;
		case BC_RGB_FLOAT:
			GAMMA_HEAD(float)
			r = row[0];
			g = row[1];
			b = row[2];
			GAMMA_MID
			row[0] = r;
			row[1] = g;
			row[2] = b;
			GAMMA_TAIL(3)
			break;
		case BC_RGBA_FLOAT:
			GAMMA_HEAD(float)
			r = row[0];
			g = row[1];
			b = row[2];
			GAMMA_MID
			row[0] = r;
			row[1] = g;
			row[2] = b;
			GAMMA_TAIL(4)
			break;
		case BC_YUV888:
			GAMMA_HEAD(unsigned char)
			y = row[0];
			u = row[1];
			v = row[2];
			y /= 0xff;
			u = (float)((u - 0x80) / 0xff);
			v = (float)((v - 0x80) / 0xff);
			ColorSpaces::yuv_to_rgb_f(r, g, b, y, u, v);
			GAMMA_MID
			ColorSpaces::rgb_to_yuv_f(r, g, b, y, u, v);
			y *= 0xff;
			u = u * 0xff + 0x80;
			v = v * 0xff + 0x80;
			row[0] = (int)CLIP(y, 0, 0xff);
			row[1] = (int)CLIP(u, 0, 0xff);
			row[2] = (int)CLIP(v, 0, 0xff);
			GAMMA_TAIL(3)
			break;
		case BC_YUVA8888:
			GAMMA_HEAD(unsigned char)
			y = row[0];
			u = row[1];
			v = row[2];
			y /= 0xff;
			u = (float)((u - 0x80) / 0xff);
			v = (float)((v - 0x80) / 0xff);
			ColorSpaces::yuv_to_rgb_f(r, g, b, y, u, v);
			GAMMA_MID
			ColorSpaces::rgb_to_yuv_f(r, g, b, y, u, v);
			y *= 0xff;
			u = u * 0xff + 0x80;
			v = v * 0xff + 0x80;
			row[0] = (int)CLIP(y, 0, 0xff);
			row[1] = (int)CLIP(u, 0, 0xff);
			row[2] = (int)CLIP(v, 0, 0xff);
			GAMMA_TAIL(4)
			break;
		case BC_RGBA16161616:
			GAMMA_HEAD(uint16_t)
			r = (float)row[0] / 0xffff;
			g = (float)row[1] / 0xffff;
			b = (float)row[2] / 0xffff;
			GAMMA_MID
			row[0] = (int)CLIP(r * 0xffff, 0, 0xffff);
			row[1] = (int)CLIP(g * 0xffff, 0, 0xffff);
			row[2] = (int)CLIP(b * 0xffff, 0, 0xffff);
			GAMMA_TAIL(4)
			break;
		case BC_AYUV16161616:
			GAMMA_HEAD(uint16_t)
			y = row[1];
			u = row[2];
			v = row[3];
			y /= 0xffff;
			u = (float)((u - 0x8000) / 0xffff);
			v = (float)((v - 0x8000) / 0xffff);
			ColorSpaces::yuv_to_rgb_f(r, g, b, y, u, v);
			GAMMA_MID
			ColorSpaces::rgb_to_yuv_f(r, g, b, y, u, v);
			y *= 0xffff;
			u = u * 0xffff + 0x8000;
			v = v * 0xffff + 0x8000;
			row[1] = (int)CLIP(y, 0, 0xffff);
			row[2] = (int)CLIP(u, 0, 0xffff);
			row[3] = (int)CLIP(v, 0, 0xffff);
			GAMMA_TAIL(4)
			break;
		}
	}
}

GammaEngine::GammaEngine(GammaMain *plugin)
 : LoadServer(plugin->get_project_smp() + 1, 
	plugin->get_project_smp() + 1)
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
		bzero(unit->accum, sizeof(int) * HISTOGRAM_SIZE);
	}
	bzero(accum, sizeof(int) * HISTOGRAM_SIZE);
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

PLUGIN_CLASS_METHODS

void GammaMain::process_frame(VFrame *frame)
{
	this->frame = frame;
	load_configuration();

	int use_opengl = get_use_opengl() &&
		!config.automatic && 
		(!config.plot || !gui_open());

	get_frame(frame, use_opengl);

	if(use_opengl)
	{
		run_opengl();
		return;
	}
	else
	if(config.automatic)
	{
		calculate_max(frame);
// Always plot to set the slider
		send_render_gui(this);
	}
	else
	if(config.plot) 
	{
		send_render_gui(this);
	}

	if(!engine) engine = new GammaEngine(this);
	engine->process_packages(GammaEngine::APPLY, frame);
}

void GammaMain::calculate_max(VFrame *frame)
{
	if(!engine) engine = new GammaEngine(this);
	engine->process_packages(GammaEngine::HISTOGRAM, frame);
	int total_pixels = frame->get_w() * frame->get_h() * 3;
	int max_fraction = (int)((int64_t)total_pixels * 99 / 100);
	int current = 0;
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
	GammaMain *ptr = (GammaMain*)data;
	config.max = ptr->config.max;

	if(!engine) engine = new GammaEngine(this);
	if(ptr->engine && ptr->config.automatic)
	{
		memcpy(engine->accum, 
			ptr->engine->accum, 
			sizeof(int) * HISTOGRAM_SIZE);
		thread->window->lock_window("GammaMain::render_gui");
		thread->window->update();
		thread->window->unlock_window();
	}
	else
	{
		engine->process_packages(GammaEngine::HISTOGRAM, 
			ptr->frame);
		thread->window->lock_window("GammaMain::render_gui");
		thread->window->update_histogram();
		thread->window->unlock_window();
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

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("GAMMA");
	output.tag.set_property("MAX", config.max);
	output.tag.set_property("GAMMA", config.gamma);
	output.tag.set_property("AUTOMATIC",  config.automatic);
	output.tag.set_property("PLOT",  config.plot);
	output.append_tag();
	output.tag.set_title("/GAMMA");
	output.append_tag();
	output.terminate_string();
}

void GammaMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

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
