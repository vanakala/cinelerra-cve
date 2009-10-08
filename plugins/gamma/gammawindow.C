
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

#include "bcdisplayinfo.h"
#include "gammawindow.h"
#include "language.h"





PLUGIN_THREAD_OBJECT(GammaMain, GammaThread, GammaWindow)






GammaWindow::GammaWindow(GammaMain *client, int x, int y)
 : BC_Window(client->gui_string, x,
 	y,
	400, 
	350, 
	400, 
	350, 
	0, 
	0)
{ 
	this->client = client; 
}

int GammaWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(histogram = new BC_SubWindow(x, 
		y, 
		get_w() - x * 2, 
		get_h() - 150, 
		WHITE));
	y += histogram->get_h() + 10;

	BC_Title *title;
	add_tool(title = new BC_Title(x, y, _("Maximum:")));
	x += title->get_w() + 10;
	add_tool(max_slider = new MaxSlider(client, 
		this, 
		x, 
		y, 
		190));
	x += max_slider->get_w() + 10;
	add_tool(max_text = new MaxText(client,
		this,
		x,
		y,
		100));
	y += max_text->get_h() + 10;
	x = 10;
	add_tool(automatic = new GammaAuto(client, x, y));

	y += automatic->get_h() + 10;
	add_tool(title = new BC_Title(x, y, _("Gamma:")));
	x += title->get_w() + 10;
	add_tool(gamma_slider = new GammaSlider(client, 
		this, 
		x, 
		y, 
		190));
	x += gamma_slider->get_w() + 10;
	add_tool(gamma_text = new GammaText(client,
		this,
		x,
		y,
		100));
	y += gamma_text->get_h() + 10;
	x = 10;

	add_tool(plot = new GammaPlot(client, x, y));
	y += plot->get_h() + 10;

	add_tool(new GammaColorPicker(client, this, x, y));

	show_window();
	flush();
	return 0;
}

void GammaWindow::update()
{
	max_slider->update(client->config.max);
	max_text->update(client->config.max);
	gamma_slider->update(client->config.gamma);
	gamma_text->update(client->config.gamma);
	automatic->update(client->config.automatic);
	plot->update(client->config.plot);
	update_histogram();
}

void GammaWindow::update_histogram()
{
	histogram->clear_box(0, 0, histogram->get_w(), histogram->get_h());
	if(client->engine)
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
				accum += client->engine->accum[x];
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
				accum += client->engine->accum[x];
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
	float scale = 1.0 / client->config.max;
	float gamma = client->config.gamma - 1.0;
	float max = client->config.max;
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

WINDOW_CLOSE_EVENT(GammaWindow)

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
// Get colorpicker value
	float red = plugin->get_red();
	float green = plugin->get_green();
	float blue = plugin->get_blue();
// Get maximum value
	plugin->config.max = MAX(red, green);
	plugin->config.max = MAX(plugin->config.max, blue);
	gui->max_text->update(plugin->config.max);
	gui->max_slider->update(plugin->config.max);
	plugin->send_configure_change();
	return 1;	
}


