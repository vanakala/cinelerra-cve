
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

#ifndef DENOISEVIDEO_H
#define DENOISEVIDEO_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

// Old name was "Denoise video"
#define PLUGIN_TITLE N_("Denoise")
#define PLUGIN_CLASS DenoiseVideo
#define PLUGIN_CONFIG_CLASS DenoiseVideoConfig
#define PLUGIN_THREAD_CLASS DenoiseVideoThread
#define PLUGIN_GUI_CLASS DenoiseVideoWindow

#include "pluginmacros.h"

#include "bchash.inc"
#include "language.h"
#include "pluginvclient.h"
#include "vframe.inc"


class DenoiseVideoConfig
{
public:
	DenoiseVideoConfig();

	int equivalent(DenoiseVideoConfig &that);
	void copy_from(DenoiseVideoConfig &that);
	void interpolate(DenoiseVideoConfig &prev, 
		DenoiseVideoConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int frames;
	float threshold;
	int do_r, do_g, do_b, do_a;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class DenoiseVideoFrames : public BC_ISlider
{
public:
	DenoiseVideoFrames(DenoiseVideo *plugin, int x, int y);
	int handle_event();
	DenoiseVideo *plugin;
};


class DenoiseVideoThreshold : public BC_TextBox
{
public:
	DenoiseVideoThreshold(DenoiseVideo *plugin, int x, int y);
	int handle_event();
	DenoiseVideo *plugin;
};


class DenoiseVideoToggle : public BC_CheckBox
{
public:
	DenoiseVideoToggle(DenoiseVideo *plugin, 
		DenoiseVideoWindow *gui, 
		int x, 
		int y, 
		int *output,
		const char *text);
	int handle_event();
	DenoiseVideo *plugin;
	int *output;
};


class DenoiseVideoWindow : public BC_Window
{
public:
	DenoiseVideoWindow(DenoiseVideo *plugin, int x, int y);

	void update();

	DenoiseVideoFrames *frames;
	DenoiseVideoThreshold *threshold;
	DenoiseVideoToggle *do_r, *do_g, *do_b, *do_a;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER

class DenoiseVideo : public PluginVClient
{
public:
	DenoiseVideo(PluginServer *server);
	~DenoiseVideo();

	PLUGIN_CLASS_MEMBERS

	void process_realtime(VFrame *input, VFrame *output);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	float *accumulation;
};
#endif
