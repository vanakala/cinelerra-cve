// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 1999 Winston Chang <winstonc@cs.wisc.edu> <winston@stdout.org>

#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "unsharp.h"
#include "unsharpwindow.h"
#include "picon_png.h"

#include <errno.h>
#include <unistd.h>

REGISTER_PLUGIN

UnsharpConfig::UnsharpConfig()
{
	radius = 5;
	amount = 0.5;
	threshold = 0;
}

int UnsharpConfig::equivalent(UnsharpConfig &that)
{
	return EQUIV(radius, that.radius) &&
		EQUIV(amount, that.amount) &&
		threshold == that.threshold;
}

void UnsharpConfig::copy_from(UnsharpConfig &that)
{
	radius = that.radius;
	amount = that.amount;
	threshold = that.threshold;
}

void UnsharpConfig::interpolate(UnsharpConfig &prev, 
	UnsharpConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->radius = prev.radius * prev_scale + next.radius * next_scale;
	this->amount = prev.amount * prev_scale + next.amount * next_scale;
	this->threshold = round(prev.threshold * prev_scale + next.threshold * next_scale);
}


UnsharpMain::UnsharpMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

UnsharpMain::~UnsharpMain()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void UnsharpMain::reset_plugin()
{
	if(engine)
		delete engine;
}

PLUGIN_CLASS_METHODS

void UnsharpMain::load_defaults()
{
	defaults = load_defaults_file("unsharp.rc");

	config.radius = defaults->get("RADIUS", config.radius);
	config.amount = defaults->get("AMOUNT", config.amount);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
}

void UnsharpMain::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("AMOUNT", config.amount);
	defaults->update("THRESHOLD", config.threshold);
	defaults->save();
}

void UnsharpMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("UNSHARP");
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("AMOUNT", config.amount);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.append_tag();
	output.tag.set_title("/UNSHARP");
	output.append_tag();
	keyframe->set_data(output.string);
}

void UnsharpMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("UNSHARP"))
		{
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.amount = input.tag.get_property("AMOUNT", config.amount);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
		}
	}
}

VFrame *UnsharpMain::process_tmpframe(VFrame *frame)
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

	if(load_configuration())
		update_gui();

	if(!engine)
		engine = new UnsharpEngine(this,
			get_project_smp(),
			get_project_smp());
	engine->do_unsharp(frame);
	return frame;
}

UnsharpPackage::UnsharpPackage()
 : LoadPackage()
{
}

UnsharpUnit::UnsharpUnit(UnsharpEngine *server, 
	UnsharpMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
	temp = 0;
}

UnsharpUnit::~UnsharpUnit()
{
	delete temp;
}

// Derived from the Gimp.
// In the original file it says
//
// Copyright (C) 1999 Winston Chang
//                    <winstonc@cs.wisc.edu>
//                    <winston@stdout.org>
//
// Adapted for Cinelerra by Heroine Virtual Ltd.

static int calculate_convolution_matrix(double radius, double **cmatrix)
{
	radius = fabs(radius) + 1.0;
	double std_dev = radius;
	radius = std_dev * 2;
	int matrix_length = (int)(2 * ceil(radius - 0.5) + 1);
	matrix_length = MAX(1, matrix_length);
	int matrix_midpoint = matrix_length / 2 + 1;
	(*cmatrix) = new double[matrix_length];

// Top right of matrix
	for(int i = matrix_length / 2 + 1; i < matrix_length; i++)
	{
		double base_x = i - floor(matrix_length / 2) - 0.5;
		double sum = 0;
		for(int j = 1; j <= 50; j++)
		{
			if(base_x + 0.02 * j <= radius)
			{
				sum += exp(-(base_x + 0.02 * j) * 
					(base_x + 0.02 * j) /
					(2 * std_dev * std_dev));
			}
		}
		(*cmatrix)[i] = sum / 50;
	}

// Top left of matrix
	for(int i = 0; i < matrix_length / 2; i++)
	{
		(*cmatrix)[i] = (*cmatrix)[matrix_length - 1 - i];
	}

// Center value
	double sum = 0;
	for(int j = 0; j <= 50; j++)
	{
		sum += exp(-(0.5 + 0.02 * j) *
			(0.5 + 0.02 * j) /
			(2 * std_dev * std_dev));
	}
	(*cmatrix)[matrix_length / 2] = sum / 51;

// Normalize
	sum = 0;
	for(int i = 0; i < matrix_length; i++)
		sum += (*cmatrix)[i];
	for(int i = 0; i < matrix_length; i++)
		(*cmatrix)[i] = (*cmatrix)[i] / sum;

	return matrix_length;
}

