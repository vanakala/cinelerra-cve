// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "colorspaces.h"
#include "filexml.h"
#include "histogram.h"
#include "histogramconfig.h"
#include "histogramwindow.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "vframe.h"
#include "picon_png.h"

REGISTER_PLUGIN


HistogramMain::HistogramMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		lookup[i] = 0;
		linear[i] = 0;
		accum[i] = 0;
	}
	mode = HISTOGRAM_VALUE;
	input = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

HistogramMain::~HistogramMain()
{
	for(int i = 0; i < HISTOGRAM_MODES;i++)
	{
		delete [] lookup[i];
		delete [] linear[i];
		delete [] accum[i];
	}
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void HistogramMain::reset_plugin()
{
	if(engine)
	{
		for(int i = 0; i < HISTOGRAM_MODES;i++)
		{
			delete [] lookup[i];
			lookup[i] = 0;
			delete [] linear[i];
			linear[i] = 0;
			delete [] accum[i];
			accum[i] = 0;
		}
		delete engine;
		engine = 0;
	}
}

PLUGIN_CLASS_METHODS

void HistogramMain::load_defaults()
{
	char string[BCTEXTLEN];

	defaults = load_defaults_file("histogram.rc");

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
}

void HistogramMain::save_defaults()
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
}

void HistogramMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("HISTOGRAM");

	char string[BCTEXTLEN];

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		sprintf(string, "OUTPUT_MIN_%d", i);
		output.tag.set_property(string, config.output_min[i]);
		sprintf(string, "OUTPUT_MAX_%d", i);
		output.tag.set_property(string, config.output_max[i]);
	}

	output.tag.set_property("AUTOMATIC", config.automatic);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("PLOT", config.plot);
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
	output.tag.set_title("/HISTOGRAM");
	output.append_tag();
	keyframe->set_data(output.string);
}

void HistogramMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int current_input_mode = 0;

	while(!input.read_tag())
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
				while(!input.read_tag())
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
			current_input_mode++;
		}
	}

	config.boundaries();
}

double HistogramMain::calculate_linear(double input, int subscript)
{
	double output;
	double x1 = 0;
	double y1 = 0;
	double x2 = 1;
	double y2 = 1;
	HistogramPoint *current;
	HistogramPoints *points = &config.points[subscript];

// Get 2 points surrounding current position
	for(current = points->first; current; current = NEXT)
	{
		if(current->x > input)
		{
			x2 = current->x;
			y2 = current->y;
			break;
		}
	}

	for(current = points->last; current; current = PREVIOUS)
	{
		if(current->x <= input)
		{
			x1 = current->x;
			y1 = current->y;
			break;
		}
	}
// Linear
	if(!EQUIV(x2 - x1, 0))
		output = (input - x1) * (y2 - y1) / (x2 - x1) + y1;
	else
		output = input * y2;

	double output_min = config.output_min[subscript];
	double output_max = config.output_max[subscript];

// Compress output for value followed by channel
	output = output_min + output * (output_max - output_min);

	return output;
}

double HistogramMain::calculate_smooth(double input, int subscript)
{
	double x_f = (input - HISTOGRAM_MIN_INPUT) * HISTOGRAM_SLOTS / HISTOGRAM_FLOAT_RANGE;
	int x_i1 = round(x_f);
	int x_i2 = x_i1 + 1;

	CLAMP(x_i1, 0, HISTOGRAM_SLOTS - 1);
	CLAMP(x_i2, 0, HISTOGRAM_SLOTS - 1);
	CLAMP(x_f, 0, HISTOGRAM_SLOTS - 1);

	double smooth1 = linear[subscript][x_i1];
	double smooth2 = linear[subscript][x_i2];
	double result = smooth1 + (smooth2 - smooth1) * (x_f - x_i1);
	CLAMP(result, 0, 1.0);
	return result;
}

