// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "language.h"
#include "timeavgwindow.h"

PLUGIN_THREAD_METHODS

#define MAX_DURATION 4.0

TimeAvgWindow::TimeAvgWindow(TimeAvgMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y, 
	210, 
	210)
{ 
	x = y = 10;

	add_tool(new BC_Title(x, y, _("Duration to average")));
	y += 20;
	add_tool(total_frames = new TimeAvgSlider(plugin, x, y));
	y += 30;
	add_tool(accum = new TimeAvgAccum(plugin, this, x, y));
	y += 30;
	add_tool(avg = new TimeAvgAvg(plugin, this, x, y));
	y += 30;
	add_tool(inclusive_or = new TimeAvgOr(plugin, this, x, y));
	y += 30;
	add_tool(paranoid = new TimeAvgParanoid(plugin, x, y));
	y += 30;
	add_tool(no_subtract = new TimeAvgNoSubtract(plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void TimeAvgWindow::update()
{
	total_frames->update(plugin->config.duration);
	accum->update(plugin->config.mode == TimeAvgConfig::ACCUMULATE);
	avg->update(plugin->config.mode == TimeAvgConfig::AVERAGE);
	inclusive_or->update(plugin->config.mode == TimeAvgConfig::OR);
	paranoid->update(plugin->config.paranoid);
	no_subtract->update(plugin->config.nosubtract);
}


TimeAvgSlider::TimeAvgSlider(TimeAvgMain *client, int x, int y)
 : BC_FSlider(x, 
	y, 
	0,
	190, 
	200, 
	1.0 / MAX_DURATION,
	MAX_DURATION,
	client->config.duration)
{
	this->client = client;
}

int TimeAvgSlider::handle_event()
{
	float result = get_value();
	if(result < 1) result = 1.0 / MAX_DURATION;
	client->config.duration = result;
	client->send_configure_change();
	return 1;
}


TimeAvgAccum::TimeAvgAccum(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y)
 : BC_Radial(x, 
	y, 
	client->config.mode == TimeAvgConfig::ACCUMULATE,
	_("Accumulate"))
{
	this->client = client;
	this->gui = gui;
}

int TimeAvgAccum::handle_event()
{
	int result = get_value();
	client->config.mode = TimeAvgConfig::ACCUMULATE;
	gui->avg->update(0);
	gui->inclusive_or->update(0);
	client->send_configure_change();
	return 1;
}

TimeAvgAvg::TimeAvgAvg(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y)
 : BC_Radial(x, 
	y,
	client->config.mode == TimeAvgConfig::AVERAGE,
	_("Average"))
{
	this->client = client;
	this->gui = gui;
}

int TimeAvgAvg::handle_event()
{
	int result = get_value();
	client->config.mode = TimeAvgConfig::AVERAGE;
	gui->accum->update(0);
	gui->inclusive_or->update(0);
	client->send_configure_change();
	return 1;
}


TimeAvgOr::TimeAvgOr(TimeAvgMain *client, TimeAvgWindow *gui, int x, int y)
 : BC_Radial(x, 
	y,
	client->config.mode == TimeAvgConfig::OR,
	_("Inclusive Or"))
{
	this->client = client;
	this->gui = gui;
}

int TimeAvgOr::handle_event()
{
	int result = get_value();
	client->config.mode = TimeAvgConfig::OR;
	gui->accum->update(0);
	gui->avg->update(0);
	client->send_configure_change();
	return 1;
}


TimeAvgParanoid::TimeAvgParanoid(TimeAvgMain *client, int x, int y)
 : BC_CheckBox(x, 
	y,
	client->config.paranoid,
	_("Reprocess frame again"))
{
	this->client = client;
}

int TimeAvgParanoid::handle_event()
{
	int result = get_value();
	client->config.paranoid = result;
	client->send_configure_change();
	return 1;
}


TimeAvgNoSubtract::TimeAvgNoSubtract(TimeAvgMain *client, int x, int y)
 : BC_CheckBox(x, 
	y,
	client->config.nosubtract,
	_("Disable subtraction"))
{
	this->client = client;
}

int TimeAvgNoSubtract::handle_event()
{
	int result = get_value();
	client->config.nosubtract = result;
	client->send_configure_change();
	return 1;
}
