
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

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "histogram.h"
#include "histogramconfig.h"
#include "histogramwindow.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "playback3d.h"
#include "plugincolors.h"
#include "vframe.h"
#include "workarounds.h"

#include "aggregated.h"
#include "../colorbalance/aggregated.h"
#include "../interpolate/aggregated.h"
#include "../gamma/aggregated.h"

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
		preview_lookup[i] = 0;
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
		delete [] preview_lookup[i];
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
SET_TRACE
// Process just the RGB values to determine the automatic points or
// all the points if manual
		if(!config.automatic)
		{
// Generate curves for value histogram
// Lock out changes to curves
			thread->window->lock_window("HistogramMain::render_gui 1");
			tabulate_curve(HISTOGRAM_RED, 0);
			tabulate_curve(HISTOGRAM_GREEN, 0);
			tabulate_curve(HISTOGRAM_BLUE, 0);
			thread->window->unlock_window();
		}

		calculate_histogram((VFrame*)data, !config.automatic);

SET_TRACE

		if(config.automatic)
		{
SET_TRACE
			calculate_automatic((VFrame*)data);

SET_TRACE
// Generate curves for value histogram
// Lock out changes to curves
			thread->window->lock_window("HistogramMain::render_gui 1");
			tabulate_curve(HISTOGRAM_RED, 0);
			tabulate_curve(HISTOGRAM_GREEN, 0);
			tabulate_curve(HISTOGRAM_BLUE, 0);
			thread->window->unlock_window();

SET_TRACE
// Need a second pass to get the luminance values.
			calculate_histogram((VFrame*)data, 1);
SET_TRACE
		}

SET_TRACE
		thread->window->lock_window("HistogramMain::render_gui 2");
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
	defaults = new BC_Hash(directory);
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
	config.plot = defaults->get("PLOT", config.plot);
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
	defaults->update("PLOT", config.plot);
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
	output.tag.set_property("PLOT", config.plot);
	output.tag.set_property("SPLIT", config.split);
	output.append_tag();
	output.tag.set_title("/HISTOGRAM");
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
				config.plot = input.tag.get_property("PLOT", config.plot);
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

// 	if(input < 0)
// 	{
// 		output = 0;
// 		done = 1;
// 	}
// 
// 	if(input > 1)
// 	{
// 		output = 1;
// 		done = 1;
// 	}

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


void HistogramMain::calculate_histogram(VFrame *data, int do_value)
{

	if(!engine) engine = new HistogramEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);

	if(!accum[0])
	{
		for(int i = 0; i < HISTOGRAM_MODES; i++)
			accum[i] = new int[HISTOGRAM_SLOTS];
	}

	engine->process_packages(HistogramEngine::HISTOGRAM, data, do_value);

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
	calculate_histogram(data, 0);
	config.reset_points(1);

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





int HistogramMain::calculate_use_opengl()
{
// glHistogram doesn't work.
	int result = get_use_opengl() &&
		!config.automatic && 
		config.points[HISTOGRAM_RED].total() < 3 &&
		config.points[HISTOGRAM_GREEN].total() < 3 &&
		config.points[HISTOGRAM_BLUE].total() < 3 &&
		config.points[HISTOGRAM_VALUE].total() < 3 &&
		(!config.plot || !gui_open());
	return result;
}


int HistogramMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
SET_TRACE
	int need_reconfigure = load_configuration();


SET_TRACE
	int use_opengl = calculate_use_opengl();

//printf("%d\n", use_opengl);
	read_frame(frame, 
		0, 
		start_position, 
		frame_rate,
		use_opengl);

// Apply histogram in hardware
	if(use_opengl) return run_opengl();

	if(!engine) engine = new HistogramEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
	this->input = frame;
	this->output = frame;

// Always plot to set the curves if automatic
	if(config.plot || config.automatic) send_render_gui(frame);

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

