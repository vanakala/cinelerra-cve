#include "clip.h"
#include "colormodels.h"
#include "filexml.h"
#include "picon_png.h"
#include "sharpen.h"
#include "sharpenwindow.h"

#include <stdio.h>
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

REGISTER_PLUGIN(SharpenMain)







SharpenConfig::SharpenConfig()
{
	horizontal = 0;
	interlace = 0;
	sharpness = 50;
	luminance = 0;
}

void SharpenConfig::copy_from(SharpenConfig &that)
{
	horizontal = that.horizontal;
	interlace = that.interlace;
	sharpness = that.sharpness;
	luminance = that.luminance;
}

int SharpenConfig::equivalent(SharpenConfig &that)
{
	return horizontal == that.horizontal &&
		interlace == that.interlace &&
		EQUIV(sharpness, that.sharpness) &&
		luminance == that.luminance;
}

void SharpenConfig::interpolate(SharpenConfig &prev, 
	SharpenConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->sharpness = prev.sharpness * prev_scale + next.sharpness * next_scale;
	this->interlace = prev.interlace;
	this->horizontal = prev.horizontal;
	this->luminance = prev.luminance;
}











SharpenMain::SharpenMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
}

SharpenMain::~SharpenMain()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(engine)
	{
		for(int i = 0; i < total_engines; i++)
		{
			delete engine[i];
		}
		delete engine;
	}
}

SHOW_GUI_MACRO(SharpenMain, SharpenThread)

SET_STRING_MACRO(SharpenMain)

RAISE_WINDOW_MACRO(SharpenMain)

NEW_PICON_MACRO(SharpenMain)

LOAD_CONFIGURATION_MACRO(SharpenMain, SharpenConfig)

char* SharpenMain::plugin_title() { return _("Sharpen"); }
int SharpenMain::is_realtime() { return 1; }



int SharpenMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	int i, j, k;
	output = output_ptr;
	input = input_ptr;

	load_configuration();
	if(!engine)
	{

		total_engines = PluginClient::smp > 1 ? 2 : 1;
		engine = new SharpenEngine*[total_engines];
		for(int i = 0; i < total_engines; i++)
		{
			engine[i] = new SharpenEngine(this);
			engine[i]->start();
		}
	}

	get_luts(pos_lut, neg_lut, input_ptr->get_color_model());

	if(config.sharpness != 0)
	{
// Arm first row
		row_step = (config.interlace /* || config.horizontal */) ? 2 : 1;

		for(j = 0; j < row_step; j += total_engines)
		{
			for(k = 0; k < total_engines && k + j < row_step; k++)
			{
				engine[k]->start_process_frame(input_ptr, input_ptr, k + j);
			}
			for(k = 0; k < total_engines && k + j < row_step; k++)
			{
				engine[k]->wait_process_frame();
			}
		}
	}
	else
	if(input_ptr->get_rows()[0] != output_ptr->get_rows()[0])
	{
		output_ptr->copy_from(input_ptr);
	}
	return 0;
}

void SharpenMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->sharpen_slider->update((int)config.sharpness);
		thread->window->sharpen_interlace->update(config.interlace);
		thread->window->sharpen_horizontal->update(config.horizontal);
		thread->window->sharpen_luminance->update(config.luminance);
		thread->window->unlock_window();
	}
}

int SharpenMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%ssharpen.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.sharpness = defaults->get("SHARPNESS", config.sharpness);
	config.interlace = defaults->get("INTERLACE", config.interlace);
	config.horizontal = defaults->get("HORIZONTAL", config.horizontal);
	config.luminance = defaults->get("LUMINANCE", config.luminance);
//printf("SharpenMain::load_defaults 1 %f %d %d %d\n", sharpness, interlace, horizontal, luminance);
	return 0;
}

int SharpenMain::save_defaults()
{
//printf("SharpenMain::save_defaults 1 %f %d %d %d\n", sharpness, interlace, horizontal, luminance);
	defaults->update("SHARPNESS", config.sharpness);
	defaults->update("INTERLACE", config.interlace);
	defaults->update("HORIZONTAL", config.horizontal);
	defaults->update("LUMINANCE", config.luminance);
	defaults->save();
	return 0;
}




int SharpenMain::get_luts(int *pos_lut, int *neg_lut, int color_model)
{
	int i, inv_sharpness, vmax;

	vmax = cmodel_calculate_max(color_model);

	inv_sharpness = (int)(100 - config.sharpness);
	if(config.horizontal) inv_sharpness /= 2;
	if(inv_sharpness < 1) inv_sharpness = 1;

	for(i = 0; i < vmax + 1; i++)
	{
		pos_lut[i] = 800 * i / inv_sharpness;
		neg_lut[i] = (4 + pos_lut[i] - (i << 3)) >> 3;
	}

	return 0;
}

void SharpenMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("SHARPNESS");
	output.tag.set_property("VALUE", config.sharpness);
	output.append_tag();

	if(config.interlace)
	{
		output.tag.set_title("INTERLACE");
		output.append_tag();
	}

	if(config.horizontal)
	{
		output.tag.set_title("HORIZONTAL");
		output.append_tag();
	}

	if(config.luminance)
	{
		output.tag.set_title("LUMINANCE");
		output.append_tag();
	}
	output.terminate_string();
}

void SharpenMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	int new_interlace = 0;
	int new_horizontal = 0;
	int new_luminance = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SHARPNESS"))
			{
				config.sharpness = input.tag.get_property("VALUE", config.sharpness);
//printf("SharpenMain::read_data %f\n", sharpness);
			}
			else
			if(input.tag.title_is("INTERLACE"))
			{
				new_interlace = 1;
			}
			else
			if(input.tag.title_is("HORIZONTAL"))
			{
				new_horizontal = 1;
			}
			else
			if(input.tag.title_is("LUMINANCE"))
			{
				new_luminance = 1;
			}
		}
	}

	config.interlace = new_interlace;
	config.horizontal = new_horizontal;
	config.luminance = new_luminance;

	if(config.sharpness > MAXSHARPNESS) 
		config.sharpness = MAXSHARPNESS;
	else
		if(config.sharpness < 0) config.sharpness = 0;
}




SharpenEngine::SharpenEngine(SharpenMain *plugin)
 : Thread()
{
	this->plugin = plugin;
	last_frame = 0;
	for(int i = 0; i < 4; i++)
	{
		neg_rows[i] = new int[plugin->input->get_w() * 4];
	}
	input_lock.lock();
	output_lock.lock();
	set_synchronous(1);
}

SharpenEngine::~SharpenEngine()
{
	last_frame = 1;
	input_lock.unlock();
	Thread::join();

	for(int i = 0; i < 4; i++)
	{
		delete [] neg_rows[i];
	}
}

int SharpenEngine::start_process_frame(VFrame *output, VFrame *input, int field)
{
	this->output = output;
	this->input = input;
	this->field = field;
	input_lock.unlock();
	return 0;
}

int SharpenEngine::wait_process_frame()
{
	output_lock.lock();
	return 0;
}

#define FILTER(components, vmax, wordsize) \
{ \
	int *pos_lut = plugin->pos_lut; \
 \
/* Skip first pixel in row */ \
	memcpy(dst, src, components * wordsize); \
	dst += components; \
	src += components; \
 \
	w -= 2; \
 \
	while(w > 0) \
	{ \
		long pixel; \
		pixel = (long)pos_lut[src[0]] -  \
			(long)neg0[-components] -  \
			(long)neg0[0] -  \
			(long)neg0[components] -  \
			(long)neg1[-components] -  \
			(long)neg1[components] -  \
			(long)neg2[-components] -  \
			(long)neg2[0] -  \
			(long)neg2[components]; \
		pixel = (pixel + 4) >> 3; \
		if(pixel < 0) dst[0] = 0; \
		else \
		if(pixel > vmax) dst[0] = vmax; \
		else \
		dst[0] = pixel; \
 \
		pixel = (long)pos_lut[src[1]] -  \
			(long)neg0[-components + 1] -  \
			(long)neg0[1] -  \
			(long)neg0[components + 1] -  \
			(long)neg1[-components + 1] -  \
			(long)neg1[components + 1] -  \
			(long)neg2[-components + 1] -  \
			(long)neg2[1] -  \
			(long)neg2[components + 1]; \
		pixel = (pixel + 4) >> 3; \
		if(pixel < 0) dst[1] = 0; \
		else \
		if(pixel > vmax) dst[1] = vmax; \
		else \
		dst[1] = pixel; \
 \
		pixel = (long)pos_lut[src[2]] -  \
			(long)neg0[-components + 2] -  \
			(long)neg0[2] -  \
			(long)neg0[components + 2] -  \
			(long)neg1[-components + 2] -  \
			(long)neg1[components + 2] -  \
			(long)neg2[-components + 2] -  \
			(long)neg2[2] -  \
			(long)neg2[components + 2]; \
		pixel = (pixel + 4) >> 3; \
		if(pixel < 0) dst[2] = 0; \
		else \
		if(pixel > vmax) dst[2] = vmax; \
		else \
		dst[2] = pixel; \
 \
 		if(components == 4) \
			dst[3] = src[3]; \
 \
		src += components; \
		dst += components; \
 \
		neg0 += components; \
		neg1 += components; \
		neg2 += components; \
		w--; \
	} \
 \
/* Skip last pixel in row */ \
	memcpy(dst, src, components * wordsize); \
}

