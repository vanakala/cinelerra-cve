
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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "pluginaclient.h"
#include <string.h>


class OverlayAudioWindow;
class OverlayAudio;

class OverlayAudioConfig
{
public:
	OverlayAudioConfig();
	int equivalent(OverlayAudioConfig &that);
	void copy_from(OverlayAudioConfig &that);
	void interpolate(OverlayAudioConfig &prev, 
		OverlayAudioConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	static char* output_to_text(int output_layer);
	int output_track;
	enum
	{
		TOP,
		BOTTOM
	};
	
};

class OutputTrack : public BC_PopupMenu
{
public:
	OutputTrack(OverlayAudio *plugin, int x, int y);
	void create_objects();
	int handle_event();
	OverlayAudio *plugin;
};

class OverlayAudioWindow : public BC_Window
{
public:
	OverlayAudioWindow(OverlayAudio *plugin, int x, int y);

	int create_objects();
	int close_event();

	OverlayAudio *plugin;
	OutputTrack *output;
};

PLUGIN_THREAD_HEADER(OverlayAudio, OverlayAudioThread, OverlayAudioWindow)

class OverlayAudio : public PluginAClient
{
public:
	OverlayAudio(PluginServer *server);
	~OverlayAudio();

	int is_multichannel();
	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_buffer(int64_t size, 
		double **buffer,
		int64_t start_position,
		int sample_rate);
	int load_defaults();
	int save_defaults();
	void update_gui();


	PLUGIN_CLASS_MEMBERS(OverlayAudioConfig, OverlayAudioThread)
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
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	output_track = prev.output_track;
}

char* OverlayAudioConfig::output_to_text(int output_layer)
{
	switch(output_layer)
	{
		case OverlayAudioConfig::TOP:    return _("Top");
		case OverlayAudioConfig::BOTTOM: return _("Bottom");
	}
	return "";
}







OverlayAudioWindow::OverlayAudioWindow(OverlayAudio *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	400, 
	100, 
	400, 
	100, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

int OverlayAudioWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, "Output track:"));
	x += title->get_w() + 10;
	add_subwindow(output = new OutputTrack(plugin, x, y));
	output->create_objects();
	show_window();
	return 0;
}

WINDOW_CLOSE_EVENT(OverlayAudioWindow)




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


PLUGIN_THREAD_OBJECT(OverlayAudio, OverlayAudioThread, OverlayAudioWindow)




REGISTER_PLUGIN(OverlayAudio)




OverlayAudio::OverlayAudio(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

OverlayAudio::~OverlayAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

char* OverlayAudio::plugin_title() { return N_("Overlay"); }
int OverlayAudio::is_realtime() { return 1; }
int OverlayAudio::is_multichannel() { return 1; }



void OverlayAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

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
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("OVERLAY");
	output.tag.set_property("OUTPUT", config.output_track);
	output.append_tag();
	output.tag.set_title("/OVERLAY");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

int OverlayAudio::load_defaults()
{
	char directory[BCTEXTLEN];
	sprintf(directory, "%soverlayaudio.rc", BCASTDIR);
	defaults = new BC_Hash(directory);
	defaults->load();

	config.output_track = defaults->get("OUTPUT", config.output_track);
	return 0;
}

int OverlayAudio::save_defaults()
{
	defaults->update("OUTPUT", config.output_track);
	defaults->save();

	return 0;
}


void OverlayAudio::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("OverlayAudio::update_gui");
			thread->window->output->set_text(
				OverlayAudioConfig::output_to_text(config.output_track));
			thread->window->unlock_window();
		}
	}
}

NEW_PICON_MACRO(OverlayAudio)
SHOW_GUI_MACRO(OverlayAudio, OverlayAudioThread)
RAISE_WINDOW_MACRO(OverlayAudio)
SET_STRING_MACRO(OverlayAudio)
LOAD_CONFIGURATION_MACRO(OverlayAudio, OverlayAudioConfig)


int OverlayAudio::process_buffer(int64_t size, 
	double **buffer,
	int64_t start_position,
	int sample_rate)
{
	load_configuration();


	int output_track = 0;
	if(config.output_track == OverlayAudioConfig::BOTTOM)
		output_track = get_total_buffers() - 1;

// Direct copy the output track
	read_samples(buffer[output_track],
		output_track,
		sample_rate,
		start_position,
		size);

// Add remaining tracks
	double *output_buffer = buffer[output_track];
	for(int i = 0; i < get_total_buffers(); i++)
	{
		if(i != output_track)
		{
			double *input_buffer = buffer[i];
			read_samples(buffer[i],
				i,
				sample_rate,
				start_position,
				size);
			for(int j = 0; j < size; j++)
			{
				output_buffer[j] += input_buffer[j];
			}
		}
	}

	return 0;
}






