#include "clip.h"
#include "colormodels.h"
#include "effecttv.h"
#include "filexml.h"
#include "picon_png.h"
#include "burn.h"
#include "burnwindow.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

PluginClient* new_plugin(PluginServer *server)
{
	return new BurnMain(server);
}












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
	PLUGIN_DESTRUCTOR_MACRO

	if(buffer) delete [] buffer;
	if(burn_server) delete burn_server;
	if(effecttv) delete effecttv;
}

char* BurnMain::plugin_title() { return N_("BurningTV"); }
int BurnMain::is_realtime() { return 1; }


NEW_PICON_MACRO(BurnMain)
SHOW_GUI_MACRO(BurnMain, BurnThread)
SET_STRING_MACRO(BurnMain)
RAISE_WINDOW_MACRO(BurnMain)

int BurnMain::load_defaults()
{
	return 0;
}

int BurnMain::save_defaults()
{
	return 0;
}

void BurnMain::load_configuration()
{
//printf("BurnMain::load_configuration %d\n", source_position);
}


void BurnMain::save_data(KeyFrame *keyframe)
{
}

void BurnMain::read_data(KeyFrame *keyframe)
{
}



#define MAXCOLOR 120

static void HSItoRGB(double H, 
	double S, 
	double I, 
	int *r, 
	int *g, 
	int *b)
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


void BurnMain::make_palette()
{
	int i, r, g, b;

	for(i = 0; i < MAXCOLOR; i++)
	{
		HSItoRGB(4.6 - 1.5 * i / MAXCOLOR, 
			(double)i / MAXCOLOR, 
			(double)i / MAXCOLOR,  
			&r, 
			&g, 
			&b);
		palette[0][i] = r;
		palette[1][i] = g;
		palette[2][i] = b;
//printf("BurnMain::make_palette %d\n", palette[0][i]);
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




int BurnMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	this->input_ptr = input_ptr;
	this->output_ptr = output_ptr;

	load_configuration();

	if(!burn_server)
	{
		effecttv = new EffectTV(input_ptr->get_w(), input_ptr->get_h());
		buffer = (unsigned char *)new unsigned char[input_ptr->get_w() * input_ptr->get_h()];
		make_palette();

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
//	if(total >= config.recycle * project_frame_rate) total = 0;
	return 0;
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
	for(int i = 0; i < total_packages; i++)
	{
		BurnPackage *package = (BurnPackage*)packages[i];
		package->row1 = plugin->input_ptr->get_h() / total_packages * i;
		package->row2 = package->row1 + plugin->input_ptr->get_h() / total_packages;
		if(i >= total_packages - 1)
			package->row2 = plugin->input_ptr->get_h();
	}
}








BurnClient::BurnClient(BurnServer *server)
 : LoadClient(server)
{
	this->plugin = server->plugin;
}









#define BURN(type, components) \
{ \
	i = 1; \
	for(y = 0; y < height; y++)  \
	{ \
		for(x = 1; x < width - 1; x++)  \
		{ \
			for(c = 0; c < components; c++) \
			{ \
				if(c == 3) \
					output_rows[0][i * components + c] = input_rows[0][i * components + c]; \
				else \
				if(sizeof(type) == 2) \
				{ \
					a = (input_rows[0][i * components + c] & 0xffff) >> 8; \
					b = plugin->palette[c][plugin->buffer[i]] & 0xff; \
					a += b; \
					b = a & 0x10000; \
					output_rows[0][i * components + c] = a | (b - (b >> 16)); \
				} \
				else \
				{ \
					a = input_rows[0][i * components + c] & 0xff; \
					b = plugin->palette[c][plugin->buffer[i]] & 0xff; \
					a += b; \
					b = a & 0x100; \
					output_rows[0][i * components + c] = a | (b - (b >> 8)); \
				} \
			} \
			i++; \
		} \
		i += 2; \
	} \
}




void BurnClient::process_package(LoadPackage *package)
{
	BurnPackage *local_package = (BurnPackage*)package;
	unsigned char **input_rows = plugin->input_ptr->get_rows() + local_package->row1;
	unsigned char **output_rows = plugin->output_ptr->get_rows() + local_package->row1;
	int width = plugin->input_ptr->get_w();
	int height = local_package->row2 - local_package->row1;
	unsigned char *diff;
	int pitch = width * plugin->input_ptr->get_bytes_per_pixel();
	int i, x, y;
	unsigned int v, w;
	int a, b, c;


	diff = plugin->effecttv->image_bgsubtract_y(input_rows, 
		plugin->input_ptr->get_color_model());

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
		case BC_YUV888:
			BURN(uint8_t, 3);
			break;

		case BC_RGBA8888:
		case BC_YUVA8888:
			BURN(uint8_t, 4);
			break;

		case BC_RGB161616:
		case BC_YUV161616:
			BURN(uint16_t, 3);
			break;

		case BC_RGBA16161616:
		case BC_YUVA16161616:
			BURN(uint16_t, 4);
			break;
	}


}



BurnPackage::BurnPackage()
{
}