void SharpenEngine::filter(int components,
	int wordsize,
	int vmax,
	int w, 
	u_int16_t *src, 
	u_int16_t *dst,
	int *neg0, 
	int *neg1, 
	int *neg2)
{
	FILTER(components, vmax, wordsize);
}

void SharpenEngine::filter(int components,
	int wordsize,
	int vmax,
	int w, 
	unsigned char *src, 
	unsigned char *dst,
	int *neg0, 
	int *neg1, 
	int *neg2)
{
	FILTER(components, vmax, wordsize);
}







#define SHARPEN(components, wordsize, wordtype, vmax) \
{ \
	int count, row; \
	unsigned char **input_rows, **output_rows; \
	int w = plugin->input->get_w(); \
	int h = plugin->input->get_h(); \
 \
	input_rows = input->get_rows(); \
	output_rows = output->get_rows(); \
	src_rows[0] = input_rows[field]; \
	src_rows[1] = input_rows[field]; \
	src_rows[2] = input_rows[field]; \
	src_rows[3] = input_rows[field]; \
 \
	for(int j = 0; j < w; j++) \
	{ \
		for(int k = 0; k < components; k++) \
			neg_rows[0][j * components + k] = plugin->neg_lut[((wordtype*)src_rows[0])[j * components + k]]; \
	} \
 \
	row = 1; \
	count = 1; \
 \
	for(int i = field; i < h; i += plugin->row_step) \
	{ \
		if((i + plugin->row_step) < h) \
		{ \
			if(count >= 3) count--; \
/* Arm next row */ \
			src_rows[row] = input_rows[i + plugin->row_step]; \
			for(int k = 0; k < w; k++) \
			{ \
				for(int j = 0; j < components; j++) \
					neg_rows[row][k * components + j] = plugin->neg_lut[((wordtype*)src_rows[row])[k * components + j]]; \
			} \
 \
			count++; \
			row = (row + 1) & 3; \
		} \
		else \
		{ \
			count--; \
		} \
 \
		dst_row = output_rows[i]; \
		if(count == 3) \
		{ \
/* Do the filter */ \
			if(plugin->config.horizontal) \
				filter(components, \
					wordsize, \
					vmax, \
					w,  \
					(wordtype*)src_rows[(row + 2) & 3],  \
					(wordtype*)dst_row, \
					neg_rows[(row + 2) & 3] + components, \
					neg_rows[(row + 2) & 3] + components, \
					neg_rows[(row + 2) & 3] + components); \
			else \
				filter(components, \
					wordsize, \
					vmax, \
					w,  \
					(wordtype*)src_rows[(row + 2) & 3],  \
					(wordtype*)dst_row, \
					neg_rows[(row + 1) & 3] + components, \
					neg_rows[(row + 2) & 3] + components, \
					neg_rows[(row + 3) & 3] + components); \
		} \
		else  \
		if(count == 2) \
		{ \
			if(i == 0) \
				memcpy(dst_row, src_rows[0], w * components * wordsize); \
			else \
				memcpy(dst_row, src_rows[2], w * components * wordsize); \
		} \
	} \
}

void SharpenEngine::sharpen_888()
{
	SHARPEN(3, 1, unsigned char, 0xff);
}

void SharpenEngine::sharpen_8888()
{
	SHARPEN(4, 1, unsigned char, 0xff);
}

void SharpenEngine::sharpen_161616()
{
	SHARPEN(3, 2, u_int16_t, 0xffff);
}

void SharpenEngine::sharpen_16161616()
{
	SHARPEN(4, 2, u_int16_t, 0xffff);
}


void SharpenEngine::run()
{
	while(1)
	{
		input_lock.lock();
		if(last_frame)
		{
			output_lock.unlock();
			return;
		}


		switch(input->get_color_model())
		{
			case BC_RGB888:
			case BC_YUV888:
				sharpen_888();
				break;
			
			case BC_RGBA8888:
			case BC_YUVA8888:
				sharpen_8888();
				break;
			
			case BC_RGB161616:
			case BC_YUV161616:
				sharpen_161616();
				break;
			
			case BC_RGBA16161616:
			case BC_YUVA16161616:
				sharpen_16161616();
				break;
		}

		output_lock.unlock();
	}
}

