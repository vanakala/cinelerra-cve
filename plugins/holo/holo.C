#include "clip.h"
#include "colormodels.h"
#include "filexml.h"
#include "picon_png.h"
#include "holo.h"
#include "holowindow.h"
#include "effecttv.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

PluginClient* new_plugin(PluginServer *server)
{
	return new HoloMain(server);
}











HoloConfig::HoloConfig()
{
	threshold = 40;
	recycle = 1.0;
}





HoloMain::HoloMain(PluginServer *server)
 : PluginVClient(server)
{
	thread = 0;
	defaults = 0;
	effecttv = 0;
	bgimage = 0;
	do_reconfigure = 1;
	load_defaults();
}

HoloMain::~HoloMain()
{
	PLUGIN_DESTRUCTOR_MACRO


	if(effecttv)
	{
		delete holo_server;
		delete effecttv;
	}

	if(bgimage)
		delete bgimage;
}

char* HoloMain::plugin_title() { return "HolographicTV"; }
int HoloMain::is_realtime() { return 1; }

VFrame* HoloMain::new_picon()
{
	return new VFrame(picon_png);
}

int HoloMain::load_defaults()
{
	return 0;
}

int HoloMain::save_defaults()
{
	return 0;
}

void HoloMain::load_configuration()
{
}


void HoloMain::save_data(KeyFrame *keyframe)
{
}

void HoloMain::read_data(KeyFrame *keyframe)
{
}

void HoloMain::reconfigure()
{
	do_reconfigure = 0;

	effecttv->image_set_threshold_y(config.threshold);
}


#define ADD_FRAMES(type, components) \
{ \
	type **input_rows = (type**)input->get_rows(); \
	type **output_rows = (type**)output->get_rows(); \
	int w = input->get_w(); \
	int h = input->get_h(); \
 \
	for(int i = 0; i < h; i++) \
	{ \
		type *output_row = (type*)output_rows[i]; \
		type *input_row = (type*)input_rows[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			for(int k = 0; k < components; k++) \
			{ \
				*output_row = (*input_row & *output_row) +  \
					((*input_row ^ *output_row) >> 1); \
				output_row++; \
				input_row++; \
			} \
		} \
	} \
}


