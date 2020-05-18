// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "flash.h"
#include "picon_png.h"
#include "vframe.h"
#include <stdint.h>


REGISTER_PLUGIN


FlashMain::FlashMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

FlashMain::~FlashMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

#define FLASH(type, temp_type, components, max, chroma_offset) \
{ \
	float round_factor = (sizeof(type) < 4) ? 0.5 : 0; \
	temp_type foreground = (temp_type)(fraction * max + round_factor); \
	temp_type chroma_foreground = foreground; \
	if(chroma_offset) chroma_foreground = foreground * chroma_offset / max; \
	temp_type transparency = max - foreground; \
 \
	for(int i = 0; i < h; i++) \
	{ \
		type *in_row = (type*)incoming->get_row_ptr(i); \
		type *out_row = (type*)outgoing->get_row_ptr(i); \
		if(is_before) \
		{ \
			for(int j = 0; j < w; j++) \
			{ \
				*out_row = foreground + transparency * *out_row / max; \
				out_row++; \
				*out_row = chroma_foreground + transparency * *out_row / max; \
				out_row++; \
				*out_row = chroma_foreground + transparency * *out_row / max; \
				out_row++; \
				if(components == 4) \
				{ \
					*out_row = foreground + transparency * *out_row / max; \
					out_row++; \
				} \
			} \
		} \
		else \
		{ \
			for(int j = 0; j < w; j++) \
			{ \
				*out_row = foreground + transparency * *in_row / max; \
				out_row++; \
				in_row++; \
				*out_row = chroma_foreground + transparency * *in_row / max; \
				out_row++; \
				in_row++; \
				*out_row = chroma_foreground + transparency * *in_row / max; \
				out_row++; \
				in_row++; \
				if(components == 4) \
				{ \
					*out_row = foreground + transparency * *in_row / max; \
					out_row++; \
					in_row++; \
				} \
			} \
		} \
	} \
}

void FlashMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	if(total_len_pts < EPSILON)
		return;

	ptstime half = total_len_pts / 2;
	ptstime pts = half - fabs(source_pts - half);
	double fraction = pts / half;
	int is_before = source_pts < half;
	int w = incoming->get_w();
	int h = incoming->get_h();

	switch(incoming->get_color_model())
	{
	case BC_RGB888:
		FLASH(unsigned char, int, 3, 0xff, 0x0);
		break;
	case BC_RGB_FLOAT:
		FLASH(float, float, 3, 1.0, 0x0);
		break;
	case BC_RGBA8888:
		FLASH(unsigned char, int, 4, 0xff, 0x0);
		break;
	case BC_RGBA_FLOAT:
		FLASH(float, float, 4, 1.0, 0x0);
		break;
	case BC_YUV888:
		FLASH(unsigned char, int, 3, 0xff, 0x80);
		break;
	case BC_YUVA8888:
		FLASH(unsigned char, int, 4, 0xff, 0x80);
		break;
	case BC_RGB161616:
		FLASH(uint16_t, int, 3, 0xffff, 0x0);
		break;
	case BC_RGBA16161616:
		FLASH(uint16_t, int, 4, 0xffff, 0x0);
		break;
	case BC_YUV161616:
		FLASH(uint16_t, int, 3, 0xffff, 0x8000);
		break;
	case BC_YUVA16161616:
		FLASH(uint16_t, int, 4, 0xffff, 0x8000);
		break;

	case BC_AYUV16161616:
		{
			int foreground = (int)(fraction * 0xffff);
			int chroma_foreground = foreground * 0x8000 / 0xffff;
			int transparency = 0xffff - foreground;

			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(i);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(i);
				if(is_before)
				{
					for(int j = 0; j < w; j++)
					{
						*out_row = foreground + transparency * *out_row / 0xffff;
						out_row++;
						*out_row = foreground + transparency * *out_row / 0xffff;
						out_row++;
						*out_row = chroma_foreground + transparency * *out_row / 0xffff;
						out_row++;
						*out_row = chroma_foreground + transparency * *out_row / 0xffff;
						out_row++;
					}
				}
				else
				{
					for(int j = 0; j < w; j++)
					{
						*out_row = foreground + transparency * *in_row / 0xffff;
						out_row++;
						in_row++;
						*out_row = foreground + transparency * *in_row / 0xffff;
						out_row++;
						in_row++;
						*out_row = chroma_foreground + transparency * *in_row / 0xffff;
						out_row++;
						in_row++;
						*out_row = chroma_foreground + transparency * *in_row / 0xffff;
						out_row++;
						in_row++;
					}
				}
			}
		}
		break;
	}
}
