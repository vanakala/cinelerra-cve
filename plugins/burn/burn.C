
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

#include "clip.h"
#include "colormodels.inc"
#include "colorspaces.h"
#include "effecttv.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "burn.h"
#include "burnwindow.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN


BurnConfig::BurnConfig()
{
	threshold = 50;
	decay = 15;
	recycle = 1.0;
}

BurnMain::BurnMain(PluginServer *server)
 : PluginVClient(server)
{
	input_ptr = 0;
	output_ptr = 0;
	burn_server = 0;
	buffer = 0;
	effecttv = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

BurnMain::~BurnMain()
{
	if(buffer) delete [] buffer;
	if(burn_server) delete burn_server;
	if(effecttv) delete effecttv;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

int BurnMain::load_configuration()
{
}

#define MAXCOLOR 120

void BurnMain::HSItoRGB(double H, 
	double S, 
	double I, 
	int *r, 
	int *g, 
	int *b,
	int color_model)
{
	double T, Rv, Gv, Bv;

	T = H;
	Rv = 1 + S * sin(T - 2 * M_PI / 3);
	Gv = 1 + S * sin(T);
	Bv = 1 + S * sin(T + 2  * M_PI / 3);
	T = 255.999 * I / 2;

	*r = (int)CLIP(Rv * T, 0, 255);
	*g = (int)CLIP(Gv * T, 0, 255);
	*b = (int)CLIP(Bv * T, 0, 255);
}


void BurnMain::make_palette(int color_model)
{
	int i, r, g, b;

	for(i = 0; i < MAXCOLOR; i++)
	{
		HSItoRGB(4.6 - 1.5 * i / MAXCOLOR, 
			(double)i / MAXCOLOR, 
			(double)i / MAXCOLOR,  
			&r, 
			&g, 
			&b, 
			color_model);
		palette[0][i] = r;
		palette[1][i] = g;
		palette[2][i] = b;
	}


	for(i = MAXCOLOR; i < 256; i++)
	{
		if(r < 255) r++;
		if(r < 255) r++;
		if(r < 255) r++;
		if(g < 255) g++;
		if(g < 255) g++;
		if(b < 255) b++;
		if(b < 255) b++;
		palette[0][i] = r;
		palette[1][i] = g;
		palette[2][i] = b;
	}
}

void BurnMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	this->input_ptr = input_ptr;
	this->output_ptr = output_ptr;

	load_configuration();

	if(!burn_server)
	{
		effecttv = new EffectTV(input_ptr->get_w(), input_ptr->get_h());
		buffer = (unsigned char *)new unsigned char[input_ptr->get_w() * input_ptr->get_h()];
		make_palette(input_ptr->get_color_model());

		effecttv->image_set_threshold_y(config.threshold);
		total = 0;

		burn_server = new BurnServer(this, 1, 1);
	}

	if(total == 0)
	{
		bzero(buffer, input_ptr->get_w() * input_ptr->get_h());
		effecttv->image_bgset_y(input_ptr);
	}
	burn_server->process_packages();

	total++;
}


BurnServer::BurnServer(BurnMain *plugin, int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}


LoadClient* BurnServer::new_client() 
{
	return new BurnClient(this);
}


LoadPackage* BurnServer::new_package() 
{ 
	return new BurnPackage; 
}

void BurnServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		BurnPackage *package = (BurnPackage*)get_package(i);
		package->row1 = plugin->input_ptr->get_h() * i / get_total_packages();
		package->row2 = plugin->input_ptr->get_h() * (i + 1) / get_total_packages();
	}
}


BurnClient::BurnClient(BurnServer *server)
 : LoadClient(server)
{
	this->plugin = server->plugin;
}