// Add input to output and store result in output
void HoloMain::add_frames(VFrame *output, VFrame *input)
{
	switch(output->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			ADD_FRAMES(uint8_t, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			ADD_FRAMES(uint8_t, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			ADD_FRAMES(uint16_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			ADD_FRAMES(uint16_t, 4);
			break;
	}
}

void HoloMain::set_background()
{
/*
 * grab 4 frames and composite them to get a quality background image
 */
total = 0;

	switch(total)
	{
		case 0:
/* step 1: grab frame-1 to buffer-1 */
// 			tmp = new VFrame(0, 
// 				input_ptr->get_w(), 
// 				input_ptr->get_h(),
// 				project_color_model);
			bgimage->copy_from(input_ptr);
			break;

		case 1:
/* step 2: add frame-2 to buffer-1 */
			add_frames(bgimage, input_ptr);
			break;

		case 2:
/* step 3: grab frame-3 to buffer-2 */
			tmp->copy_from(input_ptr);
			break;

		case 3:
/* step 4: add frame-4 to buffer-2 */
			add_frames(tmp, input_ptr);



/* step 5: add buffer-3 to buffer-1 */
			add_frames(bgimage, tmp);

			effecttv->image_bgset_y(bgimage);


			delete tmp;
			break;
	}
}


int HoloMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
//printf("HoloMain::process_realtime 1\n");
	this->input_ptr = input_ptr;
	this->output_ptr = output_ptr;




	load_configuration();

//printf("HoloMain::process_realtime 1\n");


	if(do_reconfigure)
	{
		if(!effecttv)
		{
			effecttv = new EffectTV(input_ptr->get_w(), input_ptr->get_h());
			bgimage = new VFrame(0, 
				input_ptr->get_w(), 
				input_ptr->get_h(), 
				input_ptr->get_color_model());

			for(int i = 0; i < 256; i++)
			{
				noisepattern[i] = (i * i * i / 40000)* i / 256;
			}

			holo_server = new HoloServer(this, 1, 1);
		}

		reconfigure();
	}

//printf("HoloMain::process_realtime 1\n");
	set_background();
//printf("HoloMain::process_realtime 1\n");
	
	holo_server->process_packages();
//printf("HoloMain::process_realtime 1\n");
	
	total++;
	if(total >= config.recycle * project_frame_rate)
		total = 0;
//printf("HoloMain::process_realtime 2\n");

	return 0;
}

int HoloMain::show_gui()
{
	load_configuration();
	thread = new HoloThread(this);
	thread->start();
	return 0;
}

int HoloMain::set_string()
{
	if(thread) thread->window->set_title(gui_string);
	return 0;
}

void HoloMain::raise_window()
{
	if(thread)
	{
		thread->window->raise_window();
		thread->window->flush();
	}
}




HoloServer::HoloServer(HoloMain *plugin, int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
// : LoadServer(1, 1)
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
	for(int i = 0; i < total_packages; i++)
	{
		HoloPackage *package = (HoloPackage*)packages[i];
		package->row1 = plugin->input_ptr->get_h() / total_packages * i;
		package->row2 = package->row1 + plugin->input_ptr->get_h() / total_packages;
		if(i >= total_packages - 1)
			package->row2 = plugin->input_ptr->get_h();
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
	int x, y;
	int sx, sy;
	HoloPackage *local_package = (HoloPackage*)package;
	unsigned char **input_rows = plugin->input_ptr->get_rows() + local_package->row1;
	unsigned char **output_rows = plugin->output_ptr->get_rows() + local_package->row1;
	int width = plugin->input_ptr->get_w();
	int height = local_package->row2 - local_package->row1;
	unsigned char *diff;
	uint32_t s, t;
	int r, g, b;

	diff = plugin->effecttv->image_diff_filter(plugin->effecttv->image_bgsubtract_y(input_rows, 
		plugin->input_ptr->get_color_model()));

	diff += width;
	output_rows++;
	input_rows++;



#define STORE_PIXEL(type, components, dest, src) \
if(sizeof(type) == 2) \
{ \
	dest = (src[0] << 8) | \
		(src[1]) | \
		(src[2]) >> 8; \
} \
else \
{ \
	dest = (src[0] << 16) | \
		(src[1] << 8) | \
		src[2]; \
}




#define HOLO_CORE(type, components) \
	for(y = 1; y < height - 1; y++) \
	{ \
		type *src = (type*)input_rows[y]; \
		type *bg = (type*)plugin->bgimage->get_rows()[y]; \
		type *dest = (type*)output_rows[y]; \
 \
 \
 \
		if(((y + phase) & 0x7f) < 0x58)  \
		{ \
			for(x = 0 ; x < width; x++)  \
			{ \
				if(*diff) \
				{ \
					STORE_PIXEL(type, components, s, src); \
 \
					t = (s & 0xff) +  \
						((s & 0xff00) >> 7) +  \
						((s & 0xff0000) >> 16); \
					t += plugin->noisepattern[EffectTV::fastrand() >> 24]; \
 \
					r = ((s & 0xff0000) >> 17) + t; \
					g = ((s & 0xff00) >> 8) + t; \
					b = (s & 0xff) + t; \
 \
					r = (r >> 1) - 100; \
					g = (g >> 1) - 100; \
					b = b >> 2; \
 \
					if(r < 20) r = 20; \
					if(g < 20) g = 20; \
 \
					STORE_PIXEL(type, components, s, bg); \
 \
					r += (s & 0xff0000) >> 17; \
					g += (s & 0xff00) >> 9; \
					b += ((s & 0xff) >> 1) + 40; \
 \
					if(r > 255) r = 255; \
					if(g > 255) g = 255; \
					if(b > 255) b = 255; \
 \
					dest[0] = r; \
					dest[1] = g; \
					dest[2] = b; \
					if(components == 4) dest[3] = src[3]; \
				}  \
				else  \
				{ \
					dest[0] = bg[0]; \
					dest[1] = bg[1]; \
					dest[2] = bg[2]; \
					if(components == 4) dest[3] = bg[3]; \
				} \
 \
				diff++; \
				src += components; \
				dest += components; \
				bg += components; \
			} \
		}  \
		else  \
		{ \
			for(x = 0; x < width; x++) \
			{ \
				if(*diff) \
				{ \
					STORE_PIXEL(type, components, s, src); \
 \
 \
					t = (s & 0xff) + ((s & 0xff00) >> 6) + ((s & 0xff0000) >> 16); \
					t += plugin->noisepattern[EffectTV::fastrand() >> 24]; \
 \
					r = ((s & 0xff0000) >> 16) + t; \
					g = ((s & 0xff00) >> 8) + t; \
					b = (s & 0xff) + t; \
 \
					r = (r >> 1) - 100; \
					g = (g >> 1) - 100; \
					b = b >> 2; \
 \
					if(r < 0) r = 0; \
					if(g < 0) g = 0; \
 \
					STORE_PIXEL(type, components, s, bg); \
 \
					r += ((s & 0xff0000) >> 17) + 10; \
					g += ((s & 0xff00) >> 9) + 10; \
					b += ((s & 0xff) >> 1) + 40; \
 \
					if(r > 255) r = 255; \
					if(g > 255) g = 255; \
					if(b > 255) b = 255; \
 \
					dest[0] = r; \
					dest[1] = g; \
					dest[2] = b; \
					if(components == 4) dest[3] = src[3]; \
				}  \
				else  \
				{ \
					dest[0] = bg[0]; \
					dest[1] = bg[1]; \
					dest[2] = bg[2]; \
					if(components == 4) dest[3] = bg[3]; \
				} \
 \
				diff++; \
				src += components; \
				dest += components; \
				bg += components; \
			} \
		} \
	}




	switch(plugin->input_ptr->get_color_model())
	{
		case BC_RGB888:
		case BC_YUV888:
			HOLO_CORE(uint8_t, 3);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			HOLO_CORE(uint8_t, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			HOLO_CORE(uint16_t, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			HOLO_CORE(uint16_t, 4);
			break;
	}



	phase -= 37;
}



HoloPackage::HoloPackage()
{
}



