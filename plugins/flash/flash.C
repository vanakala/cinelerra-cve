
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

#include "flash.h"
#include "edl.inc"
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
		type *in_row = (type*)incoming->get_rows()[i]; \
		type *out_row = (type*)outgoing->get_rows()[i]; \
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
	ptstime half = total_len_pts / 2;
	ptstime pts = half - fabs(source_pts - half);
	float fraction = pts / half;
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
	}
}