// Generate transfer tables with value function for integer colormodels.
		for(int i = 0; i < 3; i++)
			tabulate_curve(i, 1);
SET_TRACE
	}



// Apply histogram
	engine->process_packages(HistogramEngine::APPLY, input, 0);

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
	if(!preview_lookup[subscript])
		preview_lookup[subscript] = new int[HISTOGRAM_SLOTS];


	float *current_smooth = smoothed[subscript];
	float *current_linear = linear[subscript];

// Make linear curve
	for(i = 0; i < HISTOGRAM_SLOTS; i++)
	{
		float input = (float)i / HISTOGRAM_SLOTS * FLOAT_RANGE + MIN_INPUT;
		current_linear[i] = calculate_linear(input, subscript, use_value);
	}






// Make smooth curve (currently a copy of the linear curve)
	float prev = 0.0;
	for(i = 0; i < HISTOGRAM_SLOTS; i++)
	{
//		current_smooth[i] = current_linear[i] * 0.001 +
//			prev * 0.999;
//		prev = current_smooth[i];

		current_smooth[i] = current_linear[i];
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

// Lookup table for preview only used for GUI
	if(!use_value)
	{
		for(i = 0; i < 0x10000; i++)
			preview_lookup[subscript][i] = 
				(int)(calculate_smooth((float)i / 0xffff, subscript) * 0xffff);
	}
}

