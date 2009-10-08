
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
#include "colormodels.h"
#include "filexml.h"
#include "picon_png.h"
#include "dot.h"
#include "dotwindow.h"
#include "effecttv.h"
#include "language.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>






REGISTER_PLUGIN(DotMain)







DotConfig::DotConfig()
{
	dot_depth = 5;
	dot_size = 8;
}





DotMain::DotMain(PluginServer *server)
 : PluginVClient(server)
{
	pattern = 0;
	sampx = 0;
	sampy = 0;
	effecttv = 0;
	need_reconfigure = 1;
	PLUGIN_CONSTRUCTOR_MACRO
}

DotMain::~DotMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	
	if(pattern) delete [] pattern;
	if(sampx) delete [] sampx;
	if(sampy) delete [] sampy;
	if(effecttv)
	{
		delete dot_server;
		delete effecttv;
	}
}

char* DotMain::plugin_title() { return N_("DotTV"); }
int DotMain::is_realtime() { return 1; }

NEW_PICON_MACRO(DotMain)

SHOW_GUI_MACRO(DotMain, DotThread)

SET_STRING_MACRO(DotMain)

RAISE_WINDOW_MACRO(DotMain)


int DotMain::load_defaults()
{
	return 0;
}

int DotMain::save_defaults()
{
	return 0;
}

void DotMain::load_configuration()
{
}


void DotMain::save_data(KeyFrame *keyframe)
{
}

void DotMain::read_data(KeyFrame *keyframe)
{
}

void DotMain::make_pattern()
{
	int i, x, y, c;
	int u, v;
	double p, q, r;
	uint32_t *pat;

	for(i = 0; i < config.dot_max(); i++) 
	{
/* Generated pattern is a quadrant of a disk. */
		pat = pattern + (i + 1) * dot_hsize * dot_hsize - 1;

//		r = (0.2 * i / config.dot_max() + 0.8) * dot_hsize;
//		r = r * r;
		r = ((double)i / config.dot_max()) * dot_hsize;
		r *= 5;
//printf("make_pattern %f\n", r);

		for(y = 0; y < dot_hsize; y++) 
		{
			for(x = 0; x < dot_hsize; x++) 
			{
				c = 0;
				for(u = 0; u < 4; u++) 
				{
					p = (double)u / 4.0 + y;
					p = p * p;

					for(v = 0; v < 4; v++) 
					{
						q = (double)v / 4.0 + x;

						if(p + q * q < r) 
						{
							c++;
						}
					}
				}


				c = (c > 15) ? 15 : c;
//printf("DotMain::make_pattern %d\n", c);
				*pat-- = (c << 20) | (c << 12) | (c << 4);
/* The upper left part of a disk is needed, but generated pattern is a bottom
 * right part. So I spin the pattern. */
			}
		}
	}
}

void DotMain::init_sampxy_table()
{
	int i, j;

// Need aspect ratio
	j = dot_hsize;
	for(i = 0; i < dots_width; i++) 
	{
		sampx[i] = j;
		j += dot_size;
	}
	j = dot_hsize;
	for(i = 0; i < dots_height; i++) 
	{
		sampy[i] = j;
		j += dot_size;
	}
}


void DotMain::reconfigure()
{
	if(!effecttv)
	{
		effecttv = new EffectTV(input_ptr->get_w(), input_ptr->get_h());
		dot_server = new DotServer(this, 1, 1);
	}

	dot_size = config.dot_size;
	dot_size = dot_size & 0xfe;
	dot_hsize = dot_size / 2;
	dots_width = input_ptr->get_w() / dot_size;
	dots_height = input_ptr->get_h() / dot_size;
	pattern = new uint32_t[config.dot_max() * 
		dot_hsize * 
		dot_hsize];
	sampx = new int[input_ptr->get_w()];
	sampy = new int[input_ptr->get_h()];
	
	make_pattern();
	
	init_sampxy_table();
	
	
	need_reconfigure = 0;
}



int DotMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	this->input_ptr = input_ptr;
	this->output_ptr = output_ptr;
	load_configuration();
	if(need_reconfigure) reconfigure();


	dot_server->process_packages();

	return 0;
}



DotServer::DotServer(DotMain *plugin, int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}


LoadClient* DotServer::new_client() 
{
	return new DotClient(this);
}




LoadPackage* DotServer::new_package() 
{ 
	return new DotPackage; 
}



void DotServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		DotPackage *package = (DotPackage*)get_package(i);
		package->row1 = plugin->input_ptr->get_h() * i / get_total_packages();
		package->row2 = plugin->input_ptr->get_h() * (i + 1) / get_total_packages();
	}
}








