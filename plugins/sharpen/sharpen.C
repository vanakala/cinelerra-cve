// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "clip.h"
#include "colormodels.h"
#include "condition.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "sharpen.h"
#include "sharpenwindow.h"

#include <stdio.h>
#include <string.h>

REGISTER_PLUGIN


SharpenConfig::SharpenConfig()
{
	horizontal = 0;
	interlace = 0;
	sharpness = 50;
	luminance = 0;
}

void SharpenConfig::copy_from(SharpenConfig &that)
{
	horizontal = that.horizontal;
	interlace = that.interlace;
	sharpness = that.sharpness;
	luminance = that.luminance;
}

int SharpenConfig::equivalent(SharpenConfig &that)
{
	return horizontal == that.horizontal &&
		interlace == that.interlace &&
		EQUIV(sharpness, that.sharpness) &&
		luminance == that.luminance;
}

void SharpenConfig::interpolate(SharpenConfig &prev, 
	SharpenConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->sharpness = prev.sharpness * prev_scale + next.sharpness * next_scale;
	this->interlace = prev.interlace;
	this->horizontal = prev.horizontal;
	this->luminance = prev.luminance;
}


SharpenMain::SharpenMain(PluginServer *server)
 : PluginVClient(server)
{
	for(int i = 0; i < MAX_ENGINES; i++)
		engine[i] = 0;
	total_engines = 0;
	pos_lut = 0;
	neg_lut = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

SharpenMain::~SharpenMain()
{
	for(int i = 0; i < MAX_ENGINES; i++)
		delete engine[i];
	delete [] pos_lut;
	delete [] neg_lut;
	PLUGIN_DESTRUCTOR_MACRO
}

void SharpenMain::reset_plugin()
{
	if(total_engines)
	{
		for(int i = 0; i < total_engines; i++)
		{
			delete engine[i];
			engine[i] = 0;
		}
		delete [] pos_lut;
		delete [] neg_lut;
		pos_lut = 0;
		neg_lut = 0;
		total_engines = 0;
	}
}

PLUGIN_CLASS_METHODS

VFrame *SharpenMain::process_tmpframe(VFrame *input)
{
	int color_model = input->get_color_model();
	int do_reconfigure = 0;

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return input;
	}

	if(load_configuration())
	{
		update_gui();
		do_reconfigure = 1;
	}

	if(!total_engines)
	{
		total_engines = get_project_smp() > 1 ? MAX_ENGINES : 1;

		for(int i = 0; i < total_engines; i++)
		{
			engine[i] = new SharpenEngine(this);
			engine[i]->start();
		}
	}

	if(config.sharpness)
	{
		if(do_reconfigure || !pos_lut)
			get_luts();
// Arm first row
		row_step = (config.interlace) ? 2 : 1;

		for(int j = 0; j < row_step; j += total_engines)
		{
			for(int k = 0; k < total_engines && k + j < row_step; k++)
				engine[k]->start_process_frame(input, k + j);

			for(int k = 0; k < total_engines && k + j < row_step; k++)
				engine[k]->wait_process_frame();
		}
	}
	return input;
}

void SharpenMain::load_defaults()
{
	defaults = load_defaults_file("sharpen.rc");

	config.sharpness = defaults->get("SHARPNESS", config.sharpness);
	config.interlace = defaults->get("INTERLACE", config.interlace);
	config.horizontal = defaults->get("HORIZONTAL", config.horizontal);
	config.luminance = defaults->get("LUMINANCE", config.luminance);
}

void SharpenMain::save_defaults()
{
	defaults->update("SHARPNESS", config.sharpness);
	defaults->update("INTERLACE", config.interlace);
	defaults->update("HORIZONTAL", config.horizontal);
	defaults->update("LUMINANCE", config.luminance);
	defaults->save();
}

void SharpenMain::get_luts()
{
	int inv_sharpness = round(100 - config.sharpness);

	if(!pos_lut)
		pos_lut = new int[LUTS_SIZE];
	if(!neg_lut)
		neg_lut = new int[LUTS_SIZE];

	if(config.horizontal)
		inv_sharpness /= 2;

	if(inv_sharpness < 1)
		inv_sharpness = 1;

	for(int i = 0; i < LUTS_SIZE; i++)
	{
		pos_lut[i] = 800 * i / inv_sharpness;
		neg_lut[i] = (4 + pos_lut[i] - (i << 3)) >> 3;
	}
}

void SharpenMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("SHARPNESS");
	output.tag.set_property("VALUE", config.sharpness);
	output.append_tag();

	if(config.interlace)
	{
		output.tag.set_title("INTERLACE");
		output.append_tag();
		output.tag.set_title("/INTERLACE");
		output.append_tag();
	}

	if(config.horizontal)
	{
		output.tag.set_title("HORIZONTAL");
		output.append_tag();
		output.tag.set_title("/HORIZONTAL");
		output.append_tag();
	}

	if(config.luminance)
	{
		output.tag.set_title("LUMINANCE");
		output.append_tag();
		output.tag.set_title("/LUMINANCE");
		output.append_tag();
	}
	output.tag.set_title("/SHARPNESS");
	output.append_tag();
	keyframe->set_data(output.string);
}

void SharpenMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	int new_interlace = 0;
	int new_horizontal = 0;
	int new_luminance = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SHARPNESS"))
			{
				config.sharpness = input.tag.get_property("VALUE", config.sharpness);
			}
			else
			if(input.tag.title_is("INTERLACE"))
			{
				new_interlace = 1;
			}
			else
			if(input.tag.title_is("HORIZONTAL"))
			{
				new_horizontal = 1;
			}
			else
			if(input.tag.title_is("LUMINANCE"))
			{
				new_luminance = 1;
			}
		}
	}

	config.interlace = new_interlace;
	config.horizontal = new_horizontal;
	config.luminance = new_luminance;

	if(config.sharpness > MAXSHARPNESS) 
		config.sharpness = MAXSHARPNESS;
	else
		if(config.sharpness < 0) config.sharpness = 0;
}

SharpenEngine::SharpenEngine(SharpenMain *plugin)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->plugin = plugin;
	input_lock = new Condition(0,"SharpenEngine::input_lock");
	output_lock = new Condition(0, "SharpenEngine::output_lock");
	last_frame = 0;
	for(int i = 0; i < 4; i++)
		neg_rows[i] = 0;
}

SharpenEngine::~SharpenEngine()
{
	last_frame = 1;
	input_lock->unlock();
	Thread::join();

	for(int i = 0; i < 4; i++)
	{
		delete [] neg_rows[i];
	}
	delete input_lock;
	delete output_lock;
}

void SharpenEngine::start_process_frame(VFrame *output, int field)
{
	this->output = output;
	this->field = field;

// Get coefficient for floating point
	sharpness_coef = 100 - plugin->config.sharpness;
	if(plugin->config.horizontal)
		sharpness_coef /= 2;
	if(sharpness_coef < 1)
		sharpness_coef = 1;
	sharpness_coef = 800.0 / sharpness_coef;

	if(!neg_rows[0])
	{
		for(int i = 0; i < 4; i++)
			neg_rows[i] = new int[output->get_w() * 4];
	}
	input_lock->unlock();
}

void SharpenEngine::wait_process_frame()
{
	output_lock->lock("SharpenEngine::wait_process_frame");
}

double SharpenEngine::calculate_pos(double value)
{
	return sharpness_coef * value;
}

double SharpenEngine::calculate_neg(double value)
{
	return (calculate_pos(value) - (value * 8)) / 8;
}

void SharpenEngine::filter(int w,
	u_int16_t *src, 
	u_int16_t *dst,
	int *neg0, 
	int *neg1, 
	int *neg2)
{
	int *pos_lut = plugin->pos_lut;

	// Skip first pixel in row
	memcpy(dst, src, 4 * sizeof(uint16_t));
	dst += 4;
	src += 4;

	w -= 2;

	while(w > 0)
	{
		int pixel = pos_lut[src[0]] - neg0[-4] - neg0[0] -
			neg0[4] - neg1[-4] - neg1[4] -
			neg2[-4] - neg2[0] - neg2[4];
		pixel = (pixel + 4) >> 3;
		dst[0] = CLIP(pixel, 0, 0xffff);

		pixel = pos_lut[src[1]] - neg0[-3] - neg0[1] -
			neg0[5] - neg1[-3] - neg1[5] -
			neg2[-3] - neg2[1] - neg2[5];
		pixel = (pixel + 4) >> 3;
		dst[1] = CLIP(pixel, 0, 0xffff);

		pixel = pos_lut[src[2]] - neg0[-2] - neg0[2] -
			neg0[6] - neg1[-2] - neg1[6] -
			neg2[-2] - neg2[2] - neg2[6];
		pixel = (pixel + 4) >> 3;
		dst[2] = CLIP(pixel, 0, 0xffff);

		src += 4;
		dst += 4;

		neg0 += 4;
		neg1 += 4;
		neg2 += 4;
		w--;
	}
// Skip last pixel in row
	memcpy(dst, src, 4 * sizeof(uint16_t));
}

