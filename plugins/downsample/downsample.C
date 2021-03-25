// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "downsample.h"
#include "filexml.h"
#include "keyframe.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

REGISTER_PLUGIN

const char *DownSampleWindow::ds_chn_rgba[] =
{
    N_("Red"),
    N_("Green"),
    N_("Blue"),
    N_("Alpha")
};

const char *DownSampleWindow::ds_chn_ayuv[] =
{
    N_("Alpha"),
    N_("Y"),
    N_("U"),
    N_("V")
};

#define CHNL_MAX 4

DownSampleConfig::DownSampleConfig()
{
	horizontal = 2;
	vertical = 2;
	horizontal_x = 0;
	vertical_y = 0;
	chan0 = 1;
	chan1 = 1;
	chan2 = 1;
	chan3 = 1;
}

int DownSampleConfig::equivalent(DownSampleConfig &that)
{
	return horizontal == that.horizontal &&
		vertical == that.vertical &&
		horizontal_x == that.horizontal_x &&
		vertical_y == that.vertical_y &&
		chan0 == that.chan0 &&
		chan1 == that.chan1 &&
		chan2 == that.chan2 &&
		chan3 == that.chan3;
}

void DownSampleConfig::copy_from(DownSampleConfig &that)
{
	horizontal = that.horizontal;
	vertical = that.vertical;
	horizontal_x = that.horizontal_x;
	vertical_y = that.vertical_y;
	chan0 = that.chan0;
	chan1 = that.chan1;
	chan2 = that.chan2;
	chan3 = that.chan3;
}

void DownSampleConfig::interpolate(DownSampleConfig &prev, 
	DownSampleConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->horizontal = round(prev.horizontal * prev_scale + next.horizontal * next_scale);
	this->vertical = round(prev.vertical * prev_scale + next.vertical * next_scale);
	this->horizontal_x = round(prev.horizontal_x * prev_scale + next.horizontal_x * next_scale);
	this->vertical_y = round(prev.vertical_y * prev_scale + next.vertical_y * next_scale);
	chan0 = prev.chan0;
	chan1 = prev.chan1;
	chan2 = prev.chan2;
	chan3 = prev.chan3;
}


PLUGIN_THREAD_METHODS

