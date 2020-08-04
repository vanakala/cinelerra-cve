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

void FlashMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	ptstime length = get_length();
	int cmodel = incoming->get_color_model();

	if(length < EPSILON)
		return;

	ptstime half = length / 2;
	ptstime pts = half - fabs(source_pts - half);
	double fraction = pts / half;
	int is_before = source_pts < half;
	int w = incoming->get_w();
	int h = incoming->get_h();
	int foreground = round(fraction * 0xffff);
	int transparency = 0xffff - foreground;
	int chroma_foreground = foreground * 0x8000 / 0xffff;


	switch(cmodel)
	{
	case BC_RGBA16161616:
		if(is_before)
		{
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(i);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*out_row = foreground + transparency * *out_row / 0xffff;
					out_row++;
					*out_row = foreground + transparency * *out_row / 0xffff;
					out_row++; \
					*out_row = foreground + transparency * *out_row / 0xffff;
					out_row++;
					*out_row = foreground + transparency * *out_row / 0xffff;
					out_row++;
				}
			}
		}
		else
		{
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(i);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					*out_row = foreground + transparency * *in_row / 0xffff;
					out_row++;
					in_row++;
					*out_row = foreground + transparency * *in_row / 0xffff;
					out_row++;
					in_row++;
					*out_row =  foreground + transparency * *in_row / 0xffff;
					out_row++;
					in_row++;
					*out_row = foreground + transparency * *in_row / 0xffff;
					out_row++;
					in_row++;
				}
			}
		}
		break;

	case BC_AYUV16161616:
		if(is_before)
		{
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(i);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(i);

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
		}
		else
		{
			for(int i = 0; i < h; i++)
			{
				uint16_t *in_row = (uint16_t*)incoming->get_row_ptr(i);
				uint16_t *out_row = (uint16_t*)outgoing->get_row_ptr(i);

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
		break;

	default:
		unsupported(cmodel);
		break;
	}
}
