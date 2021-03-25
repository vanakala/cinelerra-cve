// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bchash.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "motionblur.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"


REGISTER_PLUGIN

MotionBlurConfig::MotionBlurConfig()
{
	radius = 10;
	steps = 10;
}

int MotionBlurConfig::equivalent(MotionBlurConfig &that)
{
	return radius == that.radius &&
		steps == that.steps;
}

void MotionBlurConfig::copy_from(MotionBlurConfig &that)
{
	radius = that.radius;
	steps = that.steps;
}

void MotionBlurConfig::interpolate(MotionBlurConfig &prev, 
	MotionBlurConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->radius = round((double)prev.radius * prev_scale + next.radius * next_scale);
	this->steps = round((double)prev.steps * prev_scale + next.steps * next_scale);
}


PLUGIN_THREAD_METHODS

MotionBlurWindow::MotionBlurWindow(MotionBlurMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	260, 
	120)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Length:")));
	y += 20;
	add_subwindow(radius = new MotionBlurSize(plugin, x, y, &plugin->config.radius, 0, 100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Steps:")));
	y += 20;
	add_subwindow(steps = new MotionBlurSize(plugin, x, y, &plugin->config.steps, 1, 100));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void MotionBlurWindow::update()
{
	radius->update(plugin->config.radius);
	steps->update(plugin->config.steps);
}


MotionBlurSize::MotionBlurSize(MotionBlurMain *plugin, 
	int x, 
	int y, 
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, 240, 240, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int MotionBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


MotionBlurMain::MotionBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
	accum = 0;
	accum_size = 0;
	table_steps = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

MotionBlurMain::~MotionBlurMain()
{
	if(engine) delete engine;
	delete_tables();
	if(accum) delete [] accum;
	PLUGIN_DESTRUCTOR_MACRO
}

void MotionBlurMain::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
		delete_tables();
		delete [] accum;
		accum = 0;
	}
}

PLUGIN_CLASS_METHODS

void MotionBlurMain::delete_tables()
{
	if(scale_x_table)
	{
		for(int i = 0; i < table_entries; i++)
			delete [] scale_x_table[i];
		delete [] scale_x_table;
	}

	if(scale_y_table)
	{
		for(int i = 0; i < table_entries; i++)
			delete [] scale_y_table[i];
		delete [] scale_y_table;
	}
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
}

VFrame *MotionBlurMain::process_tmpframe(VFrame *input_ptr)
{
	double xa, ya, za;
	double xb, yb, zb;
	double xd, yd, zd;
	ptstime frame_pts = input_ptr->get_pts();
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

	if(frame_pts < EPSILON)
		get_camera(&xa, &ya, &za, frame_pts);
	else
		get_camera(&xa, &ya, &za, frame_pts - input_ptr->get_duration());
	get_camera(&xb, &yb, &zb, frame_pts);

	xd = xb - xa;
	yd = yb - ya;
	zd = zb - za;

	if(load_configuration())
		update_gui();

	if(!engine)
		engine = new MotionBlurEngine(this,
			get_project_smp(),
			get_project_smp());
	if(!accum)
	{
		accum_size = input_ptr->get_w() * input_ptr->get_h() * 4 * sizeof(int);
		accum = new unsigned char[accum_size];
	}

	input = input_ptr;
	output = clone_vframe(input_ptr);
	output->copy_from(input);

// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	int w = input->get_w();
	int h = input->get_h();
	int x_offset;
	int y_offset;

	double fradius = config.radius * 0.5;
	double zradius = zd * fradius / 4 + 1;
	double center_x = w / 2;
	double center_y = h / 2;

	double min_w, min_h;
	double max_w, max_h;
	double min_x1, min_y1, min_x2, min_y2;
	double max_x1, max_y1, max_x2, max_y2;

	table_steps = config.steps ? config.steps : 1;

	x_offset = round(xd * fradius);
	y_offset = round(yd * fradius);

	min_w = w * zradius;
	min_h = h * zradius;
	max_w = w;
	max_h = h;
	min_x1 = center_x - min_w / 2;
	min_y1 = center_y - min_h / 2;
	min_x2 = center_x + min_w / 2;
	min_y2 = center_y + min_h / 2;
	max_x1 = 0;
	max_y1 = 0;
	max_x2 = w;
	max_y2 = h;

	if(table_entries < table_steps)
	{
		delete_tables();

		table_entries = 2 * table_steps;
		scale_x_table = new int*[table_entries];
		scale_y_table = new int*[table_entries];
		memset(scale_x_table, 0, table_entries * sizeof(int*));
		memset(scale_y_table, 0, table_entries * sizeof(int*));
	}

	for(int i = 0; i < table_steps; i++)
	{
		double fraction = ((double)i - config.steps / 2) / config.steps;
		double inv_fraction = 1.0 - fraction;

		int x = round(fraction * x_offset);
		int y = round(fraction * y_offset);

		double out_x1 = min_x1 * fraction + max_x1 * inv_fraction;
		double out_x2 = min_x2 * fraction + max_x2 * inv_fraction;
		double out_y1 = min_y1 * fraction + max_y1 * inv_fraction;
		double out_y2 = min_y2 * fraction + max_y2 * inv_fraction;
		double out_w = out_x2 - out_x1;
		double out_h = out_y2 - out_y1;

		if(out_w < 1)
			out_w = 1;
		if(out_h < 1)
			out_h = 1;
		double scale_x = w / out_w;
		double scale_y = h / out_h;

		if(!scale_y_table[i])
			scale_y_table[i] = new int[h + 1];
		if(!scale_x_table[i])
			scale_x_table[i] = new int[w + 1];

		int *x_table = scale_x_table[i];
		int *y_table = scale_y_table[i];

		for(int j = 0; j < h; j++)
			y_table[j] = (int)((j - out_y1) * scale_y) + y;

		for(int j = 0; j < w; j++)
			x_table[j] = (int)((j - out_x1) * scale_x) + x;
	}
	memset(accum, 0, accum_size);
	engine->process_packages();
	release_vframe(input);
	return output;
}

void MotionBlurMain::load_defaults()
{
	defaults = load_defaults_file("motionblur.rc");

	config.radius = defaults->get("RADIUS", config.radius);
	config.steps = defaults->get("STEPS", config.steps);
}


void MotionBlurMain::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("STEPS", config.steps);
	defaults->save();
}

void MotionBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("MOTIONBLUR");
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("STEPS", config.steps);
	output.append_tag();
	output.tag.set_title("/MOTIONBLUR");
	output.append_tag();
	keyframe->set_data(output.string);
}

void MotionBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("MOTIONBLUR"))
		{
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.steps = input.tag.get_property("STEPS", config.steps);
		}
	}
}


MotionBlurPackage::MotionBlurPackage()
 : LoadPackage()
{
}


MotionBlurUnit::MotionBlurUnit(MotionBlurEngine *server, 
	MotionBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

void MotionBlurUnit::process_package(LoadPackage *package)
{
	MotionBlurPackage *pkg = (MotionBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();

	int fraction = 0x10000 / plugin->table_steps;

	for(int i = 0; i < plugin->table_steps; i++)
	{
		int *x_table = plugin->scale_x_table[i];
		int *y_table = plugin->scale_y_table[i];

		switch(plugin->input->get_color_model())
		{
		case BC_RGBA16161616:
			for(int j = pkg->y1; j < pkg->y2; j++)
			{
				int *out_row = (int*)plugin->accum + 4 * w * j;
				int in_y = y_table[j];

				// Blend image
				if(in_y >= 0 && in_y < h)
				{
					uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(in_y);

					for(int k = 0; k < w; k++)
					{
						int in_x = x_table[k];

						// Blend pixel
						if(in_x >= 0 && in_x < w)
						{
							int in_offset = in_x * 4;
							*out_row++ += in_row[in_offset];
							*out_row++ += in_row[in_offset + 1];
							*out_row++ += in_row[in_offset + 2];
							*out_row++ += in_row[in_offset + 3];
						}
						// Blend nothing
						else
							out_row += 4;
					}
				}
			}
			// Copy to output
			if(i == plugin->table_steps - 1)
			{
				for(int j = pkg->y1; j < pkg->y2; j++)
				{
					int *in_row = (int*)plugin->accum + 4 * w * j;
					uint16_t *in_backup = (uint16_t*)plugin->input->get_row_ptr(j);
					uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(j);

					for(int k = 0; k < w; k++)
					{
						*out_row++ = (*in_row++ * fraction) / 0x10000;
						in_backup++;
						*out_row++ = (*in_row++ * fraction) / 0x10000;
						in_backup++;
						*out_row++ = (*in_row++ * fraction) / 0x10000;
						in_backup++;
						*out_row++ = (*in_row++ * fraction) / 0x10000;
						in_backup++;
					}
				}
			}
			break;

		case BC_AYUV16161616:
			for(int j = pkg->y1; j < pkg->y2; j++)
			{
				int *out_row = (int*)plugin->accum + 4 * w * j;
				int in_y = y_table[j];

				// Blend image
				if(in_y >= 0 && in_y < h)
				{
					uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(in_y);

					for(int k = 0; k < w; k++)
					{
						int in_x = x_table[k];
						// Blend pixel
						if(in_x >= 0 && in_x < w)
						{
							int in_offset = in_x * 4;

							*out_row++ += in_row[in_offset];
							*out_row++ += in_row[in_offset + 1];
							*out_row++ += in_row[in_offset + 2];
							*out_row++ += in_row[in_offset + 3];
						}
						// Blend nothing
						else
						{
							out_row++;
							out_row++;
							*out_row++ += 0x8000;
							*out_row++ += 0x8000;
						}
					}
				}
				else
				{
					for(int k = 0; k < w; k++)
					{
						out_row++;
						out_row++;
						*out_row++ += 0x8000;
						*out_row++ += 0x8000;
					}
				}
			}

			// Copy to output
			if(i == plugin->table_steps - 1)
			{
				for(int j = pkg->y1; j < pkg->y2; j++)
				{
					int *in_row = (int*)plugin->accum + 4 * w * j;
					uint16_t *in_backup = (uint16_t*)plugin->input->get_row_ptr(j);
					uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(j);
					for(int k = 0; k < w; k++)
					{
						*out_row++ = (*in_row++ * fraction) / 0x10000;
						in_backup++;
						*out_row++ = (*in_row++ * fraction) / 0x10000;
						in_backup++;
						*out_row++ = (*in_row++ * fraction) / 0x10000;
						in_backup++;
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++;
					}
				}
			}
			break;
		}
	}
}


MotionBlurEngine::MotionBlurEngine(MotionBlurMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void MotionBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		MotionBlurPackage *package = (MotionBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* MotionBlurEngine::new_client()
{
	return new MotionBlurUnit(this, plugin);
}

LoadPackage* MotionBlurEngine::new_package()
{
	return new MotionBlurPackage;
}
