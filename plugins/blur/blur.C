// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "filexml.h"
#include "blur.h"
#include "blurwindow.h"
#include "bchash.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "thread.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>


BlurConfig::BlurConfig()
{
	vertical = 1;
	horizontal = 1;
	radius = 5;
	chan0 = chan1 = chan2 = chan3 = 1;
}

int BlurConfig::equivalent(BlurConfig &that)
{
	return vertical == that.vertical &&
		horizontal == that.horizontal &&
		radius == that.radius &&
		chan0 == that.chan0 &&
		chan1 == that.chan1 &&
		chan2 == that.chan2 &&
		chan3 == that.chan3;
}

void BlurConfig::copy_from(BlurConfig &that)
{
	vertical = that.vertical;
	horizontal = that.horizontal;
	radius = that.radius;
	chan0 = that.chan0;
	chan1 = that.chan1;
	chan2 = that.chan2;
	chan3 = that.chan3;
}

void BlurConfig::interpolate(BlurConfig &prev, 
	BlurConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->vertical = prev.vertical;
	this->horizontal = prev.horizontal;
	this->radius = round(prev.radius * prev_scale + next.radius * next_scale);
	chan0 = prev.chan0;
	chan1 = prev.chan1;
	chan2 = prev.chan2;
	chan3 = prev.chan3;
}

REGISTER_PLUGIN

BlurMain::BlurMain(PluginServer *server)
 : PluginVClient(server)
{
	defaults = 0;
	temp = 0;
	engine = 0;
	num_engines = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

BlurMain::~BlurMain()
{
	if(engine)
	{
		for(int i = 0; i < num_engines; i++)
			delete engine[i];
		delete [] engine;
	}
	PLUGIN_DESTRUCTOR_MACRO
}

void BlurMain::reset_plugin()
{
	if(engine)
	{
		for(int i = 0; i < num_engines; i++)
			delete engine[i];
		delete [] engine;
		engine = 0;
		num_engines = 0;
	}
}

PLUGIN_CLASS_METHODS

VFrame *BlurMain::process_tmpframe(VFrame *input_ptr)
{
	int i, j, k, l;
	int do_reconfigure = 0;
	int color_model = input_ptr->get_color_model();

	this->input = input_ptr;
	do_reconfigure |= load_configuration();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return input_ptr;
	}

	if(do_reconfigure)
		update_gui();

	if(!engine)
	{
		num_engines = get_project_smp();
		engine = new BlurEngine*[num_engines];

		for(int i = 0; i < num_engines; i++)
		{
			engine[i] = new BlurEngine(this,
				input->get_h() * i / num_engines,
				input->get_h() * (i + 1) / num_engines);
			engine[i]->start();
		}
		do_reconfigure = 1;
	}

	if(do_reconfigure)
	{
		for(i = 0; i < num_engines; i++)
			engine[i]->reconfigure();
	}
	temp = clone_vframe(input);

	if((color_model == BC_RGBA16161616 && config.chan3) ||
			(color_model == BC_AYUV16161616 && config.chan0))
		input->set_transparent();

	if(config.radius > 2 &&
		(config.vertical || config.horizontal))
	{
// Process blur
// TODO
// Can't blur recursively.  Need to blur vertically to a temp and 
// horizontally to the output in 2 discrete passes.
		for(i = 0; i < num_engines; i++)
		{
			engine[i]->start_process_frame(input);
		}

		for(i = 0; i < num_engines; i++)
		{
			engine[i]->wait_process_frame();
		}
	}
	release_vframe(temp);
	return input_ptr;
}

void BlurMain::load_defaults()
{
	defaults = load_defaults_file("blur.rc");

	config.vertical = defaults->get("VERTICAL", config.vertical);
	config.horizontal = defaults->get("HORIZONTAL", config.horizontal);
	config.radius = defaults->get("RADIUS", config.radius);
	// Backwards compatibility
	config.chan0 = defaults->get("R", config.chan0);
	config.chan1 = defaults->get("G", config.chan1);
	config.chan2 = defaults->get("B", config.chan2);
	config.chan3 = defaults->get("A", config.chan3);

	config.chan0 = defaults->get("CHAN0", config.chan0);
	config.chan1 = defaults->get("CHAN1", config.chan1);
	config.chan2 = defaults->get("CHAN2", config.chan2);
	config.chan3 = defaults->get("CHAN3", config.chan3);
}