#define BURN(type, components, is_yuv) \
{ \
	i = 1; \
	type *row = (type*)input_row; \
	for(y = 0; y < height; y++)  \
	{ \
		for(x = 1; x < width - 1; x++)  \
		{ \
			if(sizeof(type) == 4) \
			{ \
				a1 = (int)(row[i * components] * 0xff); \
				a2 = (int)(row[i * components + 1] * 0xff); \
				a3 = (int)(row[i * components + 2] * 0xff); \
				CLAMP(a1, 0, 0xff); \
				CLAMP(a2, 0, 0xff); \
				CLAMP(a3, 0, 0xff); \
				b1 = plugin->palette[0][plugin->buffer[i]]; \
				b2 = plugin->palette[1][plugin->buffer[i]]; \
				b3 = plugin->palette[2][plugin->buffer[i]]; \
				a1 += b1; \
				a2 += b2; \
				a3 += b3; \
				b1 = a1 & 0x100; \
				b2 = a2 & 0x100; \
				b3 = a3 & 0x100; \
				row[i * components] = (type)(a1 | (b1 - (b1 >> 8))) / 0xff; \
				row[i * components + 1] = (type)(a2 | (b2 - (b2 >> 8))) / 0xff; \
				row[i * components + 2] = (type)(a3 | (b3 - (b3 >> 8))) / 0xff; \
			} \
			else \
			if(sizeof(type) == 2) \
			{ \
				a1 = ((int)row[i * components + 0]) >> 8; \
				a2 = ((int)row[i * components + 1]) >> 8; \
				a3 = ((int)row[i * components + 2]) >> 8; \
				b1 = plugin->palette[0][plugin->buffer[i]]; \
				b2 = plugin->palette[1][plugin->buffer[i]]; \
				b3 = plugin->palette[2][plugin->buffer[i]]; \
				if(is_yuv) ColorSpaces::yuv_to_rgb_8(a1, a2, a3); \
				a1 += b1; \
				a2 += b2; \
				a3 += b3; \
				b1 = a1 & 0x100; \
				b2 = a2 & 0x100; \
				b3 = a3 & 0x100; \
				a1 = (a1 | (b1 - (b1 >> 8))); \
				a2 = (a2 | (b2 - (b2 >> 8))); \
				a3 = (a3 | (b3 - (b3 >> 8))); \
				if(is_yuv) \
				{ \
					CLAMP(a1, 0, 0xff); \
					CLAMP(a2, 0, 0xff); \
					CLAMP(a3, 0, 0xff); \
					ColorSpaces::rgb_to_yuv_8(a1, a2, a3); \
				} \
				row[i * components + 0] = a1 | (a1 << 8); \
				row[i * components + 1] = a2 | (a2 << 8); \
				row[i * components + 2] = a3 | (a3 << 8); \
			} \
			else \
			{ \
				a1 = (int)row[i * components + 0]; \
				a2 = (int)row[i * components + 1]; \
				a3 = (int)row[i * components + 2]; \
				b1 = plugin->palette[0][plugin->buffer[i]]; \
				b2 = plugin->palette[1][plugin->buffer[i]]; \
				b3 = plugin->palette[2][plugin->buffer[i]]; \
				if(is_yuv) ColorSpaces::yuv_to_rgb_8(a1, a2, a3); \
				a1 += b1; \
				a2 += b2; \
				a3 += b3; \
				b1 = a1 & 0x100; \
				b2 = a2 & 0x100; \
				b3 = a3 & 0x100; \
				a1 = (a1 | (b1 - (b1 >> 8))); \
				a2 = (a2 | (b2 - (b2 >> 8))); \
				a3 = (a3 | (b3 - (b3 >> 8))); \
				if(is_yuv) \
				{ \
					CLAMP(a1, 0, 0xff); \
					CLAMP(a2, 0, 0xff); \
					CLAMP(a3, 0, 0xff); \
					ColorSpaces::rgb_to_yuv_8(a1, a2, a3); \
				} \
				row[i * components + 0] = a1; \
				row[i * components + 1] = a2; \
				row[i * components + 2] = a3; \
			} \
			i++; \
		} \
		i += 2; \
	} \
}

