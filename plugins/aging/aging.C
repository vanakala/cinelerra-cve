// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2001 FUKUCHI Kentarou

#include "clip.h"
#include "colormodels.inc"
#include "filexml.h"
#include "picon_png.h"
#include "aging.h"
#include "agingwindow.h"
#include "effecttv.h"
#include "language.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


REGISTER_PLUGIN


int AgingConfig::dx[] = { 1, 1, 0, -1, -1, -1,  0, 1};
int AgingConfig::dy[] = { 0, -1, -1, -1, 0, 1, 1, 1};

AgingConfig::AgingConfig()
{
	dust_interval = 0;
	pits_interval = 0;
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
	aging_server = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

AgingMain::~AgingMain()
{
	delete aging_server;
	PLUGIN_DESTRUCTOR_MACRO
}

void AgingMain::reset_plugin()
{
	delete aging_server;
	aging_server = 0;
}

PLUGIN_CLASS_METHODS

int AgingMain::load_configuration()
{
	return 0;
}

VFrame *AgingMain::process_tmpframe(VFrame *input_ptr)
{
	int color_model = input_ptr->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return input_ptr;
	}

	if(load_configuration());
		update_gui();

	input = input_ptr;

	if(!aging_server) aging_server = new AgingServer(this, 
		PluginClient::smp,
		PluginClient::smp);
	aging_server->process_packages();
	return input;
}


AgingServer::AgingServer(AgingMain *plugin, int total_clients, int total_packages)
 : LoadServer(1, 1)
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
		package->row1 = plugin->input->get_h() * i / get_total_packages();
		package->row2 = plugin->input->get_h() * (i + 1) / get_total_packages();
	}
}


AgingClient::AgingClient(AgingServer *server)
 : LoadClient(server)
{
	this->plugin = server->plugin;
}


void AgingClient::coloraging(VFrame *output, int row1, int row2)
{
	int h = row2 - row1;
	int w = output->get_w();

	switch(output->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = 0; i < h; i++) \
		{
			uint16_t *outrow = (uint16_t*)output->get_row_ptr(row1 + i);

			for(int j = 0; j < w; j++)
			{
				for(int k = 0; k < 3; k++)
				{
					int a = outrow[j * 4 + k];
					int b = (a & 0xffff) >> 2;
					outrow[j * 4 + k] =
						(uint16_t)(a - b + 0x1800 + (EffectTV::fastrand() & 0x1000));
				}
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < h; i++) \
		{
			uint16_t *outrow = (uint16_t*)output->get_row_ptr(row1 + i);

			for(int j = 0; j < w; j++)
			{
				for(int k = 1; k < 4; k++)
				{
					int a = outrow[j * 4 + k];
					int b = (a & 0xffff) >> 2;
					outrow[j * 4 + k] =
						(uint16_t)(a - b + 0x1800 + (EffectTV::fastrand() & 0x1000));
				}
			}
		}
		break;
	}
}

void AgingClient::scratching(VFrame *output_frame,
	int row1, int row2)
{
	int h = row2 - row1;
	int w = output_frame->get_w();
	int w_256 = w * 256;
	uint16_t *p;
	uint16_t *row = (uint16_t*)output_frame->get_row_ptr(row1);
	int y1, y2;

	switch(output_frame->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = 0; i < plugin->config.scratch_lines; i++)
		{
			if(plugin->config.scratches[i].life)
			{
				plugin->config.scratches[i].x =
					plugin->config.scratches[i].x +
					plugin->config.scratches[i].dx;

				if(plugin->config.scratches[i].x < 0 ||
					plugin->config.scratches[i].x > w_256)
				{
					plugin->config.scratches[i].life = 0;
					break;
				}

				p =  row +
					(plugin->config.scratches[i].x >> 8) * 4;

				if(plugin->config.scratches[i].init)
				{
					y1 = plugin->config.scratches[i].init;
					plugin->config.scratches[i].init = 0;
				}
				else
					y1 = 0;

				plugin->config.scratches[i].life--;

				if(plugin->config.scratches[i].life)
					y2 = h;
				else
					y2 = EffectTV::fastrand() % h;

				for(int y = y1; y < y2; y++)
				{
					for(int j = 0; j < 3; j++)
					{
						int temp = p[j];
						int a = temp & 0xfeff;
						a += 0x2000;
						int b = a & 0x10000;
						p[j] = a | (b - (b >> 8));
					}
					p += w * 4;
				}
			}
			else
			{
				if((EffectTV::fastrand() & 0xf0000000) == 0)
				{
					plugin->config.scratches[i].life = 2 + (EffectTV::fastrand() >> 27);
					plugin->config.scratches[i].x = EffectTV::fastrand() % (w_256);
					plugin->config.scratches[i].dx = ((int)EffectTV::fastrand()) >> 23;
					plugin->config.scratches[i].init = (EffectTV::fastrand() % (h - 1)) + 1;
				}
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < plugin->config.scratch_lines; i++)
		{
			if(plugin->config.scratches[i].life)
			{
				plugin->config.scratches[i].x =
					plugin->config.scratches[i].x +
					plugin->config.scratches[i].dx;
				if(plugin->config.scratches[i].x < 0 ||
					plugin->config.scratches[i].x > w_256)
				{
					plugin->config.scratches[i].life = 0;
					break;
				}

				p =  row +
					(plugin->config.scratches[i].x >> 8) * 4;

				if(plugin->config.scratches[i].init)
				{
					y1 = plugin->config.scratches[i].init;
					plugin->config.scratches[i].init = 0;
				}
				else
					y1 = 0;

				plugin->config.scratches[i].life--;
				if(plugin->config.scratches[i].life)
					y2 = h;
				else
					y2 = EffectTV::fastrand() % h;

				for(int y = y1; y < y2; y++)
				{
					int a = p[1] & 0xfeff;
					a += 0x2000;
					int b = a & 0x10000;
					p[1] = a | (b - (b >> 8));
					p += w * 4;
				}
			}
			else
			{
				if((EffectTV::fastrand() & 0xf0000000) == 0)
				{
					plugin->config.scratches[i].life = 2 + (EffectTV::fastrand() >> 27);
					plugin->config.scratches[i].x = EffectTV::fastrand() % (w_256);
					plugin->config.scratches[i].dx = ((int)EffectTV::fastrand()) >> 23;
					plugin->config.scratches[i].init = (EffectTV::fastrand() % (h - 1)) + 1;
				}
			}
		}
		break;
	}
}

