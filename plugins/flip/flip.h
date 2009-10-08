
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


class FlipMain;

#include "filexml.h"
#include "flipwindow.h"
#include "guicast.h"
#include "pluginvclient.h"

class FlipConfig
{
public:
	FlipConfig();
	void copy_from(FlipConfig &that);
	int equivalent(FlipConfig &that);
	void interpolate(FlipConfig &prev, 
		FlipConfig &next, 
		long prev_frame, 
		long next_frame, 
		long current_frame);
	int flip_horizontal;
	int flip_vertical;
};

class FlipMain : public PluginVClient
{
public:
	FlipMain(PluginServer *server);
	~FlipMain();

	PLUGIN_CLASS_MEMBERS(FlipConfig, FlipThread);

// required for all realtime plugins
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int load_defaults();
	int save_defaults();
	int handle_opengl();
};


#endif
