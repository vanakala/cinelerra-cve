#include "filexml.h"
#include "colorbalance.h"
#include "defaults.h"
#include "picon_png.h"

#include <stdio.h>
#include <string.h>

#define SQR(a) ((a) * (a))

REGISTER_PLUGIN(ColorBalanceMain)



ColorBalanceConfig::ColorBalanceConfig()
{
	cyan = 0;
	magenta = 0;
	yellow = 0;
	lock_params = 0;
    preserve = 0;
}

int ColorBalanceConfig::equivalent(ColorBalanceConfig &that)
{
	return (cyan == that.cyan && 
		magenta == that.magenta && 
		yellow == that.yellow && 
		lock_params == that.lock_params && 
    	preserve == that.preserve);
}

void ColorBalanceConfig::copy_from(ColorBalanceConfig &that)
{
	cyan = that.cyan;
	magenta = that.magenta;
	yellow = that.yellow;
	lock_params = that.lock_params;
    preserve = that.preserve;
}

void ColorBalanceConfig::interpolate(ColorBalanceConfig &prev, 
	ColorBalanceConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->cyan = prev.cyan * prev_scale + next.cyan * next_scale;
	this->magenta = prev.magenta * prev_scale + next.magenta * next_scale;
	this->yellow = prev.yellow * prev_scale + next.yellow * next_scale;
}










ColorBalanceEngine::ColorBalanceEngine(ColorBalanceMain *plugin)
 : Thread()
{
	this->plugin = plugin;
	last_frame = 0;
	input_lock.lock();
	output_lock.lock();
	set_synchronous(1);
}

ColorBalanceEngine::~ColorBalanceEngine()
{
	last_frame = 1;
	input_lock.unlock();
	Thread::join();
}


int ColorBalanceEngine::start_process_frame(VFrame *output, VFrame *input, int row_start, int row_end)
{
	this->output = output;
	this->input = input;
	this->row_start = row_start;
	this->row_end = row_end;
	input_lock.unlock();
	return 0;
}


int ColorBalanceEngine::wait_process_frame()
{
	output_lock.lock();
	return 0;
}


void ColorBalanceEngine::run()
{
	while(1)
	{
		input_lock.lock();
		if(last_frame)
		{
			output_lock.unlock();
			return;
		}
		
#define PROCESS(yuvtorgb,  \
	rgbtoyuv,  \
	r_lookup,  \
	g_lookup,  \
	b_lookup,  \
	type,  \
	max,  \
	components,  \
	do_yuv) \
{ \
	int i, j, k; \
	int y, cb, cr, r, g, b, r_n, g_n, b_n; \
    float h, s, v, h_old, s_old, r_f, g_f, b_f; \
	type **input_rows, **output_rows; \
	input_rows = (type**)input->get_rows(); \
	output_rows = (type**)output->get_rows(); \
 \
	for(j = row_start; j < row_end; j++) \
	{ \
		for(k = 0; k < input->get_w() * components; k += components) \
		{ \
			if(do_yuv) \
			{ \
				y = input_rows[j][k]; \
				cb = input_rows[j][k + 1]; \
				cr = input_rows[j][k + 2]; \
				yuvtorgb(r, g, b, y, cb, cr); \
			} \
			else \
			{ \
            	r = input_rows[j][k]; \
            	g = input_rows[j][k + 1]; \
            	b = input_rows[j][k + 2]; \
			} \
 \
            r_n = plugin->r_lookup[r]; \
            g_n = plugin->g_lookup[g]; \
            b_n = plugin->b_lookup[b]; \
 \
			if(plugin->config.preserve) \
            { \
				HSV::rgb_to_hsv((float)r_n, (float)g_n, (float)b_n, h, s, v); \
				HSV::rgb_to_hsv((float)r, (float)g, (float)b, h_old, s_old, v); \
                HSV::hsv_to_rgb(r_f, g_f, b_f, h, s, v); \
                r = (type)r_f; \
                g = (type)g_f; \
                b = (type)b_f; \
			} \
            else \
            { \
                r = r_n; \
                g = g_n; \
                b = b_n; \
			} \
 \
			if(do_yuv) \
			{ \
				rgbtoyuv(CLAMP(r, 0, max), CLAMP(g, 0, max), CLAMP(b, 0, max), y, cb, cr); \
                output_rows[j][k] = y; \
                output_rows[j][k + 1] = cb; \
                output_rows[j][k + 2] = cr; \
			} \
			else \
			{ \
                output_rows[j][k] = CLAMP(r, 0, max); \
                output_rows[j][k + 1] = CLAMP(g, 0, max); \
                output_rows[j][k + 2] = CLAMP(b, 0, max); \
			} \
		} \
	} \
}

		switch(input->get_color_model())
		{
			case BC_RGB888:
				PROCESS(yuv.yuv_to_rgb_8, 
					yuv.rgb_to_yuv_8, 
					r_lookup_8, 
					g_lookup_8, 
					b_lookup_8, 
					unsigned char, 
					0xff, 
					3,
					0);
				break;

			case BC_YUV888:
				PROCESS(yuv.yuv_to_rgb_8, 
					yuv.rgb_to_yuv_8, 
					r_lookup_8, 
					g_lookup_8, 
					b_lookup_8, 
					unsigned char, 
					0xff, 
					3,
					1);
				break;
			
			case BC_RGBA8888:
				PROCESS(yuv.yuv_to_rgb_8, 
					yuv.rgb_to_yuv_8, 
					r_lookup_8, 
					g_lookup_8, 
					b_lookup_8, 
					unsigned char, 
					0xff, 
					4,
					0);
				break;

			case BC_YUVA8888:
				PROCESS(yuv.yuv_to_rgb_8, 
					yuv.rgb_to_yuv_8, 
					r_lookup_8, 
					g_lookup_8, 
					b_lookup_8, 
					unsigned char, 
					0xff, 
					4,
					1);
				break;
			
			case BC_RGB161616:
				PROCESS(yuv.yuv_to_rgb_16, 
					yuv.rgb_to_yuv_16, 
					r_lookup_16, 
					g_lookup_16, 
					b_lookup_16, 
					u_int16_t, 
					0xffff, 
					3,
					0);
				break;

			case BC_YUV161616:
				PROCESS(yuv.yuv_to_rgb_16, 
					yuv.rgb_to_yuv_16, 
					r_lookup_16, 
					g_lookup_16, 
					b_lookup_16, 
					u_int16_t, 
					0xffff, 
					3,
					1);
				break;

			case BC_RGBA16161616:
				PROCESS(yuv.yuv_to_rgb_16, 
					yuv.rgb_to_yuv_16, 
					r_lookup_16, 
					g_lookup_16, 
					b_lookup_16, 
					u_int16_t, 
					0xffff, 
					4,
					0);
				break;

			case BC_YUVA16161616:
				PROCESS(yuv.yuv_to_rgb_16, 
					yuv.rgb_to_yuv_16, 
					r_lookup_16, 
					g_lookup_16, 
					b_lookup_16, 
					u_int16_t, 
					0xffff, 
					4,
					1);
				break;
		}

		
		
		output_lock.unlock();
	}
}




