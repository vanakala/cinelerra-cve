
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
#include "aging.h"
#include "agingwindow.h"
#include "effecttv.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)





REGISTER_PLUGIN(AgingMain)






int AgingConfig::dx[] = { 1, 1, 0, -1, -1, -1,  0, 1};
int AgingConfig::dy[] = { 0, -1, -1, -1, 0, 1, 1, 1};

AgingConfig::AgingConfig()
{
	dust_interval = 0;
	pits_interval = 0;
	aging_mode = 0;
	area_scale = 10;
	scratch_lines = 7;
	colorage = 1;
	scratch = 1;
	pits = 1;
	dust = 1;
}

AgingMain::AgingMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	aging_server = 0;
}

AgingMain::~AgingMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(aging_server) delete aging_server;
}

char* AgingMain::plugin_title() { return N_("AgingTV"); }
int AgingMain::is_realtime() { return 1; }

NEW_PICON_MACRO(AgingMain)

SHOW_GUI_MACRO(AgingMain, AgingThread)

SET_STRING_MACRO(AgingMain)

RAISE_WINDOW_MACRO(AgingMain)

int AgingMain::load_defaults()
{
	return 0;
}

int AgingMain::save_defaults()
{
	return 0;
}

void AgingMain::load_configuration()
{
}


void AgingMain::save_data(KeyFrame *keyframe)
{
}

void AgingMain::read_data(KeyFrame *keyframe)
{
}


int AgingMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
//printf("AgingMain::process_realtime 1\n");
	load_configuration();
//printf("AgingMain::process_realtime 1\n");
	this->input_ptr = input_ptr;
	this->output_ptr = output_ptr;

	if(!aging_server) aging_server = new AgingServer(this, 
		PluginClient::smp + 1, 
		PluginClient::smp + 1);
	aging_server->process_packages();
//printf("AgingMain::process_realtime 2\n");

	return 0;
}



AgingServer::AgingServer(AgingMain *plugin, int total_clients, int total_packages)
 : LoadServer(1, 1 /* total_clients, total_packages */)
{
	this->plugin = plugin;
}


LoadClient* AgingServer::new_client() 
{
	return new AgingClient(this);
}




LoadPackage* AgingServer::new_package() 
{ 
	return new AgingPackage; 
}



void AgingServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		AgingPackage *package = (AgingPackage*)get_package(i);
		package->row1 = plugin->input_ptr->get_h() * i / get_total_packages();
		package->row2 = plugin->input_ptr->get_h() * (i + 1) / get_total_packages();
	}
}








AgingClient::AgingClient(AgingServer *server)
 : LoadClient(server)
{
	this->plugin = server->plugin;
}









#define COLORAGE(type, components) \
{ \
	int a, b; \
	int i, j, k; \
 \
	for(i = 0; i < h; i++) \
	{ \
		for(j = 0; j < w; j++) \
		{ \
			for(k = 0; k < 3; k++) \
			{ \
				if(sizeof(type) == 4) \
				{ \
					a = (int)(((type**)input_rows)[i][j * components + k] * 0xffff); \
					CLAMP(a, 0, 0xffff); \
				} \
				else \
					a = (int)((type**)input_rows)[i][j * components + k]; \
 \
				if(sizeof(type) == 4) \
				{ \
					b = (a & 0xffff) >> 2; \
					((type**)output_rows)[i][j * components + k] = \
						(type)(a - b + 0x1800 + (EffectTV::fastrand() & 0x1000)) / 0xffff; \
				} \
				else \
				if(sizeof(type) == 2) \
				{ \
					b = (a & 0xffff) >> 2; \
					((type**)output_rows)[i][j * components + k] = \
						(type)(a - b + 0x1800 + (EffectTV::fastrand() & 0x1000)); \
				} \
				else \
				{ \
					b = (a & 0xff) >> 2; \
					((type**)output_rows)[i][j * components + k] =  \
						(type)(a - b + 0x18 + ((EffectTV::fastrand() >> 8) & 0x10)); \
				} \
			} \
		} \
	} \
}