void AgingClient::pits(VFrame *output,
	int row1, int row2)
{
	int h = row2 - row1;
	int w = output->get_w();
	int pnumscale = plugin->config.area_scale * 2;
	int pnum;

	if(plugin->config.pits_interval)
	{
		pnum = pnumscale + (EffectTV::fastrand() % pnumscale);
		plugin->config.pits_interval--;
	}
	else
	{
		pnum = EffectTV::fastrand() % pnumscale;
		if((EffectTV::fastrand() & 0xf8000000) == 0)
			plugin->config.pits_interval = (EffectTV::fastrand() >> 28) + 20; \
	}

	switch(output->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = 0; i < pnum; i++)
		{
			int x = EffectTV::fastrand() % (w - 1);
			int y = EffectTV::fastrand() % (h - 1);

			int size = EffectTV::fastrand() >> 28;

			for(int j = 0; j < size; j++)
			{
				int x = x + EffectTV::fastrand() % 3 - 1;
				int y = y + EffectTV::fastrand() % 3 - 1;

				CLAMP(x, 0, w - 1);
				CLAMP(y, 0, h - 1);

				uint16_t *row = (uint16_t*)output->get_row_ptr(row1 + y);

				for(int k = 0; k < 3; k++)
					row[x * 4 + k] = 0xc000;
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < pnum; i++)
		{
			int x = EffectTV::fastrand() % (w - 1);
			int y = EffectTV::fastrand() % (h - 1);

			int size = EffectTV::fastrand() >> 28;

			for(int j = 0; j < size; j++)
			{
				x = x + EffectTV::fastrand() % 3 - 1;
				y = y + EffectTV::fastrand() % 3 - 1;

				CLAMP(x, 0, w - 1);
				CLAMP(y, 0, h - 1);
				uint16_t *row = (uint16_t*)output->get_row_ptr(row1 + y);

				row[x * 4 + 1] = 0xc000;
				row[x * 4 + 2] = 0x8000;
				row[x * 4 + 2] = 0x8000;
			}
		}
		break;
	}
}

void AgingClient::dusts(VFrame *output_frame,
	int row1, int row2)
{
	int h = row2 - row1;
	int w = output_frame->get_w();
	int dnum = plugin->config.area_scale * 4 + (EffectTV::fastrand() >> 27);

	if(plugin->config.dust_interval == 0)
	{
		if((EffectTV::fastrand() & 0xf0000000) == 0)
			plugin->config.dust_interval = EffectTV::fastrand() >> 29;
		return;
	}

	switch(output_frame->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = 0; i < dnum; i++)
		{
			int x = EffectTV::fastrand() % w;
			int y = EffectTV::fastrand() % h;
			int d = EffectTV::fastrand() >> 29;
			int len = EffectTV::fastrand() % plugin->config.area_scale + 5;

			for(int j = 0; j < len; j++)
			{
				CLAMP(x, 0, w - 1);
				CLAMP(y, 0, h - 1);

				uint16_t *row = (uint16_t*)output_frame->get_row_ptr(row1 + y);

				for(int k = 0; k < 3; k++)
					row[x * 4 + k] = 0x1000;

				y += AgingConfig::dy[d];
				x += AgingConfig::dx[d];

				if(x < 0 || x >= w)
					break;
				if(y < 0 || y >= h)
					break;
				d = (d + EffectTV::fastrand() % 3 - 1) & 7; \
			}
		}
		plugin->config.dust_interval--;
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < dnum; i++)
		{
			int x = EffectTV::fastrand() % w;
			int y = EffectTV::fastrand() % h;
			int d = EffectTV::fastrand() >> 29;
			int len = EffectTV::fastrand() % plugin->config.area_scale + 5;

			for(int j = 0; j < len; j++)
			{
				CLAMP(x, 0, w - 1);
				CLAMP(y, 0, h - 1);

				uint16_t *row = (uint16_t*)output_frame->get_row_ptr(row1 + y);

				row[x * 4 + 1] = 0x1000;
				row[x * 4 + 2] = 0x8000;
				row[x * 4 + 3] = 0x8000;

				y += AgingConfig::dy[d];
				x += AgingConfig::dx[d];

				if(x < 0 || x >= w)
					break;
				if(y < 0 || y >= h)
					break;

				d = (d + EffectTV::fastrand() % 3 - 1) & 7;
			}
		}
		plugin->config.dust_interval--;
		break;
	}
}

void AgingClient::process_package(LoadPackage *package)
{
	AgingPackage *local_package = (AgingPackage*)package;

	if(plugin->config.colorage)
		coloraging(plugin->input,
			local_package->row1,
			local_package->row2);

	if(plugin->config.scratch)
		scratching(plugin->input,
			local_package->row1,
			local_package->row2);

	if(plugin->config.pits)
		pits(plugin->input,
			local_package->row1,
			local_package->row2);

	if(plugin->config.dust)
		dusts(plugin->input,
			local_package->row1,
			local_package->row2);
}


AgingPackage::AgingPackage()
{
}
