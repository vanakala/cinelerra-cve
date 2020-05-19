// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SHAPEWIPE_H
#define SHAPEWIPE_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Shape Wipe")
#define PLUGIN_CLASS ShapeWipeMain
#define PLUGIN_THREAD_CLASS ShapeWipeThread
#define PLUGIN_GUI_CLASS ShapeWipeWindow

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class ShapeWipeW2B : public BC_Radial
{
public:
	ShapeWipeW2B(ShapeWipeMain *plugin, 
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};

class ShapeWipeB2W : public BC_Radial
{
public:
	ShapeWipeB2W(ShapeWipeMain *plugin, 
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};

class ShapeWipeFilename : public BC_TextBox
{
public:
	ShapeWipeFilename(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		char *value,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
	char *value;
};

class ShapeWipeBrowseButton : public BC_GenericButton
{
public:
	ShapeWipeBrowseButton(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		ShapeWipeFilename *filename,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
	ShapeWipeFilename *filename;
};

class ShapeWipeAntiAlias : public BC_CheckBox
{
public:
	ShapeWipeAntiAlias(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};


class ShapeWipePreserveAspectRatio : public BC_CheckBox
{
public:
	ShapeWipePreserveAspectRatio(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};


class ShapeWipeLoad : public BC_FileBox
{
public:
	ShapeWipeLoad(ShapeWipeFilename *filename,
		char *init_directory);
	ShapeWipeFilename *filename;
};

class ShapeWipeWindow : public PluginWindow
{
public:
	ShapeWipeWindow(ShapeWipeMain *plugin, int x, int y);
	void reset_pattern_image();

	ShapeWipeW2B *left;
	ShapeWipeB2W *right;
	ShapeWipeFilename *filename_widget;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ShapeWipeMain : public PluginVClient
{
public:
	ShapeWipeMain(PluginServer *server);
	~ShapeWipeMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_realtime(VFrame *incoming, VFrame *outgoing);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int read_pattern_image(int new_frame_width, int new_frame_height);
	void reset_pattern_image();
	int load_configuration();

	int direction;
	char filename[BCTEXTLEN];
	char last_read_filename[BCTEXTLEN];
	unsigned char **pattern_image;
	unsigned char min_value;
	unsigned char max_value;
	int frame_width;
	int frame_height;
	int antialias;
	int preserve_aspect;
	int last_preserve_aspect;
};

#endif
