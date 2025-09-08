// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "rotateframe.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define SQR(x) ((x) * (x))

RotateFrame::RotateFrame(int cpus, int width, int height)
{
	int y1, y2, y_increment;

	y_increment = height / cpus;
	y1 = 0;
	this->cpus = cpus;

	engine = new RotateEngine*[cpus];
	for(int i = 0; i < cpus; i++)
	{
		y2 = y1 + y_increment;
		if(i == cpus - 1 && y2 < height - 1) y2 = height - 1;
		engine[i] = new RotateEngine(this, y1, y2);
		engine[i]->start();
		y1 += y_increment;
	}

	float_matrix = 0;
	float_rows = 0;
	last_angle = 0;
}

RotateFrame::~RotateFrame()
{
	for(int i = 0; i < cpus; i++)
		delete engine[i];
	delete [] engine;
	if(float_matrix) delete [] float_matrix;
	if(float_rows) delete [] float_rows;
}

void RotateFrame::rotate(VFrame *output, 
	VFrame *input, 
	double angle)
{
	this->angle = angle;
	this->output = output;
	this->input = input;

	if(angle != 0)
	{
		if(EQUIV(angle, 90) || EQUIV(angle, 180) || EQUIV(angle, 270))
			rotate_rightangle(input,
				output, 
				round(angle));
		else
		{
			rotate_obliqueangle(input, 
				output, 
				angle);
		}
	}
	else
// Data never processed so copy if necessary
	{
		output->copy_from(input);
	}
	this->last_angle = angle;
}

void RotateFrame::get_rightdimensions(VFrame *frame,
	int &diameter, 
	int &in_x1, 
	int &in_y1, 
	int &in_x2, 
	int &in_y2, 
	int &out_x1, 
	int &out_y1, 
	int &out_x2, 
	int &out_y2)
{
	diameter = frame->get_w() < frame->get_h() ? frame->get_w() : frame->get_h();
	out_x1 = in_x1 = frame->get_w() / 2 - diameter / 2;
	out_x2 = in_x2 = in_x1 + diameter - 1;
	out_y1 = in_y1 = frame->get_h() / 2 - diameter / 2;
	out_y2 = in_y2 = in_y1 + diameter - 1;
}


#define ROTATE_RIGHTANGLE(type, components) \
{ \
	int height = output->get_h(); \
	int width = output->get_w(); \
 \
	switch(angle) \
	{ \
	case 90: \
		get_rightdimensions(input, \
			diameter, \
			in_x1, \
			in_y1, \
			in_x2, \
			in_y2, \
			out_x1, \
			out_y1, \
			out_x2, \
			out_y2); \
		while(in_x2 > in_x1) \
		{ \
			diameter = in_x2 - in_x1; \
			for(int i = 0; i < diameter; i++) \
			{ \
				type temp_pixel[components]; \
				type *inrowy1i = (type*)input->get_row_ptr(in_y1 + i); \
				type *inrowy2mi = (type*)input->get_row_ptr(in_y2 - i); \
				type *inrowy1 = (type*)input->get_row_ptr(in_y1); \
				type *inrowy2 = (type*)input->get_row_ptr(in_y2); \
				type *outrowy1i = (type*)output->get_row_ptr(in_y1 + i); \
				type *outrowy2mi = (type*)output->get_row_ptr(in_y2 - i); \
				type *outrowy1 = (type*)output->get_row_ptr(in_y1); \
				type *outrowy2 = (type*)output->get_row_ptr(in_y2); \
\
				for(int j = 0; j < components; j++) \
				{ \
					temp_pixel[j] = inrowy1i[in_x2 * components + j]; \
 \
					outrowy1i[in_x2 * components + j] = inrowy1[(in_x1 + i) * components + j]; \
					outrowy1[(in_x1 + i) * components + j] = inrowy2mi[in_x1 * components + j]; \
					outrowy2mi[in_x1 * components + j] = inrowy2[(in_x2 - i) * components + j]; \
					outrowy2[(in_x2 - i) * components + j] = temp_pixel[j]; \
				} \
			} \
 \
			in_x2--; \
			in_x1++; \
			in_y2--; \
			in_y1++; \
		} \
		break; \
 \
	case 180: \
		for(int i = 0, j = height - 1; i < j; i++, j--) \
		{ \
			type *inrowj = (type*)input->get_row_ptr(j); \
			type *inrowi = (type*)input->get_row_ptr(i); \
			type *outrowi = (type*)output->get_row_ptr(i); \
			type *outrowj = (type*)output->get_row_ptr(j); \
			for(int k = 0, l = width - 1; k < width; k++, l--) \
			{ \
				type temp_pixel[components]; \
				for(int m = 0; m < components; m++) \
				{ \
					temp_pixel[m] = inrowj[k * components + m]; \
					outrowj[k * components + m] = inrowi[l * components + m]; \
					outrowi[l * components + m] = temp_pixel[m]; \
				} \
			} \
		} \
		break; \
 \
	case 270: \
		get_rightdimensions(input, \
			diameter, \
			in_x1, \
			in_y1, \
			in_x2, \
			in_y2, \
			out_x1, \
			out_y1, \
			out_x2, \
			out_y2); \
 \
		while(in_x2 > in_x1) \
		{ \
			diameter = in_x2 - in_x1; \
			for(int i = 0; i < diameter; i++) \
			{ \
				type temp_pixel[components]; \
				type *inrowy1i = (type*)input->get_row_ptr(in_y1 + i); \
				type *inrowy2mi = (type*)input->get_row_ptr(in_y2 - i); \
				type *inrowy1 = (type*)input->get_row_ptr(in_y1); \
				type *inrowy2 = (type*)input->get_row_ptr(in_y2); \
				type *outrowy1i = (type*)output->get_row_ptr(in_y1 + i); \
				type *outrowy2mi = (type*)output->get_row_ptr(in_y2 - i); \
				type *outrowy1 = (type*)output->get_row_ptr(in_y1); \
				type *outrowy2 = (type*)output->get_row_ptr(in_y2); \
				for(int j = 0; j < components; j++) \
				{ \
					temp_pixel[j] = inrowy1i[in_x1 * components + j]; \
					outrowy1i[in_x1 * components + j]   = inrowy1[(in_x2 - i) * components + j]; \
					outrowy1[(in_x2 - i) * components + j] = inrowy2mi[in_x2 * components + j]; \
					outrowy2mi[in_x2 * components + j]   = inrowy2[(in_x1 + i) * components + j]; \
					outrowy2[(in_x1 + i) * components + j] = temp_pixel[j]; \
				} \
			} \
 \
			in_x2--; \
			in_x1++; \
			in_y2--; \
			in_y1++; \
		} \
		break; \
	} \
}


