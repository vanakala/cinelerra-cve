
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
#define PLUGIN_TITLE N_("Overlay")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_CLASS OverlayAudio
#define PLUGIN_CONFIG_CLASS OverlayAudioConfig
#define PLUGIN_THREAD_CLASS OverlayAudioThread
#define PLUGIN_GUI_CLASS OverlayAudioWindow

#include "pluginmacros.h"

#include "bchash.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bctitle.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "pluginaclient.h"
#include "pluginwindow.h"
#include <string.h>


class OverlayAudioConfig
{
public:
	OverlayAudioConfig();
	int equivalent(OverlayAudioConfig &that);
	void copy_from(OverlayAudioConfig &that);
	void interpolate(OverlayAudioConfig &prev, 
		OverlayAudioConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);
	static const char* output_to_text(int output_layer);
	int output_track;
	enum
	{
		TOP,
		BOTTOM
	};
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class OutputTrack : public BC_PopupMenu
{
public:
	OutputTrack(OverlayAudio *plugin, int x, int y);

	void create_objects();
	int handle_event();

	OverlayAudio *plugin;
};

class OverlayAudioWindow : public PluginWindow
{
public:
	OverlayAudioWindow(OverlayAudio *plugin, int x, int y);

	void update();

	OutputTrack *output;
	PLUGIN_GUI_CLASS_MEMBERS
};

PLUGIN_THREAD_HEADER

class OverlayAudio : public PluginAClient
{
public:
	OverlayAudio(PluginServer *server);
	~OverlayAudio();

	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void process_frame(AFrame **aframes);
	void load_defaults();
	void save_defaults();

	PLUGIN_CLASS_MEMBERS
};


OverlayAudioConfig::OverlayAudioConfig()
{
	output_track = OverlayAudioConfig::TOP;
}

int OverlayAudioConfig::equivalent(OverlayAudioConfig &that)
{
	return that.output_track == output_track;
}

void OverlayAudioConfig::copy_from(OverlayAudioConfig &that)
{
	output_track = that.output_track;
}

void OverlayAudioConfig::interpolate(OverlayAudioConfig &prev, 
	OverlayAudioConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	output_track = prev.output_track;
}

const char* OverlayAudioConfig::output_to_text(int output_layer)
{
	switch(output_layer)
	{
	case OverlayAudioConfig::TOP:
		return _("Top");
	case OverlayAudioConfig::BOTTOM:
		return _("Bottom");
	}
	return "";
}


OverlayAudioWindow::OverlayAudioWindow(OverlayAudio *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	400, 
	100)
{
	x = y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Output track:")));
	x += title->get_w() + 10;
	add_subwindow(output = new OutputTrack(plugin, x, y));
	output->create_objects();
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void OverlayAudioWindow::update()
{
	output->set_text(
		OverlayAudioConfig::output_to_text(plugin->config.output_track));
}

OutputTrack::OutputTrack(OverlayAudio *plugin, int x , int y)
 : BC_PopupMenu(x, 
	y, 
	100,
	OverlayAudioConfig::output_to_text(plugin->config.output_track),
	1)
{
	this->plugin = plugin;
}

void OutputTrack::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayAudioConfig::output_to_text(
			OverlayAudioConfig::TOP)));
	add_item(new BC_MenuItem(
		OverlayAudioConfig::output_to_text(
			OverlayAudioConfig::BOTTOM)));
}

int OutputTrack::handle_event()
{
	char *text = get_text();

	if(!strcmp(text, 
		OverlayAudioConfig::output_to_text(
			OverlayAudioConfig::TOP)))
		plugin->config.output_track = OverlayAudioConfig::TOP;
	else
	if(!strcmp(text, 
		OverlayAudioConfig::output_to_text(
			OverlayAudioConfig::BOTTOM)))
		plugin->config.output_track = OverlayAudioConfig::BOTTOM;

	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS

REGISTER_PLUGIN

OverlayAudio::OverlayAudio(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

OverlayAudio::~OverlayAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void OverlayAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("OVERLAY"))
			{
				config.output_track = input.tag.get_property("OUTPUT", config.output_track);
			}
		}
	}
}

void OverlayAudio::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("OVERLAY");
	output.tag.set_property("OUTPUT", config.output_track);
	output.append_tag();
	output.tag.set_title("/OVERLAY");
	output.append_tag();
	keyframe->set_data(output.string);
}

void OverlayAudio::load_defaults()
{
	defaults = load_defaults_file("overlayaudio.rc");

	config.output_track = defaults->get("OUTPUT", config.output_track);
}

void OverlayAudio::save_defaults()
{
	defaults->update("OUTPUT", config.output_track);
	defaults->save();
}

void OverlayAudio::process_frame(AFrame **aframes)
{
	load_configuration();

	int output_track = 0;
	if(config.output_track == OverlayAudioConfig::BOTTOM)
		output_track = get_total_buffers() - 1;

// Direct copy the output track
	int size = aframes[output_track]->fill_length();
	get_frame(aframes[output_track]);

// Add remaining tracks
	double *output_buffer = aframes[output_track]->buffer;

	for(int i = 0; i < get_total_buffers(); i++)
	{
		if(i != output_track)
		{
			double *input_buffer = aframes[i]->buffer;

			get_frame(aframes[i]);
			for(int j = 0; j < size; j++)
			{
				output_buffer[j] += input_buffer[j];
			}
		}
	}
}
