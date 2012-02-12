
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

#define PLUGIN_TITLE N_("SoundLevel")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CLASS SoundLevelEffect
#define PLUGIN_CONFIG_CLASS SoundLevelConfig
#define PLUGIN_THREAD_CLASS SoundLevelThread
#define PLUGIN_GUI_CLASS SoundLevelWindow

#include "pluginmacros.h"
#include "language.h"
#include "guicast.h"
#include "pluginaclient.h"

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
	PLUGIN_CONFIG_CLASS_MEMBERS
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
	void update();

	BC_Title *soundlevel_max;
	BC_Title *soundlevel_rms;
	SoundLevelDuration *duration;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class SoundLevelEffect : public PluginAClient
{
public:
	SoundLevelEffect(PluginServer *server);
	~SoundLevelEffect();

	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void process_realtime(AFrame *input, AFrame *output);

	void load_defaults();
	void save_defaults();
	void render_gui(void *data, int size);

	PLUGIN_CLASS_MEMBERS

	double rms_accum;
	double max_accum;
	int accum_size;
};

#endif
