// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsubwindow.h"
#include "clip.h"
#include "bctitle.h"
#include "gammawindow.h"
#include "language.h"


PLUGIN_THREAD_METHODS


GammaWindow::GammaWindow(GammaMain *plugin, int x, int y)
 : PluginWindow(plugin, x,
	y,
	400, 
	400)
{
	x = y = 10;

	add_subwindow(histogram = new BC_SubWindow(x, 
		y, 
		get_w() - x * 2, 
		get_h() - 200,
		WHITE));
	y += histogram->get_h() + 10;

	BC_Title *title;
	add_tool(title = new BC_Title(x, y, _("Maximum:")));
	x += title->get_w() + 10;
	add_tool(max_slider = new MaxSlider(plugin,
		this, 
		x, 
		y, 
		190));
	x += max_slider->get_w() + 10;
	add_tool(max_text = new MaxText(plugin,
		this,
		x,
		y,
		100));
	y += max_text->get_h() + 10;
	x = 10;
	add_tool(automatic = new GammaAuto(plugin, x, y));

	y += automatic->get_h() + 10;
	add_tool(title = new BC_Title(x, y, _("Gamma:")));
	x += title->get_w() + 10;
	add_tool(gamma_slider = new GammaSlider(plugin,
		this, 
		x, 
		y, 
		190));
	x += gamma_slider->get_w() + 10;
	add_tool(gamma_text = new GammaText(plugin,
		this,
		x,
		y,
		100));
	y += gamma_text->get_h() + 10;
	x = 10;

	add_tool(plot = new GammaPlot(plugin, x, y));
	y += plot->get_h() + 10;

	add_tool(new GammaColorPicker(plugin, this, x, y));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void GammaWindow::update()
{
	max_slider->update(plugin->config.max);
	max_text->update(plugin->config.max);
	gamma_slider->update(plugin->config.gamma);
	gamma_text->update(plugin->config.gamma);
	automatic->update(plugin->config.automatic);
	plot->update(plugin->config.plot);
	update_histogram();
}

void GammaWindow::update_histogram()
{
	histogram->clear_box(0, 0, histogram->get_w(), histogram->get_h());
	if(plugin->engine)
	{
		int max = 0;
		histogram->set_color(MEGREY);
		for(int i = 0; i < histogram->get_w(); i++)
		{
			int x1 = (int64_t)i * HISTOGRAM_SIZE / histogram->get_w();
			int x2 = (int64_t)(i + 1) * HISTOGRAM_SIZE / histogram->get_w();
			if(x2 == x1) x2++;
			int accum = 0;
			for(int x = x1; x < x2; x++)
			{
				accum += plugin->engine->accum[x];
			}
			if(accum > max) max = accum;
		}
		for(int i = 0; i < histogram->get_w(); i++)
		{
			int x1 = (int64_t)i * HISTOGRAM_SIZE / histogram->get_w();
			int x2 = (int64_t)(i + 1) * HISTOGRAM_SIZE / histogram->get_w();
			if(x2 == x1) x2++;
			int accum = 0;
			for(int x = x1; x < x2; x++)
			{
				accum += plugin->engine->accum[x];
			}

			int h = (int)(log(accum) / log(max) * histogram->get_h());
			histogram->draw_line(i, 
				histogram->get_h(), 
				i, 
				histogram->get_h() - h);
		}
	}

	histogram->set_color(GREEN);
	int y1 = histogram->get_h();
	float scale = 1.0 / plugin->config.max;
	float gamma = plugin->config.gamma - 1.0;
	float max = plugin->config.max;
	for(int i = 1; i < histogram->get_w(); i++)
	{
		float in = (float)i / histogram->get_w();
		float out = in * (scale * pow(in * 2 / max, gamma));
		int y2 = (int)(histogram->get_h() - out * histogram->get_h());
		histogram->draw_line(i - 1, y1, i, y2);
		y1 = y2;
	}
	histogram->flash();
}


MaxSlider::MaxSlider(GammaMain *client, 
	GammaWindow *gui, 
	int x, 
	int y,
	int w)
 : BC_FSlider(x, 
	y, 
	0, 
	w, 
	w,
	0.0, 
	1.0, 
	client->config.max)
{
	this->client = client;
	this->gui = gui;
	set_precision(0.01);
}

int MaxSlider::handle_event()
{
	client->config.max = get_value();
	gui->max_text->update(client->config.max);
	gui->update_histogram();
	client->send_configure_change();
	return 1;
}

MaxText::MaxText(GammaMain *client,
	GammaWindow *gui,
	int x,
	int y,
	int w)
 : BC_TextBox(x, y, w, 1, client->config.max)
{
	this->client = client;
	this->gui = gui;
}

int MaxText::handle_event()
{
	client->config.max = atof(get_text());
	gui->max_slider->update(client->config.max);
	client->send_configure_change();
	return 1;
}

GammaSlider::GammaSlider(GammaMain *client, 
	GammaWindow *gui, 
	int x, 
	int y,
	int w)
 : BC_FSlider(x, 
	y,
	0, 
	w, 
	w,
	0.0, 
	1.0, 
	client->config.gamma)
{
	this->client = client;
	this->gui = gui;
	set_precision(0.01);
}

int GammaSlider::handle_event()
{
	client->config.gamma = get_value();
	gui->gamma_text->update(client->config.gamma);
	gui->update_histogram();
	client->send_configure_change();
	return 1;
}

GammaText::GammaText(GammaMain *client,
	GammaWindow *gui,
	int x,
	int y,
	int w)
 : BC_TextBox(x, y, w, 1, client->config.gamma)
{
	this->client = client;
	this->gui = gui;
}

int GammaText::handle_event()
{
	client->config.gamma = atof(get_text());
	gui->gamma_slider->update(client->config.gamma);
	client->send_configure_change();
	return 1;
}

GammaAuto::GammaAuto(GammaMain *client, int x, int y)
 : BC_CheckBox(x, 
	y,
	client->config.automatic, 
	_("Automatic"))
{
	this->plugin = client;
}

int GammaAuto::handle_event()
{
	plugin->config.automatic = get_value();
	plugin->send_configure_change();
	return 1;
}


GammaPlot::GammaPlot(GammaMain *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.plot, _("Plot histogram"))
{
	this->plugin = plugin;
}

int GammaPlot::handle_event()
{
	plugin->config.plot = get_value();
	plugin->send_configure_change();
	return 1;
}


GammaColorPicker::GammaColorPicker(GammaMain *plugin, 
	GammaWindow *gui, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Use Color Picker"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int GammaColorPicker::handle_event()
{
	int red, green, blue;
	int colormax;
// Get colorpicker value
	plugin->get_picker_rgb(&red, &green, &blue);
// Get maximum value
	colormax = MAX(red, green);
	colormax = MAX(colormax, blue);
	plugin->config.max = (double)colormax / 0x10000;
	gui->max_text->update(plugin->config.max);
	gui->max_slider->update(plugin->config.max);
	plugin->send_configure_change();
	return 1;
}