static double get_convolution(double *cmatrix,
	double input,
	int index)
{
	return cmatrix[index] * input;
}

static void blur_pixels(double *cmatrix, 
	int cmatrix_length,
	float *input,
	float *output,
	int pixels)
{
	if(cmatrix_length > pixels)
	{
		for(int pixel = 0; pixel < pixels; pixel++)
		{
			double scale = 0;
			for(int j = 0; j < pixels; j++)
			{
				if((j + cmatrix_length / 2 - pixel >= 0) &&
					(j + cmatrix_length / 2 - pixel < cmatrix_length))
				{
					scale += cmatrix[j + cmatrix_length / 2 - pixel];
				}
			}

			for(int i = 0; i < 4; i++)
			{
				double sum = 0;
				for(int j = 0; j < pixels; j++)
				{
					if((j >= pixel - cmatrix_length / 2) &&
						(j <= pixel + cmatrix_length / 2))
					{
						sum += input[j * 4 + i] * cmatrix[i];
					}
				}
				output[pixel * 4 + i] = sum / scale;
			}
		}
	}
	else
	{
		int cmatrix_middle = cmatrix_length / 2;
		int pixel;
		for(pixel = 0; pixel < cmatrix_middle; pixel++)
		{
			double scale = 0;
			for(int j = cmatrix_middle - pixel; j < cmatrix_length;j++)
			{
				scale += cmatrix[j];
			}

			for(int i = 0; i < 4; i++)
			{
				double sum = 0;
				for(int j = cmatrix_middle - pixel; j < cmatrix_length; j++)
				{
					sum += input[(pixel + j - cmatrix_middle) * 4 + i] *
						cmatrix[j];
				}
				output[pixel * 4 + i] = sum / scale;
			}
		}

		float *output_ptr = output + pixel * 4;
		for( ; pixel < pixels - cmatrix_middle; pixel++)
		{
			float *input_ptr = input + (pixel - cmatrix_middle) * 4;
			for(int i = 0; i < 4; i++)
			{
				double sum = 0;
				float *input_ptr2 = input_ptr;
				for(int j = cmatrix_length; j > 0; j--)
				{
					sum += get_convolution(cmatrix, 
						*input_ptr2, 
						cmatrix_length - j);
					input_ptr2 += 4;
				}
				input_ptr++;
				*output_ptr++ = sum;
			}
		}

		for( ; pixel < pixels; pixel++)
		{
			double scale = 0;
			for(int j = 0; j < pixels - pixel + cmatrix_middle; j++)
			{
				scale += cmatrix[j];
			}

			for(int i = 0; i < 4; i++)
			{
				double sum = 0;
				for(int j = 0; j < pixels - pixel + cmatrix_middle; j++)
				{
					sum += input[(pixel + j - cmatrix_middle) * 4 + i] *
						cmatrix[j];
				}
				output[pixel * 4 + i] = sum / scale;
			}
		}
	}
}

static void get_row(float *dst, VFrame *src, int row)
{
	uint16_t *in_row = (uint16_t*)src->get_row_ptr(row);
	int pixels = src->get_w() * 4;

	for(int i = 0; i < pixels; i++)
	{
		dst[i] = in_row[i];
	}
}

