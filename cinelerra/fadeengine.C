
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

#include "fadeengine.h"
#include "vframe.h"

#include <stdint.h>









FadeUnit::FadeUnit(FadeEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

FadeUnit::~FadeUnit()
{
}


#define APPLY_FADE(equivalent, \
	input_rows,  \
	output_rows,  \
	max,  \
	type,  \
	temp_type,  \
	chroma_zero,  \
	components) \
{ \
	temp_type opacity = (temp_type)(alpha * max); \
	temp_type transparency = (temp_type)(max - opacity); \
	temp_type product = (temp_type) (chroma_zero * transparency); \
 \
	for(int i = row1; i < row2; i++) \
	{ \
		type *in_row = (type*)input_rows[i]; \
		type *out_row = (type*)output_rows[i]; \
 \
		for(int j = 0; j < width; j++, out_row += components, in_row += components) \
		{ \
			if(components == 3) \
			{ \
				out_row[0] =  \
					(type)((temp_type)in_row[0] * opacity / max); \
				out_row[1] =  \
					(type)(((temp_type)in_row[1] * opacity +  \
						product) / max); \
				out_row[2] =  \
					(type)(((temp_type)in_row[2] * opacity +  \
						product) / max); \
			} \
			else \
			{ \
				if(!equivalent) \
				{ \
					out_row[0] = in_row[0]; \
					out_row[1] = in_row[1]; \
					out_row[2] = in_row[2]; \
				} \
 \
				if(in_row[3] == max) \
					out_row[3] = opacity; \
				else \
				out_row[3] =  \
					(type)((temp_type)in_row[3] * opacity / max); \
			} \
		} \
	} \
}



void FadeUnit::process_package(LoadPackage *package)
{
	FadePackage *pkg = (FadePackage*)package;
	VFrame *output = engine->output;
	VFrame *input = engine->input;
	float alpha = engine->alpha;
	int row1 = pkg->out_row1;
	int row2 = pkg->out_row2;
	unsigned char **in_rows = input->get_rows();
	unsigned char **out_rows = output->get_rows();
	int width = input->get_w();

	if(input->get_rows()[0] == output->get_rows()[0])
	{
		switch(input->get_color_model())
		{
			case BC_RGB888:
				APPLY_FADE(1, out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x0, 3);
				break;
			case BC_RGBA8888:
				APPLY_FADE(1, out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x0, 4);
				break;
			case BC_RGB_FLOAT:
				APPLY_FADE(1, out_rows, in_rows, 1.0, float, float, 0x0, 3);
				break;
			case BC_RGBA_FLOAT:
				APPLY_FADE(1, out_rows, in_rows, 1.0, float, float, 0x0, 4);
				break;
			case BC_RGB161616:
				APPLY_FADE(1, out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x0, 3);
				break;
			case BC_RGBA16161616:
				APPLY_FADE(1, out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x0, 4);
				break;
			case BC_YUV888:
				APPLY_FADE(1, out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x80, 3);
				break;
			case BC_YUVA8888:
				APPLY_FADE(1, out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x80, 4);
				break;
			case BC_YUV161616:
				APPLY_FADE(1, out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x8000, 3);
				break;
			case BC_YUVA16161616:
				APPLY_FADE(1, out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x8000, 4);
				break;
		}
	}
	else
	{
		switch(input->get_color_model())
		{
			case BC_RGB888:
				APPLY_FADE(0, out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x0, 3);
				break;
			case BC_RGBA8888:
				APPLY_FADE(0, out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x0, 4);
				break;
			case BC_RGB_FLOAT:
				APPLY_FADE(0, out_rows, in_rows, 1.0, float, float, 0x0, 3);
				break;
			case BC_RGBA_FLOAT:
				APPLY_FADE(0, out_rows, in_rows, 1.0, float, float, 0x0, 4);
				break;
			case BC_RGB161616:
				APPLY_FADE(0, out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x0, 3);
				break;
			case BC_RGBA16161616:
				APPLY_FADE(0, out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x0, 4);
				break;
			case BC_YUV888:
				APPLY_FADE(0, out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x80, 3);
				break;
			case BC_YUVA8888:
				APPLY_FADE(0, out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x80, 4);
				break;
			case BC_YUV161616:
				APPLY_FADE(0, out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x8000, 3);
				break;
			case BC_YUVA16161616:
				APPLY_FADE(0, out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x8000, 4);
				break;
		}
	}
}









FadeEngine::FadeEngine(int cpus)
 : LoadServer(cpus, cpus)
{
}

FadeEngine::~FadeEngine()
{
}

void FadeEngine::do_fade(VFrame *output, VFrame *input, float alpha)
{
	this->output = output;
	this->input = input;
	this->alpha = alpha;
	
// Sanity
	if(alpha == 1)
		output->copy_from(input);
	else
		process_packages();
}


void FadeEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		FadePackage *package = (FadePackage*)get_package(i);
		package->out_row1 = input->get_h() * i / get_total_packages();
		package->out_row2 = input->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* FadeEngine::new_client()
{
	return new FadeUnit(this);
}

LoadPackage* FadeEngine::new_package()
{
	return new FadePackage;
}


FadePackage::FadePackage()
{
}