void AgingClient::coloraging(unsigned char **output_rows, 
	unsigned char **input_rows,
	int color_model,
	int w,
	int h)
{
	switch(color_model)
	{
		case BC_RGB888:
		case BC_YUV888:
			COLORAGE(uint8_t, 3);
			break;
		
		case BC_RGB_FLOAT:
			COLORAGE(float, 3);
			break;
			
		case BC_RGBA_FLOAT:
			COLORAGE(float, 4);
			break;
			
		case BC_RGBA8888:
		case BC_YUVA8888:
			COLORAGE(uint8_t, 4);
			break;
		
		case BC_RGB161616:
		case BC_YUV161616:
			COLORAGE(uint16_t, 3);
			break;
		
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			COLORAGE(uint16_t, 4);
			break;
	}
}






#define SCRATCHES(type, components, chroma) \
{ \
	int i, j, y, y1, y2; \
	type *p; \
	int a, b; \
	int w_256 = w * 256; \
 \
	for(i = 0; i < plugin->config.scratch_lines; i++) \
	{ \
		if(plugin->config.scratches[i].life)  \
		{ \
			plugin->config.scratches[i].x = plugin->config.scratches[i].x + plugin->config.scratches[i].dx; \
			if(plugin->config.scratches[i].x < 0 || plugin->config.scratches[i].x > w_256)  \
			{ \
				plugin->config.scratches[i].life = 0; \
				break; \
			} \
\
			p = (type*)output_rows[0] + \
				(plugin->config.scratches[i].x >> 8) * \
				components; \
\
			if(plugin->config.scratches[i].init)  \
			{ \
				y1 = plugin->config.scratches[i].init; \
				plugin->config.scratches[i].init = 0; \
			}  \
			else  \
			{ \
				y1 = 0; \
			} \
\
			plugin->config.scratches[i].life--; \
			if(plugin->config.scratches[i].life)  \
			{ \
				y2 = h; \
			}  \
			else  \
			{ \
				y2 = EffectTV::fastrand() % h; \
			} \
 \
			for(y = y1; y < y2; y++)  \
			{ \
				for(j = 0; j < (chroma ? 1 : 3); j++) \
				{ \
					if(sizeof(type) == 4) \
					{ \
						int temp = (int)(p[j] * 0xffff); \
						CLAMP(temp, 0, 0xffff); \
						a = temp & 0xfeff; \
						a += 0x2000; \
						b = a & 0x10000; \
						p[j] = (type)(a | (b - (b >> 8))) / 0xffff; \
					} \
					else \
					if(sizeof(type) == 2) \
					{ \
						int temp = (int)p[j]; \
						a = temp & 0xfeff; \
						a += 0x2000; \
						b = a & 0x10000; \
						p[j] = (type)(a | (b - (b >> 8))); \
					} \
					else \
					{ \
						int temp = (int)p[j]; \
						a = temp & 0xfe; \
						a += 0x20; \
						b = a & 0x100; \
						p[j] = (type)(a | (b - (b >> 8))); \
					} \
				} \
 \
				if(chroma) \
				{ \
					p[1] = chroma; \
					p[2] = chroma; \
				} \
				p += w * components; \
			} \
		}  \
		else  \
		{ \
			if((EffectTV::fastrand() & 0xf0000000) == 0)  \
			{ \
				plugin->config.scratches[i].life = 2 + (EffectTV::fastrand() >> 27); \
				plugin->config.scratches[i].x = EffectTV::fastrand() % (w_256); \
				plugin->config.scratches[i].dx = ((int)EffectTV::fastrand()) >> 23; \
				plugin->config.scratches[i].init = (EffectTV::fastrand() % (h - 1)) + 1; \
			} \
		} \
	} \
}



