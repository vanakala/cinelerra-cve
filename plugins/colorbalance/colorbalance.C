
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

#include "clip.h"
#include "filexml.h"
#include "colorbalance.h"
#include "bchash.h"
#include "picon_png.h"

#include "aggregated.h"
#include "../gamma/aggregated.h"

#include <stdio.h>
#include <string.h>

// 1000 corresponds to (1.0 + MAX_COLOR) * input
#define MAX_COLOR 1.0


REGISTER_PLUGIN

ColorBalanceConfig::ColorBalanceConfig()
{
	cyan = 0;
	magenta = 0;
	yellow = 0;
	lock_params = 0;
	preserve = 0;
}

int ColorBalanceConfig::equivalent(ColorBalanceConfig &that)
{
	return (EQUIV(cyan, that.cyan) &&
		EQUIV(magenta, that.magenta) &&
		EQUIV(yellow, that.yellow) && 
		lock_params == that.lock_params && 
		preserve == that.preserve);
}

void ColorBalanceConfig::copy_from(ColorBalanceConfig &that)
{
	cyan = that.cyan;
	magenta = that.magenta;
	yellow = that.yellow;
	lock_params = that.lock_params;
	preserve = that.preserve;
}

void ColorBalanceConfig::interpolate(ColorBalanceConfig &prev, 
	ColorBalanceConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->cyan = prev.cyan * prev_scale + next.cyan * next_scale;
	this->magenta = prev.magenta * prev_scale + next.magenta * next_scale;
	this->yellow = prev.yellow * prev_scale + next.yellow * next_scale;
	this->preserve = prev.preserve;
	this->lock_params = prev.lock_params;
}


ColorBalanceEngine::ColorBalanceEngine(ColorBalanceMain *plugin)
 : Thread()
{
	this->plugin = plugin;
	last_frame = 0;
	set_synchronous(1);
}

ColorBalanceEngine::~ColorBalanceEngine()
{
	last_frame = 1;
	input_lock.unlock();
	Thread::join();
}

void ColorBalanceEngine::start_process_frame(VFrame *output, VFrame *input, int row_start, int row_end)
{
	this->output = output;
	this->input = input;
	this->row_start = row_start;
	this->row_end = row_end;
	input_lock.unlock();
}

void ColorBalanceEngine::wait_process_frame()
{
	output_lock.lock("ColorBalanceEngine::wait_process_frame");
}