void BlurMain::save_defaults()
{
	defaults->update("VERTICAL", config.vertical);
	defaults->update("HORIZONTAL", config.horizontal);
	defaults->update("RADIUS", config.radius);
	defaults->delete_key("R");
	defaults->delete_key("G");
	defaults->delete_key("B");
	defaults->delete_key("A");
	defaults->update("CHAN0", config.chan0);
	defaults->update("CHAN1", config.chan1);
	defaults->update("CHAN2", config.chan2);
	defaults->update("CHAN3", config.chan3);
	defaults->save();
}

// Pooleli
void BlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.tag.set_title("BLUR");
	output.tag.set_property("VERTICAL", config.vertical);
	output.tag.set_property("HORIZONTAL", config.horizontal);
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("CHAN0", config.chan0);
	output.tag.set_property("CHAN1", config.chan1);
	output.tag.set_property("CHAN2", config.chan2);
	output.tag.set_property("CHAN3", config.chan3);
	output.append_tag();
	output.tag.set_title("/BLUR");
	output.append_tag();
	keyframe->set_data(output.string);
}

void BlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("BLUR"))
		{
			config.vertical = input.tag.get_property("VERTICAL", config.vertical);
			config.horizontal = input.tag.get_property("HORIZONTAL", config.horizontal);
			config.radius = input.tag.get_property("RADIUS", config.radius);
			// Backwards compatibility
			config.chan0 = input.tag.get_property("R", config.chan0);
			config.chan1 = input.tag.get_property("G", config.chan1);
			config.chan2 = input.tag.get_property("B", config.chan2);
			config.chan3 = input.tag.get_property("A", config.chan3);

			config.chan0 = input.tag.get_property("CHAN0", config.chan0);
			config.chan1 = input.tag.get_property("CHAN1", config.chan1);
			config.chan2 = input.tag.get_property("CHAN2", config.chan2);
			config.chan3 = input.tag.get_property("CHAN3", config.chan3);
		}
	}
}

BlurEngine::BlurEngine(BlurMain *plugin, int start_out, int end_out)
 : Thread(THREAD_SYNCHRONOUS)
{
	int size = plugin->input->get_w() > plugin->input->get_h() ? 
		plugin->input->get_w() : plugin->input->get_h();
	this->plugin = plugin;
	this->start_out = start_out;
	this->end_out = end_out;
	last_frame = 0;
	val_p = new pixel[size];
	val_m = new pixel[size];
	src = new pixel[size];
	dst = new pixel[size];
	input_lock.lock();
	output_lock.lock();
}

BlurEngine::~BlurEngine()
{
	last_frame = 1;
	input_lock.unlock();
	join();
	delete [] val_p;
	delete [] val_m;
	delete [] src;
	delete [] dst;
}

void BlurEngine::start_process_frame(VFrame *input)
{
	this->input = input;
	input_lock.unlock();
}

void BlurEngine::wait_process_frame()
{
	output_lock.lock();
}

