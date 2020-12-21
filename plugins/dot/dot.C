// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2001 FUKUCHI Kentarou

#include "clip.h"
#include "colormodels.inc"
#include "colorspaces.h"
#include "filexml.h"
#include "picon_png.h"
#include "dot.h"
#include "dotwindow.h"
#include "language.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN


DotConfig::DotConfig()
{
	dot_depth = 5;
	dot_size = 8;
}

int DotConfig::equivalent(DotConfig &that)
{
	return dot_depth == that.dot_depth &&
		dot_size == that.dot_size;
}

void DotConfig::copy_from(DotConfig &that)
{
	dot_depth = that.dot_depth;
	dot_size = that.dot_size;
}

void DotConfig::interpolate(DotConfig &prev,
	DotConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	dot_depth = prev.dot_depth;
	dot_size = prev.dot_size;
}

DotMain::DotMain(PluginServer *server)
 : PluginVClient(server)
{
	pattern = 0;
	sampx = 0;
	sampy = 0;
	dot_server = 0;
	pattern_allocated = 0;
	pattern_size = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

DotMain::~DotMain()
{
	delete [] pattern;
	delete [] sampx;
	delete [] sampy;
	delete dot_server;
	PLUGIN_DESTRUCTOR_MACRO
}

void DotMain::reset_plugin()
{
	if(pattern)
	{
		pattern_allocated = 0;
		pattern_size = 0;
		delete [] pattern;
		pattern = 0;
		delete [] sampx;
		sampx = 0;
		delete [] sampy;
		sampy = 0;
		delete dot_server;
		dot_server = 0;
	}
}

PLUGIN_CLASS_METHODS

void DotMain::make_pattern()
{
	int u, v;
	double p, q, r;
	uint32_t *pat;
	int dotmax = config.dot_max();

	for(int i = 0; i < dotmax; i++)
	{
// Generated pattern is a quadrant of a disk.
		pat = pattern + (i + 1) * dot_hsize * dot_hsize - 1;

		r = ((double)i / dotmax) * dot_hsize;
		r *= 5;

		for(int y = 0; y < dot_hsize; y++)
		{
			for(int x = 0; x < dot_hsize; x++)
			{
				int c = 0;
				for(int u = 0; u < 4; u++)
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
				*pat-- = (c << 20) | (c << 12) | (c << 4);
// The upper left part of a disk is needed, but generated pattern is a bottom
//   right part. So I spin the pattern.
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
	dots_width = input_ptr->get_w() / dot_size;
	dots_height = input_ptr->get_h() / dot_size;

	make_pattern();
	init_sampxy_table();
}

VFrame *DotMain::process_tmpframe(VFrame *input_ptr)
{
	int color_model = input_ptr->get_color_model();
	int new_size;
	int do_reconfigure = 0;

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return input_ptr;
	}

	this->input_ptr = input_ptr;
	if(load_configuration())
	{
		update_gui();
		do_reconfigure = 1;
	}

	if(!dot_server)
		dot_server = new DotServer(this, 1, 1);

	dot_size = config.dot_size;
	dot_size = dot_size & 0xfe;
	dot_hsize = dot_size / 2;
	new_size = config.dot_max() * dot_hsize * dot_hsize;
	if(new_size > pattern_allocated)
	{
		delete [] pattern;
		pattern = new uint32_t[new_size];
		pattern_allocated = new_size;
		do_reconfigure = 1;
	}
	pattern_size = new_size;

	if(!sampx)
	{
		sampx = new int[input_ptr->get_w()];
		sampy = new int[input_ptr->get_h()];
		do_reconfigure = 1;
	}

	if(do_reconfigure)
		reconfigure();

	dot_server->process_packages();
	return input_ptr;
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

void DotClient::draw_dot(int xx, int yy,
	int c, int first_row)
{
	uint32_t *pat;
	int y_total = 0;
	uint16_t *output = &((uint16_t*)plugin->input_ptr->get_row_ptr(yy * plugin->dot_size))[xx * plugin->dot_size * 4];
	int width = plugin->input_ptr->get_w();
	int height = plugin->input_ptr->get_h();

	switch(plugin->input_ptr->get_color_model())
	{
	case BC_RGBA16161616:
		c = (c >> (16 - plugin->config.dot_depth));
		pat = plugin->pattern + c * plugin->dot_hsize * plugin->dot_hsize;

		for(int y = 0; y < plugin->dot_hsize &&
			y_total < height; y++)
		{
			for(int x = 0; x < plugin->dot_hsize; x++)
			{
				output[0] = (*pat & 0xff0000) >> 8;
				output[1] = (*pat & 0xff00);
				output[2] = (*pat & 0xff) << 8;
				output[3] = 0xffff;
				output += 4;
				pat++;
			}

			pat -= 2;

			for(int x = 0; x < plugin->dot_hsize - 1; x++)
			{
				output[0] = (*pat & 0xff0000) >> 8;
				output[1] = (*pat & 0xff00);
				output[2] = (*pat & 0xff) << 8;
				output[3] = 0xffff;
				output += 4;
				pat--;
			}
			output[0] = 0;
			output[1] = 0;
			output[2] = 0;
			output[3] = 0xffff;

			output += 4 * (width - plugin->dot_size + 1);
			pat += plugin->dot_hsize + 1;
			y_total++;
		}
		pat -= plugin->dot_hsize * 2;

		for(int y = 0; y < plugin->dot_hsize &&
			y_total < height; y++)
		{
			if(y < plugin->dot_hsize - 1)
			{
				for(int x = 0; x < plugin->dot_hsize; x++)
				{
					output[0] = (*pat & 0xff0000) >> 8;
					output[1] = (*pat & 0xff00);
					output[2] = (*pat & 0xff) << 8;
					output[3] = 0xffff;
					output += 4;
					pat++;
				}

				pat -= 2;

				for(int x = 0; x < plugin->dot_hsize - 1; x++)
				{
					output[0] = (*pat & 0xff0000) >> 8;
					output[1] = (*pat & 0xff00);
					output[2] = (*pat & 0xff) << 8;
					output[3] = 0xffff;
					output += 4;
					pat--;
				}
				output[0] = 0;
				output[1] = 0;
				output[2] = 0;
				output[3] = 0xffff;

				output += 4 * (width - plugin->dot_size + 1);
				pat += -plugin->dot_hsize + 1;
			}
			else
			{
				for(int x = 0; x < plugin->dot_hsize * 2; x++)
				{
					output[0] = 0;
					output[1] = 0;
					output[2] = 0;
					output[3] = 0xffff;
					output += 4;
				}
			}
			y_total++;
		}
		break;

	case BC_AYUV16161616:
		c = (c >> (16 - plugin->config.dot_depth));
		pat = plugin->pattern + c * plugin->dot_hsize * plugin->dot_hsize;

		for(int y = 0; y < plugin->dot_hsize && y_total < height; y++)
		{
			for(int x = 0; x < plugin->dot_hsize; x++)
			{
				output[0] = 0xffff;
				output[1] = (*pat & 0xff0000) >> 8;
				output[2] = 0x8000;
				output[3] = 0x8000;
				output += 4;
				pat++;
			}
			pat -= 2;

			for(int x = 0; x < plugin->dot_hsize - 1; x++)
			{
				output[0] = 0xffff;
				output[1] = (*pat & 0xff0000) >> 8;
				output[2] = 0x8000;
				output[3] = 0x8000;
				output += 4;
				pat--;
			}

			output[0] = 0xffff;
			output[1] = 0;
			output[2] = 0x8000;
			output[3] = 0x8000;

			output += 4 * (width - plugin->dot_size + 1);
			pat += plugin->dot_hsize + 1;
			y_total++;
		}

		pat -= plugin->dot_hsize * 2;

		for(int y = 0; y < plugin->dot_hsize &&
			y_total < height; y++)
		{
			if(y < plugin->dot_hsize - 1)
			{
				for(int x = 0; x < plugin->dot_hsize; x++)
				{
					output[0] = 0xffff;
					output[1] = (*pat & 0xff0000) >> 8;
					output[2] = 0x8000;
					output[3] = 0x8000;
					output += 4;
					pat++;
				}
				pat -= 2;

				for(int x = 0; x < plugin->dot_hsize - 1; x++)
				{
					output[0] = 0xffff;
					output[1] = (*pat & 0xff0000) >> 8;
					output[2] = 0x8000;
					output[3] = 0x8000;
					output += 4;
						pat--;
					}

					output[0] = 0xffff;
					output[1] = 0;
					output[2] = 0x8000;
					output[3] = 0x8000;

					output += 4 * (width - plugin->dot_size + 1);
					pat += -plugin->dot_hsize + 1;
				}
				else
				{
					for(int x = 0; x < plugin->dot_hsize * 2; x++)
					{
						output[0] = 0xffff;
						output[1] = 0;
						output[2] = 0x8000;
						output[3] = 0x8000;
						output += 4;
					}
				}

				y_total++;
			}
		break;
	}
}

void DotMain::load_defaults()
{
	defaults = load_defaults_file("dottv.rc");

	config.dot_depth = defaults->get("DEPTH", config.dot_depth);
	config.dot_size = defaults->get("SIZE", config.dot_size);
}

void DotMain::save_defaults()
{
	defaults->update("DEPTH", config.dot_depth);
	defaults->update("SIZE", config.dot_size);
}

void DotMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DOTTV");
	output.tag.set_property("DEPTH", config.dot_depth);
	output.tag.set_property("SIZE", config.dot_size);
	output.append_tag();
	output.tag.set_title("/DOTTV");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DotMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("DOTTV"))
		{
			config.dot_depth = input.tag.get_property("DEPTH",
				config.dot_depth);
			config.dot_size = input.tag.get_property("SIZE",
				config.dot_size);
		}
	}
}

int DotClient::RGBtoY(uint16_t *row, int color_model)
{
	int y, u, v;

	switch(color_model)
	{
	case BC_RGBA16161616:
		ColorSpaces::rgb_to_yuv_16(row[0], row[1], row[2], y, u, v);
		break;
	case BC_AYUV16161616:
		y = row[1];
		break;
	}
	return y;
}

void DotClient::process_package(LoadPackage *package)
{
	DotPackage *local_package = (DotPackage*)package;

	for(int y = 0; y < plugin->dots_height; y++)
	{
		int sy = plugin->sampy[y];
		for(int x = 0; x < plugin->dots_width; x++)
		{
			int sx = plugin->sampx[x];

			draw_dot(x, y,
				RGBtoY(
					&((uint16_t*)plugin->input_ptr->get_row_ptr(local_package->row1 + sy))[sx * 4],
					plugin->input_ptr->get_color_model()),
				local_package->row1);
		}
	}
}

DotPackage::DotPackage()
{
}