DownSampleWindow::DownSampleWindow(DownSampleMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	230, 
	380)
{
	BC_WindowBase *win;
	int cmodel = plugin->get_project_color_model();
	int title_h;
	const char **channels;

	if(cmodel == BC_AYUV16161616)
		channels = ds_chn_ayuv;
	else
		channels = ds_chn_rgba;

	x = y = 10;
	add_subwindow(win = print_title(x, y, "%s: %s",
		_(plugin->plugin_title()), ColorModels::name(cmodel)));
	title_h = win->get_h() + 8;
	y += title_h;
	add_subwindow(win = new BC_Title(x, y, _("Horizontal")));
	y += title_h;
	add_subwindow(h = new DownSampleSize(plugin, x, y,
		&plugin->config.horizontal,
		1, 100));
	y += h->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Horizontal offset")));
	y += title_h;
	add_subwindow(h_x = new DownSampleSize(plugin, x, y,
		&plugin->config.horizontal_x, 0, 100));
	y += h_x->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Vertical")));
	y += title_h;
	add_subwindow(v = new DownSampleSize(plugin, x, y,
		&plugin->config.vertical, 1,100));
	y += v->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Vertical offset")));
	y += title_h;
	add_subwindow(v_y = new DownSampleSize(plugin, x, y,
		&plugin->config.vertical_y, 0, 100));
	y += v_y->get_h() + 8;
	add_subwindow(chan0 = new DownSampleToggle(plugin, x, y,
		&plugin->config.chan0, _(channels[0])));
	y += chan0->get_h() + 8;
	add_subwindow(chan1 = new DownSampleToggle(plugin, x, y,
		&plugin->config.chan1, _(channels[1])));
	y += chan1->get_h() + 8;
	add_subwindow(chan2 = new DownSampleToggle(plugin, x, y,
		&plugin->config.chan2, _(channels[2])));
	y += chan2->get_h() + 8;
	add_subwindow(chan3 = new DownSampleToggle(plugin, x, y,
		&plugin->config.chan3, _(channels[3])));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void DownSampleWindow::update()
{
	h->update(plugin->config.horizontal);
	v->update(plugin->config.vertical);
	h_x->update(plugin->config.horizontal_x);
	v_y->update(plugin->config.vertical_y);
	chan0->update(plugin->config.chan0);
	chan1->update(plugin->config.chan1);
	chan2->update(plugin->config.chan2);
	chan3->update(plugin->config.chan3);
}


DownSampleToggle::DownSampleToggle(DownSampleMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int DownSampleToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

DownSampleSize::DownSampleSize(DownSampleMain *plugin, 
	int x, 
	int y, 
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int DownSampleSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


DownSampleMain::DownSampleMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

DownSampleMain::~DownSampleMain()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void DownSampleMain::reset_plugin()
{
	delete engine;
	engine = 0;
}

PLUGIN_CLASS_METHODS

VFrame *DownSampleMain::process_tmpframe(VFrame *input_ptr)
{
	if(load_configuration())
		update_gui();

	switch(input_ptr->get_color_model())
	{
	case BC_RGBA16161616:
		if(config.chan3)
			input_ptr->set_transparent();
		break;
	case BC_AYUV16161616:
		if(config.chan0)
			input_ptr->set_transparent();
		break;
	default:
		unsupported(input_ptr->get_color_model());
		return input_ptr;
	}

	this->output = input_ptr;

	if(!engine)
		engine = new DownSampleServer(this,
			get_project_smp(),
			get_project_smp());
	engine->process_packages();
	return output;
}

void DownSampleMain::load_defaults()
{
	defaults = load_defaults_file("downsample.rc");

	config.horizontal = defaults->get("HORIZONTAL", config.horizontal);
	config.vertical = defaults->get("VERTICAL", config.vertical);
	config.horizontal_x = defaults->get("HORIZONTAL_X", config.horizontal_x);
	config.vertical_y = defaults->get("VERTICAL_Y", config.vertical_y);
	// Compatibility
	config.chan0 = defaults->get("R", config.chan0);
	config.chan1 = defaults->get("G", config.chan1);
	config.chan2 = defaults->get("B", config.chan2);
	config.chan3 = defaults->get("A", config.chan3);

	config.chan0 = defaults->get("CHAN0", config.chan0);
	config.chan1 = defaults->get("CHAN1", config.chan1);
	config.chan2 = defaults->get("CHAN2", config.chan2);
	config.chan3 = defaults->get("CHAN3", config.chan3);
}

void DownSampleMain::save_defaults()
{
	defaults->update("HORIZONTAL", config.horizontal);
	defaults->update("VERTICAL", config.vertical);
	defaults->update("HORIZONTAL_X", config.horizontal_x);
	defaults->update("VERTICAL_Y", config.vertical_y);
	defaults->delete_key("R");
	defaults->delete_key("G");
	defaults->delete_key("B");
	defaults->delete_key("A");
	defaults->update("CHAN0", config.chan0);
	defaults->update("CHAN1", config.chan1);
	defaults->update("CHAN2", config.chan2);
	defaults->update("CHAN3", config.chan3);
	defaults->save();
}

void DownSampleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("DOWNSAMPLE");

	output.tag.set_property("HORIZONTAL", config.horizontal);
	output.tag.set_property("VERTICAL", config.vertical);
	output.tag.set_property("HORIZONTAL_X", config.horizontal_x);
	output.tag.set_property("VERTICAL_Y", config.vertical_y);
	output.tag.set_property("CHAN0", config.chan0);
	output.tag.set_property("CHAN1", config.chan1);
	output.tag.set_property("CHAN2", config.chan2);
	output.tag.set_property("CHAN3", config.chan3);
	output.append_tag();
	output.tag.set_title("/DOWNSAMPLE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void DownSampleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("DOWNSAMPLE"))
		{
			config.horizontal = input.tag.get_property("HORIZONTAL", config.horizontal);
			config.vertical = input.tag.get_property("VERTICAL", config.vertical);
			config.horizontal_x = input.tag.get_property("HORIZONTAL_X", config.horizontal_x);
			config.vertical_y = input.tag.get_property("VERTICAL_Y", config.vertical_y);
			// Compatibility
			config.chan0 = input.tag.get_property("R", config.chan0);
			config.chan1 = input.tag.get_property("G", config.chan1);
			config.chan2 = input.tag.get_property("B", config.chan2);
			config.chan3 = input.tag.get_property("A", config.chan3);

			config.chan0 = input.tag.get_property("CHAN0", config.chan0);
			config.chan1 = input.tag.get_property("CHAN1", config.chan1);
			config.chan2 = input.tag.get_property("CHAN2", config.chan2);
			config.chan3 = input.tag.get_property("CHAN3", config.chan3);
		}
	}
}


DownSamplePackage::DownSamplePackage()
 : LoadPackage()
{
}


DownSampleUnit::DownSampleUnit(DownSampleServer *server, 
	DownSampleMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

void DownSampleUnit::process_package(LoadPackage *package)
{
	DownSamplePackage *pkg = (DownSamplePackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();
	int ch0sum;
	int ch1sum;
	int ch2sum;
	int ch3sum;
	int do0 = plugin->config.chan0;
	int do1 = plugin->config.chan1;
	int do2 = plugin->config.chan2;
	int do3 = plugin->config.chan3;

	switch(plugin->output->get_color_model())
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		for(int i = pkg->y1; i < pkg->y2; i += plugin->config.vertical)
		{
			int y1 = MAX(i, 0);
			int y2 = MIN(i + plugin->config.vertical, h);

			for(int j = plugin->config.horizontal_x - plugin->config.horizontal;
				j < w; j += plugin->config.horizontal)
			{
				int x1 = MAX(j, 0);
				int x2 = MIN(j + plugin->config.horizontal, w);

				int scale = (x2 - x1) * (y2 - y1);

				if(x2 > x1 && y2 > y1)
				{
					// Read in values
					ch0sum = ch1sum = ch2sum = ch3sum = 0;
					for(int k = y1; k < y2; k++)
					{
						uint16_t *row = ((uint16_t*)plugin->output->get_row_ptr(k)) + x1 * 4;

						for(int l = x1; l < x2; l++)
						{
							if(do0) ch0sum += *row++; else row++;
							if(do1) ch1sum += *row++; else row++;
							if(do2) ch2sum += *row++; else row++;
							if(do3) ch3sum += *row++; else row++;
						}
					}
				}
				// Write average
				ch0sum /= scale;
				ch1sum /= scale;
				ch2sum /= scale;
				ch3sum /= scale;

				for(int k = y1; k < y2; k++)
				{
					uint16_t *row = ((uint16_t*)plugin->output->get_row_ptr(k)) + x1 * 4;

					for(int l = x1; l < x2; l++)
					{
						if(do0) *row++ = ch0sum; else row++;
						if(do1) *row++ = ch1sum; else row++;
						if(do2) *row++ = ch2sum; else row++;
						if(do3) *row++ = ch3sum; else row++;
					}
				}
			}
		}
		break;
	}
}


DownSampleServer::DownSampleServer(DownSampleMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void DownSampleServer::init_packages()
{
	int y1 = plugin->config.vertical_y - plugin->config.vertical;
	int total_strips = ((double)plugin->output->get_h() / plugin->config.vertical + 1);
	int strips_per_package = ((double)total_strips / get_total_packages() + 1);

	for(int i = 0; i < get_total_packages(); i++)
	{
		DownSamplePackage *package = (DownSamplePackage*)get_package(i);
		package->y1 = y1;
		package->y2 = y1 + strips_per_package * plugin->config.vertical;
		package->y1 = MIN(plugin->output->get_h(), package->y1);
		package->y2 = MIN(plugin->output->get_h(), package->y2);
		y1 = package->y2;
	}
}

LoadClient* DownSampleServer::new_client()
{
	return new DownSampleUnit(this, plugin);
}

LoadPackage* DownSampleServer::new_package()
{
	return new DownSamplePackage;
}