void BlurEngine::run()
{
	int i, j, k, l;
	int strip_size;

	while(1)
	{
		input_lock.lock();
		if(last_frame)
		{
			output_lock.unlock();
			return;
		}

		start_in = start_out - plugin->config.radius;
		end_in = end_out + plugin->config.radius;

		if(start_in < 0)
			start_in = 0;
		if(end_in > plugin->input->get_h())
			end_in = plugin->input->get_h();
		strip_size = end_in - start_in;

		int w = input->get_w();
		int h = input->get_h();

		VFrame *cur_input = input;
		VFrame *cur_output = input;

		if(plugin->config.vertical)
		{
			// Vertical pass
			if(plugin->config.horizontal)
				cur_output = plugin->temp;

			for(j = 0; j < w; j++)
			{
				memset(val_p, 0, sizeof(pixel) * (end_in - start_in));
				memset(val_m, 0, sizeof(pixel) * (end_in - start_in));

				for(l = 0, k = start_in; k < end_in; l++, k++)
				{
					uint16_t *row = ((uint16_t*)cur_input->get_row_ptr(k));

					if(plugin->config.chan0)
						src[l].chnl0 = row[j * 4];
					if(plugin->config.chan1)
						src[l].chnl1 = row[j * 4 + 1];
					if(plugin->config.chan2)
						src[l].chnl2 = row[j * 4 + 2];
					if(plugin->config.chan3)
						src[l].chnl3 = row[j * 4 + 3];
				}
				blur_strip4(strip_size);

				for(l = start_out - start_in, k = start_out; k < end_out; l++, k++)
				{
					uint16_t *row = ((uint16_t*)cur_output->get_row_ptr(k));

					if(plugin->config.chan0)
						row[j * 4] = dst[l].chnl0;
					if(plugin->config.chan1)
						row[j * 4 + 1] = dst[l].chnl1;
					if(plugin->config.chan2)
						row[j * 4 + 2] = dst[l].chnl2;
					if(plugin->config.chan3)
						row[j * 4 + 3] = dst[l].chnl3;
				}
			}

			cur_input = cur_output;
			cur_output = input;
		}

		if(plugin->config.horizontal)
		{
			// Horizontal pass
			for(j = start_out; j < end_out; j++)
			{
				memset(val_p, 0, sizeof(pixel) * w);
				memset(val_m, 0, sizeof(pixel) * w);

				for(k = 0; k < w; k++)
				{
					uint16_t *row = (uint16_t*)cur_input->get_row_ptr(j);

					if(plugin->config.chan0)
						src[k].chnl0 = row[k * 4];
					if(plugin->config.chan1)
						src[k].chnl1 = row[k * 4 + 1];
					if(plugin->config.chan2)
						src[k].chnl2 = row[k * 4 + 2];
					if(plugin->config.chan3)
						src[k].chnl3 = row[k * 4 + 3];
				}
				blur_strip4(w);

				for(k = 0; k < w; k++)
				{
					uint16_t *row = (uint16_t*)cur_output->get_row_ptr(j);

					if(plugin->config.chan0)
						row[k * 4] = dst[k].chnl0;
					if(plugin->config.chan1)
						row[k * 4 + 1] = dst[k].chnl1;
					if(plugin->config.chan2)
						row[k * 4 + 2] = dst[k].chnl2;
					if(plugin->config.chan3)
						row[k * 4 + 3] = dst[k].chnl3;
				}
			}
		}

		output_lock.unlock();
	}
}

void BlurEngine::reconfigure()
{
	std_dev = sqrt(-(double)(plugin->config.radius * plugin->config.radius) /
		(2 * log (1.0 / 255.0)));
	get_constants();
}

void BlurEngine::get_constants()
{
	int i;
	double constants[8];
	double div;

	div = sqrt(2 * M_PI) * std_dev;
	constants[0] = -1.783 / std_dev;
	constants[1] = -1.723 / std_dev;
	constants[2] = 0.6318 / std_dev;
	constants[3] = 1.997  / std_dev;
	constants[4] = 1.6803 / div;
	constants[5] = 3.735 / div;
	constants[6] = -0.6803 / div;
	constants[7] = -0.2598 / div;

	n_p[0] = constants[4] + constants[6];
	n_p[1] = exp(constants[1]) *
		(constants[7] * sin(constants[3]) -
		(constants[6] + 2 * constants[4]) * cos(constants[3])) +
		exp(constants[0]) * (constants[5] * sin(constants[2]) -
		(2 * constants[6] + constants[4]) * cos(constants[2]));

	n_p[2] = 2 * exp(constants[0] + constants[1]) *
		((constants[4] + constants[6]) * cos(constants[3]) *
		cos(constants[2]) - constants[5] *
		cos(constants[3]) * sin(constants[2]) -
		constants[7] * cos(constants[2]) * sin(constants[3])) +
		constants[6] * exp(2 * constants[0]) +
		constants[4] * exp(2 * constants[1]);

	n_p[3] = exp(constants[1] + 2 * constants[0]) *
		(constants[7] * sin(constants[3]) -
		constants[6] * cos(constants[3])) +
		exp(constants[0] + 2 * constants[1]) *
		(constants[5] * sin(constants[2]) - constants[4] *
		cos(constants[2]));
	n_p[4] = 0.0;

	d_p[0] = 0.0;
	d_p[1] = -2 * exp(constants[1]) * cos(constants[3]) -
		2 * exp(constants[0]) * cos(constants[2]);

	d_p[2] = 4 * cos(constants[3]) * cos(constants[2]) * 
		exp(constants[0] + constants[1]) +
		exp(2 * constants[1]) + exp (2 * constants[0]);

	d_p[3] = -2 * cos(constants[2]) * exp(constants[0] + 2 * constants[1]) -
		2 * cos(constants[3]) * exp(constants[1] + 2 * constants[0]);

	d_p[4] = exp(2 * constants[0] + 2 * constants[1]);

	for(i = 0; i < 5; i++)
		d_m[i] = d_p[i];

	n_m[0] = 0.0;
	for(i = 1; i <= 4; i++)
		n_m[i] = n_p[i] - d_p[i] * n_p[0];

	double sum_n_p, sum_n_m, sum_d;
	double a, b;

	sum_n_p = 0.0;
	sum_n_m = 0.0;
	sum_d = 0.0;
	for(i = 0; i < 5; i++)
	{
		sum_n_p += n_p[i];
		sum_n_m += n_m[i];
		sum_d += d_p[i];
	}

	a = sum_n_p / (1 + sum_d);
	b = sum_n_m / (1 + sum_d);

	for (i = 0; i < 5; i++)
	{
		bd_p[i] = d_p[i] * a;
		bd_m[i] = d_m[i] * b;
	}
}

