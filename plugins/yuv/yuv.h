
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

#ifndef YUV_H
#define YUV_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("YUV")
#define PLUGIN_CLASS YUVEffect
#define PLUGIN_CONFIG_CLASS YUVConfig
#define PLUGIN_THREAD_CLASS YUVThread
#define PLUGIN_GUI_CLASS YUVWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "keyframe.inc"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"


class YUVConfig
{
public:
	YUVConfig();

	void copy_from(YUVConfig &src);
	int equivalent(YUVConfig &src);
	void interpolate(YUVConfig &prev, 
		YUVConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	double y, u, v;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class YUVLevel : public BC_FSlider
{
public:
	YUVLevel(YUVEffect *plugin, double *output, int x, int y);
	int handle_event();
	YUVEffect *plugin;
	double *output;
};

class YUVWindow : public PluginWindow
{
public:
	YUVWindow(YUVEffect *plugin, int x, int y);

	void update();

	YUVLevel *y, *u, *v;
	YUVEffect *plugin;
};

PLUGIN_THREAD_HEADER

class YUVEffect : public PluginVClient
{
public:
	YUVEffect(PluginServer *server);
	~YUVEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
};

#endif