void AgingClient::scratching(unsigned char **output_rows,
	int color_model,
	int w,
	int h)
{
	switch(color_model)
	{
		case BC_RGB888:
			SCRATCHES(uint8_t, 3, 0);
			break;

		case BC_RGB_FLOAT:
			SCRATCHES(float, 3, 0);
			break;

		case BC_YUV888:
			SCRATCHES(uint8_t, 3, 0x80);
			break;
		
		case BC_RGBA_FLOAT:
			SCRATCHES(float, 4, 0);
			break;

		case BC_RGBA8888:
			SCRATCHES(uint8_t, 4, 0);
			break;

		case BC_YUVA8888:
			SCRATCHES(uint8_t, 4, 0x80);
			break;
		
		case BC_RGB161616:
			SCRATCHES(uint16_t, 3, 0);
			break;

		case BC_YUV161616:
			SCRATCHES(uint16_t, 3, 0x8000);
			break;
		
		case BC_RGBA16161616:
			SCRATCHES(uint16_t, 4, 0);
			break;

		case BC_YUVA16161616:
			SCRATCHES(uint16_t, 4, 0x8000);
			break;
	}
}



#define PITS(type, components, luma, chroma) \
{ \
	int i, j, k; \
	int pnum, size, pnumscale; \
	int x, y; \
 \
	pnumscale = plugin->config.area_scale * 2; \
 \
	if(plugin->config.pits_interval)  \
	{ \
		pnum = pnumscale + (EffectTV::fastrand() % pnumscale); \
		plugin->config.pits_interval--; \
	}  \
	else \
	{ \
		pnum = EffectTV::fastrand() % pnumscale; \
		if((EffectTV::fastrand() & 0xf8000000) == 0)  \
		{ \
			plugin->config.pits_interval = (EffectTV::fastrand() >> 28) + 20; \
		} \
	} \
 \
	for(i = 0; i < pnum; i++)  \
	{ \
		x = EffectTV::fastrand() % (w - 1); \
		y = EffectTV::fastrand() % (h - 1); \
 \
		size = EffectTV::fastrand() >> 28; \
 \
		for(j = 0; j < size; j++)  \
		{ \
			x = x + EffectTV::fastrand() % 3 - 1; \
			y = y + EffectTV::fastrand() % 3 - 1; \
 \
			CLAMP(x, 0, w - 1); \
			CLAMP(y, 0, h - 1); \
			for(k = 0; k < (chroma ? 1 : 3); k++) \
			{ \
				((type**)output_rows)[y][x * components + k] = luma; \
			} \
 \
            if(chroma) \
			{ \
				((type**)output_rows)[y][x * components + 1] = chroma; \
				((type**)output_rows)[y][x * components + 2] = chroma; \
			} \
 \
		} \
	} \
}






void AgingClient::pits(unsigned char **output_rows,
	int color_model,
	int w,
	int h)
{
	switch(color_model)
	{
		case BC_RGB888:
			PITS(uint8_t, 3, 0xc0, 0);
			break;
		case BC_RGB_FLOAT:
			PITS(float, 3, (float)0xc0 / 0xff, 0);
			break;
		case BC_YUV888:
			PITS(uint8_t, 3, 0xc0, 0x80);
			break;
		
		case BC_RGBA_FLOAT:
			PITS(float, 4, (float)0xc0 / 0xff, 0);
			break;
		case BC_RGBA8888:
			PITS(uint8_t, 4, 0xc0, 0);
			break;
		case BC_YUVA8888:
			PITS(uint8_t, 4, 0xc0, 0x80);
			break;
		
		case BC_RGB161616:
			PITS(uint16_t, 3, 0xc000, 0);
			break;
		case BC_YUV161616:
			PITS(uint16_t, 3, 0xc000, 0x8000);
			break;
		
		case BC_RGBA16161616:
			PITS(uint16_t, 4, 0xc000, 0);
			break;
		case BC_YUVA16161616:
			PITS(uint16_t, 4, 0xc000, 0x8000);
			break;
	}
}


