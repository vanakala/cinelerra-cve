
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

#ifndef SOUNDLEVEL_H
#define SOUNDLEVEL_H

#include "guicast.h"
#include "pluginaclient.h"

class SoundLevelEffect;
class SoundLevelWindow;

class SoundLevelConfig
{
public:
	SoundLevelConfig();
	void copy_from(SoundLevelConfig &that);
	int equivalent(SoundLevelConfig &that);
	void interpolate(SoundLevelConfig &prev, 
		SoundLevelConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	float duration;
};

class SoundLevelDuration : public BC_FSlider
{
public:
	SoundLevelDuration(SoundLevelEffect *plugin, int x, int y);
	int handle_event();
	SoundLevelEffect *plugin;
};

class SoundLevelWindow : public BC_Window
{
public:
	SoundLevelWindow(SoundLevelEffect *plugin, int x, int y);
	void create_objects();

	BC_Title *soundlevel_max;
	BC_Title *soundlevel_rms;
	SoundLevelDuration *duration;
	SoundLevelEffect *plugin;
};

PLUGIN_THREAD_HEADER(SoundLevelEffect, SoundLevelThread, SoundLevelWindow)

class SoundLevelEffect : public PluginAClient
{
public:
	SoundLevelEffect(PluginServer *server);
	~SoundLevelEffect();

	int is_realtime();
	int has_pts_api();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void process_frame_realtime(AFrame *input, AFrame *output);

	void load_defaults();
	void save_defaults();
	void reset();
	void update_gui();
	void render_gui(void *data, int size);

	PLUGIN_CLASS_MEMBERS(SoundLevelConfig, SoundLevelThread)

	double rms_accum;
	double max_accum;
	int accum_size;
};

#endif