void BurnClient::process_package(LoadPackage *package)
{
	BurnPackage *local_package = (BurnPackage*)package;
	unsigned char *input_row = plugin->input_ptr->get_row_ptr(local_package->row1);
	unsigned char *output_row = plugin->output_ptr->get_row_ptr(local_package->row1);
	int width = plugin->input_ptr->get_w();
	int height = local_package->row2 - local_package->row1;
	unsigned char *diff;
	int pitch = width * plugin->input_ptr->get_bytes_per_pixel();
	int i, x, y;
	unsigned int v, w;
	int a1, b1, c1;
	int a2, b2, c2;
	int a3, b3, c3;

	diff = plugin->effecttv->image_bgsubtract_y(input_row,
		plugin->input_ptr->get_color_model(),
		plugin->input_ptr->get_bytes_per_line());

	for(x = 1; x < width - 1; x++)
	{
		v = 0;
		for(y = 0; y < height - 1; y++)
		{
			w = diff[y * width + x];
			plugin->buffer[y * width + x] |= v ^ w;
			v = w;
		}
	}

	for(x = 1; x < width - 1; x++) 
	{
		w = 0;
		i = width + x;

		for(y = 1; y < height; y++) 
		{
			v = plugin->buffer[i];

			if(v < plugin->config.decay)
				plugin->buffer[i - width] = 0;
			else
				plugin->buffer[i - width + EffectTV::fastrand() % 3 - 1] = 
					v - (EffectTV::fastrand() & plugin->config.decay);

			i += width;
		}
	}

	switch(plugin->input_ptr->get_color_model())
	{
	case BC_RGB888:
		BURN(uint8_t, 3, 0);
		break;
	case BC_YUV888:
		BURN(uint8_t, 3, 1);
		break;

	case BC_RGB_FLOAT:
		BURN(float, 3, 0);
		break;
	case BC_RGBA_FLOAT:
		BURN(float, 4, 0);
		break;

	case BC_RGBA8888:
		BURN(uint8_t, 4, 0);
		break;
	case BC_YUVA8888:
		BURN(uint8_t, 4, 1);
		break;

	case BC_RGB161616:
		BURN(uint16_t, 3, 0);
		break;
	case BC_YUV161616:
		BURN(uint16_t, 3, 1);
		break;

	case BC_RGBA16161616:
		BURN(uint16_t, 4, 0);
		break;
	case BC_YUVA16161616:
		BURN(uint16_t, 4, 1);
		break;
	case BC_AYUV16161616:
		{
			i = 1;
			uint16_t *row = (uint16_t*)input_row;
			for(y = 0; y < height; y++)
			{
				for(x = 1; x < width - 1; x++)
				{
					a1 = ((int)row[i * 4 + 1]) >> 8;
					a2 = ((int)row[i * 4 + 2]) >> 8;
					a3 = ((int)row[i * 4 + 3]) >> 8;
					b1 = plugin->palette[0][plugin->buffer[i]];
					b2 = plugin->palette[1][plugin->buffer[i]];
					b3 = plugin->palette[2][plugin->buffer[i]];
					ColorSpaces::yuv_to_rgb_8(a1, a2, a3);
					a1 += b1;
					a2 += b2;
					a3 += b3;
					b1 = a1 & 0x100;
					b2 = a2 & 0x100;
					b3 = a3 & 0x100;
					a1 = (a1 | (b1 - (b1 >> 8)));
					a2 = (a2 | (b2 - (b2 >> 8)));
					a3 = (a3 | (b3 - (b3 >> 8)));
					CLAMP(a1, 0, 0xff);
					CLAMP(a2, 0, 0xff);
					CLAMP(a3, 0, 0xff);
					ColorSpaces::rgb_to_yuv_8(a1, a2, a3);
					row[i * 4 + 1] = a1 | (a1 << 8);
					row[i * 4 + 2] = a2 | (a2 << 8);
					row[i * 4 + 3] = a3 | (a3 << 8);
					i++;
				}
				i += 2;
			}
		}
		break;
	}

}


BurnPackage::BurnPackage()
{
}