void ColorBalanceEngine::run()
{
	while(1)
	{
		input_lock.lock("ColorBalanceEngine::run");
		if(last_frame)
		{
			output_lock.unlock();
			return;
		}

#define PROCESS(yuvtorgb,  \
	rgbtoyuv,  \
	r_lookup,  \
	g_lookup,  \
	b_lookup,  \
	type,  \
	max,  \
	components,  \
	do_yuv) \
{ \
	int i, j, k; \
	int y, cb, cr, r, g, b, r_n, g_n, b_n; \
	float h, s, v, h_old, s_old, r_f, g_f, b_f; \
	type **input_rows, **output_rows; \
	input_rows = (type**)input->get_rows(); \
	output_rows = (type**)output->get_rows(); \
 \
	for(j = row_start; j < row_end; j++) \
	{ \
		for(k = 0; k < input->get_w() * components; k += components) \
		{ \
			if(do_yuv) \
			{ \
				y = input_rows[j][k]; \
				cb = input_rows[j][k + 1]; \
				cr = input_rows[j][k + 2]; \
				yuvtorgb(r, g, b, y, cb, cr); \
			} \
			else \
			{ \
				r = input_rows[j][k]; \
				g = input_rows[j][k + 1]; \
				b = input_rows[j][k + 2]; \
			} \
 \
			r = CLAMP(r, 0, max-1); g = CLAMP(g, 0, max-1); b = CLAMP(b, 0, max-1); \
			r_n = plugin->r_lookup[r]; \
			g_n = plugin->g_lookup[g]; \
			b_n = plugin->b_lookup[b]; \
 \
			if(plugin->config.preserve) \
			{ \
				HSV::rgb_to_hsv((float)r_n, (float)g_n, (float)b_n, h, s, v); \
				HSV::rgb_to_hsv((float)r, (float)g, (float)b, h_old, s_old, v); \
				HSV::hsv_to_rgb(r_f, g_f, b_f, h, s, v); \
				r = (type)r_f; \
				g = (type)g_f; \
				b = (type)b_f; \
			} \
			else \
			{ \
				r = r_n; \
				g = g_n; \
				b = b_n; \
			} \
 \
			if(do_yuv) \
			{ \
				rgbtoyuv(CLAMP(r, 0, max), CLAMP(g, 0, max), CLAMP(b, 0, max), y, cb, cr); \
				output_rows[j][k] = y; \
				output_rows[j][k + 1] = cb; \
				output_rows[j][k + 2] = cr; \
			} \
			else \
			{ \
				output_rows[j][k] = CLAMP(r, 0, max); \
				output_rows[j][k + 1] = CLAMP(g, 0, max); \
				output_rows[j][k + 2] = CLAMP(b, 0, max); \
			} \
		} \
	} \
}

#define PROCESS_F(components) \
{ \
	int i, j, k; \
	float y, cb, cr, r, g, b, r_n, g_n, b_n; \
	float h, s, v, h_old, s_old, r_f, g_f, b_f; \
	float **input_rows, **output_rows; \
	input_rows = (float**)input->get_rows(); \
	output_rows = (float**)output->get_rows(); \
	cyan_f = plugin->calculate_transfer(plugin->config.cyan); \
	magenta_f = plugin->calculate_transfer(plugin->config.magenta); \
	yellow_f = plugin->calculate_transfer(plugin->config.yellow); \
 \
	for(j = row_start; j < row_end; j++) \
	{ \
		for(k = 0; k < input->get_w() * components; k += components) \
		{ \
			r = input_rows[j][k]; \
			g = input_rows[j][k + 1]; \
			b = input_rows[j][k + 2]; \
 \
			r_n = r * cyan_f; \
			g_n = g * magenta_f; \
			b_n = b * yellow_f; \
 \
			if(plugin->config.preserve) \
			{ \
				HSV::rgb_to_hsv(r_n, g_n, b_n, h, s, v); \
				HSV::rgb_to_hsv(r, g, b, h_old, s_old, v); \
				HSV::hsv_to_rgb(r_f, g_f, b_f, h, s, v); \
				r = (float)r_f; \
				g = (float)g_f; \
				b = (float)b_f; \
			} \
			else \
			{ \
				r = r_n; \
				g = g_n; \
				b = b_n; \
			} \
 \
			output_rows[j][k] = r; \
			output_rows[j][k + 1] = g; \
			output_rows[j][k + 2] = b; \
		} \
	} \
}

		switch(input->get_color_model())
		{
		case BC_RGB888:
			PROCESS(yuv.yuv_to_rgb_8,
				yuv.rgb_to_yuv_8,
				r_lookup_8,
				g_lookup_8,
				b_lookup_8,
				unsigned char,
				0xff,
				3,
				0);
			break;

		case BC_RGB_FLOAT:
			PROCESS_F(3);
			break;

		case BC_YUV888:
			PROCESS(yuv.yuv_to_rgb_8,
				yuv.rgb_to_yuv_8,
				r_lookup_8,
				g_lookup_8,
				b_lookup_8,
				unsigned char,
				0xff,
				3,
				1);
			break;

		case BC_RGBA_FLOAT:
			PROCESS_F(4);
			break;

		case BC_RGBA8888:
			PROCESS(yuv.yuv_to_rgb_8,
				yuv.rgb_to_yuv_8,
				r_lookup_8,
				g_lookup_8,
				b_lookup_8,
				unsigned char,
				0xff,
				4,
				0);
			break;

		case BC_YUVA8888:
			PROCESS(yuv.yuv_to_rgb_8,
				yuv.rgb_to_yuv_8,
				r_lookup_8,
				g_lookup_8,
				b_lookup_8,
				unsigned char,
				0xff,
				4,
				1);
			break;

		case BC_YUV161616:
			PROCESS(yuv.yuv_to_rgb_16,
				yuv.rgb_to_yuv_16,
				r_lookup_16,
				g_lookup_16,
				b_lookup_16,
				u_int16_t,
				0xffff,
				3,
				1);
			break;

		case BC_RGBA16161616:
			PROCESS(yuv.yuv_to_rgb_16,
				yuv.rgb_to_yuv_16,
				r_lookup_16,
				g_lookup_16,
				b_lookup_16,
				uint16_t,
				0xffff,
				4,
				0);
			break;

		case BC_AYUV16161616:
			{
				int i, j, k;
				int y, cb, cr, r, g, b, r_n, g_n, b_n;
				float h, s, v, h_old, s_old, r_f, g_f, b_f;
				for(j = row_start; j < row_end; j++)
				{
					uint16_t *irow = (uint16_t*)input->get_row_ptr(j);
					uint16_t *orow = (uint16_t*)output->get_row_ptr(j);

					for(k = 0; k < input->get_w() * 4; k += 4)
					{
						y = irow[k + 1];
						cb = irow[k + 2];
						cr = irow[k + 3];
						yuv.yuv_to_rgb_16(r, g, b, y, cb, cr);

						r = CLAMP(r, 0, 0xfffe);
						g = CLAMP(g, 0, 0xfffe);
						b = CLAMP(b, 0, 0xfffe);

						r_n = plugin->r_lookup_16[r];
						g_n = plugin->g_lookup_16[g];
						b_n = plugin->b_lookup_16[b];

						if(plugin->config.preserve)
						{
							HSV::rgb_to_hsv((float)r_n,
								(float)g_n, (float)b_n, h, s, v);
							HSV::rgb_to_hsv((float)r,
								(float)g, (float)b,
							h_old, s_old, v);
							HSV::hsv_to_rgb(r_f, g_f, b_f,
								h, s, v);
							r = (uint16_t)r_f;
							g = (uint16_t)g_f;
							b = (uint16_t)b_f;
						}
						else
						{
							r = r_n;
							g = g_n;
							b = b_n;
						}

						yuv.rgb_to_yuv_16(CLAMP(r, 0, 0xffff),
							CLAMP(g, 0, 0xffff),
							CLAMP(b, 0, 0xffff),
							y, cb, cr);

						orow[k + 1] = y;
						orow[k + 2] = cb;
						orow[k + 3] = cr;
					}
				}
			}
			break;
		}

		output_lock.unlock();
	}
}


