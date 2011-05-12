
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

class Despike;
class DespikeEngine;

#include "despikewindow.h"
#include "pluginaclient.h"

class DespikeConfig
{
public:
	DespikeConfig();

	int equivalent(DespikeConfig &that);
	void copy_from(DespikeConfig &that);
	void interpolate(DespikeConfig &prev, 
		DespikeConfig &next, 
		posnum prev_frame,
		posnum next_frame, 
		posnum current_frame);

	double level;
	double slope;
};

class Despike : public PluginAClient
{
public:
	Despike(PluginServer *server);
	~Despike();

	PLUGIN_CLASS_MEMBERS(DespikeConfig, DespikeThread);

	void update_gui();

	DB db;

	int is_realtime();
	int process_realtime(int size, double *input_ptr, double *output_ptr);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

// non realtime support
	void load_defaults();
	void save_defaults();

	double last_sample;
};

#endif
