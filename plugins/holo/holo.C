// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2001 FUKUCHI Kentarou

#include "clip.h"
#include "colormodels.inc"
#include "colorspaces.h"
#include "effecttv.h"
#include "holo.h"
#include "holowindow.h"
#include "picon_png.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN


HoloConfig::HoloConfig()
{
	threshold = 40;
	recycle = 1.0;
}


HoloMain::HoloMain(PluginServer *server)
 : PluginVClient(server)
{
	effecttv = 0;
	holo_server = 0;
	bgimage = 0;
	do_reconfigure = 1;
	PLUGIN_CONSTRUCTOR_MACRO
}

HoloMain::~HoloMain()
{
	delete holo_server;
	delete effecttv;

	release_vframe(bgimage);
	PLUGIN_DESTRUCTOR_MACRO
}

void HoloMain::reset_plugin()
{
	if(effecttv)
	{
		delete holo_server;
		holo_server = 0;
		delete effecttv;
		effecttv = 0;
		release_vframe(bgimage);
		bgimage = 0;
	}
}

PLUGIN_CLASS_METHODS

int HoloMain::load_configuration()
{
	return 0;
}

void HoloMain::reconfigure()
{
	do_reconfigure = 0;

	effecttv->image_set_threshold_y(config.threshold);
}

// Add input to output and store result in output
void HoloMain::add_frames(VFrame *output, VFrame *input)
{
	int w = input->get_w();
	int h = input->get_h();

	switch(output->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *output_row = (uint16_t*)output->get_row_ptr(i);
			uint16_t *input_row = (uint16_t*)input->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				for(int k = 0; k < 3; k++)
				{
					*output_row = (*input_row & *output_row) +
						((*input_row ^ *output_row) >> 1);
					output_row++;
					input_row++;
				}
				output_row++;
				input_row++;
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *output_row = (uint16_t*)output->get_row_ptr(i);
			uint16_t *input_row = (uint16_t*)input->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				output_row++;
				input_row++;
				for(int k = 0; k < 3; k++)
				{
					*output_row = (*input_row & *output_row) +
						((*input_row ^ *output_row) >> 1);
					output_row++;
					input_row++;
				}
			}
		}
		break;
	}
}

void HoloMain::set_background()
{
// grab 4 frames and composite them to get a quality background image
// For Cinelerra, we make every frame a holograph and expect the user to
// provide a matte.
	total = 0;

	switch(total)
	{
	case 0:
// step 1: grab frame-1 to buffer-1
		bgimage->copy_from(input_ptr);
		break;

	case 1:
// step 2: add frame-2 to buffer-1
		add_frames(bgimage, input_ptr);
		break;

	case 2:
// step 3: grab frame-3 to buffer-2
		tmp->copy_from(input_ptr);
		break;

	case 3:
// step 4: add frame-4 to buffer-2
		add_frames(tmp, input_ptr);

// step 5: add buffer-3 to buffer-1
		add_frames(bgimage, tmp);

		effecttv->image_bgset_y(bgimage);
		delete tmp;
		break;
	}
}

VFrame *HoloMain::process_tmpframe(VFrame *input_ptr)
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

	this->input_ptr = input_ptr;
	this->output_ptr = input_ptr;

	if(load_configuration())
	{
		update_gui();
		do_reconfigure = 1;
	}

	if(!effecttv)
	{
		effecttv = new EffectTV(input_ptr->get_w(), input_ptr->get_h());
		bgimage = clone_vframe(input_ptr);
		do_reconfigure = 1;
	}

	if(do_reconfigure)
	{
		for(int i = 0; i < 256; i++)
			noisepattern[i] = (i * i * i / 40000)* i / 256;

		holo_server = new HoloServer(this, 1, 1);

		reconfigure();
	}

	set_background();

	holo_server->process_packages();

	total++;
	if(total >= config.recycle * get_project_framerate())
		total = 0;
	return input_ptr;
}


HoloServer::HoloServer(HoloMain *plugin, int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

LoadClient* HoloServer::new_client() 
{
	return new HoloClient(this);
}

LoadPackage* HoloServer::new_package() 
{ 
	return new HoloPackage; 
}

void HoloServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		HoloPackage *package = (HoloPackage*)get_package(i);
		package->row1 = plugin->input_ptr->get_h() * i / get_total_packages();
		package->row2 = plugin->input_ptr->get_h() * (i + 1) / get_total_packages();
	}
}

