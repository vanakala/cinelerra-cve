
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

#ifndef GAIN_H
#define GAIN_H

#define PLUGIN_IS_AUDIO
#define PLUGIN_TITLE "Gain"
#define PLUGIN_CLASS Gain
#define PLUGIN_CONFIG_CLASS GainConfig
#define PLUGIN_THREAD_CLASS GainThread
#define PLUGIN_GUI_CLASS GainWindow

#include "pluginmacros.h"
#include "gainwindow.h"
#include "pluginaclient.h"

class GainConfig
{
public:
	GainConfig();
	int equivalent(GainConfig &that);
	void copy_from(GainConfig &that);
	void interpolate(GainConfig &prev, 
		GainConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double level;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class Gain : public PluginAClient
{
public:
	Gain(PluginServer *server);
	~Gain();

	void process_frame_realtime(AFrame *input, AFrame *output);

	PLUGIN_CLASS_MEMBERS
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void load_defaults();
	void save_defaults();

	DB db;
};

#endif