static void get_column(float *dst, VFrame *src, int column)
{
	for(int i = 0; i < src->get_h(); i++)
	{
		float *input_pixel = (float*)src->get_row_ptr(i) + column * 4;
		memcpy(dst, input_pixel, sizeof(float) * 4);
		dst += 4;
	}
}

static void put_column(float *src, VFrame *dst, int column)
{
	for(int i = 0; i < dst->get_h(); i++)
	{
		float *output_pixel = (float*)dst->get_row_ptr(i) + column * 4;
		memcpy(output_pixel, src, sizeof(float) * 4);
		src += 4;
	}
}

void UnsharpUnit::process_package(LoadPackage *package)
{
	UnsharpPackage *pkg = (UnsharpPackage*)package;
	int w = server->src->get_w();
	int h = server->src->get_h();
	int color_model = server->src->get_color_model();
	double *cmatrix = 0;
	int cmatrix_length = 0;
	int padded_y1 = pkg->y1;
	int padded_y2 = pkg->y2;
	float threshold = (float)plugin->config.threshold * 0xffff / 0xff;
	float amount = plugin->config.amount;
	int value;

	cmatrix_length = calculate_convolution_matrix(
		plugin->config.radius, &cmatrix);

	if(padded_y2 < server->src->get_h())
	{
		padded_y2 += cmatrix_length / 2;
		padded_y2 = MIN(server->src->get_h(), padded_y2);
	}
	if(padded_y1 > 0)
	{
		padded_y1 -= cmatrix_length / 2;
		padded_y1 = MAX(0, padded_y1);
	}

	int padded_rows = padded_y2 - padded_y1;

	if(!temp || temp->get_h() != padded_rows)
	{
		delete temp;
		temp = 0;
	}

	if(!temp)
	{
		temp = new VFrame(0,
			server->src->get_w(),
			padded_rows,
			BC_RGBA_FLOAT);
	}

	float *temp_in = new float[MAX(temp->get_w(), padded_rows) * 4];
	float *temp_out = new float[MAX(temp->get_w(), padded_rows) * 4];

// Blur rows
	for(int i = padded_y1; i < padded_y2; i++)
	{
		get_row(temp_in, server->src, i);
		blur_pixels(cmatrix, 
			cmatrix_length,
			temp_in,
			temp_out,
			temp->get_w());
		memcpy(temp->get_row_ptr(i - padded_y1),
			temp_out,
			temp->get_bytes_per_line());
	}

// Now we're 100% floating point.  Blur the columns
	for(int i = 0; i < temp->get_w(); i++)
	{
		get_column(temp_in, temp, i);
		blur_pixels(cmatrix,
			cmatrix_length,
			temp_in,
			temp_out,
			padded_rows);
		put_column(temp_out, temp, i);
	}

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		for(int i = pkg->y1; i < pkg->y2; i++)
		{
			float *blurry_row = (float*)temp->get_row_ptr(i - padded_y1);
			uint16_t *orig_row = (uint16_t*)server->src->get_row_ptr(i);

			for(int j = 0; j < server->src->get_w(); j++)
			{
				for(int k = 0; k < 4; k++)
				{
					float diff = *orig_row - *blurry_row;
					if(fabsf(2 * diff) >= threshold)
					{
						value = *orig_row + amount * diff;
						CLAMP(value, 0, 0xffff);
						*orig_row = value;
					}
					blurry_row++;
					orig_row++;
				}
			}
		}
		break;
	}

	delete [] temp_in;
	delete [] temp_out;
	delete [] cmatrix;
}


UnsharpEngine::UnsharpEngine(UnsharpMain *plugin, 
	int total_clients,
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void UnsharpEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		UnsharpPackage *pkg = (UnsharpPackage*)get_package(i);
		pkg->y1 = src->get_h() * i / get_total_packages();
		pkg->y2 = src->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* UnsharpEngine::new_client()
{
	return new UnsharpUnit(this, plugin);
}

LoadPackage* UnsharpEngine::new_package()
{
	return new UnsharpPackage;
}


void UnsharpEngine::do_unsharp(VFrame *src)
{
	this->src = src;

	process_packages();
}
