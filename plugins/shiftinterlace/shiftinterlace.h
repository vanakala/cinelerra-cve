// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SHIFTINTERLACE_H
#define SHIFTINTERLACE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("ShiftInterlace")
#define PLUGIN_CLASS ShiftInterlaceMain
#define PLUGIN_CONFIG_CLASS ShiftInterlaceConfig
#define PLUGIN_THREAD_CLASS ShiftInterlaceThread
#define PLUGIN_GUI_CLASS ShiftInterlaceWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"


class ShiftInterlaceConfig
{
public:
	ShiftInterlaceConfig();

	int equivalent(ShiftInterlaceConfig &that);
	void copy_from(ShiftInterlaceConfig &that);
	void interpolate(ShiftInterlaceConfig &prev, 
		ShiftInterlaceConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int odd_offset;
	int even_offset;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class ShiftInterlaceOdd : public BC_ISlider
{
public:
	ShiftInterlaceOdd(ShiftInterlaceMain *plugin, int x, int y);
	int handle_event();
	ShiftInterlaceMain *plugin;
};

class ShiftInterlaceEven : public BC_ISlider
{
public:
	ShiftInterlaceEven(ShiftInterlaceMain *plugin, int x, int y);
	int handle_event();
	ShiftInterlaceMain *plugin;
};

class ShiftInterlaceWindow : public PluginWindow
{
public:
	ShiftInterlaceWindow(ShiftInterlaceMain *plugin, int x, int y);

	void update();

	ShiftInterlaceOdd *odd_offset;
	ShiftInterlaceEven *even_offset;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class ShiftInterlaceMain : public PluginVClient
{
public:
	ShiftInterlaceMain(PluginServer *server);
	~ShiftInterlaceMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input_ptr);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	void shift_row(VFrame *input, int offset, int row);
};

#endif