void HistogramMain::calculate_histogram(VFrame *data)
{
	if(!accum[0])
	{
		for(int i = 0; i < HISTOGRAM_MODES; i++)
			accum[i] = new int[HISTOGRAM_SLOTS];
	}
	engine->process_packages(HistogramEngine::HISTOGRAM, data);

	int copy_done = 0;
	for(int i = 0; i < engine->get_total_clients(); i++)
	{
		HistogramUnit *unit = (HistogramUnit*)engine->get_client(i);

		// If unit has not run, skip it
		if(!unit->package_done)
			continue;

		if(!copy_done && unit->package_done)
		{
			for(int j = 0; j < HISTOGRAM_MODES; j++)
				memcpy(accum[j], unit->accum[j],
					sizeof(int) * HISTOGRAM_SLOTS);
			copy_done = 1;
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
	config.reset_points(1);
// Do each channel
	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		int *accum = this->accum[i];
		int pixels = data->get_w() * data->get_h();
		double white_fraction = 1.0 - (1.0 - config.threshold) / 2;
		int threshold = round(white_fraction * pixels);
		int total = 0;
		double max_level = 1.0;
		double min_level = 0.0;

		if(i == HISTOGRAM_VALUE || !accum)
			continue;
// Get histogram slot above threshold of pixels
		for(int j = 0; j < HISTOGRAM_SLOTS; j++)
		{
			total += accum[j];
			if(total >= threshold)
			{
				max_level = (double)j / HISTOGRAM_SLOTS * HISTOGRAM_FLOAT_RANGE + HISTOGRAM_MIN_INPUT;
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
				min_level = (double)j / HISTOGRAM_SLOTS * HISTOGRAM_FLOAT_RANGE + HISTOGRAM_MIN_INPUT;
				break;
			}
		}
		config.points[i].insert(max_level, 1.0);
		config.points[i].insert(min_level, 0.0);
	}
}

VFrame *HistogramMain::process_tmpframe(VFrame *frame)
{
	int color_model = frame->get_color_model();
	int do_reconfigure = 0;

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return frame;
	}

	do_reconfigure |= load_configuration();

	if(!engine)
		engine = new HistogramEngine(this,
			get_project_smp(),
			get_project_smp());
	input = frame;

// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if(do_reconfigure || !lookup[0] ||
		!linear[0] || config.automatic)
	{
// Calculate new curves
		if(config.automatic)
			calculate_automatic(input);

// Generate transfer tables with value function for integer colormodels.
		for(int i = 0; i < HISTOGRAM_MODES; i++)
			tabulate_curve(i);
	}
	calculate_histogram(input);

// Apply histogram
	engine->process_packages(HistogramEngine::APPLY, input);
// Always plot to set the curves if automatic
	if(do_reconfigure || config.plot || config.automatic)
		update_gui();
	return frame;
}

void HistogramMain::tabulate_curve(int subscript)
{
	if(!lookup[subscript])
		lookup[subscript] = new int[HISTOGRAM_SLOTS];
	if(!linear[subscript])
		linear[subscript] = new float[HISTOGRAM_SLOTS];

	float *current_linear = linear[subscript];

// Make linear curve and smooth curve as a copy of linear
	for(int i = 0; i < HISTOGRAM_SLOTS; i++)
	{
		double input = (double)i / HISTOGRAM_SLOTS * HISTOGRAM_FLOAT_RANGE + HISTOGRAM_MIN_INPUT;
		current_linear[i] = calculate_linear(input, subscript);
	}
// Generate lookup tables for integer colormodels
	for(int i = 0; i < 0x10000; i++)
		lookup[subscript][i] =
			round(calculate_smooth((double)i / 0xffff,
				subscript) * 0xffff);
}

void HistogramMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
// Functions to get pixel from either previous effect or texture
	static const char *histogram_get_pixel1 =
		"vec4 histogram_get_pixel()\n"
		"{\n"
		"	return gl_FragColor;\n"
		"}\n";

	static const char *histogram_get_pixel2 =
		"uniform sampler2D tex;\n"
		"vec4 histogram_get_pixel()\n"
		"{\n"
		"	return texture2D(tex, gl_TexCoord[0].st);\n"
		"}\n";

	static const char *head_frag = 
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

	static const char *get_rgb_frag =
		"	vec4 pixel = histogram_get_pixel();\n";
	static const char *get_yuv_frag =
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

	static const char *apply_histogram_frag = 
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

	static const char *put_rgb_frag =
		"	gl_FragColor = pixel;\n"
		"}\n";
	static const char *put_yuv_frag =
			RGB_TO_YUV_FRAG("pixel")
		"	gl_FragColor = pixel;\n"
		"}\n";
	*/