void RotateFrame::rotate_rightangle(VFrame *input,
	VFrame *output, 
	int angle)
{
	int in_x1 = 0;
	int in_y1 = 0;
	int in_x2 = input->get_w();
	int in_y2 = input->get_h();
	int out_x1, out_y1, out_x2, out_y2;
	int diameter;

	output->clear_frame();
	switch(output->get_color_model())
	{
	case BC_RGB888:
		ROTATE_RIGHTANGLE(unsigned char, 3);
		break;
	case BC_RGBA8888:
		ROTATE_RIGHTANGLE(unsigned char, 4);
		break;
	case BC_RGB_FLOAT:
		ROTATE_RIGHTANGLE(float, 3);
		break;
	case BC_RGBA_FLOAT:
		ROTATE_RIGHTANGLE(float, 4);
		break;
	case BC_YUV888:
		ROTATE_RIGHTANGLE(unsigned char, 3);
		break;
	case BC_YUVA8888:
		ROTATE_RIGHTANGLE(unsigned char, 4);
		break;
	case BC_RGB161616:
		ROTATE_RIGHTANGLE(uint16_t, 3);
		break;
	case BC_RGBA16161616:
		ROTATE_RIGHTANGLE(uint16_t, 4);
		break;
	case BC_YUV161616:
		ROTATE_RIGHTANGLE(uint16_t, 3);
		break;
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		ROTATE_RIGHTANGLE(uint16_t, 4);
		break;
	}
}

