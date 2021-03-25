// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "language.h"
#include "seltempavg.h"

PLUGIN_THREAD_METHODS

#define MAX_DURATION 4.0

SelTempAvgWindow::SelTempAvgWindow(SelTempAvgMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	310,
	540)
{
	BC_WindowBase *win;
	int cmodel = plugin->get_project_color_model();
	int x1 = 10, x2 = 40, x3 = 80, x4 = 175, x5 = 260;

	y = 10;

	add_tool(win = print_title(x1, y, "%s: %s", plugin->plugin_title(),
		ColorModels::name(cmodel)));
	y += win->get_h() + 8;
	add_tool(new BC_Title(x1, y, _("Duration to average")));
	y += 20;
	add_tool(total_frames = new SelTempAvgSlider(plugin, x1, y));
	y += 20;

	add_tool(new BC_Title(x1, y, _("Use Method:")));
	y += 20;

	add_tool(method_none = new SelTempAvgMethodRadial(plugin, this, x1, y, SelTempAvgConfig::METHOD_NONE, _("None ")));
	y += 20;

	add_tool(method_seltempavg = new SelTempAvgMethodRadial(plugin, this, x1, y, SelTempAvgConfig::METHOD_SELTEMPAVG, _("Selective Temporal Averaging: ")));
	y += 25;

	add_tool(new BC_Title(x3, y, _("Av. Thres.")));
	add_tool(new BC_Title(x4, y, _("S.D. Thres.")));
	add_tool(new BC_Title(x5, y, _("Mask")));
	y += 25;

	add_tool(new BC_Title(x2, y, _("R / Y")));
	add_tool(avg_threshold_RY = new SelTempAvgThreshSlider(plugin, x3, y, AVG_RY, plugin->config.avg_threshold_RY));
	add_tool(std_threshold_RY = new SelTempAvgThreshSlider(plugin, x4, y, STD_RY, plugin->config.std_threshold_RY));
	add_tool(mask_RY = new SelTempAvgMask(plugin, x5, y, MASK_RY, plugin->config.mask_RY));

	y += 25;
	add_tool(new BC_Title(x2, y, _("G / U")));
	add_tool(avg_threshold_GU = new SelTempAvgThreshSlider(plugin, x3, y, AVG_GU, plugin->config.avg_threshold_GU));
	add_tool(std_threshold_GU = new SelTempAvgThreshSlider(plugin, x4, y, STD_GU, plugin->config.std_threshold_GU));
	add_tool(mask_GU = new SelTempAvgMask(plugin, x5, y, MASK_GU, plugin->config.mask_GU));

	y += 25;
	add_tool(new BC_Title(x2, y, _("B / V")));
	add_tool(avg_threshold_BV = new SelTempAvgThreshSlider(plugin, x3, y, AVG_BV, plugin->config.avg_threshold_BV));
	add_tool(std_threshold_BV = new SelTempAvgThreshSlider(plugin, x4, y, STD_BV, plugin->config.std_threshold_BV));
	add_tool(mask_BV = new SelTempAvgMask(plugin, x5, y, MASK_BV, plugin->config.mask_BV));

	y += 30;
	add_tool(method_average = new SelTempAvgMethodRadial(plugin, this, x1, y, SelTempAvgConfig::METHOD_AVERAGE, _("Average")));
	y += 20;
	add_tool(method_stddev = new SelTempAvgMethodRadial(plugin, this, x1, y, SelTempAvgConfig::METHOD_STDDEV, _("Standard Deviation")));

	y += 35;
	add_tool(new BC_Title(x1, y, _("First frame in average:")));
	y += 20;
	add_tool(offset_fixed = new SelTempAvgOffsetRadial(plugin, this, x1, y, SelTempAvgConfig::OFFSETMODE_FIXED, _("Fixed offset: ")));
	add_tool(offset_fixed_value = new SelTempAvgOffsetValue(plugin, x4, y));
	y += 25;

	add_tool(offset_restartmarker = new SelTempAvgOffsetRadial(plugin, this, x1, y, SelTempAvgConfig::OFFSETMODE_RESTARTMARKERSYS, _("Restart marker system:")));
	add_tool(offset_restartmarker_pos = new BC_TextBox(x4+20, y, 100, 1, ""));
	offset_restartmarker_pos->disable();
	y += 20;
	add_tool(offset_restartmarker_keyframe = new SelTempAvgStartKeyframe(plugin, x2 + 10, y));

	y += 35;

	add_tool(new BC_Title(x1, y, _("Other Options:")));
	y += 20;
	add_tool(paranoid = new SelTempAvgParanoid(plugin, x1, y));
	y += 25;
	add_tool(no_subtract = new SelTempAvgNoSubtract(plugin, x1, y));
	y += 30;
	add_tool(new BC_Title(x2, y, _("Gain:")));
	add_tool(gain = new SelTempAvgGainValue(plugin, x3, y));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void SelTempAvgWindow::update()
{
	total_frames->update(plugin->config.duration);

	method_none->update(plugin->config.method == SelTempAvgConfig::METHOD_NONE);
	method_seltempavg->update(plugin->config.method == SelTempAvgConfig::METHOD_SELTEMPAVG);
	method_average->update(plugin->config.method == SelTempAvgConfig::METHOD_AVERAGE);
	method_stddev->update(plugin->config.method == SelTempAvgConfig::METHOD_STDDEV);

	offset_fixed->update(plugin->config.offsetmode == SelTempAvgConfig::OFFSETMODE_FIXED);
	offset_restartmarker->update(plugin->config.offsetmode == SelTempAvgConfig::OFFSETMODE_RESTARTMARKERSYS);

	paranoid->update(plugin->config.paranoid);
	no_subtract->update(plugin->config.nosubtract);

	offset_fixed_value->update(plugin->config.offset_fixed_pts);
	gain->update(plugin->config.gain);

	avg_threshold_RY->update(plugin->config.avg_threshold_RY);
	avg_threshold_GU->update(plugin->config.avg_threshold_GU);
	avg_threshold_BV->update(plugin->config.avg_threshold_BV);
	std_threshold_RY->update(plugin->config.std_threshold_RY);
	std_threshold_GU->update(plugin->config.std_threshold_GU);
	std_threshold_BV->update(plugin->config.std_threshold_BV);

	mask_RY->update(plugin->config.mask_RY);
	mask_GU->update(plugin->config.mask_GU);
	mask_BV->update(plugin->config.mask_BV);
	offset_restartmarker_pos->update((float)plugin->restartoffset);
	offset_restartmarker_keyframe->update((plugin->config.offset_restartmarker_keyframe) && (plugin->onakeyframe));
}

SelTempAvgThreshSlider::SelTempAvgThreshSlider(SelTempAvgMain *client, int x, int y, int id, float currentval)
  : BC_TextBox(x,y, 80, 1, currentval)
{
	this->type = id;
	this->client = client;
}

int SelTempAvgThreshSlider::handle_event()
{
	float val = atof(get_text());

	if(val < 0)
		val = 0;

	switch(type)
	{
	case AVG_RY:
		client->config.avg_threshold_RY = val;
		break;
	case AVG_GU:
		client->config.avg_threshold_GU = val;
		break;
	case AVG_BV:
		client->config.avg_threshold_BV = val;
		break;
	case STD_RY:
		client->config.std_threshold_RY = val;
		break;
	case STD_GU:
		client->config.std_threshold_GU = val;
		break;
	case STD_BV:
		client->config.std_threshold_BV = val;
		break;
	}
	client->send_configure_change();
	return 1;
}


SelTempAvgOffsetValue::SelTempAvgOffsetValue(SelTempAvgMain *client, int x, int y)
  : BC_TextBox(x,y, 80, 1, client->config.offset_fixed_pts, 1, MEDIUMFONT, 2)
{
	this->client = client;
}

int SelTempAvgOffsetValue::handle_event()
{
	client->config.offset_fixed_pts = atof(get_text());
	client->send_configure_change();
	return 1;
}


SelTempAvgGainValue::SelTempAvgGainValue(SelTempAvgMain *client, int x, int y)
  : BC_TextBox(x,y, 80, 1, client->config.gain)
{
	this->client = client;
}

int SelTempAvgGainValue::handle_event()
{
	float val = atof(get_text());

	if(val < 0) val = 0;
	client->config.gain = val;
	client->send_configure_change();
	return 1;
}


SelTempAvgSlider::SelTempAvgSlider(SelTempAvgMain *client, int x, int y)
 : BC_FSlider(x,
	y,
	0,
	190,
	200,
	0.0,
	HISTORY_MAX_DURATION,
	client->config.duration)
{
	this->client = client;
}

int SelTempAvgSlider::handle_event()
{
	ptstime result = get_value();

	if(result < 0)
		result = 0;
	client->config.duration = result;
	client->send_configure_change();
	return 1;
}


SelTempAvgOffsetRadial::SelTempAvgOffsetRadial(SelTempAvgMain *client, SelTempAvgWindow *gui, int x, int y, int type, char *caption)
 : BC_Radial(x, 
	y,
	client->config.offsetmode == type,
	caption)
{
	this->client = client;
	this->gui = gui;
	this->type = type;
}

int SelTempAvgOffsetRadial::handle_event()
{
	int result = get_value();
	client->config.offsetmode = type;

	gui->offset_fixed->update(client->config.offsetmode == SelTempAvgConfig::OFFSETMODE_FIXED);
	gui->offset_restartmarker->update(client->config.offsetmode == SelTempAvgConfig::OFFSETMODE_RESTARTMARKERSYS);

	client->send_configure_change();
	return 1;
}


SelTempAvgMethodRadial::SelTempAvgMethodRadial(SelTempAvgMain *client, SelTempAvgWindow *gui, int x, int y, int type, char *caption)
 : BC_Radial(x, 
	y, 
	client->config.method == type,
	caption)
{
	this->client = client;
	this->gui = gui;
	this->type = type;
}

int SelTempAvgMethodRadial::handle_event()
{
	int result = get_value();
	client->config.method = type;

	gui->method_none->update(client->config.method       == SelTempAvgConfig::METHOD_NONE);
	gui->method_seltempavg->update(client->config.method == SelTempAvgConfig::METHOD_SELTEMPAVG);
	gui->method_average->update(client->config.method    == SelTempAvgConfig::METHOD_AVERAGE);
	gui->method_stddev->update(client->config.method     == SelTempAvgConfig::METHOD_STDDEV);

	client->send_configure_change();
	return 1;
}


SelTempAvgParanoid::SelTempAvgParanoid(SelTempAvgMain *client, int x, int y)
 : BC_CheckBox(x, 
	y,
	client->config.paranoid,
	_("Reprocess frame again"))
{
	this->client = client;
}

int SelTempAvgParanoid::handle_event()
{
	int result = get_value();
	client->config.paranoid = result;
	client->send_configure_change();
	return 1;
}


SelTempAvgNoSubtract::SelTempAvgNoSubtract(SelTempAvgMain *client, int x, int y)
 : BC_CheckBox(x, 
	y,
	client->config.nosubtract,
	_("Disable subtraction"))
{
	this->client = client;
}

int SelTempAvgNoSubtract::handle_event()
{
	int result = get_value();
	client->config.nosubtract = result;
	client->send_configure_change();
	return 1;
}


SelTempAvgMask::SelTempAvgMask(SelTempAvgMain *client, int x, int y, int type, int val)
 : BC_CheckBox(x, 
	y,
	val,
	0)
{
	this->client = client;
	this->type = type;
}

int SelTempAvgMask::handle_event()
{
	int result = get_value();

	switch(type)
	{
	case MASK_RY:
		client->config.mask_RY = result;
		break;
	case MASK_GU:
		client->config.mask_GU = result;
		break;
	case MASK_BV:
		client->config.mask_BV = result;
		break;
	}
	client->send_configure_change();
	return 1;
}


SelTempAvgStartKeyframe::SelTempAvgStartKeyframe(SelTempAvgMain *client, int x, int y)
 : BC_CheckBox(x, 
	y,
	client->config.nosubtract,
	_("This Frame is a start of a section"))
{
	this->client = client;
}

int SelTempAvgStartKeyframe::handle_event()
{
	int result = get_value();
	client->config.offset_restartmarker_keyframe = result;
	client->send_configure_change();
	return 1;
}