int HistogramMain::handle_opengl()
{
#ifdef HAVE_GL
// Functions to get pixel from either previous effect or texture
	static char *histogram_get_pixel1 =
		"vec4 histogram_get_pixel()\n"
		"{\n"
		"	return gl_FragColor;\n"
		"}\n";

	static char *histogram_get_pixel2 =
		"uniform sampler2D tex;\n"
		"vec4 histogram_get_pixel()\n"
		"{\n"
		"	return texture2D(tex, gl_TexCoord[0].st);\n"
		"}\n";

	static char *head_frag = 
		"// first input point\n"
		"uniform vec2 input_min_r;\n"
		"uniform vec2 input_min_g;\n"
		"uniform vec2 input_min_b;\n"
		"uniform vec2 input_min_v;\n"
		"// second input point\n"
		"uniform vec2 input_max_r;\n"
		"uniform vec2 input_max_g;\n"
		"uniform vec2 input_max_b;\n"
		"uniform vec2 input_max_v;\n"
		"// output points\n"
		"uniform vec4 output_min;\n"
		"uniform vec4 output_scale;\n"
		"void main()\n"
		"{\n";

	static char *get_rgb_frag =
		"	vec4 pixel = histogram_get_pixel();\n";

	static char *get_yuv_frag =
		"	vec4 pixel = histogram_get_pixel();\n"
			YUV_TO_RGB_FRAG("pixel");

#define APPLY_INPUT_CURVE(PIXEL, INPUT_MIN, INPUT_MAX) \
		"// apply input curve\n" \
		"	if(" PIXEL " < 0.0)\n" \
		"		" PIXEL " = 0.0;\n" \
		"	else\n" \
		"	if(" PIXEL " < " INPUT_MIN ".x)\n" \
		"		" PIXEL " = " PIXEL " * " INPUT_MIN ".y / " INPUT_MIN ".x;\n" \
		"	else\n" \
		"	if(" PIXEL " < " INPUT_MAX ".x)\n" \
		"		" PIXEL " = (" PIXEL " - " INPUT_MIN ".x) * \n" \
		"			(" INPUT_MAX ".y - " INPUT_MIN ".y) / \n" \
		"			(" INPUT_MAX ".x - " INPUT_MIN ".x) + \n" \
		"			" INPUT_MIN ".y;\n" \
		"	else\n" \
		"	if(" PIXEL " < 1.0)\n" \
		"		" PIXEL " = (" PIXEL " - " INPUT_MAX ".x) * \n" \
		"			(1.0 - " INPUT_MAX ".y) / \n" \
		"			(1.0 - " INPUT_MAX ".x) + \n" \
		"			" INPUT_MAX ".y;\n" \
		"	else\n" \
		"		" PIXEL " = 1.0;\n"



	static char *apply_histogram_frag = 
		APPLY_INPUT_CURVE("pixel.r", "input_min_r", "input_max_r")
		APPLY_INPUT_CURVE("pixel.g", "input_min_g", "input_max_g")
		APPLY_INPUT_CURVE("pixel.b", "input_min_b", "input_max_b")
		"// apply output curve\n"
		"	pixel.rgb *= output_scale.rgb;\n"
		"	pixel.rgb += output_min.rgb;\n"
		APPLY_INPUT_CURVE("pixel.r", "input_min_v", "input_max_v")
		APPLY_INPUT_CURVE("pixel.g", "input_min_v", "input_max_v")
		APPLY_INPUT_CURVE("pixel.b", "input_min_v", "input_max_v")
		"// apply output curve\n"
		"	pixel.rgb *= vec3(output_scale.a, output_scale.a, output_scale.a);\n"
		"	pixel.rgb += vec3(output_min.a, output_min.a, output_min.a);\n";

	static char *put_rgb_frag =
		"	gl_FragColor = pixel;\n"
		"}\n";

	static char *put_yuv_frag =
			RGB_TO_YUV_FRAG("pixel")
		"	gl_FragColor = pixel;\n"
		"}\n";



	get_output()->to_texture();
	get_output()->enable_opengl();

	char *shader_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int current_shader = 0;
	int aggregate_interpolation = 0;
	int aggregate_gamma = 0;
	int aggregate_colorbalance = 0;
// All aggregation possibilities must be accounted for because unsupported
// effects can get in between the aggregation members.
	if(!strcmp(get_output()->get_prev_effect(2), "Interpolate Pixels") &&
		!strcmp(get_output()->get_prev_effect(1), "Gamma") &&
		!strcmp(get_output()->get_prev_effect(0), "Color Balance"))
	{
		aggregate_interpolation = 1;
		aggregate_gamma = 1;
		aggregate_colorbalance = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(1), "Gamma") &&
		!strcmp(get_output()->get_prev_effect(0), "Color Balance"))
	{
		aggregate_gamma = 1;
		aggregate_colorbalance = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(1), "Interpolate Pixels") &&
		!strcmp(get_output()->get_prev_effect(0), "Gamma"))
	{
		aggregate_interpolation = 1;
		aggregate_gamma = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(1), "Interpolate Pixels") &&
		!strcmp(get_output()->get_prev_effect(0), "Color Balance"))
	{
		aggregate_interpolation = 1;
		aggregate_colorbalance = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(0), "Interpolate Pixels"))
		aggregate_interpolation = 1;
	else
	if(!strcmp(get_output()->get_prev_effect(0), "Gamma"))
		aggregate_gamma = 1;
	else
	if(!strcmp(get_output()->get_prev_effect(0), "Color Balance"))
		aggregate_colorbalance = 1;


