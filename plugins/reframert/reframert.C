
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS

#define PLUGIN_TITLE N_("ReframeRT")
#define PLUGIN_CLASS ReframeRT
#define PLUGIN_CONFIG_CLASS ReframeRTConfig
#define PLUGIN_THREAD_CLASS ReframeRTThread
#define PLUGIN_GUI_CLASS ReframeRTWindow

#include "pluginmacros.h"

#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"

#include <string.h>
#include "picon_png.h"


class ReframeRTConfig
{
public:
	ReframeRTConfig();

	void boundaries();
	int equivalent(ReframeRTConfig &src);
	void copy_from(ReframeRTConfig &src);
	void interpolate(ReframeRTConfig &prev, 
		ReframeRTConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	double scale;
	int stretch;
	int interp;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class ReframeRTScale : public BC_TumbleTextBox
{
public:
	ReframeRTScale(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
};


class ReframeRTStretch : public BC_Radial
{
public:
	ReframeRTStretch(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};


class ReframeRTDownsample : public BC_Radial
{
public:
	ReframeRTDownsample(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};


class ReframeRTInterpolate : public BC_CheckBox
{
public:
	ReframeRTInterpolate(ReframeRT *plugin,
		ReframeRTWindow *gui,
		int x,
		int y);
	int handle_event();
	ReframeRT *plugin;
	ReframeRTWindow *gui;
};


class ReframeRTWindow : public PluginWindow
{
public:
	ReframeRTWindow(ReframeRT *plugin, int x, int y);
	~ReframeRTWindow();

	void update();

	ReframeRTScale *scale;
	ReframeRTStretch *stretch;
	ReframeRTDownsample *downsample;
	ReframeRTInterpolate *interpolate;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class ReframeRT : public PluginVClient
{
public:
	ReframeRT(PluginServer *server);
	~ReframeRT();

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void process_frame(VFrame *frame);

	KeyFrame *fake_keyframe;
};


REGISTER_PLUGIN


ReframeRTConfig::ReframeRTConfig()
{
	scale = 1.0;
	stretch = 0;
	interp = 0;
}

int ReframeRTConfig::equivalent(ReframeRTConfig &src)
{
	return fabs(scale - src.scale) < 0.0001 &&
		stretch == src.stretch &&
		interp == src.interp;
}

void ReframeRTConfig::copy_from(ReframeRTConfig &src)
{
	this->scale = src.scale;
	this->stretch = src.stretch;
	this->interp = src.interp;
}

void ReframeRTConfig::interpolate(ReframeRTConfig &prev, 
	ReframeRTConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	this->interp = prev.interp;
	this->stretch = prev.stretch;

	if (this->interp && !PTSEQU(prev_pts, next_pts))
	{
// for interpolation, this is (for now) a simple linear slope to the next keyframe.
		double slope = (next.scale - prev.scale) / (next_pts - prev_pts);
		this->scale = (slope * (current_pts - prev_pts)) + prev.scale;
	}
	else
	{
		this->scale = prev.scale;
	}
}

void ReframeRTConfig::boundaries()
{
	if(fabs(scale) < 0.0001) scale = 0.0001;
}


ReframeRTWindow::ReframeRTWindow(ReframeRT *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	210, 
	160)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Scale by amount:")));
	y += 20;
	scale = new ReframeRTScale(plugin, 
		this,
		x, 
		y);
	scale->set_increment(0.1);
	y += 30;
	add_subwindow(stretch = new ReframeRTStretch(plugin, 
		this,
		x, 
		y));
	y += 30;
	add_subwindow(downsample = new ReframeRTDownsample(plugin, 
		this,
		x, 
		y));
	y += 30;
	add_subwindow(interpolate = new ReframeRTInterpolate(plugin,
		this,
		x, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

ReframeRTWindow::~ReframeRTWindow()
{
}

void ReframeRTWindow::update()
{
	scale->update((float)plugin->config.scale);
	stretch->update(plugin->config.stretch);
	downsample->update(!plugin->config.stretch);
	interpolate->update(plugin->config.interp);
}


PLUGIN_THREAD_METHODS


ReframeRTScale::ReframeRTScale(ReframeRT *plugin, 
	ReframeRTWindow *gui,
	int x, 
	int y)
 : BC_TumbleTextBox(gui,
	(float)plugin->config.scale,
	(float)-1000,
	(float)1000,
	x,
	y, 
	100)
{
	set_precision(8);
	this->plugin = plugin;
}

int ReframeRTScale::handle_event()
{
	plugin->config.scale = atof(get_text());
	plugin->config.boundaries();
	plugin->send_configure_change();
	return 1;
}


ReframeRTStretch::ReframeRTStretch(ReframeRT *plugin,
	ReframeRTWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, plugin->config.stretch, _("Stretch"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ReframeRTStretch::handle_event()
{
	plugin->config.stretch = get_value();
	gui->downsample->update(!get_value());
	plugin->send_configure_change();
	return 1;
}


ReframeRTDownsample::ReframeRTDownsample(ReframeRT *plugin,
	ReframeRTWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, !plugin->config.stretch, _("Downsample"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ReframeRTDownsample::handle_event()
{
	plugin->config.stretch = !get_value();
	gui->stretch->update(!get_value());
	plugin->send_configure_change();
	return 1;
}


ReframeRTInterpolate::ReframeRTInterpolate(ReframeRT *plugin,
	ReframeRTWindow *gui,
	int x,
	int y)
 : BC_CheckBox(x, y, 0, _("Interpolate"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ReframeRTInterpolate::handle_event()
{
	plugin->config.interp = get_value();
	gui->interpolate->update(get_value());
	plugin->send_configure_change();
	return 1;
}


ReframeRT::ReframeRT(PluginServer *server)
 : PluginVClient(server)
{
	fake_keyframe = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}


ReframeRT::~ReframeRT()
{
	if(fake_keyframe)
		delete fake_keyframe;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void ReframeRT::process_frame(VFrame *frame)
{
	ptstime input_pts = source_start_pts;
	ReframeRTConfig prev_config, next_config;
	KeyFrame *tmp_keyframe, *next_keyframe = prev_keyframe_pts(source_start_pts);
	ptstime duration;
	ptstime tmp_pts, next_pts;
	int is_current_keyframe;
// FIXPOS
// if there are no keyframes, the default keyframe is used, and its position is always 0;
// if there are keyframes, the first keyframe can be after the effect start (and it controls settings before it)
// so let's calculate using a fake keyframe with the same settings but position == effect start
	if(!fake_keyframe)
		fake_keyframe = new KeyFrame();
	fake_keyframe->copy_from(next_keyframe);
	fake_keyframe->pos_time = source_start_pts;
	next_keyframe = fake_keyframe;

	// calculate input_frame accounting for all previous keyframes
	do
	{
		tmp_keyframe = next_keyframe;
		next_keyframe = next_keyframe_pts(tmp_keyframe->pos_time + EPSILON);

		tmp_pts = tmp_keyframe->pos_time;
		next_pts = next_keyframe->pos_time;

		is_current_keyframe =
			next_pts > source_pts // the next keyframe is after the current position
			|| PTSEQU(next_keyframe->pos_time, tmp_keyframe->pos_time) // there are no more keyframes
			|| !next_keyframe->pos_time; // there are no keyframes at all

		if (is_current_keyframe)
			duration = source_pts - tmp_pts;
		else
			duration = next_pts - tmp_pts;

		read_data(next_keyframe);
		next_config.copy_from(config);
		read_data(tmp_keyframe);
		prev_config.copy_from(config);
		config.interpolate(prev_config, next_config, tmp_pts, next_pts, tmp_pts + duration);

// the area under the curve is the number of frames to advance
// as long as interpolate() uses a linear slope we can use geometry to determine this
// if interpolate() changes to use a curve then this needs use (possibly) the definite integral
		input_pts += (duration * ((prev_config.scale + config.scale) / 2));
	} while (!is_current_keyframe);

	frame->set_pts(input_pts);
	get_frame(frame);
	frame->set_pts(source_pts);

	if (!config.stretch)
		frame->set_duration(frame->get_duration() / config.scale);
}

void ReframeRT::load_defaults()
{
	defaults = load_defaults_file("reframert.rc");

	config.scale = defaults->get("SCALE", config.scale);
	config.stretch = defaults->get("STRETCH", config.stretch);
	config.interp = defaults->get("INTERPOLATE", config.interp);
}

void ReframeRT::save_defaults()
{
	defaults->update("SCALE", config.scale);
	defaults->update("STRETCH", config.stretch);
	defaults->update("INTERPOLATE", config.interp);
	defaults->save();
}

void ReframeRT::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("REFRAMERT");
	output.tag.set_property("SCALE", config.scale);
	output.tag.set_property("STRETCH", config.stretch);
	output.tag.set_property("INTERPOLATE", config.interp);
	output.append_tag();
	output.tag.set_title("/REFRAMERT");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ReframeRT::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("REFRAMERT"))
		{
			config.scale = input.tag.get_property("SCALE", config.scale);
			config.stretch = input.tag.get_property("STRETCH", config.stretch);
			config.interp = input.tag.get_property("INTERPOLATE", config.interp);
		}
	}
}
