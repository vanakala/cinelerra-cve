// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "clip.h"
#include "filexml.h"
#include "colorbalance.h"
#include "colorspaces.h"
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
	return EQUIV(cyan, that.cyan) &&
		EQUIV(magenta, that.magenta) &&
		EQUIV(yellow, that.yellow) && 
		lock_params == that.lock_params && 
		preserve == that.preserve;
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
 : Thread(THREAD_SYNCHRONOUS)
{
	this->plugin = plugin;
	last_frame = 0;
	input_lock.title = "ColorBalanceEngine::input_lock";
	output_lock.title = "ColorBalanceEngine::output_lock";
}

ColorBalanceEngine::~ColorBalanceEngine()
{
	last_frame = 1;
	input_lock.unlock();
	Thread::join();
}

void ColorBalanceEngine::start_process_frame(VFrame *input, int row_start, int row_end)
{
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

		int i, j, k;
		int y, cb, cr, r, g, b, r_n, g_n, b_n;
		int h, h_old;
		double s, v, s_old;
		int compwidth = input->get_w() * 4;

		switch(input->get_color_model())
		{
		case BC_RGBA16161616:
			for(j = row_start; j < row_end; j++)
			{
				uint16_t *in_row = (uint16_t*)input->get_row_ptr(j);

				for(k = 0; k < compwidth; k += 4)
				{
					r = in_row[k];
					g = in_row[k + 1];
					b = in_row[k + 2];

					CLAMP(r, 0, 0xfffe);
					CLAMP(g, 0, 0xfffe);
					CLAMP(b, 0, 0xfffe);

					r_n = plugin->r_lookup_16[r];
					g_n = plugin->g_lookup_16[g];
					b_n = plugin->b_lookup_16[b];

					if(plugin->config.preserve)
					{
						ColorSpaces::rgb_to_hsv(r_n, g_n, b_n, &h, &s, &v);
						ColorSpaces::rgb_to_hsv(r, g, b, &h_old, &s_old, &v);
						ColorSpaces::hsv_to_rgb(&r, &g, &b, h, s, v);
					}
					else
					{
						r = r_n;
						g = g_n;
						b = b_n;
					}

					in_row[k] = CLIP(r, 0, 0xffff);
					in_row[k + 1] = CLIP(g, 0, 0xffff);
					in_row[k + 2] = CLIP(b, 0, 0xffff);
				}
			}
			break;

		case BC_AYUV16161616:
			for(j = row_start; j < row_end; j++)
			{
				uint16_t *irow = (uint16_t*)input->get_row_ptr(j);

				for(k = 0; k < compwidth; k += 4)
				{
					y = irow[k + 1];
					cb = irow[k + 2];
					cr = irow[k + 3];
					ColorSpaces::yuv_to_rgb_16(r, g, b, y, cb, cr);

					r = CLAMP(r, 0, 0xfffe);
					g = CLAMP(g, 0, 0xfffe);
					b = CLAMP(b, 0, 0xfffe);

					r_n = plugin->r_lookup_16[r];
					g_n = plugin->g_lookup_16[g];
					b_n = plugin->b_lookup_16[b];

					if(plugin->config.preserve)
					{
						ColorSpaces::rgb_to_hsv(r_n,
							g_n, b_n, &h, &s, &v);
						ColorSpaces::rgb_to_hsv(r, g, b,
							&h_old, &s_old, &v);
						ColorSpaces::hsv_to_rgb(&r, &g, &b,
							h, s, v);
					}
					else
					{
						r = r_n;
						g = g_n;
						b = b_n;
					}

					ColorSpaces::rgb_to_yuv_16(CLAMP(r, 0, 0xffff),
						CLAMP(g, 0, 0xffff),
						CLAMP(b, 0, 0xffff),
						y, cb, cr);

					irow[k + 1] = y;
					irow[k + 2] = cb;
					irow[k + 3] = cr;
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
	total_engines = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ColorBalanceMain::~ColorBalanceMain()
{
	for(int i = 0; i < total_engines; i++)
		delete engines[i];

	PLUGIN_DESTRUCTOR_MACRO
}

void ColorBalanceMain::reset_plugin()
{
	for(int i = 0; i < total_engines; i++)
		delete engines[i];
	total_engines = 0;
}

PLUGIN_CLASS_METHODS

void ColorBalanceMain::reconfigure()
{
	double r_scale = calculate_transfer(config.cyan);
	double g_scale = calculate_transfer(config.magenta);
	double b_scale = calculate_transfer(config.yellow);

	for(int i = 0; i < 0x10000; i++)
	{
		int val;

		val = round(r_scale * i);
		r_lookup_16[i] = CLIP(val, 0, 0xffff);
		val = round(g_scale * i);
		g_lookup_16[i] = CLIP(val, 0, 0xffff);
		val = round(b_scale * i);
		b_lookup_16[i] = CLIP(val, 0, 0xffff);
	}
}

double ColorBalanceMain::calculate_slider(double in)
{
	if(in < 1.0)
	{
		return (in * 1000.0 - 1000.0);
	}
	else
	if(in > 1.0)
	{
		return (1000.0 * (in - 1.0) / MAX_COLOR);
	}
	else
		return 0;
}

double ColorBalanceMain::calculate_transfer(double in)
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

void ColorBalanceMain::test_boundary(double &value)
{
	if(value < -1000)
		value = -1000;
	if(value > 1000)
		value = 1000;
}

void ColorBalanceMain::synchronize_params(ColorBalanceSlider *slider, double difference)
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

VFrame *ColorBalanceMain::process_tmpframe(VFrame *frame)
{
	int do_reconfigure = 0;
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

	do_reconfigure |= load_configuration();

	if(!total_engines)
	{
		total_engines = get_project_smp() > 1 ? CB_MAX_ENGINES : 1;

		for(int i = 0; i < total_engines; i++)
		{
			engines[i] = new ColorBalanceEngine(this);
			engines[i]->start();
		}
		do_reconfigure = 1;
	}

	if(do_reconfigure)
	{
		update_gui();
		reconfigure();
	}

	if(!EQUIV(config.cyan, 0) || !EQUIV(config.magenta, 0) ||
		!EQUIV(config.yellow, 0))
	{
		for(int i = 0; i < total_engines; i++)
		{
			engines[i]->start_process_frame(frame,
				frame->get_h() * i / total_engines,
				frame->get_h() * (i + 1) / total_engines);
		}

		for(int i = 0; i < total_engines; i++)
		{
			engines[i]->wait_process_frame();
		}
	}
	return frame;
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

	output.tag.set_title("COLORBALANCE");
	output.tag.set_property("CYAN", config.cyan);
	output.tag.set_property("MAGENTA",  config.magenta);
	output.tag.set_property("YELLOW",  config.yellow);
	output.tag.set_property("PRESERVELUMINOSITY",  config.preserve);
	output.tag.set_property("LOCKPARAMS",  config.lock_params);
	output.append_tag();
	output.tag.set_title("/COLORBALANCE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ColorBalanceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

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
