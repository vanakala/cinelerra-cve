#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "histogram.h"
#include "histogramconfig.h"
#include "histogramwindow.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "plugincolors.h"
#include "vframe.h"



class HistogramMain;
class HistogramEngine;
class HistogramWindow;





REGISTER_PLUGIN(HistogramMain)












HistogramMain::HistogramMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		lookup[i] = 0;
		smoothed[i] = 0;
		linear[i] = 0;
		accum[i] = 0;
	}
	current_point = -1;
	mode = HISTOGRAM_VALUE;
	dragging_point = 0;
	input = 0;
	output = 0;
}

HistogramMain::~HistogramMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	for(int i = 0; i < HISTOGRAM_MODES;i++)
	{
		delete [] lookup[i];
		delete [] smoothed[i];
		delete [] linear[i];
		delete [] accum[i];
	}
	delete engine;
}

char* HistogramMain::plugin_title() { return N_("Histogram"); }
int HistogramMain::is_realtime() { return 1; }


#include "picon_png.h"
NEW_PICON_MACRO(HistogramMain)

SHOW_GUI_MACRO(HistogramMain, HistogramThread)

SET_STRING_MACRO(HistogramMain)

RAISE_WINDOW_MACRO(HistogramMain)

LOAD_CONFIGURATION_MACRO(HistogramMain, HistogramConfig)

void HistogramMain::render_gui(void *data)
{
	if(thread)
	{
		calculate_histogram((VFrame*)data);

		if(config.automatic)
		{
SET_TRACE
			calculate_automatic((VFrame*)data);

		}

SET_TRACE
		thread->window->lock_window("HistogramMain::render_gui");
		thread->window->update_canvas();
		if(config.automatic)
		{
			thread->window->update_input();
		}
		thread->window->unlock_window();
SET_TRACE
	}
}

void HistogramMain::update_gui()
{
	if(thread)
	{
		thread->window->lock_window("HistogramMain::update_gui");
		int reconfigure = load_configuration();
		if(reconfigure) 
		{
			thread->window->update(0);
			if(!config.automatic)
			{
				thread->window->update_input();
			}
		}
		thread->window->unlock_window();
	}
}


int HistogramMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%shistogram.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	for(int j = 0; j < HISTOGRAM_MODES; j++)
	{
		while(config.points[j].last) delete config.points[j].last;

		sprintf(string, "TOTAL_POINTS_%d", j);
		int total_points = defaults->get(string, 0);

		for(int i = 0; i < total_points; i++)
		{
			HistogramPoint *point = new HistogramPoint;
			sprintf(string, "INPUT_X_%d_%d", j, i);
			point->x = defaults->get(string, point->x);
			sprintf(string, "INPUT_Y_%d_%d", j, i);
			point->y = defaults->get(string, point->y);
			config.points[j].append(point);
		}
	}


	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		sprintf(string, "OUTPUT_MIN_%d", i);
		config.output_min[i] = defaults->get(string, config.output_min[i]);
		sprintf(string, "OUTPUT_MAX_%d", i);
		config.output_max[i] = defaults->get(string, config.output_max[i]);
	}

	config.automatic = defaults->get("AUTOMATIC", config.automatic);
	mode = defaults->get("MODE", mode);
	CLAMP(mode, 0, HISTOGRAM_MODES - 1);
	config.threshold = defaults->get("THRESHOLD", config.threshold);
	config.split = defaults->get("SPLIT", config.split);
	config.boundaries();
	return 0;
}


