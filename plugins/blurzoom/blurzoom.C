
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
#include "blurzoom.h"
#include "blurzoomwindow.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

PluginClient* new_plugin(PluginServer *server)
{
	return new BlurZoomMain(server);
}












BlurZoomConfig::BlurZoomConfig()
{
}

BlurZoomMain::BlurZoomMain(PluginServer *server)
 : PluginVClient(server)
{
	thread = 0;
	defaults = 0;
	load_defaults();
}

BlurZoomMain::~BlurZoomMain()
{
	if(thread)
	{
// Set result to 0 to indicate a server side close
		thread->window->set_done(0);
		thread->completion.lock();
		delete thread;
	}

	save_defaults();
	if(defaults) delete defaults;
}

char* BlurZoomMain::plugin_title() { return N_("RadioacTV"); }
int BlurZoomMain::is_realtime() { return 1; }

VFrame* BlurZoomMain::new_picon()
{
	return new VFrame(picon_png);
}

int BlurZoomMain::load_defaults()
{
	return 0;
}

int BlurZoomMain::save_defaults()
{
	return 0;
}

void BlurZoomMain::load_configuration()
{
}


void BlurZoomMain::save_data(KeyFrame *keyframe)
{
}

void BlurZoomMain::read_data(KeyFrame *keyframe)
{
}


#define VIDEO_HWIDTH (buf_width/2)
#define VIDEO_HHEIGHT (buf_height/2)


#define MAGIC_THRESHOLD 40
#define RATIO 0.95

void BlurZoomMain::image_set_threshold_y(int threshold)
{
	y_threshold = threshold * 7; /* fake-Y value is timed by 7 */
}


void BlurZoomMain::set_table()
{
	int bits, x, y, tx, ty, xx;
	int ptr, prevptr;

	prevptr = (int)(0.5 + RATIO * (-VIDEO_HWIDTH) + VIDEO_HWIDTH);

	for(xx = 0; xx < (buf_width_blocks); xx++)
	{
		bits = 0;
		for(x = 0; x < 32; x++)
		{
			ptr = (int)(0.5 + RATIO * (xx * 32 + x - VIDEO_HWIDTH) + VIDEO_HWIDTH);
			bits = bits << 1;
			if(ptr != prevptr)
				bits |= 1;
			prevptr = ptr;
		}
		blurzoomx[xx] = bits;
	}

	ty = (int)(0.5 + RATIO * (-VIDEO_HHEIGHT) + VIDEO_HHEIGHT);
	tx = (int)(0.5 + RATIO*  (-VIDEO_HWIDTH) + VIDEO_HWIDTH);
	xx = (int)(0.5 + RATIO *( buf_width - 1 - VIDEO_HWIDTH) + VIDEO_HWIDTH);
	blurzoomy[0] = ty * buf_width + tx;
	prevptr = ty * buf_width + xx;

	for(y = 1; y < buf_height; y++)
	{
		ty = (int)(0.5 + RATIO * (y - VIDEO_HHEIGHT) + VIDEO_HHEIGHT);
		blurzoomy[y] = ty * buf_width + tx - prevptr;
		prevptr = ty * buf_width + xx;
	}
}

void BlurZoomMain::make_palette()
{
	int i;

#define DELTA (255 / (COLORS / 2 - 1))

	for(i = 0; i < COLORS / 2; i++) 
	{
		palette_r[i] = i * DELTA;
		palette_g[i] = i * DELTA;
		palette_b[i] = i * DELTA;
	}

	for(i = 0; i < COLORS / 2; i++) 
	{
		palette_r[i + COLORS / 2] = (i * DELTA);
		palette_g[i + COLORS / 2] = (i * DELTA);
		palette_b[i + COLORS / 2] = 255;
	}
}



















int BlurZoomMain::start_realtime()
{
	buf_width_blocks = project_frame_w / 32;
	buf_width = buf_width_blocks * 32;
	buf_height = project_frame_h;
	buf_area = buf_width * buf_height;
	buf_margin_left = (project_frame_w - buf_width) / 2;
	buf_margin_right = project_frame_w - buf_width - buf_margin_left;
	blurzoombuf = new unsigned char[buf_area * 2];
	blurzoomx = new int[buf_width];
	blurzoomy = new int[buf_height];

	set_table();
	make_palette();
	
	bzero(blurzoombuf, buf_area * 2);
	
	
	background = new uint16_t[project_frame_w * project_frame_h];
	diff = new unsigned char[project_frame_w * project_frame_h];
	image_set_threshold_y(MAGIC_THRESHOLD);
	
	blurzoom_server = new BlurZoomServer(this, 1, 1);
	return 0;
}

int BlurZoomMain::stop_realtime()
{
	delete blurzoom_server;





	delete [] blurzoombuf;
	delete [] blurzoomx;
	delete [] blurzoomy;





	delete [] background;
	delete [] diff;




	return 0;
}

int BlurZoomMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();
	this->input_ptr = input_ptr;
	this->output_ptr = output_ptr;


	blurzoom_server->process_packages();

	return 0;
}

int BlurZoomMain::show_gui()
{
	load_configuration();
	thread = new BlurZoomThread(this);
	thread->start();
	return 0;
}

int BlurZoomMain::set_string()
{
	if(thread) thread->window->set_title(gui_string);
	return 0;
}

void BlurZoomMain::raise_window()
{
	if(thread)
	{
		thread->window->raise_window();
		thread->window->flush();
	}
}




BlurZoomServer::BlurZoomServer(BlurZoomMain *plugin, int total_clients, total_packages)
 : LoadServer(total_clients, get_total_packages())
{
	this->plugin = plugin;
}


LoadClient* BlurZoomServer::new_client() 
{
	return new BlurZoomClient(this);
}




LoadPackage* BlurZoomServer::new_package() 
{ 
	return new BlurZoomPackage; 
}



void BlurZoomServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		BlurZoomPackage *package = (BlurZoomPackage*)get_package(i);
		package->row1 = plugin->input_ptr->get_h() / get_total_packages() * i;
		package->row2 = package->row1 + plugin->input_ptr->get_h() / get_total_packages();
		if(i >= get_total_packages() - 1)
			package->row2 = plugin->input_ptr->get_h();
	}
}








BlurZoomClient::BlurZoomClient(BlurZoomServer *server)
 : LoadClient(server)
{
	this->plugin = server->plugin;
}









/* Background image is refreshed every frame */
#define IMAGE_BGSUBTRACT_UPDATE_Y(result, \
	input_rows,  \
	type,  \
	components) \
{ \
	int i; \
	int R, G, B; \
	type *p; \
	int16_t *q; \
	unsigned char *r; \
	int v; \
 \
	q = (int16_t *)background; \
	r = diff; \
 \
	for(i = 0; i < project_frame_h; i++)  \
	{ \
		p = input_rows[j]; \
 \
		for(j = 0; j < project_frame_w; j++) \
		{ \
			if(sizeof(type) == 2) \
			{ \
				R = p[0] >> (8 - 1); \
				G = p[1] >> (8 - 2); \
				B = p[2] >> 8; \
			} \
			else \
			{ \
				R = p[0] << 1; \
				G = p[1] << 2; \
				B = p[2]; \
			} \
 \
			v = (R + G + B) - (int)(*q); \
			*q = (int16_t)(R + G + B); \
			*r = ((v + y_threshold) >> 24) | ((y_threshold - v) >> 24); \
 \
			p += components; \
			q++; \
			r++; \
		} \
	} \
 \
	result = diff; \
}




#define BLURZOOM_FINAL(type, components)
{
	for(y = 0; y < h; y++) 
	{
		memcpy(output_rows[y], 
			input_rows[y], 
			buf_margin_left * plugin->input_ptr->get_bytes_per_pixel());


		for(x = 0; x < buf_width; x++)
		{
			for(c = 0; c < components; c++)
			{
				a = *src++ & 0xfefeff;
				b = palette[*p++];
				a += b;
				b = a & 0x1010100;
				*dest++ = a | (b - (b >> 8));
			}
		}

		memcpy(output_rows[y] + project_frame_w - buf_margin_right, 
			input_rows[y] + project_frame_w - buf_margin_right, 
			buf_margin_right * plugin->input_ptr->get_bytes_per_pixel());
	}
}



void BlurZoomClient::process_package(LoadPackage *package)
{
	BlurZoomPackage *local_package = (BlurZoomPackage*)package;
	unsigned char **input_rows = plugin->input_ptr->get_rows() + local_package->row1;
	unsigned char **output_rows = plugin->output_ptr->get_rows() + local_package->row1;
	int w = plugin->input_ptr->get_w();
	int h = local_package->row2 - local_package->row1;
	int x, y;
	int a, b, c;
	unsigned char *diff, *p;

	switch(plugin->input_ptr->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint8_t, 
				3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint8_t, 
				4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint16_t, 
				3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			IMAGE_BGSUBTRACT_UPDATE_Y(diff, 
				input_rows, 
				uint16_t, 
				4);
			break;
	}


	diff += buf_margin_left;
	p = blurzoombuf;



	for(y = 0; y < buf_height; y++)
	{
		for(x = 0; x < buf_width; x++)
		{
			p[x] |= diff[x] >> 3;
		}

		diff += w;
		p += buf_width;
	}


// Assembly language only
	blurzoomcore();


	p = blurzoombuf;



	switch(plugin->input_ptr->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			BLURZOOM_FINAL(uint8_t, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			BLURZOOM_FINAL(uint8_t, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			BLURZOOM_FINAL(uint16_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			BLURZOOM_FINAL(uint16_t, 4);
			break;
	}




}



BlurZoomPackage::BlurZoomPackage()
{
}