DotClient::DotClient(DotServer *server)
 : LoadClient(server)
{
	this->plugin = server->plugin;
}

#define COPY_PIXEL(type, components, output, pattern, chroma_offset) \
{ \
	if(chroma_offset) \
	{ \
		if(sizeof(type) == 2) \
		{ \
			output[0] = (pattern & 0xff0000) >> 8; \
			output[1] = chroma_offset; \
			output[2] = chroma_offset; \
			if(components > 3) output[3] = 0xffff; \
		} \
		else \
		{ \
			output[0] = (pattern & 0xff0000) >> 16; \
			output[1] = chroma_offset; \
			output[2] = chroma_offset; \
			if(components > 3) output[3] = 0xff; \
		} \
	} \
	else \
	{ \
		if(sizeof(type) == 4) \
		{ \
			output[0] = (type)(pattern & 0xff0000) / 0xff0000; \
			output[1] = (type)(pattern & 0xff00) / 0xff00; \
			output[2] = (type)(pattern & 0xff) / 0xff; \
			if(components > 3) output[3] = 1; \
		} \
		else \
		if(sizeof(type) == 2) \
		{ \
			output[0] = (pattern & 0xff0000) >> 8; \
			output[1] = (pattern & 0xff00); \
			output[2] = (pattern & 0xff) << 8; \
			if(components > 3) output[3] = 0xffff; \
		} \
		else \
		{ \
			output[0] = (pattern & 0xff0000) >> 16; \
			output[1] = (pattern & 0xff00) >> 8; \
			output[2] = (pattern & 0xff); \
			if(components > 3) output[3] = 0xff; \
		} \
	} \
}

#define DRAW_DOT(type, components, chroma_offset) \
{ \
	int x, y; \
	uint32_t *pat; \
	type *output; \
	int y_total = 0; \
 \
	c = (c >> (8 - plugin->config.dot_depth)); \
	pat = plugin->pattern + c * plugin->dot_hsize * plugin->dot_hsize; \
	output = ((type**)output_rows)[0] + \
		yy *  \
		plugin->dot_size *  \
		plugin->input_ptr->get_w() * \
		components + \
		xx *  \
		plugin->dot_size * \
		components; \
 \
	for(y = 0; y < plugin->dot_hsize && y_total < plugin->input_ptr->get_h(); y++)  \
	{ \
		for(x = 0; x < plugin->dot_hsize; x++)  \
		{ \
			COPY_PIXEL(type, components, output, *pat, chroma_offset); \
			output += components; \
			pat++; \
		} \
 \
		pat -= 2; \
 \
		for(x = 0; x < plugin->dot_hsize - 1; x++)  \
		{ \
			COPY_PIXEL(type, components, output, *pat, chroma_offset); \
			output += components; \
			pat--; \
		} \
 \
 		COPY_PIXEL(type, components, output, 0x00000000, chroma_offset); \
 \
		output += components * (plugin->input_ptr->get_w() - plugin->dot_size + 1); \
		pat += plugin->dot_hsize + 1; \
		y_total++; \
	} \
 \
	pat -= plugin->dot_hsize * 2; \
 \
	for(y = 0; y < plugin->dot_hsize && y_total < plugin->input_ptr->get_h(); y++)  \
	{ \
		if(y < plugin->dot_hsize - 1) \
		{ \
			for(x = 0; x < plugin->dot_hsize; x++)  \
			{ \
				COPY_PIXEL(type, components, output, *pat, chroma_offset); \
				output += components; \
				pat++; \
			} \
 \
			pat -= 2; \
 \
			for(x = 0; x < plugin->dot_hsize - 1; x++)  \
			{ \
				COPY_PIXEL(type, components, output, *pat, chroma_offset); \
				output += components; \
				pat--; \
			} \
 \
 			COPY_PIXEL(type, components, output, 0x00000000, chroma_offset); \
 \
			output += components * (plugin->input_ptr->get_w() - plugin->dot_size + 1); \
			pat += -plugin->dot_hsize + 1; \
		} \
		else \
		{ \
			for(x = 0; x < plugin->dot_hsize * 2; x++) \
			{ \
				COPY_PIXEL(type, components, output, 0x00000000, chroma_offset); \
				output += components; \
			} \
		} \
 \
 		y_total++; \
 \
	} \
}

