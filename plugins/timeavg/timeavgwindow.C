
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

#include "bctitle.h"
#include "language.h"
#include "timeavgwindow.h"

PLUGIN_THREAD_METHODS

#define MAX_DURATION 4.0

TimeAvgWindow::TimeAvgWindow(TimeAvgMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
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

TimeAvgWindow::~TimeAvgWindow()
{
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

TimeAvgSlider::~TimeAvgSlider()
{
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
