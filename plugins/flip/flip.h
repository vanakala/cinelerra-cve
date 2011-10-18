
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

#ifndef FLIP_H
#define FLIP_H


#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE N_("Flip")
#define PLUGIN_CLASS FlipMain
#define PLUGIN_CONFIG_CLASS FlipConfig
#define PLUGIN_THREAD_CLASS FlipThread
#define PLUGIN_GUI_CLASS FlipWindow

#include "pluginmacros.h"
#include "filexml.h"
#include "flipwindow.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"

class FlipConfig
{
public:
	FlipConfig();
	void copy_from(FlipConfig &that);
	int equivalent(FlipConfig &that);
	void interpolate(FlipConfig &prev, 
		FlipConfig &next,
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	int flip_horizontal;
	int flip_vertical;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class FlipMain : public PluginVClient
{
public:
	FlipMain(PluginServer *server);
	~FlipMain();

	PLUGIN_CLASS_MEMBERS;

// required for all realtime plugins
	void process_frame(VFrame *frame);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();
	void handle_opengl();
};
#endif