void SharpenEngine::run()
{
	int count, row;

	while(1)
	{
		input_lock->lock("SharpenEngine::run");
		if(last_frame)
		{
			output_lock->unlock();
			return;
		}

		int w = output->get_w();
		int h = output->get_h();

		switch(output->get_color_model())
		{
		case BC_RGBA16161616:
			src_rows[0] = src_rows[1] = src_rows[2] =
				src_rows[3] = (uint16_t*)output->get_row_ptr(field);

			for(int j = 0; j < w; j++)
			{
				int *neg = neg_rows[0];
				uint16_t *src = src_rows[0];

				for(int k = 0; k < 4; k++)
				{
					neg[j * 4 + k] =
						plugin->neg_lut[src[j * 4 + k]];
				}
			}

			row = 1;
			count = 1;

			for(int i = field; i < h; i += plugin->row_step)
			{
				if((i + plugin->row_step) < h)
				{
					if(count >= 3)
						count--;
					// Arm next row
					src_rows[row] = (uint16_t*)output->get_row_ptr(i + plugin->row_step);
					// Calculate neg rows
					uint16_t *src = src_rows[row];
					int *neg = neg_rows[row];

					for(int k = 0; k < w; k++)
					{
						for(int j = 0; j < 4; j++)
						{
							neg[k * 4 + j] =
								plugin->neg_lut[src[k * 4 + j]];
						}
					}

					count++;
					row = (row + 1) & 3;
				}
				else
					count--;

				dst_row = (uint16_t*)output->get_row_ptr(i);
				if(count == 3)
				{
					// Do the filter
					if(plugin->config.horizontal)
						filter(w, src_rows[(row + 2) & 3],
							dst_row,
							neg_rows[(row + 2) & 3] + 4,
							neg_rows[(row + 2) & 3] + 4,
							neg_rows[(row + 2) & 3] + 4);
				else
					filter(w, src_rows[(row + 2) & 3],
						dst_row,
						neg_rows[(row + 1) & 3] + 4,
						neg_rows[(row + 2) & 3] + 4,
						neg_rows[(row + 3) & 3] + 4);
				}
				else
				if(count == 2)
				{
					if(i == 0)
						memcpy(dst_row, src_rows[0], w * 4 * sizeof(uint16_t));
					else
						memcpy(dst_row, src_rows[2], w * 4 * sizeof(uint16_t));
				}
			}
			break;

		case BC_AYUV16161616:
			src_rows[0] = src_rows[1] =
				src_rows[2] = src_rows[3] =
				(uint16_t*)output->get_row_ptr(field);

			for(int j = 0; j < w; j++)
			{
				int *neg = neg_rows[0];
				uint16_t *src = src_rows[0];

				for(int k = 0; k < 4; k++)
				{
					neg[j * 4 + k] =
						plugin->neg_lut[(int)src[j * 4 + k]];
				}
			}

			row = 1;
			count = 1;

			for(int i = field; i < h; i += plugin->row_step)
			{
				if((i + plugin->row_step) < h)
				{
					if(count >= 3)
						count--;
					// Arm next row
					src_rows[row] = (uint16_t*)output->get_row_ptr(i + plugin->row_step);
					// Calculate neg rows
					uint16_t *src = src_rows[row];
					int *neg = neg_rows[row];

					for(int k = 0; k < w; k++)
					{
						for(int j = 0; j < 4; j++)
						{
							neg[k * 4 + j] =
								plugin->neg_lut[src[k * 4 + j]];
						}
					}

					count++;
					row = (row + 1) & 3;
				}
				else
					count--;

				dst_row = (uint16_t*)output->get_row_ptr(i);
				if(count == 3)
				{
					// Do the filter
					if(plugin->config.horizontal)
						filter(w, src_rows[(row + 2) & 3],
							dst_row,
							neg_rows[(row + 2) & 3] + 4,
							neg_rows[(row + 2) & 3] + 4,
							neg_rows[(row + 2) & 3] + 4);
					else
						filter(w,
							src_rows[(row + 2) & 3],
							dst_row,
							neg_rows[(row + 1) & 3] + 4,
							neg_rows[(row + 2) & 3] + 4,
							neg_rows[(row + 3) & 3] + 4);
				}
				else
				if(count == 2)
				{
					if(i == 0)
						memcpy(dst_row, src_rows[0], w * 4 * sizeof(uint16_t));
					else
						memcpy(dst_row, src_rows[2], w * 4 * sizeof(uint16_t));
				}
			}
			break;
		}
		output_lock->unlock();
	}
}