void RotateFrame::rotate_obliqueangle(VFrame *input,
	VFrame *output, 
	double angle)
{
	int i;
	int center_x, center_y;
	int need_matrix = 0;

	center_x = input->get_w() / 2;
	center_y = input->get_h() / 2;

	if(last_angle != angle || !float_matrix)
	{
		if(!float_matrix)
		{
			float_matrix = new SourceCoord[input->get_w() * input->get_h()];
			float_rows = new SourceCoord*[input->get_h()];

			for(i = 0; i < input->get_h(); i++)
				float_rows[i] = &float_matrix[i * input->get_w()];
		}
		need_matrix = 1;
	}

	if(last_angle != angle)
		need_matrix = 1;

	if(need_matrix)
	{
// Last angle != angle implied by first buffer needing to be allocated
		for(i = 0; i < cpus; i++)
		{
			engine[i]->generate_matrix();
		}

		for(i = 0; i < cpus; i++)
		{
			engine[i]->wait_completion();
		}
	}

	last_angle = angle;

// Perform the rotation
	for(i = 0; i < cpus; i++)
	{
		engine[i]->perform_rotation(input, output);
	}

	for(i = 0; i < cpus; i++)
	{
		engine[i]->wait_completion();
	}


#define FILL_CENTER(type, components) \
{ \
	type *out_pixel = ((type*)output->get_row_ptr(center_y)) + center_x * components; \
	type *in_pixel = ((type*)input->get_row_ptr(center_y)) + center_x * components; \
 \
	out_pixel[0] = in_pixel[0]; \
	out_pixel[1] = in_pixel[1]; \
	out_pixel[2] = in_pixel[2]; \
	if(components == 4) out_pixel[3] = in_pixel[3]; \
}

// Fill center pixel
	switch(input->get_color_model())
	{
	case BC_RGB_FLOAT:
		FILL_CENTER(float, 3)
		break;
	case BC_RGBA_FLOAT:
		FILL_CENTER(float, 4)
		break;
	case BC_RGB888:
	case BC_YUV888:
		FILL_CENTER(unsigned char, 3)
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		FILL_CENTER(unsigned char, 4)
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		FILL_CENTER(uint16_t, 3)
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
	case BC_AYUV16161616:
		FILL_CENTER(uint16_t, 4)
		break;
	}
}


RotateEngine::RotateEngine(RotateFrame *plugin, int row1, int row2)
 : Thread()
{
	this->plugin = plugin;
	Thread::set_synchronous(1);
	do_matrix = do_rotation = 0;
	done = 0;
	this->row1 = row1;
	this->row2 = row2;
	input_lock = new Condition(0, "RotateEngine::input_lock");
	output_lock = new Condition(0, "RotateEngine::output_lock");
}

RotateEngine::~RotateEngine()
{
	if(!done)
	{
		done = 1;
		input_lock->unlock();
		join();
	}
	delete input_lock;
	delete output_lock;
}

void RotateEngine::generate_matrix()
{
	this->do_matrix = 1;
	input_lock->unlock();
}

void RotateEngine::perform_rotation(VFrame *input,
	VFrame *output)
{
	this->input = input;
	this->output = output;
	this->do_rotation = 1;
	input_lock->unlock();
}

void RotateEngine::wait_completion()
{
	output_lock->lock("RotateEngine::wait_completion");
}

void RotateEngine::coords_to_pixel(SourceCoord &float_pixel, float &input_y, float &input_x)
{
	if(input_y < 0) float_pixel.y = -1;
	else
	if(input_y >= plugin->input->get_h()) float_pixel.y = -1;
	else
	float_pixel.y = input_y;

	if(input_x < 0) float_pixel.x = -1;
	else
	if(input_x >= plugin->input->get_w()) float_pixel.x = -1;
	else
	float_pixel.x = input_x;
}


void RotateEngine::create_matrix()
{
// Polar coords of pixel
	double k, l, magnitude, angle, offset_angle, offset_angle2;
	double x_offset, y_offset;
	int i, j;
	int *int_row;
	SourceCoord *float_row;
	float input_x_f, input_y_f;

// The following is the result of pure trial and error.
// Fix the angles
// The source pixels are seen as if they were rotated counterclockwise so the sign is OK.
	offset_angle = -(plugin->angle - 90) / 360 * 2 * M_PI;
	offset_angle2 = -(plugin->angle - 270) / 360 * 2 * M_PI;

// Calculate an offset to add to all the pixels to compensate for the quadrant
	y_offset = plugin->input->get_h() / 2;
	x_offset = plugin->input->get_w() / 2;

	for(i = row1, l = row1 - plugin->input->get_h() / 2; i < row2; i++, l++)
	{
		int l_suare = (int)(l * l);

		float_row = plugin->float_rows[i];

		for(j = 0, k = -plugin->input->get_w() / 2; 
			j < plugin->input->get_w(); 
			j++, k++)
		{
// Add offsets to input
// Convert to polar coords
			magnitude = sqrt(SQR(k) + l_suare);

			if(l != 0)
				angle = atan(-k / l);
			else
			if(k < 0)
				angle = M_PI / 2;
			else
				angle = M_PI * 1.5;
// Rotate
			angle += (l < 0) ? offset_angle2 : offset_angle;

// Convert back to cartesian coords
			input_y_f = y_offset + magnitude * sin(angle);
			input_x_f = x_offset + magnitude * cos(angle);
			coords_to_pixel(float_row[j], input_y_f, input_x_f);
		}
	}
}

