
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

#ifndef SCALE_H
#define SCALE_H

// the simplest plugin possible

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE N_("Scale")
#define PLUGIN_CLASS ScaleMain
#define PLUGIN_CONFIG_CLASS ScaleConfig
#define PLUGIN_THREAD_CLASS ScaleThread
#define PLUGIN_GUI_CLASS ScaleWin

#include "pluginmacros.h"

class ScaleWidth;
class ScaleHeight;
class ScaleConstrain;

#include "bchash.h"
#include "bctextbox.h"
#include "bctoggle.h"
#include "language.h"
#include "mutex.h"
#include "overlayframe.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

class ScaleConfig
{
public:
	ScaleConfig();

	void copy_from(ScaleConfig &src);
	int equivalent(ScaleConfig &src);
	void interpolate(ScaleConfig &prev, 
		ScaleConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	float w, h;
	int constrain;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class ScaleWidth : public BC_TumbleTextBox
{
public:
	ScaleWidth(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleWidth();
	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
};

class ScaleHeight : public BC_TumbleTextBox
{
public:
	ScaleHeight(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleHeight();
	int handle_event();

	ScaleMain *client;
	ScaleWin *win;
};

class ScaleConstrain : public BC_CheckBox
{
public:
	ScaleConstrain(ScaleMain *client, int x, int y);
	~ScaleConstrain();
	int handle_event();

	ScaleMain *client;
};

class ScaleWin : public PluginWindow
{
public:
	ScaleWin(ScaleMain *plugin, int x, int y);
	~ScaleWin();

	void update();

	ScaleWidth *width;
	ScaleHeight *height;
	ScaleConstrain *constrain;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ScaleMain : public PluginVClient
{
public:
	ScaleMain(PluginServer *server);
	~ScaleMain();

	PLUGIN_CLASS_MEMBERS

// required for all realtime plugins
	void process_frame(VFrame *frame);
	void calculate_transfer(VFrame *frame,
		float &in_x1, 
		float &in_x2, 
		float &in_y1, 
		float &in_y2, 
		float &out_x1, 
		float &out_x2, 
		float &out_y1, 
		float &out_y2);
	void handle_opengl();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	OverlayFrame *overlayer;   // To scale images
};
#endif
