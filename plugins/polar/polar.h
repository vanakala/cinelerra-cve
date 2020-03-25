
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
#ifndef POLAR_H
#define POLAR_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Polar")
#define PLUGIN_CLASS PolarEffect
#define PLUGIN_CONFIG_CLASS PolarConfig
#define PLUGIN_THREAD_CLASS PolarThread
#define PLUGIN_GUI_CLASS PolarWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class PolarEngine;

class PolarConfig
{
public:
	PolarConfig();

	void copy_from(PolarConfig &src);
	int equivalent(PolarConfig &src);
	void interpolate(PolarConfig &prev, 
		PolarConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	int polar_to_rectangular;
	float depth;
	float angle;
	int backwards;
	int invert;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class PolarDepth : public BC_FSlider
{
public:
	PolarDepth(PolarEffect *plugin, int x, int y);
	int handle_event();
	PolarEffect *plugin;
};

class PolarAngle : public BC_FSlider
{
public:
	PolarAngle(PolarEffect *plugin, int x, int y);
	int handle_event();
	PolarEffect *plugin;
};

class PolarWindow : public PluginWindow
{
public:
	PolarWindow(PolarEffect *plugin, int x, int y);

	void update();

	PolarDepth *depth;
	PolarAngle *angle;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class PolarPackage : public LoadPackage
{
public:
	PolarPackage();
	int row1, row2;
};

class PolarUnit : public LoadClient
{
public:
	PolarUnit(PolarEffect *plugin, PolarEngine *server);
	void process_package(LoadPackage *package);
	PolarEffect *plugin;
};

class PolarEngine : public LoadServer
{
public:
	PolarEngine(PolarEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	PolarEffect *plugin;
};

class PolarEffect : public PluginVClient
{
public:
	PolarEffect(PluginServer *server);
	~PolarEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PolarEngine *engine;
	VFrame *input, *output;
};

#endif


