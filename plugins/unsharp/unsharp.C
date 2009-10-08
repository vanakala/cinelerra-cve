
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

#include "bcdisplayinfo.h"
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

REGISTER_PLUGIN(UnsharpMain)



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
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->radius = prev.radius * prev_scale + next.radius * next_scale;
	this->amount = prev.amount * prev_scale + next.amount * next_scale;
	this->threshold = (int)(prev.threshold * prev_scale + next.threshold * next_scale);
}



















UnsharpMain::UnsharpMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
}

UnsharpMain::~UnsharpMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete engine;
}

char* UnsharpMain::plugin_title() { return N_("Unsharp"); }
int UnsharpMain::is_realtime() { return 1; }

NEW_PICON_MACRO(UnsharpMain)

SHOW_GUI_MACRO(UnsharpMain, UnsharpThread)

SET_STRING_MACRO(UnsharpMain)

RAISE_WINDOW_MACRO(UnsharpMain)

LOAD_CONFIGURATION_MACRO(UnsharpMain, UnsharpConfig)



void UnsharpMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("UnsharpMain::update_gui");
			thread->window->update();
			thread->window->unlock_window();
		}
	}
}


int UnsharpMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sunsharp.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.radius = defaults->get("RADIUS", config.radius);
	config.amount = defaults->get("AMOUNT", config.amount);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	return 0;
}


int UnsharpMain::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("AMOUNT", config.amount);
	defaults->update("THRESHOLD", config.threshold);
	defaults->save();
	return 0;
}



void UnsharpMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("UNSHARP");

	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("AMOUNT", config.amount);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.append_tag();
	output.tag.set_title("/UNSHARP");
	output.append_tag();
	output.terminate_string();
}

void UnsharpMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("UNSHARP"))
			{
				config.radius = input.tag.get_property("RADIUS", config.radius);
				config.amount = input.tag.get_property("AMOUNT", config.amount);
				config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			}
		}
	}
}







int UnsharpMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	int need_reconfigure = load_configuration();

	if(!engine) engine = new UnsharpEngine(this, 
		get_project_smp() + 1,
		get_project_smp() + 1);
	read_frame(frame,
		0, 
		get_source_position(),
		get_framerate());
	engine->do_unsharp(frame);
	return 0;
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
	float input,
	int index)
{
	return cmatrix[index] * input;
}

static void blur_pixels(double *cmatrix, 
	int cmatrix_length,
	float *input,
	float *output,
	int pixels,
	int components)
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

			for(int i = 0; i < components; i++)
			{
				double sum = 0;
				for(int j = 0; j < pixels; j++)
				{
					if((j >= pixel - cmatrix_length / 2) &&
						(j <= pixel + cmatrix_length / 2))
					{
						sum += input[j * components + i] * cmatrix[i];
					}
				}
				output[pixel * components + i] = sum / scale;
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

			for(int i = 0; i < components; i++)
			{
				double sum = 0;
				for(int j = cmatrix_middle - pixel; j < cmatrix_length; j++)
				{
					sum += input[(pixel + j - cmatrix_middle) * components + i] *
						cmatrix[j];
				}
				output[pixel * components + i] = sum / scale;
			}
		}

		float *output_ptr = output + pixel * components;
		for( ; pixel < pixels - cmatrix_middle; pixel++)
		{
			float *input_ptr = input + (pixel - cmatrix_middle) * components;
			for(int i = 0; i < components; i++)
			{
				double sum = 0;
				float *input_ptr2 = input_ptr;
				for(int j = cmatrix_length; j > 0; j--)
				{
					sum += get_convolution(cmatrix, 
						*input_ptr2, 
						cmatrix_length - j);
					input_ptr2 += components;
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

			for(int i = 0; i < components; i++)
			{
				double sum = 0;
				for(int j = 0; j < pixels - pixel + cmatrix_middle; j++)
				{
					sum += input[(pixel + j - cmatrix_middle) * components + i] *
						cmatrix[j];
				}
				output[pixel * components + i] = sum / scale;
			}
		}
	}
}

#define GET_ROW(type, components) \
{ \
	type *in_row = (type*)src->get_rows()[row]; \
	int pixels = src->get_w() * components; \
	for(int i = 0; i < pixels; i++) \
	{ \
		dst[i] = in_row[i]; \
	} \
}

