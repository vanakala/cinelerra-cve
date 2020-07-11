// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2001 FUKUCHI Kentarou

#include "clip.h"
#include "colormodels.inc"
#include "colorspaces.h"
#include "effecttv.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "burn.h"
#include "burnwindow.h"

REGISTER_PLUGIN


BurnConfig::BurnConfig()
{
	threshold = 50;
	decay = 15;
}

int BurnConfig::equivalent(BurnConfig &that)
{
	return threshold == that.threshold &&
		decay == that.decay;
}

void BurnConfig::copy_from(BurnConfig &that)
{
	threshold = that.threshold;
	decay = that.decay;
}

void BurnConfig::interpolate(BurnConfig &prev,
	BurnConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	threshold = prev.threshold;
	decay = prev.decay;
}


BurnMain::BurnMain(PluginServer *server)
 : PluginVClient(server)
{
	input_ptr = 0;
	burn_server = 0;
	buffer = 0;
	effecttv = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

BurnMain::~BurnMain()
{
	delete [] buffer;
	delete burn_server;
	delete effecttv;
	PLUGIN_DESTRUCTOR_MACRO
}

void BurnMain::reset_plugin()
{
	if(burn_server)
	{
		delete [] buffer;
		buffer = 0;
		delete burn_server;
		burn_server = 0;
		delete effecttv;
		effecttv = 0;
	}
}

PLUGIN_CLASS_METHODS

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

VFrame *BurnMain::process_tmpframe(VFrame *input)
{
	int color_model = input->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return input;
	}

	input_ptr = input;

	if(load_configuration())
		update_gui();

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
		memset(buffer, 0, input_ptr->get_w() * input_ptr->get_h());
		effecttv->image_bgset_y(input_ptr);
	}
	burn_server->process_packages();
	total++;
	return input;
}

void BurnMain::load_defaults()
{
	defaults = load_defaults_file("burningtv.rc");

	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.decay = defaults->get("DECAY", config.decay);
}

void BurnMain::save_defaults()
{
	defaults->update("THRESHOLD", config.threshold);
	defaults->update("DECAY", config.decay);
}

void BurnMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("BURNTV");
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("DECAY", config.decay);
	output.append_tag();
	output.tag.set_title("/BURNTV");
	output.append_tag();
	keyframe->set_data(output.string);
}

void BurnMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("BURNTV"))
		{
			config.threshold = input.tag.get_property("THRESHOLD",
				config.threshold);
			config.decay = input.tag.get_property("DECAY", config.decay);
		}
	}
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

void BurnClient::process_package(LoadPackage *package)
{
	BurnPackage *local_package = (BurnPackage*)package;
	uint16_t *row;
	int width = plugin->input_ptr->get_w();
	int height = local_package->row2 - local_package->row1;
	unsigned char *diff;
	unsigned int v, w;
	int a1, b1, c1;
	int a2, b2, c2;
	int a3, b3, c3;

	diff = plugin->effecttv->image_bgsubtract_y(
		plugin->input_ptr->get_row_ptr(local_package->row1),
		plugin->input_ptr->get_color_model(),
		plugin->input_ptr->get_bytes_per_line());

	for(int x = 1; x < width - 1; x++)
	{
		v = 0;
		for(int y = 0; y < height - 1; y++)
		{
			w = diff[y * width + x];
			plugin->buffer[y * width + x] |= v ^ w;
			v = w;
		}
	}

	for(int x = 1; x < width - 1; x++)
	{
		w = 0;
		int i = width + x;

		for(int y = 1; y < height; y++)
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
	case BC_RGBA16161616:
		for(int y = 0; y < height; y++)
		{
			row = (uint16_t*)plugin->input_ptr->get_row_ptr(local_package->row1 + y);

			for(int x = 1; x < width - 1; x++)
			{
				a1 = row[x * 4] >> 8;
				a2 = row[x * 4 + 1] >> 8;
				a3 = row[x * 4 + 2] >> 8;
				int i = y * width + x;
				b1 = plugin->palette[0][plugin->buffer[i]];
				b2 = plugin->palette[1][plugin->buffer[i]];
				b3 = plugin->palette[2][plugin->buffer[i]];
				a1 += b1;
				a2 += b2;
				a3 += b3;
				b1 = a1 & 0x100;
				b2 = a2 & 0x100;
				b3 = a3 & 0x100;
				a1 = (a1 | (b1 - (b1 >> 8)));
				a2 = (a2 | (b2 - (b2 >> 8)));
				a3 = (a3 | (b3 - (b3 >> 8)));
				row[x * 4] = a1 | (a1 << 8);
				row[x * 4 + 1] = a2 | (a2 << 8);
				row[x * 4 + 2] = a3 | (a3 << 8);
			}
		}
		break;

	case BC_AYUV16161616:
		for(int y = 0; y < height; y++)
		{
			row = (uint16_t*)plugin->input_ptr->get_row_ptr(local_package->row1 + y);

			for(int x = 1; x < width - 1; x++)
			{
				a1 = row[x * 4 + 1] >> 8;
				a2 = row[x * 4 + 2] >> 8;
				a3 = row[x * 4 + 3] >> 8;
				int i = y * width + x;
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
				row[x * 4 + 1] = a1 | (a1 << 8);
				row[x * 4 + 2] = a2 | (a2 << 8);
				row[x * 4 + 3] = a3 | (a3 << 8);
			}
		}
		break;
	}

}


BurnPackage::BurnPackage()
{
}