#define ROTATE_INTERPOLATE(type, components, black_chroma) \
{ \
	type zero_pixel[] = { 0, black_chroma, black_chroma, 0 }; \
	int i, j; \
	float k, l; \
	float x_fraction1, x_fraction2, y_fraction1, y_fraction2; \
	float fraction1, fraction2, fraction3, fraction4; \
	int x_pixel1, x_pixel2, y_pixel1, y_pixel2; \
	type *pixel1, *pixel2, *pixel3, *pixel4; \
 \
	for(i = row1, k = row1; i < row2; i++, k++) \
	{ \
		SourceCoord *float_row = plugin->float_rows[i]; \
		type *output_row = (type*)output->get_row_ptr(i); \
		for(j = 0, l = 0; j < width; j++, l++) \
		{ \
			if(float_row[j].x < 0 || float_row[j].y < 0) \
			{ \
				output_row[j * components + 0] = 0; \
				output_row[j * components + 1] = black_chroma; \
				output_row[j * components + 2] = black_chroma; \
				if(components == 4) output_row[j * components + 3] = 0; \
			} \
			else \
			{ \
/* Interpolate input pixels */ \
				x_pixel1 = (int)float_row[j].x; \
				x_pixel2 = (int)(float_row[j].x + 1); \
				y_pixel1 = (int)(float_row[j].y); \
				y_pixel2 = (int)(float_row[j].y + 1); \
				x_fraction1 = float_row[j].x - x_pixel1; \
				x_fraction2 = (float)x_pixel2 - float_row[j].x; \
				y_fraction1 = float_row[j].y - y_pixel1; \
				y_fraction2 = (float)y_pixel2 - float_row[j].y; \
/* By trial and error this fraction order seems to work. */ \
				fraction4 = x_fraction1 * y_fraction1; \
				fraction3 = x_fraction2 * y_fraction1; \
				fraction2 = x_fraction1 * y_fraction2; \
				fraction1 = x_fraction2 * y_fraction2; \
 \
				type *inrowp1 = (type*)input->get_row_ptr(y_pixel1); \
				type *inrowp2 = (type*)input->get_row_ptr(y_pixel2); \
 \
				pixel1 =                                                          &inrowp1[x_pixel1 * components]; \
				pixel2 = (x_pixel2 >= width)                       ? zero_pixel : &inrowp1[x_pixel2 * components]; \
				pixel3 = (y_pixel2 >= height)                      ? zero_pixel : &inrowp2[x_pixel1 * components]; \
				pixel4 = (x_pixel2 >= width || y_pixel2 >= height) ? zero_pixel : &inrowp2[x_pixel2 * components]; \
 \
				for(int m = 0; m < components; m++) \
				{ \
					output_row[j * components + m] = \
						(type)((pixel1[m] * fraction1) + \
							(pixel2[m] * fraction2) + \
							(pixel3[m] * fraction3) + \
							(pixel4[m] * fraction4)); \
				} \
			} \
		} \
	} \
}

void RotateEngine::perform_rotation()
{
	int width = input->get_w();
	int height = input->get_h();

	switch(input->get_color_model())
	{
	case BC_RGB888:
		ROTATE_INTERPOLATE(unsigned char, 3, 0x0);
		break;
	case BC_RGB_FLOAT:
		ROTATE_INTERPOLATE(float, 3, 0x0);
		break;
	case BC_YUV888:
		ROTATE_INTERPOLATE(unsigned char, 3, 0x80);
		break;
	case BC_RGBA8888:
		ROTATE_INTERPOLATE(unsigned char, 4, 0x0);
		break;
	case BC_RGBA_FLOAT:
		ROTATE_INTERPOLATE(float, 4, 0x0);
		break;
	case BC_YUVA8888:
		ROTATE_INTERPOLATE(unsigned char, 4, 0x80);
		break;
	case BC_RGB161616:
		ROTATE_INTERPOLATE(uint16_t, 3, 0x0);
		break;
	case BC_YUV161616:
		ROTATE_INTERPOLATE(uint16_t, 3, 0x8000);
		break;
	case BC_RGBA16161616:
		ROTATE_INTERPOLATE(uint16_t, 4, 0x0);
		break;
	case BC_YUVA16161616:
		ROTATE_INTERPOLATE(uint16_t, 4, 0x8000);
		break;
	}
}

void RotateEngine::run()
{
	while(!done)
	{
		input_lock->lock("RotateEngine::run");
		if(done) return;

		if(do_matrix)
		{
			create_matrix();
		}
		else
		if(do_rotation)
		{
			perform_rotation();
		}

		do_matrix = 0;
		do_rotation = 0;
		output_lock->unlock();
	}
}