static void get_row(float *dst, VFrame *src, int row)
{
	switch(src->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			GET_ROW(unsigned char, 3);
			break;
		case BC_RGB_FLOAT:
			GET_ROW(float, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			GET_ROW(unsigned char, 4);
			break;
		case BC_RGBA_FLOAT:
			GET_ROW(float, 4);
			break;
		case BC_YUV161616:
			GET_ROW(uint16_t, 3);
			break;
		case BC_YUVA16161616:
			GET_ROW(uint16_t, 4);
			break;
	}
}

static void get_column(float *dst, VFrame *src, int column)
{
	int components = cmodel_components(src->get_color_model());
	for(int i = 0; i < src->get_h(); i++)
	{
		float *input_pixel = (float*)src->get_rows()[i] + column * components;
		memcpy(dst, input_pixel, sizeof(float) * components);
		dst += components;
	}
}

static void put_column(float *src, VFrame *dst, int column)
{
	int components = cmodel_components(dst->get_color_model());
	for(int i = 0; i < dst->get_h(); i++)
	{
		float *output_pixel = (float*)dst->get_rows()[i] + column * components;
		memcpy(output_pixel, src, sizeof(float) * components);
		src += components;
	}
}

void UnsharpUnit::process_package(LoadPackage *package)
{
	UnsharpPackage *pkg = (UnsharpPackage*)package;
	int w = server->src->get_w();
	int h = server->src->get_h();
	int color_model = server->src->get_color_model();
	int components = cmodel_components(color_model);
	double *cmatrix = 0;
	int cmatrix_length = 0;
	int padded_y1 = pkg->y1;
	int padded_y2 = pkg->y2;

	cmatrix_length = calculate_convolution_matrix(
		plugin->config.radius, 
		&cmatrix);


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
			components == 3 ? BC_RGB_FLOAT : BC_RGBA_FLOAT);
	}

	float *temp_in = new float[MAX(temp->get_w(), padded_rows) * components];
	float *temp_out = new float[MAX(temp->get_w(), padded_rows) * components];

// Blur rows
	for(int i = padded_y1; i < padded_y2; i++)
	{
		get_row(temp_in, server->src, i);
		blur_pixels(cmatrix, 
			cmatrix_length,
			temp_in,
			temp_out,
			temp->get_w(),
			components);
		memcpy(temp->get_rows()[i - padded_y1],
		 	temp_out,
			temp->get_bytes_per_line());
	}

//Now we're 100% floating point.  Blur the columns
	for(int i = 0; i < temp->get_w(); i++)
	{
		get_column(temp_in, temp, i);
		blur_pixels(cmatrix,
			cmatrix_length,
			temp_in,
			temp_out,
			padded_rows,
			components);
		put_column(temp_out, temp, i);
	}


//printf("%f %f %d\n", plugin->config.radius,plugin->config.amount, plugin->config.threshold);


#define UNSHARPEN(type, components, max) \
{ \
	float threshold = (float)plugin->config.threshold * max / 0xff; \
	float amount = plugin->config.amount; \
 \
	for(int i = pkg->y1; i < pkg->y2; i++) \
	{ \
		float *blurry_row = (float*)temp->get_rows()[i - padded_y1]; \
		type *orig_row = (type*)server->src->get_rows()[i]; \
		for(int j = 0; j < server->src->get_w(); j++) \
		{ \
			for(int k = 0; k < components; k++) \
			{ \
				float diff = *orig_row - *blurry_row; \
				if(fabsf(2 * diff) < threshold) \
					diff = 0; \
				float value = *orig_row + amount * diff; \
				if(sizeof(type) == 4) \
					*orig_row = (type)value; \
				else \
					*orig_row = (type)CLIP(value, 0, max); \
				blurry_row++; \
				orig_row++; \
			} \
		} \
	} \
}

// Apply unsharpening
	float threshold;
	switch(color_model)
	{
		case BC_RGB888:
		case BC_YUV888:
			UNSHARPEN(unsigned char, 3, 0xff);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			UNSHARPEN(unsigned char, 4, 0xff);
			break;
		case BC_RGB_FLOAT:
			UNSHARPEN(float, 3, 1.0);
			break;
		case BC_RGBA_FLOAT:
			UNSHARPEN(float, 4, 1.0);
			break;
		case BC_YUV161616:
			UNSHARPEN(uint16_t, 3, 0xffff);
			break;
		case BC_YUVA16161616:
			UNSHARPEN(uint16_t, 4, 0xffff);
			break;
	}

	delete [] temp_in;
	delete [] temp_out;
	delete [] cmatrix;
}









UnsharpEngine::UnsharpEngine(UnsharpMain *plugin, 
	int total_clients,
	int total_packages)
 : LoadServer(
//1, 1 
total_clients, total_packages 
)
{
	this->plugin = plugin;
}

UnsharpEngine::~UnsharpEngine()
{
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









