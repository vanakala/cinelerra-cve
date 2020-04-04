
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

#ifndef FRAMEFIELD_H
#define FRAMEFIELD_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Frames to fields")
#define PLUGIN_CLASS FrameField
#define PLUGIN_CONFIG_CLASS FrameFieldConfig
#define PLUGIN_THREAD_CLASS FrameFieldThread
#define PLUGIN_GUI_CLASS FrameFieldWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "keyframe.inc"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

#include <string.h>
#include <stdint.h>

#define TOP_FIELD_FIRST 0
#define BOTTOM_FIELD_FIRST 1

class FrameFieldConfig
{
public:
	FrameFieldConfig();

	int equivalent(FrameFieldConfig &src);

	int field_dominance;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class FrameFieldTop : public BC_Radial
{
public:
	FrameFieldTop(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};


class FrameFieldBottom : public BC_Radial
{
public:
	FrameFieldBottom(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};


class FrameFieldDouble : public BC_CheckBox
{
public:
	FrameFieldDouble(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};

class FrameFieldShift : public BC_CheckBox
{
public:
	FrameFieldShift(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};

class FrameFieldAvg : public BC_CheckBox
{
public:
	FrameFieldAvg(FrameField *plugin, FrameFieldWindow *gui, int x, int y);
	int handle_event();
	FrameField *plugin;
	FrameFieldWindow *gui;
};

class FrameFieldWindow : public PluginWindow
{
public:
	FrameFieldWindow(FrameField *plugin, int x, int y);

	void update();

	FrameFieldTop *top;
	FrameFieldBottom *bottom;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class FrameField : public PluginVClient
{
public:
	FrameField(PluginServer *server);
	~FrameField();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

// Constructs odd or even rows from the average of the surrounding rows.
	void average_rows(int offset, VFrame *frame);

	void handle_opengl();

// Field needed
	int field_number;

// Frame stored
	ptstime current_frame_pts;
	ptstime current_frame_duration;
};

#endif