int HistogramMain::save_defaults()
{
	char string[BCTEXTLEN];



	for(int j = 0; j < HISTOGRAM_MODES; j++)
	{
		int total_points = config.points[j].total();
		sprintf(string, "TOTAL_POINTS_%d", j);
		defaults->update(string, total_points);
		HistogramPoint *current = config.points[j].first;
		int number = 0;
		while(current)
		{
			sprintf(string, "INPUT_X_%d_%d", j, number);
			defaults->update(string, current->x);
			sprintf(string, "INPUT_Y_%d_%d", j, number);
			defaults->update(string, current->y);
			current = NEXT;
			number++;
		}
	}


	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		sprintf(string, "OUTPUT_MIN_%d", i);
		defaults->update(string, config.output_min[i]);
		sprintf(string, "OUTPUT_MAX_%d", i);
	   	defaults->update(string, config.output_max[i]);
	}

	defaults->update("AUTOMATIC", config.automatic);
	defaults->update("MODE", mode);
	defaults->update("THRESHOLD", config.threshold);
	defaults->update("SPLIT", config.split);
	defaults->save();
	return 0;
}



void HistogramMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("HISTOGRAM");

	char string[BCTEXTLEN];


	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		sprintf(string, "OUTPUT_MIN_%d", i);
		output.tag.set_property(string, config.output_min[i]);
		sprintf(string, "OUTPUT_MAX_%d", i);
	   	output.tag.set_property(string, config.output_max[i]);
//printf("HistogramMain::save_data %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
	}

	output.tag.set_property("AUTOMATIC", config.automatic);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("SPLIT", config.split);
	output.append_tag();
	output.append_newline();





	for(int j = 0; j < HISTOGRAM_MODES; j++)
	{
		output.tag.set_title("POINTS");
		output.append_tag();
		output.append_newline();


		HistogramPoint *current = config.points[j].first;
		while(current)
		{
			output.tag.set_title("POINT");
			output.tag.set_property("X", current->x);
			output.tag.set_property("Y", current->y);
			output.append_tag();
			output.append_newline();
			current = NEXT;
		}


		output.tag.set_title("/POINTS");
		output.append_tag();
		output.append_newline();
	}






	output.terminate_string();
}

void HistogramMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	int current_input_mode = 0;


	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("HISTOGRAM"))
			{
				char string[BCTEXTLEN];
				for(int i = 0; i < HISTOGRAM_MODES; i++)
				{
					sprintf(string, "OUTPUT_MIN_%d", i);
					config.output_min[i] = input.tag.get_property(string, config.output_min[i]);
					sprintf(string, "OUTPUT_MAX_%d", i);
					config.output_max[i] = input.tag.get_property(string, config.output_max[i]);
//printf("HistogramMain::read_data %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
				}
				config.automatic = input.tag.get_property("AUTOMATIC", config.automatic);
				config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
				config.split = input.tag.get_property("SPLIT", config.split);
			}
			else
			if(input.tag.title_is("POINTS"))
			{
				if(current_input_mode < HISTOGRAM_MODES)
				{
					HistogramPoints *points = &config.points[current_input_mode];
					while(points->last) 
						delete points->last;
					while(!result)
					{
						result = input.read_tag();
						if(!result)
						{
							if(input.tag.title_is("/POINTS"))
							{
								break;
							}
							else
							if(input.tag.title_is("POINT"))
							{
								points->insert(
									input.tag.get_property("X", 0.0),
									input.tag.get_property("Y", 0.0));
							}
						}
					}

				}
				current_input_mode++;
			}
		}
	}

	config.boundaries();

}