/* FIXIT
	get_output()->to_texture();
	get_output()->enable_opengl();

	const char *shader_stack[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int current_shader = 0;
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

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		output_min[i] = config.output_min[i];
		output_scale[i] = config.output_max[i] - config.output_min[i];
	}

	if(shader > 0)
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
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
	*/
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
	package_done = 0;
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
		VFrame *data = server->data;
		int w = data->get_w();
		int h = data->get_h();
		int *accum_r = accum[HISTOGRAM_RED];
		int *accum_g = accum[HISTOGRAM_GREEN];
		int *accum_b = accum[HISTOGRAM_BLUE];
		int *accum_v = accum[HISTOGRAM_VALUE];
		int r, g, b, a, y, u, v;
		int r_out, g_out, b_out;
		int *lookup_r = plugin->lookup[HISTOGRAM_RED];
		int *lookup_g = plugin->lookup[HISTOGRAM_GREEN];
		int *lookup_b = plugin->lookup[HISTOGRAM_BLUE];
		memset(accum_r, 0, sizeof(int) * HISTOGRAM_SLOTS);
		memset(accum_g, 0, sizeof(int) * HISTOGRAM_SLOTS);
		memset(accum_b, 0, sizeof(int) * HISTOGRAM_SLOTS);
		memset(accum_v, 0, sizeof(int) * HISTOGRAM_SLOTS);

		switch(data->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = pkg->start; i < pkg->end; i++)
			{
				uint16_t *row = (uint16_t*)data->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					r = row[0];
					g = row[1];
					b = row[2];

					// Value takes the maximum of
					//  the output RGB values
					CLAMP(r, 0, HISTOGRAM_SLOTS - 1);
					CLAMP(g, 0, HISTOGRAM_SLOTS - 1);
					CLAMP(b, 0, HISTOGRAM_SLOTS - 1);
					r_out = lookup_r[r];
					g_out = lookup_g[g];
					b_out = lookup_b[b];
					v = MAX(r_out, g_out);
					v = MAX(v, b_out); \
					v += -HISTOGRAM_MIN * 0xffff / 100;
					CLAMP(v, 0, HISTOGRAM_SLOTS - 1);
					accum_v[v]++;

					r += -HISTOGRAM_MIN * 0xffff / 100;
					g += -HISTOGRAM_MIN * 0xffff / 100;
					b += -HISTOGRAM_MIN * 0xffff / 100;
					CLAMP(r, 0, HISTOGRAM_SLOTS - 1);
					CLAMP(g, 0, HISTOGRAM_SLOTS - 1);
					CLAMP(b, 0, HISTOGRAM_SLOTS - 1);
					accum_r[r]++;
					accum_g[g]++;
					accum_b[b]++;
					row += 4;
				}
			}
			break;

		case BC_AYUV16161616:
			for(int i = pkg->start; i < pkg->end; i++)
			{
				uint16_t *row = (uint16_t*)data->get_row_ptr(i);

				for(int j = 0; j < w; j++) \
				{
					y = row[1];
					u = row[2];
					v = row[3];
					ColorSpaces::yuv_to_rgb_16(r, g, b, y, u, v);

					// Value takes the maximum of
					//  the output RGB values
					CLAMP(r, 0, HISTOGRAM_SLOTS - 1);
					CLAMP(g, 0, HISTOGRAM_SLOTS - 1);
					CLAMP(b, 0, HISTOGRAM_SLOTS - 1);
					r_out = lookup_r[r];
					g_out = lookup_g[g];
					b_out = lookup_b[b];
					v = MAX(r_out, g_out);
					v = MAX(v, b_out);
					v += -HISTOGRAM_MIN * 0xffff / 100;
					CLAMP(v, 0, HISTOGRAM_SLOTS - 1);
					accum_v[v]++;

					r += -HISTOGRAM_MIN * 0xffff / 100;
					g += -HISTOGRAM_MIN * 0xffff / 100;
					b += -HISTOGRAM_MIN * 0xffff / 100;
					CLAMP(r, 0, HISTOGRAM_SLOTS - 1);
					CLAMP(g, 0, HISTOGRAM_SLOTS - 1);
					CLAMP(b, 0, HISTOGRAM_SLOTS - 1);
					accum_r[r]++;
					accum_g[g]++;
					accum_b[b]++;
					row += 4;
				}
			}
			break;
		}
	}
	else
	if(server->operation == HistogramEngine::APPLY)
	{
		VFrame *input = plugin->input;
		int w = input->get_w();
		int h = input->get_h();
		int *lookup_r = plugin->lookup[HISTOGRAM_RED];
		int *lookup_g = plugin->lookup[HISTOGRAM_GREEN];
		int *lookup_b = plugin->lookup[HISTOGRAM_BLUE];
		int r, g, b, y, u, v, a;
		switch(input->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = pkg->start; i < pkg->end; i++)
			{
				uint16_t *row = (uint16_t*)input->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					if(plugin->config.split && ((j + i * w / h) < w) )
						continue;

					row[0] = lookup_r[row[0]];
					row[1] = lookup_g[row[1]];
					row[2] = lookup_b[row[2]];
					row += 4;
				}
			}
			break;

		case BC_AYUV16161616:
			for(int i = pkg->start; i < pkg->end; i++)
			{
				uint16_t *row = (uint16_t*)input->get_row_ptr(i);
				for(int j = 0; j < w; j++)
				{
					if(plugin->config.split && ((j + i * w / h) < w)) 
						continue;
					// Convert to 16 bit RGB
					y = row[1];
					u = row[2];
					v = row[3];

					ColorSpaces::yuv_to_rgb_16(r, g, b, y, u, v);

					// Look up in RGB domain
					r = lookup_r[r];
					g = lookup_g[g];
					b = lookup_b[b];

					// Convert to 16 bit YUV
					ColorSpaces::rgb_to_yuv_16(r, g, b, y, u, v);

					row[1] = y;
					row[2] = u;
					row[3] = v;
					row += 4;
				}
			}
			break;
		}
	}
	package_done = 1;
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
	total_size = data->get_h();

	for(int i = 0; i < get_total_packages(); i++)
	{
		HistogramPackage *package = (HistogramPackage*)get_package(i);
		package->start = total_size * i / get_total_packages();
		package->end = total_size * (i + 1) / get_total_packages();
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
