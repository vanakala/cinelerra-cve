
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

#ifndef DESPIKE_H
#define DESPIKE_H

#define PLUGIN_TITLE N_("Despike")
#define PLUGIN_CLASS Despike
#define PLUGIN_CONFIG_CLASS DespikeConfig
#define PLUGIN_THREAD_CLASS DespikeThread
#define PLUGIN_GUI_CLASS DespikeWindow

class DespikeEngine;

#include "pluginmacros.h"
#include "despikewindow.h"
#include "language.h"
#include "pluginaclient.h"

class DespikeConfig
{
public:
	DespikeConfig();

	int equivalent(DespikeConfig &that);
	void copy_from(DespikeConfig &that);
	void interpolate(DespikeConfig &prev, 
		DespikeConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double level;
	double slope;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class Despike : public PluginAClient
{
public:
	Despike(PluginServer *server);
	~Despike();

	PLUGIN_CLASS_MEMBERS

	DB db;

	void process_frame_realtime(AFrame *input, AFrame *output);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

// non realtime support
	void load_defaults();
	void save_defaults();

	double last_sample;
};

#endif