float HistogramMain::calculate_linear(float input, 
	int subscript,
	int use_value)
{
	int done = 0;
	float output;

	if(input < 0)
	{
		output = 0;
		done = 1;
	}

	if(input > 1)
	{
		output = 1;
		done = 1;
	}

	if(!done)
	{

		float x1 = 0;
		float y1 = 0;
		float x2 = 1;
		float y2 = 1;



// Get 2 points surrounding current position
		HistogramPoints *points = &config.points[subscript];
		HistogramPoint *current = points->first;
		int done = 0;
		while(current && !done)
		{
			if(current->x > input)
			{
				x2 = current->x;
				y2 = current->y;
				done = 1;
			}
			else
				current = NEXT;
		}

		current = points->last;
		done = 0;
		while(current && !done)
		{
			if(current->x <= input)
			{
				x1 = current->x;
				y1 = current->y;
				done = 1;
			}
			else
				current = PREVIOUS;
		}




// Linear
		if(!EQUIV(x2 - x1, 0))
			output = (input - x1) * (y2 - y1) / (x2 - x1) + y1;
		else
			output = input * y2;





	}

// Apply value curve
	if(use_value)
	{
		output = calculate_linear(output, HISTOGRAM_VALUE, 0);
	}


	float output_min = config.output_min[subscript];
	float output_max = config.output_max[subscript];
	float output_left;
	float output_right;
	float output_linear;

// Compress output for value followed by channel
	output = output_min + 
		output * 
		(output_max - output_min);


	return output;
}

float HistogramMain::calculate_smooth(float input, int subscript)
{
	float x_f = (input - MIN_INPUT) * HISTOGRAM_SLOTS / FLOAT_RANGE;
	int x_i1 = (int)x_f;
	int x_i2 = x_i1 + 1;
	CLAMP(x_i1, 0, HISTOGRAM_SLOTS - 1);
	CLAMP(x_i2, 0, HISTOGRAM_SLOTS - 1);
	CLAMP(x_f, 0, HISTOGRAM_SLOTS - 1);

	float smooth1 = smoothed[subscript][x_i1];
	float smooth2 = smoothed[subscript][x_i2];
	float result = smooth1 + (smooth2 - smooth1) * (x_f - x_i1);
	CLAMP(result, 0, 1.0);
	return result;
}


void HistogramMain::calculate_histogram(VFrame *data)
{

	if(!engine) engine = new HistogramEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);

	if(!accum[0])
	{
		for(int i = 0; i < HISTOGRAM_MODES; i++)
			accum[i] = new int[HISTOGRAM_SLOTS];
	}

	engine->process_packages(HistogramEngine::HISTOGRAM, data);

	for(int i = 0; i < engine->get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)engine->get_client(i);

		if(i == 0)
		{
			for(int j = 0; j < HISTOGRAM_MODES; j++)
			{
				memcpy(accum[j], unit->accum[j], sizeof(int) * HISTOGRAM_SLOTS);
			}
		}
		else
		{
			for(int j = 0; j < HISTOGRAM_MODES; j++)
			{
				int *out = accum[j];
				int *in = unit->accum[j];
				for(int k = 0; k < HISTOGRAM_SLOTS; k++)
					out[k] += in[k];
			}
		}
	}

// Remove top and bottom from calculations.  Doesn't work in high
// precision colormodels.
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		accum[i][0] = 0;
		accum[i][HISTOGRAM_SLOTS - 1] = 0;
	}
}


void HistogramMain::calculate_automatic(VFrame *data)
{
	calculate_histogram(data);
	config.reset_points();

// Do each channel
	for(int i = 0; i < 3; i++)
	{
		int *accum = this->accum[i];
		int pixels = data->get_w() * data->get_h();
		float white_fraction = 1.0 - (1.0 - config.threshold) / 2;
		int threshold = (int)(white_fraction * pixels);
		int total = 0;
		float max_level = 1.0;
		float min_level = 0.0;

// Get histogram slot above threshold of pixels
		for(int j = 0; j < HISTOGRAM_SLOTS; j++)
		{
			total += accum[j];
			if(total >= threshold)
			{
				max_level = (float)j / HISTOGRAM_SLOTS * FLOAT_RANGE + MIN_INPUT;
				break;
			}
		}

// Get slot below 99% of pixels
		total = 0;
		for(int j = HISTOGRAM_SLOTS - 1; j >= 0; j--)
		{
			total += accum[j];
			if(total >= threshold)
			{
				min_level = (float)j / HISTOGRAM_SLOTS * FLOAT_RANGE + MIN_INPUT;
				break;
			}
		}


		config.points[i].insert(max_level, 1.0);
		config.points[i].insert(min_level, 0.0);
	}
}






int HistogramMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
SET_TRACE
	int need_reconfigure = load_configuration();


SET_TRACE

	if(!engine) engine = new HistogramEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
	this->input = input_ptr;
	this->output = output_ptr;

	send_render_gui(input_ptr);

	if(input_ptr->get_rows()[0] != output_ptr->get_rows()[0])
	{
		output_ptr->copy_from(input_ptr);
	}

SET_TRACE
// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if(need_reconfigure || 
		!lookup[0] || 
		!smoothed[0] || 
		!linear[0] || 
		config.automatic)
	{
SET_TRACE
// Calculate new curves
		if(config.automatic)
		{
			calculate_automatic(input);
		}
SET_TRACE

// Generate transfer tables for integer colormodels.
		for(int i = 0; i < 3; i++)
			tabulate_curve(i, 1);
SET_TRACE
	}



// Apply histogram
	engine->process_packages(HistogramEngine::APPLY, input);

SET_TRACE

	return 0;
}

void HistogramMain::tabulate_curve(int subscript, int use_value)
{
	int i;
	if(!lookup[subscript])
		lookup[subscript] = new int[HISTOGRAM_SLOTS];
	if(!smoothed[subscript])
		smoothed[subscript] = new float[HISTOGRAM_SLOTS];
	if(!linear[subscript])
		linear[subscript] = new float[HISTOGRAM_SLOTS];

	float *current_smooth = smoothed[subscript];
	float *current_linear = linear[subscript];

// Make linear curve
	for(i = 0; i < HISTOGRAM_SLOTS; i++)
	{
		float input = (float)i / HISTOGRAM_SLOTS * FLOAT_RANGE + MIN_INPUT;
		current_linear[i] = calculate_linear(input, subscript, use_value);
	}






// Make smooth curve
	float prev = 0.0;
	for(i = 0; i < HISTOGRAM_SLOTS; i++)
	{
//		current_smooth[i] = current_linear[i] * 0.001 +
//			prev * 0.999;
		current_smooth[i] = current_linear[i];
		prev = current_smooth[i];
	}

// Generate lookup tables for integer colormodels
	if(input)
	{
		switch(input->get_color_model())
		{
			case BC_RGB888:
			case BC_RGBA8888:
				for(i = 0; i < 0x100; i++)
					lookup[subscript][i] = 
						(int)(calculate_smooth((float)i / 0xff, subscript) * 0xff);
				break;
// All other integer colormodels are converted to 16 bit RGB
			default:
				for(i = 0; i < 0x10000; i++)
					lookup[subscript][i] = 
						(int)(calculate_smooth((float)i / 0xffff, subscript) * 0xffff);
				break;
		}
	}
}












HistogramPackage::HistogramPackage()
 : LoadPackage()
{
}




HistogramUnit::HistogramUnit(HistogramEngine *server, 
	HistogramMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
	for(int i = 0; i < HISTOGRAM_MODES; i++)
		accum[i] = new int[HISTOGRAM_SLOTS];
}

HistogramUnit::~HistogramUnit()
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
		delete [] accum[i];
}