#define DUSTS(type, components, luma, chroma) \
{ \
	int i, j, k; \
	int dnum; \
	int d, len; \
	int x, y; \
 \
	if(plugin->config.dust_interval == 0)  \
	{ \
		if((EffectTV::fastrand() & 0xf0000000) == 0)  \
		{ \
			plugin->config.dust_interval = EffectTV::fastrand() >> 29; \
		} \
		return; \
	} \
 \
	dnum = plugin->config.area_scale * 4 + (EffectTV::fastrand() >> 27); \
 \
	for(i = 0; i < dnum; i++)  \
	{ \
		x = EffectTV::fastrand() % w; \
		y = EffectTV::fastrand() % h; \
		d = EffectTV::fastrand() >> 29; \
		len = EffectTV::fastrand() % plugin->config.area_scale + 5; \
 \
		for(j = 0; j < len; j++)  \
		{ \
			CLAMP(x, 0, w - 1); \
			CLAMP(y, 0, h - 1); \
			for(k = 0; k < (chroma ? 1 : 3); k++) \
			{ \
				((type**)output_rows)[y][x * components + k] = luma; \
			} \
 \
			if(chroma) \
			{ \
				((type**)output_rows)[y][x * components + 1] = chroma; \
				((type**)output_rows)[y][x * components + 2] = chroma; \
			} \
 \
			y += AgingConfig::dy[d]; \
			x += AgingConfig::dx[d]; \
 \
			if(x < 0 || x >= w) break; \
			if(y < 0 || y >= h) break; \
 \
 \
			d = (d + EffectTV::fastrand() % 3 - 1) & 7; \
		} \
	} \
	plugin->config.dust_interval--; \
}




void AgingClient::dusts(unsigned char **output_rows,
	int color_model,
	int w,
	int h)
{
	switch(color_model)
	{
		case BC_RGB888:
			DUSTS(uint8_t, 3, 0x10, 0);
			break;

		case BC_RGB_FLOAT:
			DUSTS(float, 3, (float)0x10 / 0xff, 0);
			break;

		case BC_YUV888:
			DUSTS(uint8_t, 3, 0x10, 0x80);
			break;
		
		case BC_RGBA_FLOAT:
			DUSTS(float, 4, (float)0x10 / 0xff, 0);
			break;

		case BC_RGBA8888:
			DUSTS(uint8_t, 4, 0x10, 0);
			break;

		case BC_YUVA8888:
			DUSTS(uint8_t, 4, 0x10, 0x80);
			break;
		
		case BC_RGB161616:
			DUSTS(uint16_t, 3, 0x1000, 0);
			break;

		case BC_YUV161616:
			DUSTS(uint16_t, 3, 0x1000, 0x8000);
			break;
		
		case BC_RGBA16161616:
			DUSTS(uint16_t, 4, 0x1000, 0);
			break;

		case BC_YUVA16161616:
			DUSTS(uint16_t, 4, 0x1000, 0x8000);
			break;
	}
}



void AgingClient::process_package(LoadPackage *package)
{
//printf("AgingClient::process_package 1\n");
	AgingPackage *local_package = (AgingPackage*)package;
	unsigned char **input_rows = plugin->input_ptr->get_rows() + local_package->row1;
	unsigned char **output_rows = plugin->output_ptr->get_rows() + local_package->row1;

//printf("AgingClient::process_package 1\n");
	if(plugin->config.colorage)
		coloraging(output_rows, 
			input_rows, 
			plugin->input_ptr->get_color_model(), 
			plugin->input_ptr->get_w(), 
			local_package->row2 - local_package->row1);
//printf("AgingClient::process_package 2\n");
	if(plugin->config.scratch)
		scratching(output_rows, 
			plugin->input_ptr->get_color_model(), 
			plugin->input_ptr->get_w(), 
			local_package->row2 - local_package->row1);
//printf("AgingClient::process_package 3\n");
	if(plugin->config.pits)
		pits(output_rows, 
			plugin->input_ptr->get_color_model(), 
			plugin->input_ptr->get_w(), 
			local_package->row2 - local_package->row1);
//printf("AgingClient::process_package 4 %d\n", plugin->config.dust);
	if(plugin->config.dust)
		dusts(output_rows, 
			plugin->input_ptr->get_color_model(), 
			plugin->input_ptr->get_w(), 
			local_package->row2 - local_package->row1);
//printf("AgingClient::process_package 5\n");
}



AgingPackage::AgingPackage()
{
}