#define BOUNDARY(x) if((x) > 0xffff) (x) = 0xffff; else if((x) < 0) (x) = 0;

void BlurEngine::transfer_pixels(pixel *src1, pixel *src2, pixel *dest, int size)
{
	int i;
	int sum;

	for(i = 0; i < size; i++)
	{
		sum = src1[i].chnl0 + src2[i].chnl0;
		BOUNDARY(sum);
		dest[i].chnl0 = sum;
		sum = src1[i].chnl1 + src2[i].chnl1;
		BOUNDARY(sum);
		dest[i].chnl1 = sum;
		sum = src1[i].chnl2 + src2[i].chnl2;
		BOUNDARY(sum);
		dest[i].chnl2 = sum;
		sum = src1[i].chnl3 + src2[i].chnl3;
		BOUNDARY(sum);
		dest[i].chnl3 = sum;
	}
}

void BlurEngine::blur_strip4(int size)
{
	sp_p = src;
	sp_m = src + size - 1;
	vp = val_p;
	vm = val_m + size - 1;

	initial_p = sp_p[0];
	initial_m = sp_m[0];

	int l;
	for(int k = 0; k < size; k++)
	{
		int terms = (k < 4) ? k : 4;

		for(l = 0; l <= terms; l++)
		{
			if(plugin->config.chan0)
			{
				vp->chnl0 += n_p[l] * sp_p[-l].chnl0 - d_p[l] * vp[-l].chnl0;
				vm->chnl0 += n_m[l] * sp_m[l].chnl0 - d_m[l] * vm[l].chnl0;
			}
			if(plugin->config.chan1)
			{
				vp->chnl1 += n_p[l] * sp_p[-l].chnl1 - d_p[l] * vp[-l].chnl1;
				vm->chnl1 += n_m[l] * sp_m[l].chnl1 - d_m[l] * vm[l].chnl1;
			}
			if(plugin->config.chan2)
			{
				vp->chnl2 += n_p[l] * sp_p[-l].chnl2 - d_p[l] * vp[-l].chnl2;
				vm->chnl2 += n_m[l] * sp_m[l].chnl2 - d_m[l] * vm[l].chnl2;
			}
			if(plugin->config.chan3)
			{
				vp->chnl3 += n_p[l] * sp_p[-l].chnl3 - d_p[l] * vp[-l].chnl3;
				vm->chnl3 += n_m[l] * sp_m[l].chnl3 - d_m[l] * vm[l].chnl3;
			}
		}

		for(; l <= 4; l++)
		{
			if(plugin->config.chan0)
			{
				vp->chnl0 += (n_p[l] - bd_p[l]) * initial_p.chnl0;
				vm->chnl0 += (n_m[l] - bd_m[l]) * initial_m.chnl0;
			}
			if(plugin->config.chan1)
			{
				vp->chnl1 += (n_p[l] - bd_p[l]) * initial_p.chnl1;
				vm->chnl1 += (n_m[l] - bd_m[l]) * initial_m.chnl1;
			}
			if(plugin->config.chan2)
			{
				vp->chnl2 += (n_p[l] - bd_p[l]) * initial_p.chnl2;
				vm->chnl2 += (n_m[l] - bd_m[l]) * initial_m.chnl2;
			}
			if(plugin->config.chan3)
			{
				vp->chnl3 += (n_p[l] - bd_p[l]) * initial_p.chnl3;
				vm->chnl3 += (n_m[l] - bd_m[l]) * initial_m.chnl3;
			}
		}

		sp_p++;
		sp_m--;
		vp++;
		vm--;
	}
	transfer_pixels(val_p, val_m, dst, size);
}
