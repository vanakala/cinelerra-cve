
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

#ifndef WHIRL_H
#define WHIRL_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Whirl")
#define PLUGIN_CLASS WhirlEffect
#define PLUGIN_CONFIG_CLASS WhirlConfig
#define PLUGIN_THREAD_CLASS WhirlThread
#define PLUGIN_GUI_CLASS WhirlWindow

#include "pluginmacros.h"

#include "bcslider.h"
#include "bctitle.h"
#include "keyframe.inc"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.inc"

class WhirlEffect;
class WhirlWindow;
class WhirlEngine;

#define MAXRADIUS 100
#define MAXPINCH 100


class WhirlConfig
{
public:
	WhirlConfig();

	void copy_from(WhirlConfig &src);
	int equivalent(WhirlConfig &src);
	void interpolate(WhirlConfig &prev, 
		WhirlConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	double angle;
	double pinch;
	double radius;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class WhirlAngle : public BC_FSlider
{
public:
	WhirlAngle(WhirlEffect *plugin, int x, int y);

	int handle_event();
	WhirlEffect *plugin;
};


class WhirlPinch : public BC_FSlider
{
public:
	WhirlPinch(WhirlEffect *plugin, int x, int y);

	int handle_event();
	WhirlEffect *plugin;
};


class WhirlRadius : public BC_FSlider
{
public:
	WhirlRadius(WhirlEffect *plugin, int x, int y);

	int handle_event();
	WhirlEffect *plugin;
};

class WhirlWindow : public PluginWindow
{
public:
	WhirlWindow(WhirlEffect *plugin, int x, int y);

	void update();

	WhirlRadius *radius;
	WhirlPinch *pinch;
	WhirlAngle *angle;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class WhirlPackage : public LoadPackage
{
public:
	WhirlPackage();

	int row1, row2;
};

class WhirlUnit : public LoadClient
{
public:
	WhirlUnit(WhirlEffect *plugin, WhirlEngine *server);

	void process_package(LoadPackage *package);
	WhirlEngine *server;
	WhirlEffect *plugin;
};


class WhirlEngine : public LoadServer
{
public:
	WhirlEngine(WhirlEffect *plugin, int cpus);

	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	WhirlEffect *plugin;
};


class WhirlEffect : public PluginVClient
{
public:
	WhirlEffect(PluginServer *server);
	~WhirlEffect();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *input);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	WhirlEngine *engine;
	VFrame *input, *output;
};

#endif