HoloClient::HoloClient(HoloServer *server)
 : LoadClient(server)
{
	this->plugin = server->plugin;
	phase = 0;
}

void HoloClient::process_package(LoadPackage *package)
{
	int sx, sy;
	HoloPackage *local_package = (HoloPackage*)package;
	unsigned char *input_row = plugin->input_ptr->get_row_ptr(local_package->row1);
	unsigned char *output_row = plugin->output_ptr->get_row_ptr(local_package->row1);
	int width = plugin->input_ptr->get_w();
	int height = local_package->row2 - local_package->row1;
	int bytes_per_line = plugin->input_ptr->get_bytes_per_line();
	unsigned char *diff;
	uint32_t s, t;
	int r, g, b;

	diff = plugin->effecttv->image_diff_filter(plugin->effecttv->image_bgsubtract_y(input_row,
		plugin->input_ptr->get_color_model(), bytes_per_line));

	diff += width;

	switch(plugin->input_ptr->get_color_model())
	{
	case BC_RGBA16161616:
		for(int y = 1; y < height - 1; y++)
		{
			uint16_t *src = (uint16_t*)plugin->input_ptr->get_row_ptr(y +
				local_package->row1);
			uint16_t *bg = (uint16_t*)plugin->bgimage->get_row_ptr(y);
			uint16_t *dest = (uint16_t*)plugin->output_ptr->get_row_ptr(y +
				local_package->row1);

			if(((y + phase) & 0x7f) < 0x58)
			{
				for(int x = 0 ; x < width; x++)
				{
					if(*diff)
					{
						s = ((src[0] << 8) & 0xff0000) |
							(src[1] & 0xff00) |
							src[2] >> 8;

						t = (s & 0xff) +
							((s & 0xff00) >> 7) +
							((s & 0xff0000) >> 16);
						t += plugin->noisepattern[EffectTV::fastrand() >> 24];

						r = ((s & 0xff0000) >> 17) + t;
						g = ((s & 0xff00) >> 8) + t;
						b = (s & 0xff) + t;

						r = (r >> 1) - 100;
						g = (g >> 1) - 100;
						b = b >> 2;

						if(r < 20) r = 20;
						if(g < 20) g = 20;

						s = ((bg[0] << 8) & 0xff0000) |
							(bg[1] & 0xff00) |
							bg[2] >> 8;

						r += (s & 0xff0000) >> 17;
						g += (s & 0xff00) >> 9;
						b += ((s & 0xff) >> 1) + 40;

						if(r > 255) r = 255;
						if(g > 255) g = 255;
						if(b > 255) b = 255;

						dest[0] = (r << 8) | r;
						dest[1] = (g << 8) | g;
						dest[2] = (b << 8) | b;
					}
					else
					{
						dest[0] = bg[0];
						dest[1] = bg[1];
						dest[2] = bg[2];
					}

					diff++;
					src += 4;
					dest += 4;
					bg += 4;
				}
			}
			else
			{
				for(int x = 0; x < width; x++)
				{
					if(*diff)
					{
						s = ((src[0] << 8) & 0xff0000) |
							(src[1] & 0xff00) |
							src[2] >> 8;

						t = (s & 0xff) + ((s & 0xff00) >> 6) +
							((s & 0xff0000) >> 16);
						t += plugin->noisepattern[EffectTV::fastrand() >> 24];

						r = ((s & 0xff0000) >> 16) + t;
						g = ((s & 0xff00) >> 8) + t;
						b = (s & 0xff) + t;

						r = (r >> 1) - 100;
						g = (g >> 1) - 100;
						b = b >> 2;

						if(r < 0) r = 0;
						if(g < 0) g = 0;

						s = ((bg[0] << 8) & 0xff0000) |
							(bg[1] & 0xff00) |
							bg[2] >> 8;

						r += ((s & 0xff0000) >> 17) + 10;
						g += ((s & 0xff00) >> 9) + 10;
						b += ((s & 0xff) >> 1) + 40;

						if(r > 255) r = 255;
						if(g > 255) g = 255;
						if(b > 255) b = 255;

						dest[0] = (r << 8) | r;
						dest[1] = (g << 8) | g;
						dest[2] = (b << 8) | b;
					}
					else
					{
						dest[0] = bg[0];
						dest[1] = bg[1];
						dest[2] = bg[2];
					}

					diff++;
					src += 4;
					dest += 4;
					bg += 4;
				}
			}
		}
		break;

	case BC_AYUV16161616:
		for(int y = 1; y < height - 1; y++)
		{
			uint16_t *src = (uint16_t*)plugin->input_ptr->get_row_ptr(y + local_package->row1);
			uint16_t *bg = (uint16_t*)plugin->bgimage->get_row_ptr(y);
			uint16_t *dest = (uint16_t*)plugin->output_ptr->get_row_ptr(y + local_package->row1);

			if(((y + phase) & 0x7f) < 0x58)
			{
				for(int x = 0; x < width; x++)
				{
					if(*diff)
					{
						r = src[1] >> 8;
						g = src[2] >> 8;
						b = src[3] >> 8;
						ColorSpaces::yuv_to_rgb_8(r, g, b);
						s = (r << 16) | (g << 8) | b;

						t = (s & 0xff) +
							((s & 0xff00) >> 7) +
							((s & 0xff0000) >> 16);
						t += plugin->noisepattern[EffectTV::fastrand() >> 24];

						r = ((s & 0xff0000) >> 17) + t;
						g = ((s & 0xff00) >> 8) + t;
						b = (s & 0xff) + t;

						r = (r >> 1) - 100;
						g = (g >> 1) - 100;
						b = b >> 2;

						if(r < 20) r = 20;
						if(g < 20) g = 20;

						r = bg[1] >> 8;
						g = bg[2] >> 8;
						b = bg[3] >> 8;
						ColorSpaces::yuv_to_rgb_8(r, g, b);
						s = (r << 16) | (g << 8) | b;

						r += (s & 0xff0000) >> 17;
						g += (s & 0xff00) >> 9;
						b += ((s & 0xff) >> 1) + 40;

						if(r > 255) r = 255;
						if(g > 255) g = 255;
						if(b > 255) b = 255;

						ColorSpaces::rgb_to_yuv_8(r, g, b);

						dest[1] = (r << 8) | r;
						dest[2] = (g << 8) | g;
						dest[3] = (b << 8) | b;
					}
					else
					{
						dest[1] = bg[1];
						dest[2] = bg[2];
						dest[3] = bg[3];
					}

					diff++;
					src += 4;
					dest += 4;
					bg += 4;
				}
			}
			else
			{
				for(int x = 0; x < width; x++)
				{
					if(*diff)
					{

						r = src[1] >> 8;
						g = src[2] >> 8;
						b = src[3] >> 8;
						ColorSpaces::yuv_to_rgb_8(r, g, b);
						s = (r << 16) | (g << 8) | b;

						t = (s & 0xff) + ((s & 0xff00) >> 6) + ((s & 0xff0000) >> 16);
						t += plugin->noisepattern[EffectTV::fastrand() >> 24];

						r = ((s & 0xff0000) >> 16) + t;
						g = ((s & 0xff00) >> 8) + t;
						b = (s & 0xff) + t;

						r = (r >> 1) - 100;
						g = (g >> 1) - 100;
						b = b >> 2;

						if(r < 0) r = 0;
						if(g < 0) g = 0;

						r = bg[1] >> 8;
						g = bg[2] >> 8;
						b = bg[3] >> 8;
						ColorSpaces::yuv_to_rgb_8(r, g, b);
						s = (r << 16) | (g << 8) | b;

						r += ((s & 0xff0000) >> 17) + 10;
						g += ((s & 0xff00) >> 9) + 10;
						b += ((s & 0xff) >> 1) + 40;

						if(r > 255) r = 255;
						if(g > 255) g = 255;
						if(b > 255) b = 255;

						ColorSpaces::rgb_to_yuv_8(r, g, b);

						dest[1] = (r << 8) | r;
						dest[2] = (g << 8) | g;
						dest[3] = (b << 8) | b;
					}
					else
					{
						r = bg[1] >> 8;
						g = bg[2] >> 8;
						b = bg[3] >> 8;
						ColorSpaces::yuv_to_rgb_8(r, g, b);
						dest[1] = (r << 8) | r;
						dest[2] = (g << 8) | g;
						dest[3] = (b << 8) | b;
					}

					diff++;
					src += 4;
					dest += 4;
					bg += 4;
				}
			}
		}
		break;
	}

	phase -= 37;
}

HoloPackage::HoloPackage()
{
}
