// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "blur.h"
#include "blurwindow.h"

PLUGIN_THREAD_METHODS

const char *BlurWindow::blur_chn_rgba[] =
{
    N_("Blur red"),
    N_("Blur green"),
    N_("Blur blue"),
    N_("Blur alpha")
};

const char *BlurWindow::blur_chn_ayuv[] =
{
    N_("Blur alpha"),
    N_("Blur Y"),
    N_("Blur U"),
    N_("Blur V")
};

BlurWindow::BlurWindow(BlurMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	150, 
	270)
{
	BC_WindowBase *win;
	const char **names;
	int vspan = 8;
	int cmodel = plugin->get_project_color_model();
	x = 10;
	y = 10;

	if(cmodel == BC_AYUV16161616)
		names = blur_chn_ayuv;
	else
		names = blur_chn_rgba;

	win = add_subwindow(print_title(x, y, "%s: %s", _(plugin->plugin_title()),
		ColorModels::name(cmodel)));
	y += win->get_h() + vspan;
	add_subwindow(horizontal = new BlurCheckBox(plugin, x, y,
		&plugin->config.horizontal, _("Horizontal")));
	y += horizontal->get_h() + vspan;
	add_subwindow(vertical = new BlurCheckBox(plugin, x, y,
		&plugin->config.vertical, _("Vertical")));
	y += vertical->get_h() + vspan;
	add_subwindow(radius = new BlurRadius(plugin, x, y));
	add_subwindow(new BC_Title(x + radius->get_w() +10, y, _("Radius")));
	y += radius->get_h() + vspan;
	add_subwindow(chan0 = new BlurCheckBox(plugin, x, y,
		&plugin->config.chan0, _(names[0])));
	y += chan0->get_h() + vspan ;
	add_subwindow(chan1 = new BlurCheckBox(plugin, x, y,
		&plugin->config.chan1, _(names[1])));
	y += chan1->get_h() + vspan ;
	add_subwindow(chan2 = new BlurCheckBox(plugin, x, y,
		&plugin->config.chan2, _(names[2])));
	y += chan2->get_h() + vspan;
	add_subwindow(chan3 = new BlurCheckBox(plugin, x, y,
		&plugin->config.chan3, _(names[3])));
	y += chan3->get_h() + vspan;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void BlurWindow::update()
{
	horizontal->update(plugin->config.horizontal);
	vertical->update(plugin->config.vertical);
	radius->update(plugin->config.radius);
	chan0->update(plugin->config.chan0);
	chan1->update(plugin->config.chan1);
	chan2->update(plugin->config.chan2);
	chan3->update(plugin->config.chan3);
}


BlurRadius::BlurRadius(BlurMain *client, int x, int y)
 : BC_IPot(x, y, client->config.radius, 0, MAXRADIUS)
{
	this->client = client;
}

int BlurRadius::handle_event()
{
	client->config.radius = get_value();
	client->send_configure_change();
	return 1;
}


BlurCheckBox::BlurCheckBox(BlurMain *plugin, int x, int y, int *value,
	const char *boxname)
 : BC_CheckBox(x, y, *value, boxname)
{
	this->value = value;
	this->plugin = plugin;
}

int BlurCheckBox::handle_event()
{
	*value = get_value();
	plugin->send_configure_change();
	return 1;
}