void HistogramUnit::process_package(LoadPackage *package)
{
	HistogramPackage *pkg = (HistogramPackage*)package;

	if(server->operation == HistogramEngine::HISTOGRAM)
	{


#define HISTOGRAM_HEAD(type) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)data->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{

#define HISTOGRAM_TAIL(components) \
/*			v = (r * 76 + g * 150 + b * 29) >> 8; */ \
			v = MAX(r, g); \
			v = MAX(v, b); \
			r += -HISTOGRAM_MIN * 0xffff / 100; \
			g += -HISTOGRAM_MIN * 0xffff / 100; \
			b += -HISTOGRAM_MIN * 0xffff / 100; \
			v += -HISTOGRAM_MIN * 0xffff / 100; \
			CLAMP(r, 0, HISTOGRAM_SLOTS); \
			CLAMP(g, 0, HISTOGRAM_SLOTS); \
			CLAMP(b, 0, HISTOGRAM_SLOTS); \
			CLAMP(v, 0, HISTOGRAM_SLOTS); \
			accum_r[r]++; \
			accum_g[g]++; \
			accum_b[b]++; \
			accum_v[v]++; \
			row += components; \
		} \
	} \
}




		VFrame *data = server->data;
		int w = data->get_w();
		int h = data->get_h();
		int *accum_r = accum[HISTOGRAM_RED];
		int *accum_g = accum[HISTOGRAM_GREEN];
		int *accum_b = accum[HISTOGRAM_BLUE];
		int *accum_v = accum[HISTOGRAM_VALUE];
		int r, g, b, a, y, u, v;

		switch(data->get_color_model())
		{
			case BC_RGB888:
				HISTOGRAM_HEAD(unsigned char)
				r = (row[0] << 8) | row[0];
				g = (row[1] << 8) | row[1];
				b = (row[2] << 8) | row[2];
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGB_FLOAT:
				HISTOGRAM_HEAD(float)
				r = (int)(row[0] * 0xffff);
				g = (int)(row[1] * 0xffff);
				b = (int)(row[2] * 0xffff);
				HISTOGRAM_TAIL(3)
				break;
			case BC_YUV888:
				HISTOGRAM_HEAD(unsigned char)
				y = (row[0] << 8) | row[0];
				u = (row[1] << 8) | row[1];
				v = (row[2] << 8) | row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGBA8888:
				HISTOGRAM_HEAD(unsigned char)
				r = (row[0] << 8) | row[0];
				g = (row[1] << 8) | row[1];
				b = (row[2] << 8) | row[2];
				HISTOGRAM_TAIL(4)
				break;
			case BC_RGBA_FLOAT:
				HISTOGRAM_HEAD(float)
				r = (int)(row[0] * 0xffff);
				g = (int)(row[1] * 0xffff);
				b = (int)(row[2] * 0xffff);
				HISTOGRAM_TAIL(4)
				break;
			case BC_YUVA8888:
				HISTOGRAM_HEAD(unsigned char)
				y = (row[0] << 8) | row[0];
				u = (row[1] << 8) | row[1];
				v = (row[2] << 8) | row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(4)
				break;
			case BC_RGB161616:
				HISTOGRAM_HEAD(uint16_t)
				r = row[0];
				g = row[1];
				b = row[2];
				HISTOGRAM_TAIL(3)
				break;
			case BC_YUV161616:
				HISTOGRAM_HEAD(uint16_t)
				y = row[0];
				u = row[1];
				v = row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(3)
				break;
			case BC_RGBA16161616:
				HISTOGRAM_HEAD(uint16_t)
				r = row[0];
				g = row[1];
				b = row[2];
				HISTOGRAM_TAIL(3)
				break;
			case BC_YUVA16161616:
				HISTOGRAM_HEAD(uint16_t)
				y = row[0];
				u = row[1];
				v = row[2];
				plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v);
				HISTOGRAM_TAIL(4)
				break;
		}
	}
	else
	if(server->operation == HistogramEngine::APPLY)
	{



#define PROCESS(type, components) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
		    	continue; \
			row[0] = lookup_r[row[0]]; \
			row[1] = lookup_g[row[1]]; \
			row[2] = lookup_b[row[2]]; \
			row += components; \
		} \
	} \
}

#define PROCESS_YUV(type, components, max) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
		    	continue; \
/* Convert to 16 bit RGB */ \
			if(max == 0xff) \
			{ \
				y = (row[0] << 8) | row[0]; \
				u = (row[1] << 8) | row[1]; \
				v = (row[2] << 8) | row[2]; \
			} \
			else \
			{ \
				y = row[0]; \
				u = row[1]; \
				v = row[2]; \
			} \
 \
			plugin->yuv.yuv_to_rgb_16(r, g, b, y, u, v); \
 \
