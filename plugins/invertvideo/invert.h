
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

#ifndef INVERT_H
#define INVERT_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

// Old name was "Invert Video"

#define PLUGIN_TITLE N_("Invert")
#define PLUGIN_CLASS InvertVideoEffect
#define PLUGIN_CONFIG_CLASS InvertVideoConfig
#define PLUGIN_THREAD_CLASS InvertVideoThread
#define PLUGIN_GUI_CLASS InvertVideoWindow

#define GL_GLEXT_PROTOTYPES

#include "pluginmacros.h"

#include "bctoggle.h"
#include "clip.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class InvertVideoConfig
{
public:
	InvertVideoConfig();

	void copy_from(InvertVideoConfig &src);
	int equivalent(InvertVideoConfig &src);
	void interpolate(InvertVideoConfig &prev, 
		InvertVideoConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int r, g, b, a;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class InvertVideoEnable : public BC_CheckBox
{
public:
	InvertVideoEnable(InvertVideoEffect *plugin, int *output, int x, int y, char *text);
	int handle_event();
	InvertVideoEffect *plugin;
	int *output;
};

class InvertVideoWindow : public PluginWindow
{
public:
	InvertVideoWindow(InvertVideoEffect *plugin, int x, int y);

	void update();

	InvertVideoEnable *r, *g, *b, *a;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class InvertVideoEffect : public PluginVClient
{
public:
	InvertVideoEffect(PluginServer *server);
	~InvertVideoEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();
};

#endif