ColorBalanceMain::ColorBalanceMain(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ColorBalanceMain::~ColorBalanceMain()
{
	PLUGIN_DESTRUCTOR_MACRO


	if(engine)
	{
		for(int i = 0; i < total_engines; i++)
		{
			delete engine[i];
		}
		delete [] engine;
	}
}

char* ColorBalanceMain::plugin_title() { return "Color Balance"; }
int ColorBalanceMain::is_realtime() { return 1; }


int ColorBalanceMain::reconfigure()
{
	int r_n, g_n, b_n;
    double *cyan_red_transfer;
    double *magenta_green_transfer;
    double *yellow_blue_transfer;


#define RECONFIGURE(highlights_add, highlights_sub, r_lookup, g_lookup, b_lookup, max) \
    cyan_red_transfer = config.cyan > 0 ? highlights_add : highlights_sub; \
    magenta_green_transfer = config.magenta > 0 ? highlights_add : highlights_sub; \
    yellow_blue_transfer = config.yellow > 0 ? highlights_add : highlights_sub; \
	for(int i = 0; i < max; i++) \
    { \
    	r_n = g_n = b_n = i; \
	    r_n += (int)(config.cyan / 100 * max * cyan_red_transfer[r_n]); \
	    g_n += (int)(config.magenta / 100 * max  * magenta_green_transfer[g_n]); \
	    b_n += (int)(config.yellow / 100 * max  * yellow_blue_transfer[b_n]); \
 \
        if(r_n > max) r_n = max; \
        else \
		if(r_n < 0) r_n = 0; \
        if(g_n > max) g_n = max; \
        else \
        if(g_n < 0) g_n = 0; \
        if(b_n > max) b_n = max; \
        else \
        if(b_n < 0) b_n = 0; \
 \
        r_lookup[i] = r_n; \
        g_lookup[i] = g_n; \
        b_lookup[i] = b_n; \
    }


	RECONFIGURE(highlights_add_8, highlights_sub_8, r_lookup_8, g_lookup_8, b_lookup_8, 0xff);
	RECONFIGURE(highlights_add_16, highlights_sub_16, r_lookup_16, g_lookup_16, b_lookup_16, 0xffff);
	
	return 0;
}

int ColorBalanceMain::test_boundary(float &value)
{
	if(value < -100) value = -100;
    if(value > 100) value = 100;
	return 0;
}

int ColorBalanceMain::synchronize_params(ColorBalanceSlider *slider, float difference)
{
	if(thread && config.lock_params)
    {
	    if(slider != thread->window->cyan)
        {
        	config.cyan += difference;
            test_boundary(config.cyan);
        	thread->window->cyan->update((int)config.cyan);
        }
	    if(slider != thread->window->magenta)
        {
        	config.magenta += difference;
            test_boundary(config.magenta);
        	thread->window->magenta->update((int)config.magenta);
        }
	    if(slider != thread->window->yellow)
        {
        	config.yellow += difference;
            test_boundary(config.yellow);
        	thread->window->yellow->update((int)config.yellow);
        }
    }
	return 0;
}






NEW_PICON_MACRO(ColorBalanceMain)
LOAD_CONFIGURATION_MACRO(ColorBalanceMain, ColorBalanceConfig)
SHOW_GUI_MACRO(ColorBalanceMain, ColorBalanceThread)
RAISE_WINDOW_MACRO(ColorBalanceMain)
SET_STRING_MACRO(ColorBalanceMain)





int ColorBalanceMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	need_reconfigure |= load_configuration();

//printf("ColorBalanceMain::process_realtime 1 %d\n", need_reconfigure);
	if(need_reconfigure)
	{
		int i;

		if(!engine)
		{
#define CALCULATE_HIGHLIGHTS(add, sub, max) \
for(i = 0; i < max; i++) \
{ \
	add[i] = sub[i] = 0.667 * (1 - SQR(((double)i - (max / 2)) / (max / 2))); \
}

			CALCULATE_HIGHLIGHTS(highlights_add_8, highlights_sub_8, 0xff);
			CALCULATE_HIGHLIGHTS(highlights_add_16, highlights_sub_16, 0xffff);

			total_engines = PluginClient::smp > 1 ? 2 : 1;
			engine = new ColorBalanceEngine*[total_engines];
			for(int i = 0; i < total_engines; i++)
			{
				engine[i] = new ColorBalanceEngine(this);
				engine[i]->start();
			}
		}

		reconfigure();
		need_reconfigure = 0;
	}


	if(config.cyan != 0 || config.magenta != 0 || config.yellow != 0)
	{
		long row_step = input_ptr->get_h() / total_engines + 1;
		for(int i = 0; i < input_ptr->get_h(); i += row_step)
		{
			if(i + row_step > input_ptr->get_h()) 
				row_step = input_ptr->get_h() - i;
			engine[i]->start_process_frame(output_ptr, 
				input_ptr, 
				i, 
				i + row_step);
		}

		for(int i = 0; i < total_engines; i++)
		{
			engine[i]->wait_process_frame();
		}
	}
	else
// Data never processed so copy if necessary
	if(input_ptr->get_rows()[0] != output_ptr->get_rows()[0])
	{
		output_ptr->copy_from(input_ptr);
	}
//printf("ColorBalanceMain::process_realtime 2\n");
	return 0;
}



int ColorBalanceMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%scolorbalance.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.cyan = defaults->get("CYAN", config.cyan);
	config.magenta = defaults->get("MAGENTA", config.magenta);
	config.yellow = defaults->get("YELLOW", config.yellow);
	config.preserve = defaults->get("PRESERVELUMINOSITY", config.preserve);
	config.lock_params = defaults->get("LOCKPARAMS", config.lock_params);
	return 0;
}

int ColorBalanceMain::save_defaults()
{
	defaults->update("CYAN", config.cyan);
	defaults->update("MAGENTA", config.magenta);
	defaults->update("YELLOW", config.yellow);
	defaults->update("PRESERVELUMINOSITY", config.preserve);
	defaults->update("LOCKPARAMS", config.lock_params);
	defaults->save();
	return 0;
}

void ColorBalanceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("COLORBALANCE");
	output.tag.set_property("CYAN", config.cyan);
	output.tag.set_property("MAGENTA",  config.magenta);
	output.tag.set_property("YELLOW",  config.yellow);
	output.tag.set_property("PRESERVELUMINOSITY",  config.preserve);
	output.tag.set_property("LOCKPARAMS",  config.lock_params);
	output.append_tag();
	output.terminate_string();
}

void ColorBalanceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

//printf("ColorBalanceMain::read_data 1\n");
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
//printf("ColorBalanceMain::read_data 1\n");

	int result = 0;
//printf("ColorBalanceMain::read_data 1\n");

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("COLORBALANCE"))
			{
				config.cyan = input.tag.get_property("CYAN", config.cyan);
				config.magenta = input.tag.get_property("MAGENTA", config.magenta);
				config.yellow = input.tag.get_property("YELLOW", config.yellow);
				config.preserve = input.tag.get_property("PRESERVELUMINOSITY", config.preserve);
				config.lock_params = input.tag.get_property("LOCKPARAMS", config.lock_params);
			}
		}
	}
//printf("ColorBalanceMain::read_data 2\n");
}