ColorBalanceMain::ColorBalanceMain(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ColorBalanceMain::~ColorBalanceMain()
{
	if(engine)
	{
		for(int i = 0; i < total_engines; i++)
		{
			delete engine[i];
		}
		delete [] engine;
	}
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void ColorBalanceMain::reconfigure()
{
	int r_n, g_n, b_n;
	float r_scale = calculate_transfer(config.cyan);
	float g_scale = calculate_transfer(config.magenta);
	float b_scale = calculate_transfer(config.yellow);


#define RECONFIGURE(r_lookup, g_lookup, b_lookup, max) \
	for(int i = 0; i <= max; i++) \
	{ \
		r_lookup[i] = CLIP((int)(r_scale * i), 0, max); \
		g_lookup[i] = CLIP((int)(g_scale * i), 0, max); \
		b_lookup[i] = CLIP((int)(b_scale * i), 0, max); \
	}

	RECONFIGURE(r_lookup_8, g_lookup_8, b_lookup_8, 0xff);
	RECONFIGURE(r_lookup_16, g_lookup_16, b_lookup_16, 0xffff);
}

float ColorBalanceMain::calculate_slider(float in)
{
	if(in < 1.0)
	{
		return (in * 1000 - 1000.0);
	}
	else
	if(in > 1.0)
	{
		return (1000 * (in - 1.0) / MAX_COLOR);
	}
	else
		return 0;
}

float ColorBalanceMain::calculate_transfer(float in)
{
	if(in < 0)
	{
		return (1000.0 + in) / 1000.0;
	}
	else
	if(in > 0)
	{
		return 1.0 + in / 1000.0 * MAX_COLOR;
	}
	else
		return 1.0;
}


void ColorBalanceMain::test_boundary(float &value)
{
	if(value < -1000) value = -1000;
	if(value > 1000) value = 1000;
}

void ColorBalanceMain::synchronize_params(ColorBalanceSlider *slider, float difference)
{
	if(thread && config.lock_params)
	{
		if(slider != thread->window->cyan)
		{
			config.cyan += difference;
			test_boundary(config.cyan);
			thread->window->cyan->update(config.cyan);
		}
		if(slider != thread->window->magenta)
		{
			config.magenta += difference;
			test_boundary(config.magenta);
			thread->window->magenta->update(config.magenta);
		}
		if(slider != thread->window->yellow)
		{
			config.yellow += difference;
			test_boundary(config.yellow);
			thread->window->yellow->update(config.yellow);
		}
	}
}

void ColorBalanceMain::process_frame(VFrame *frame)
{
	need_reconfigure |= load_configuration();

	if(need_reconfigure)
	{
		int i;

		if(!engine)
		{
			total_engines = PluginClient::smp > 1 ? 2 : 1;
			engine = new ColorBalanceEngine*[total_engines];
			for(int i = 0; i < total_engines; i++)
			{
				engine[i] = new ColorBalanceEngine(this);
				engine[i]->start();
			}
		}

		reconfigure();
		need_reconfigure = 0;
	}

	get_frame(frame, get_use_opengl());

	if(!EQUIV(config.cyan, 0) || 
		!EQUIV(config.magenta, 0) || 
		!EQUIV(config.yellow, 0))
	{
		if(get_use_opengl())
		{
			run_opengl();
			return;
		}

		for(int i = 0; i < total_engines; i++)
		{
			engine[i]->start_process_frame(frame, 
				frame, 
				frame->get_h() * i / total_engines, 
				frame->get_h() * (i + 1) / total_engines);
		}

		for(int i = 0; i < total_engines; i++)
		{
			engine[i]->wait_process_frame();
		}
	}
}

void ColorBalanceMain::load_defaults()
{
	defaults = load_defaults_file("colorbalance.rc");

	config.cyan = defaults->get("CYAN", config.cyan);
	config.magenta = defaults->get("MAGENTA", config.magenta);
	config.yellow = defaults->get("YELLOW", config.yellow);
	config.preserve = defaults->get("PRESERVELUMINOSITY", config.preserve);
	config.lock_params = defaults->get("LOCKPARAMS", config.lock_params);
}

void ColorBalanceMain::save_defaults()
{
	defaults->update("CYAN", config.cyan);
	defaults->update("MAGENTA", config.magenta);
	defaults->update("YELLOW", config.yellow);
	defaults->update("PRESERVELUMINOSITY", config.preserve);
	defaults->update("LOCKPARAMS", config.lock_params);
	defaults->save();
}

void ColorBalanceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("COLORBALANCE");
	output.tag.set_property("CYAN", config.cyan);
	output.tag.set_property("MAGENTA",  config.magenta);
	output.tag.set_property("YELLOW",  config.yellow);
	output.tag.set_property("PRESERVELUMINOSITY",  config.preserve);
	output.tag.set_property("LOCKPARAMS",  config.lock_params);
	output.append_tag();
	output.tag.set_title("/COLORBALANCE");
	output.append_tag();
	output.terminate_string();
}

void ColorBalanceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("COLORBALANCE"))
		{
			config.cyan = input.tag.get_property("CYAN", config.cyan);
			config.magenta = input.tag.get_property("MAGENTA", config.magenta);
			config.yellow = input.tag.get_property("YELLOW", config.yellow);
			config.preserve = input.tag.get_property("PRESERVELUMINOSITY", config.preserve);
			config.lock_params = input.tag.get_property("LOCKPARAMS", config.lock_params);
		}
	}
}

void ColorBalanceMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int shader = 0;
	const char *shader_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int current_shader = 0;

	COLORBALANCE_COMPILE(shader_stack, 
		current_shader, 0)

	shader = VFrame::make_shader(0, 
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

		COLORBALANCE_UNIFORMS(shader);

	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}
