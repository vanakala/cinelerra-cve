// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TIMEAVGWINDOW_H
#define TIMEAVGWINDOW_H

class SelTempAvgParanoid;
class SelTempAvgNoSubtract;
class SelTempAvgStartKeyframe;
class SelTempAvgMask;

class SelTempAvgOffsetRadial;
class SelTempAvgMethodRadial;

class SelTempAvgSlider;

class SelTempAvgThreshSlider;
class SelTempAvgOffsetValue;
class SelTempAvgGainValue;

#include "bctextbox.h"
#include "bcslider.h"
#include "bctoggle.h"
#include "mutex.h"
#include "seltempavg.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER

enum
{
	AVG_RY,
	AVG_GU,
	AVG_BV,
	STD_RY,
	STD_GU,
	STD_BV
};

enum
{
	MASK_RY,
	MASK_GU,
	MASK_BV
};


class SelTempAvgWindow : public PluginWindow
{
public:
	SelTempAvgWindow(SelTempAvgMain *plugin, int x, int y);

	void update();

	SelTempAvgSlider *total_frames;

	SelTempAvgThreshSlider *avg_threshold_RY, *avg_threshold_GU, *avg_threshold_BV;
	SelTempAvgThreshSlider *std_threshold_RY, *std_threshold_GU, *std_threshold_BV;
	SelTempAvgMask *mask_RY, *mask_GU, *mask_BV;

	SelTempAvgOffsetRadial *offset_fixed, *offset_restartmarker;
	SelTempAvgMethodRadial *method_none, *method_seltempavg, *method_stddev, *method_average;

	SelTempAvgParanoid *paranoid;
	SelTempAvgNoSubtract *no_subtract;
	SelTempAvgStartKeyframe *offset_restartmarker_keyframe;
	BC_TextBox                *offset_restartmarker_pos;

	SelTempAvgOffsetValue *offset_fixed_value;
	SelTempAvgGainValue *gain;
	PLUGIN_GUI_CLASS_MEMBERS
};


class SelTempAvgThreshSlider : public BC_TextBox
{
public:
	SelTempAvgThreshSlider(SelTempAvgMain *client, int x, int y, int type, float curval);

	int handle_event();
	int type;
	SelTempAvgMain *client;
};


class SelTempAvgOffsetValue : public BC_TextBox
{
public:
	SelTempAvgOffsetValue(SelTempAvgMain *client, int x, int y);

	int handle_event();
	SelTempAvgMain *client;
};


class SelTempAvgGainValue : public BC_TextBox
{
public:
	SelTempAvgGainValue(SelTempAvgMain *client, int x, int y);

	int handle_event();
	SelTempAvgMain *client;
};


class SelTempAvgSlider : public BC_FSlider
{
public:
	SelTempAvgSlider(SelTempAvgMain *client, int x, int y);

	int handle_event();

	SelTempAvgMain *client;
};


class SelTempAvgOffsetRadial : public BC_Radial
{
public:
	SelTempAvgOffsetRadial(SelTempAvgMain *client, SelTempAvgWindow *gui, int x, int y, int type, char *caption);

	int handle_event();
	SelTempAvgMain *client;
	SelTempAvgWindow *gui;

	int type;
};


class SelTempAvgMethodRadial : public BC_Radial
{
public:
	SelTempAvgMethodRadial(SelTempAvgMain *client, SelTempAvgWindow *gui, int x, int y, int type, char *caption);

	int handle_event();

	SelTempAvgMain *client;
	SelTempAvgWindow *gui;
	int type;
};

class SelTempAvgParanoid : public BC_CheckBox
{
public:
	SelTempAvgParanoid(SelTempAvgMain *client, int x, int y);

	int handle_event();

	SelTempAvgMain *client;
};

class SelTempAvgNoSubtract : public BC_CheckBox
{
public:
	SelTempAvgNoSubtract(SelTempAvgMain *client, int x, int y);

	int handle_event();

	SelTempAvgMain *client;
};

class SelTempAvgMask : public BC_CheckBox
{
public:
	SelTempAvgMask(SelTempAvgMain *client, int x, int y,int type, int val);

	int handle_event();

	SelTempAvgMain *client;
	int type;
};


class SelTempAvgStartKeyframe : public BC_CheckBox
{
public:
	SelTempAvgStartKeyframe(SelTempAvgMain *client, int x, int y);

	int handle_event();

	SelTempAvgMain *client;
};

#endif
