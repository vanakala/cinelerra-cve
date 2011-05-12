
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

class ScaleMain;
class ScaleWidth;
class ScaleHeight;
class ScaleConstrain;
class ScaleThread;
class ScaleWin;

#include "bchash.h"
#include "guicast.h"
#include "mutex.h"
#include "scalewin.h"
#include "overlayframe.h"
#include "pluginvclient.h"

class ScaleConfig
{
public:
	ScaleConfig();

	void copy_from(ScaleConfig &src);
	int equivalent(ScaleConfig &src);
	void interpolate(ScaleConfig &prev, 
		ScaleConfig &next, 
		posnum prev_frame, 
		posnum next_frame, 
		posnum current_frame);

	float w, h;
	int constrain;
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

class ScaleWin : public BC_Window
{
public:
	ScaleWin(ScaleMain *client, int x, int y);
	~ScaleWin();

	int create_objects();
	void close_event();

	ScaleMain *client;
	ScaleWidth *width;
	ScaleHeight *height;
	ScaleConstrain *constrain;
};

PLUGIN_THREAD_HEADER(ScaleMain, ScaleThread, ScaleWin)

class ScaleMain : public PluginVClient
{
public:
	ScaleMain(PluginServer *server);
	~ScaleMain();

	PLUGIN_CLASS_MEMBERS(ScaleConfig, ScaleThread);

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		framenum start_position,
		double frame_rate);
	void calculate_transfer(VFrame *frame,
		float &in_x1, 
		float &in_x2, 
		float &in_y1, 
		float &in_y2, 
		float &out_x1, 
		float &out_x2, 
		float &out_y1, 
		float &out_y2);
	int handle_opengl();
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	OverlayFrame *overlayer;   // To scale images
};


#endif