// The order of processing is fixed by this sequence
	if(aggregate_interpolation)
		INTERPOLATE_COMPILE(shader_stack, 
			current_shader)

	if(aggregate_gamma)
		GAMMA_COMPILE(shader_stack, 
			current_shader, 
			aggregate_interpolation)

	if(aggregate_colorbalance)
		COLORBALANCE_COMPILE(shader_stack, 
			current_shader, 
			aggregate_interpolation || aggregate_gamma)


	if(aggregate_interpolation || aggregate_gamma || aggregate_colorbalance)
		shader_stack[current_shader++] = histogram_get_pixel1;
	else
		shader_stack[current_shader++] = histogram_get_pixel2;

	unsigned int shader = 0;
	switch(get_output()->get_color_model())
	{
		case BC_YUV888:
		case BC_YUVA8888:
			shader_stack[current_shader++] = head_frag;
			shader_stack[current_shader++] = get_yuv_frag;
			shader_stack[current_shader++] = apply_histogram_frag;
			shader_stack[current_shader++] = put_yuv_frag;
			break;
		default:
			shader_stack[current_shader++] = head_frag;
			shader_stack[current_shader++] = get_rgb_frag;
			shader_stack[current_shader++] = apply_histogram_frag;
			shader_stack[current_shader++] = put_rgb_frag;
			break;
	}

	shader = VFrame::make_shader(0,
		shader_stack[0],
		shader_stack[1],
		shader_stack[2],
		shader_stack[3],
		shader_stack[4],
		shader_stack[5],
		shader_stack[6],
		shader_stack[7],
		shader_stack[8],
		shader_stack[9],
		shader_stack[10],
		shader_stack[11],
		shader_stack[12],
		shader_stack[13],
		shader_stack[14],
		shader_stack[15],
		0);

printf("HistogramMain::handle_opengl %d %d %d %d shader=%d\n", 
aggregate_interpolation, 
aggregate_gamma,
aggregate_colorbalance,
current_shader,
shader);

	float input_min_r[2] = { 0, 0 };
	float input_min_g[2] = { 0, 0 };
	float input_min_b[2] = { 0, 0 };
	float input_min_v[2] = { 0, 0 };
	float input_max_r[2] = { 1, 1 };
	float input_max_g[2] = { 1, 1 };
	float input_max_b[2] = { 1, 1 };
	float input_max_v[2] = { 1, 1 };
	float output_min[4] = { 0, 0, 0, 0 };
	float output_scale[4] = { 1, 1, 1, 1 };

// Red
	HistogramPoint *point1, *point2;
	
#define CONVERT_POINT(index, input_min, input_max) \
	point1 = config.points[index].first; \
	point2 = config.points[index].last; \
	if(point1) \
	{ \
		input_min[0] = point1->x; \
		input_min[1] = point1->y; \
		if(point2 != point1) \
		{ \
			input_max[0] = point2->x; \
			input_max[1] = point2->y; \
		} \
	}

	CONVERT_POINT(HISTOGRAM_RED, input_min_r, input_max_r);
	CONVERT_POINT(HISTOGRAM_GREEN, input_min_g, input_max_g);
	CONVERT_POINT(HISTOGRAM_BLUE, input_min_b, input_max_b);
	CONVERT_POINT(HISTOGRAM_VALUE, input_min_v, input_max_v);

// printf("min x    min y    max x    max y\n");
// printf("%f %f %f %f\n", input_min_r[0], input_min_r[1], input_max_r[0], input_max_r[1]);
// printf("%f %f %f %f\n", input_min_g[0], input_min_g[1], input_max_g[0], input_max_g[1]);
// printf("%f %f %f %f\n", input_min_b[0], input_min_b[1], input_max_b[0], input_max_b[1]);
// printf("%f %f %f %f\n", input_min_v[0], input_min_v[1], input_max_v[0], input_max_v[1]);

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		output_min[i] = config.output_min[i];
		output_scale[i] = config.output_max[i] - config.output_min[i];
	}

	if(shader > 0)
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		if(aggregate_gamma) GAMMA_UNIFORMS(shader)
		if(aggregate_interpolation) INTERPOLATE_UNIFORMS(shader)
		if(aggregate_colorbalance) COLORBALANCE_UNIFORMS(shader)
		glUniform2fv(glGetUniformLocation(shader, "input_min_r"), 1, input_min_r);
		glUniform2fv(glGetUniformLocation(shader, "input_min_g"), 1, input_min_g);
		glUniform2fv(glGetUniformLocation(shader, "input_min_b"), 1, input_min_b);
		glUniform2fv(glGetUniformLocation(shader, "input_min_v"), 1, input_min_v);
		glUniform2fv(glGetUniformLocation(shader, "input_max_r"), 1, input_max_r);
		glUniform2fv(glGetUniformLocation(shader, "input_max_g"), 1, input_max_g);
		glUniform2fv(glGetUniformLocation(shader, "input_max_b"), 1, input_max_b);
		glUniform2fv(glGetUniformLocation(shader, "input_max_v"), 1, input_max_v);
		glUniform4fv(glGetUniformLocation(shader, "output_min"), 1, output_min);
		glUniform4fv(glGetUniformLocation(shader, "output_scale"), 1, output_scale);
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);

	glDisable(GL_BLEND);