/* Look up in RGB domain */ \
			r = lookup_r[r]; \
			g = lookup_g[g]; \
			b = lookup_b[b]; \
 \
/* Convert to 16 bit YUV */ \
			plugin->yuv.rgb_to_yuv_16(r, g, b, y, u, v); \
 \
			if(max == 0xff) \
			{ \
				row[0] = y >> 8; \
				row[1] = u >> 8; \
				row[2] = v >> 8; \
			} \
			else \
			{ \
				row[0] = y; \
				row[1] = u; \
				row[2] = v; \
			} \
			row += components; \
		} \
	} \
}

#define PROCESS_FLOAT(components) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		float *row = (float*)input->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
		    	continue; \
			float r = row[0]; \
			float g = row[1]; \
			float b = row[2]; \
 \
			r = plugin->calculate_smooth(r, HISTOGRAM_RED); \
			g = plugin->calculate_smooth(g, HISTOGRAM_GREEN); \
			b = plugin->calculate_smooth(b, HISTOGRAM_BLUE); \
 \
 			row[0] = r; \
			row[1] = g; \
			row[2] = b; \
 \
			row += components; \
		} \
	} \
}


		VFrame *input = plugin->input;
		VFrame *output = plugin->output;
		int w = input->get_w();
		int h = input->get_h();
		int *lookup_r = plugin->lookup[0];
		int *lookup_g = plugin->lookup[1];
		int *lookup_b = plugin->lookup[2];
		int r, g, b, y, u, v, a;
		switch(input->get_color_model())
		{
			case BC_RGB888:
				PROCESS(unsigned char, 3)
				break;
			case BC_RGB_FLOAT:
				PROCESS_FLOAT(3);
				break;
			case BC_RGBA8888:
				PROCESS(unsigned char, 4)
				break;
			case BC_RGBA_FLOAT:
				PROCESS_FLOAT(4);
				break;
			case BC_RGB161616:
				PROCESS(uint16_t, 3)
				break;
			case BC_RGBA16161616:
				PROCESS(uint16_t, 4)
				break;
			case BC_YUV888:
				PROCESS_YUV(unsigned char, 3, 0xff)
				break;
			case BC_YUVA8888:
				PROCESS_YUV(unsigned char, 4, 0xff)
				break;
			case BC_YUV161616:
				PROCESS_YUV(uint16_t, 3, 0xffff)
				break;
			case BC_YUVA16161616:
				PROCESS_YUV(uint16_t, 4, 0xffff)
				break;
		}
	}
}






HistogramEngine::HistogramEngine(HistogramMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void HistogramEngine::init_packages()
{
	switch(operation)
	{
		case HISTOGRAM:
			total_size = data->get_h();
			break;
		case APPLY:
			total_size = data->get_h();
			break;
	}


	int package_size = (int)((float)total_size / 
			get_total_packages() + 1);
	int start = 0;

	for(int i = 0; i < get_total_packages(); i++)
	{
		HistogramPackage *package = (HistogramPackage*)get_package(i);
		package->start = total_size * i / get_total_packages();
		package->end = total_size * (i + 1) / get_total_packages();
	}

// Initialize clients here in case some don't get run.
	for(int i = 0; i < get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)get_client(i);
		for(int i = 0; i < HISTOGRAM_MODES; i++)
			bzero(unit->accum[i], sizeof(int) * HISTOGRAM_SLOTS);
	}

}

LoadClient* HistogramEngine::new_client()
{
	return new HistogramUnit(this, plugin);
}

LoadPackage* HistogramEngine::new_package()
{
	return new HistogramPackage;
}

void HistogramEngine::process_packages(int operation, VFrame *data)
{
	this->data = data;
	this->operation = operation;
	LoadServer::process_packages();
}



