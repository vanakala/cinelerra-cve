
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

#include "bcsignals.h"
#include "clip.h"
#include "fadeengine.h"
#include "vframe.h"

#include <stdint.h>

FadeUnit::FadeUnit(FadeEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

#define APPLY_FADE(input_rows, \
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
	if(fade_colors) \
	{ \
		if(alpha_pos) \
		{ \
			for(int i = row1; i < row2; i++) \
			{ \
				type *row = (type*)frame->get_row_ptr(i); \
 \
				for(int j = 0; j < width; j++, row += components) \
				{ \
					row[0] =  \
						(type)((temp_type)row[0] * opacity / max); \
					row[1] =  \
						(type)(((temp_type)row[1] * opacity +  \
							product) / max); \
					row[2] =  \
						(type)(((temp_type)row[2] * opacity +  \
							product) / max); \
					if(components == 4) \
					{ \
						if(row[3] == max) \
							row[3] = opacity; \
						else \
							row[3] = \
								(type)((temp_type)row[3] * opacity / max); \
					} \
				} \
			} \
		} \
		else \
		{ \
			for(int i = row1; i < row2; i++) \
			{ \
				type *row = (type*)frame->get_row_ptr(i); \
 \
				for(int j = 0; j < width; j++, row += components) \
				{ \
					if(row[0] == max) \
						row[0] = opacity; \
					else \
						row[0] = \
							(type)((temp_type)row[0] * opacity / max); \
					row[1] =  \
						(type)((temp_type)row[1] * opacity / max); \
					row[2] =  \
						(type)(((temp_type)row[2] * opacity +  \
							product) / max); \
					row[3] =  \
						(type)(((temp_type)row[3] * opacity +  \
							product) / max); \
				} \
			} \
		} \
	} \
	else \
	{ \
		for(int i = row1; i < row2; i++) \
		{ \
			type *row = (type*)frame->get_row_ptr(i); \
 \
			for(int j = 0; j < width; j++, row += components) \
			{ \
				if(components == 3) \
				{ \
					row[0] =  \
						(type)((temp_type)row[0] * opacity / max); \
					row[1] =  \
						(type)(((temp_type)row[1] * opacity +  \
							product) / max); \
					row[2] =  \
						(type)(((temp_type)row[2] * opacity +  \
							product) / max); \
				} \
				else \
				{ \
					if(row[alpha_pos] == max) \
						row[alpha_pos] = opacity; \
					else \
						row[alpha_pos] = \
							(type)((temp_type)row[alpha_pos] * opacity / max); \
				} \
			} \
		} \
	} \
}

void FadeUnit::process_package(LoadPackage *package)
{
	FadePackage *pkg = (FadePackage*)package;
	VFrame *frame = engine->frame;
	float alpha = engine->alpha;
	int fade_colors = engine->fade_colors;
	int row1 = pkg->out_row1;
	int row2 = pkg->out_row2;
	int width = frame->get_w();
	int alpha_pos = 3;

	switch(frame->get_color_model())
	{
	case BC_RGB888:
		APPLY_FADE(out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x0, 3);
		break;
	case BC_RGBA8888:
		APPLY_FADE(out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x0, 4);
		break;
	case BC_RGB_FLOAT:
		APPLY_FADE(out_rows, in_rows, 1.0, float, float, 0x0, 3);
		break;
	case BC_RGBA_FLOAT:
		APPLY_FADE(out_rows, in_rows, 1.0, float, float, 0x0, 4);
		break;
	case BC_RGB161616:
		APPLY_FADE(out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x0, 3);
		break;
	case BC_RGBA16161616:
		APPLY_FADE(out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x0, 4);
		break;
	case BC_YUV888:
		APPLY_FADE(out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x80, 3);
		break;
	case BC_YUVA8888:
		APPLY_FADE(out_rows, in_rows, 0xff, unsigned char, uint16_t, 0x80, 4);
		break;
	case BC_YUV161616:
		APPLY_FADE(out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x8000, 3);
		break;
	case BC_YUVA16161616:
		APPLY_FADE(out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x8000, 4);
		break;
	case BC_AYUV16161616:
		alpha_pos = 0;
		APPLY_FADE(out_rows, in_rows, 0xffff, uint16_t, uint32_t, 0x8000, 4);
		break;
	}
}


FadeEngine::FadeEngine(int cpus)
 : LoadServer(cpus, cpus)
{
}

void FadeEngine::do_fade(VFrame *frame, double alpha, int fade_colors)
{
	this->frame = frame;
	this->alpha = alpha;
	this->fade_colors;

// Sanity
	if(!EQUIV(alpha, 1.0))
		process_packages();
}

void FadeEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		FadePackage *package = (FadePackage*)get_package(i);
		package->out_row1 = frame->get_h() * i / get_total_packages();
		package->out_row2 = frame->get_h() * (i + 1) / get_total_packages();
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