void DotClient::draw_dot(int xx, 
	int yy, 
	unsigned char c, 
	unsigned char **output_rows,
	int color_model)
{
	switch(plugin->input_ptr->get_color_model())
	{
		case BC_RGB888:
			DRAW_DOT(uint8_t, 3, 0x0);
			break;

		case BC_RGB_FLOAT:
			DRAW_DOT(float, 3, 0x0);
			break;

		case BC_YUV888:
			DRAW_DOT(uint8_t, 3, 0x80);
			break;

		case BC_RGBA_FLOAT:
			DRAW_DOT(float, 4, 0x0);
			break;

		case BC_RGBA8888:
			DRAW_DOT(uint8_t, 4, 0x0);
			break;

		case BC_YUVA8888:
			DRAW_DOT(uint8_t, 4, 0x80);
			break;

		case BC_RGB161616:
			DRAW_DOT(uint16_t, 3, 0x0);
			break;

		case BC_YUV161616:
			DRAW_DOT(uint16_t, 3, 0x8000);
			break;

		case BC_RGBA16161616:
			DRAW_DOT(uint16_t, 4, 0x0);
			break;
		case BC_YUVA16161616:
			DRAW_DOT(uint16_t, 4, 0x8000);
			break;
	}
}

#define RGB_TO_Y(type, is_yuv) \
{ \
	type *row_local = (type*)row; \
 \
 	if(sizeof(type) == 4) \
	{ \
		int r = (int)(row_local[0] * 0xff); \
		int g = (int)(row_local[0] * 0xff); \
		int b = (int)(row_local[0] * 0xff); \
		CLAMP(r, 0, 0xff); \
		CLAMP(g, 0, 0xff); \
		CLAMP(b, 0, 0xff); \
		i = plugin->effecttv->RtoY[r] + \
			plugin->effecttv->RtoY[g] + \
			plugin->effecttv->RtoY[b]; \
	} \
	else \
	if(sizeof(type) == 2) \
	{ \
		if(is_yuv) \
			i = (int)row_local[0] >> 8; \
		else \
		{ \
			i =  plugin->effecttv->RtoY[(int)row_local[0] >> 8]; \
			i += plugin->effecttv->GtoY[(int)row_local[1] >> 8]; \
			i += plugin->effecttv->BtoY[(int)row_local[2] >> 8]; \
		} \
	} \
	else \
	{ \
		if(is_yuv) \
			i = (int)row_local[0]; \
		else \
		{ \
			i =  plugin->effecttv->RtoY[(int)row_local[0]]; \
			i += plugin->effecttv->GtoY[(int)row_local[1]]; \
			i += plugin->effecttv->BtoY[(int)row_local[2]]; \
		} \
	} \
}

unsigned char DotClient::RGBtoY(unsigned char *row, int color_model)
{
	unsigned char i;

	switch(color_model)
	{
		case BC_RGB888:
		case BC_RGBA8888:
			RGB_TO_Y(uint8_t, 0);
			break;
		case BC_RGB_FLOAT:
		case BC_RGBA_FLOAT:
			RGB_TO_Y(float, 0);
			break;
		case BC_YUV888:
		case BC_YUVA8888:
			RGB_TO_Y(uint8_t, 1);
			break;
		case BC_RGB161616:
		case BC_RGBA16161616:
			RGB_TO_Y(uint16_t, 0);
			break;
		case BC_YUV161616:
		case BC_YUVA16161616:
			RGB_TO_Y(uint16_t, 1);
			break;
	}

	return i;
}


void DotClient::process_package(LoadPackage *package)
{
	int x, y;
	int sx, sy;
	DotPackage *local_package = (DotPackage*)package;
	unsigned char **input_rows = plugin->input_ptr->get_rows() + local_package->row1;
	unsigned char **output_rows = plugin->output_ptr->get_rows() + local_package->row1;
	int width = plugin->input_ptr->get_w();
	int height = local_package->row2 - local_package->row1;


	for(y = 0; y < plugin->dots_height; y++)
	{
		sy = plugin->sampy[y];
		for(x = 0; x < plugin->dots_width; x++)
		{
			sx = plugin->sampx[x];

//printf("DotClient::process_package %d\n", 
//					RGBtoY(&input_rows[sy][sx * plugin->input_ptr->get_bytes_per_pixel()], 
//					plugin->input_ptr->get_color_model()));
			draw_dot(x, 
				y,
				RGBtoY(&input_rows[sy][sx * plugin->input_ptr->get_bytes_per_pixel()], 
					plugin->input_ptr->get_color_model()),
				output_rows,
				plugin->input_ptr->get_color_model());
		}
	}
}



DotPackage::DotPackage()
{
}