// Draw the affected half
	if(config.split)
	{
		glBegin(GL_TRIANGLES);
		glNormal3f(0, 0, 1.0);

		glTexCoord2f(0.0 / get_output()->get_texture_w(), 
			0.0 / get_output()->get_texture_h());
		glVertex3f(0.0, -(float)get_output()->get_h(), 0);


		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(), 
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), -0.0, 0);

		glTexCoord2f(0.0 / get_output()->get_texture_w(), 
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f(0.0, -0.0, 0);


		glEnd();
	}
	else
	{
		get_output()->draw_texture();
	}

	glUseProgram(0);

// Draw the unaffected half
	if(config.split)
	{
		glBegin(GL_TRIANGLES);
		glNormal3f(0, 0, 1.0);


		glTexCoord2f(0.0 / get_output()->get_texture_w(), 
			0.0 / get_output()->get_texture_h());
		glVertex3f(0.0, -(float)get_output()->get_h(), 0);

		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(), 
			0.0 / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), 
			-(float)get_output()->get_h(), 0);

		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(), 
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), -0.0, 0);


 		glEnd();
	}

	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
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
		int do_value = server->do_value;


#define HISTOGRAM_HEAD(type) \
{ \
	for(int i = pkg->start; i < pkg->end; i++) \
	{ \
		type *row = (type*)data->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{

#define HISTOGRAM_TAIL(components) \
/* Value takes the maximum of the output RGB values */ \
			if(do_value) \
			{ \
				CLAMP(r, 0, HISTOGRAM_SLOTS - 1); \
				CLAMP(g, 0, HISTOGRAM_SLOTS - 1); \
				CLAMP(b, 0, HISTOGRAM_SLOTS - 1); \
				r_out = lookup_r[r]; \
				g_out = lookup_g[g]; \
				b_out = lookup_b[b]; \
/*				v = (r * 76 + g * 150 + b * 29) >> 8; */ \
				v = MAX(r_out, g_out); \
				v = MAX(v, b_out); \
				v += -HISTOGRAM_MIN * 0xffff / 100; \
				CLAMP(v, 0, HISTOGRAM_SLOTS - 1); \
				accum_v[v]++; \
			} \
 \
			r += -HISTOGRAM_MIN * 0xffff / 100; \
			g += -HISTOGRAM_MIN * 0xffff / 100; \
			b += -HISTOGRAM_MIN * 0xffff / 100; \
			CLAMP(r, 0, HISTOGRAM_SLOTS - 1); \
			CLAMP(g, 0, HISTOGRAM_SLOTS - 1); \
			CLAMP(b, 0, HISTOGRAM_SLOTS - 1); \
			accum_r[r]++; \
			accum_g[g]++; \
			accum_b[b]++; \
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
		int32_t r, g, b, a, y, u, v;
		int r_out, g_out, b_out;
		int *lookup_r = plugin->preview_lookup[HISTOGRAM_RED];
		int *lookup_g = plugin->preview_lookup[HISTOGRAM_GREEN];
		int *lookup_b = plugin->preview_lookup[HISTOGRAM_BLUE];

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

void HistogramEngine::process_packages(int operation, VFrame *data, int do_value)
{
	this->data = data;
	this->operation = operation;
	this->do_value = do_value;
	LoadServer::process_packages();
}



