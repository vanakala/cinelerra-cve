
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
#include "language.h"
#include "seltempavgwindow.h"

PLUGIN_THREAD_OBJECT(SelTempAvgMain, SelTempAvgThread, SelTempAvgWindow)


#define MAX_FRAMES 1024


SelTempAvgWindow::SelTempAvgWindow(SelTempAvgMain *client, int x, int y)
 : BC_Window(client->gui_string,
	x,
	y,
	310,
	540,
	300,
	540,
	0,
	0,
	1)
{
	this->client = client;
}

SelTempAvgWindow::~SelTempAvgWindow()
{
}

int SelTempAvgWindow::create_objects()
{
	int x1 = 10, x2 = 40, x3 = 80, x4 = 175, x5 = 260, y = 10;

	add_tool(new BC_Title(x1, y, _("Frames to average")));
	y += 20;
	add_tool(total_frames = new SelTempAvgSlider(client, x1, y));
	y += 20;

	add_tool(new BC_Title(x1, y, _("Use Method:")));
	y += 20;

	add_tool(method_none = new SelTempAvgMethodRadial(client, this, x1, y, SelTempAvgConfig::METHOD_NONE, _("None ")));
	y += 20;

	add_tool(method_seltempavg = new SelTempAvgMethodRadial(client, this, x1, y, SelTempAvgConfig::METHOD_SELTEMPAVG, _("Selective Temporal Averaging: ")));
	y += 25;

	add_tool(new BC_Title(x3, y, _("Av. Thres.")));
	add_tool(new BC_Title(x4, y, _("S.D. Thres.")));
	add_tool(new BC_Title(x5, y, _("Mask")));
	y += 25;

	add_tool(new BC_Title(x2, y, _("R / Y")));
        add_tool(avg_threshold_RY = new SelTempAvgThreshSlider(client, x3, y, AVG_RY,client->config.avg_threshold_RY));
        add_tool(std_threshold_RY = new SelTempAvgThreshSlider(client, x4, y, STD_RY,client->config.std_threshold_RY));
        add_tool(mask_RY = new SelTempAvgMask(client, x5, y, MASK_RY, client->config.mask_RY));

	y += 25;
	add_tool(new BC_Title(x2, y, _("G / U")));
        add_tool(avg_threshold_GU = new SelTempAvgThreshSlider(client, x3, y, AVG_GU,client->config.avg_threshold_GU));
        add_tool(std_threshold_GU = new SelTempAvgThreshSlider(client, x4, y, STD_GU,client->config.std_threshold_GU));
        add_tool(mask_GU = new SelTempAvgMask(client, x5, y, MASK_GU,client->config.mask_GU));

	y += 25;
	add_tool(new BC_Title(x2, y, _("B / V")));
        add_tool(avg_threshold_BV = new SelTempAvgThreshSlider(client, x3, y, AVG_BV,client->config.avg_threshold_BV));
        add_tool(std_threshold_BV = new SelTempAvgThreshSlider(client, x4, y, STD_BV,client->config.std_threshold_BV));
        add_tool(mask_BV = new SelTempAvgMask(client, x5, y, MASK_BV,client->config.mask_BV));

	y += 30;
	add_tool(method_average = new SelTempAvgMethodRadial(client, this, x1, y, SelTempAvgConfig::METHOD_AVERAGE, _("Average")));
	y += 20;
	add_tool(method_stddev = new SelTempAvgMethodRadial(client, this, x1, y, SelTempAvgConfig::METHOD_STDDEV, _("Standard Deviation")));

	y += 35;
	add_tool(new BC_Title(x1, y, _("First frame in average:")));
	y += 20;
	add_tool(offset_fixed = new SelTempAvgOffsetRadial(client, this, x1, y, SelTempAvgConfig::OFFSETMODE_FIXED, _("Fixed offset: ")));
        add_tool(offset_fixed_value = new SelTempAvgOffsetValue(client, x4, y));
	y += 25;

	add_tool(offset_restartmarker = new SelTempAvgOffsetRadial(client, this, x1, y, SelTempAvgConfig::OFFSETMODE_RESTARTMARKERSYS, _("Restart marker system:")));
	add_tool(offset_restartmarker_pos = new BC_TextBox(x4+20, y, 100, 1, ""));
	offset_restartmarker_pos->disable();
	y += 20;
	add_tool(offset_restartmarker_keyframe = new SelTempAvgStartKeyframe(client, x2 + 10, y));

	y += 35;

	add_tool(new BC_Title(x1, y, _("Other Options:")));
	y += 20;
	add_tool(paranoid = new SelTempAvgParanoid(client, x1, y));
	y += 25;
	add_tool(no_subtract = new SelTempAvgNoSubtract(client, x1, y));
	y += 30;
	add_tool(new BC_Title(x2, y, _("Gain:")));
        add_tool(gain = new SelTempAvgGainValue(client, x3, y));

	show_window();
	flush();
	return 0;
}

WINDOW_CLOSE_EVENT(SelTempAvgWindow)




SelTempAvgThreshSlider::SelTempAvgThreshSlider(SelTempAvgMain *client, int x, int y, int id, float currentval)
  : BC_TextBox(x,y, 80, 1, currentval)
{
  //	float val;
  //	int   ival;
	this->type = id;
	this->client = client;
}
SelTempAvgThreshSlider::~SelTempAvgThreshSlider()
{
}
int SelTempAvgThreshSlider::handle_event()
{
	float val = atof(get_text());

	if(val < 0) val = 0;

	switch (type) {
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
  : BC_TextBox(x,y, 80, 1, client->config.offset_fixed_value)
{
	this->client = client;
}
SelTempAvgOffsetValue::~SelTempAvgOffsetValue()
{
}
int SelTempAvgOffsetValue::handle_event()
{
	int val = atoi(get_text());

	client->config.offset_fixed_value = val;
	client->send_configure_change();
	return 1;
}


SelTempAvgGainValue::SelTempAvgGainValue(SelTempAvgMain *client, int x, int y)
  : BC_TextBox(x,y, 80, 1, client->config.gain)
{
	this->client = client;
}
SelTempAvgGainValue::~SelTempAvgGainValue()
{
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
 : BC_ISlider(x,
	y,
	0,
	190,
	200,
	1,
	MAX_FRAMES,
	client->config.frames)
{
	this->client = client;
}
SelTempAvgSlider::~SelTempAvgSlider()
{
}
int SelTempAvgSlider::handle_event()
{
	int result = get_value();
	if(result < 1) result = 1;
	client->config.frames = result;
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
	"")
{
	this->client = client;
	this->type = type;
}
int SelTempAvgMask::handle_event()
{
	int result = get_value();
	switch (type) {
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
