
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

#ifndef INTERPOLATEVIDEO_H
#define INTERPOLATEVIDEO_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Interpolate")
#define PLUGIN_CLASS InterpolateVideo
#define PLUGIN_CONFIG_CLASS InterpolateVideoConfig
#define PLUGIN_THREAD_CLASS InterpolateVideoThread
#define PLUGIN_GUI_CLASS InterpolateVideoWindow

#include "pluginmacros.h"

#include "bctoggle.h"
#include "keyframe.inc"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "selection.h"
#include "vframe.inc"

class InterpolateVideoConfig
{
public:
	InterpolateVideoConfig();

	void copy_from(InterpolateVideoConfig *config);
	int equivalent(InterpolateVideoConfig *config);

// Frame rate of input
	double input_rate;
// If 1, use the keyframes as beginning and end frames and ignore input rate
	int use_keyframes;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class InterpolateVideoRate : public FrameRateSelection
{
public:
	InterpolateVideoRate(InterpolateVideo *plugin, 
		InterpolateVideoWindow *gui, 
		int x, 
		int y);

	int handle_event();

	InterpolateVideo *plugin;
};


class InterpolateVideoKeyframes : public BC_CheckBox
{
public:
	InterpolateVideoKeyframes(InterpolateVideo *plugin,
		InterpolateVideoWindow *gui,
		int x, 
		int y);
	int handle_event();
	InterpolateVideoWindow *gui;
	InterpolateVideo *plugin;
};

class InterpolateVideoWindow : public PluginWindow
{
public:
	InterpolateVideoWindow(InterpolateVideo *plugin, int x, int y);

	void update();
	void update_enabled();

	InterpolateVideoRate *rate;
	InterpolateVideoKeyframes *keyframes;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class InterpolateVideo : public PluginVClient
{
public:
	InterpolateVideo(PluginServer *server);
	~InterpolateVideo();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	void fill_border(ptstime start_pts);

// beginning and end frames
	VFrame *frames[2];

// Current requested positions
	ptstime range_start_pts;
	ptstime range_end_pts;
};

#endif
