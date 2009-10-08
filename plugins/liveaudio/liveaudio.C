
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

#include "asset.h"
#include "audiodevice.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "edlsession.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "picon_png.h"
#include "pluginaclient.h"
#include "transportque.inc"
#include "vframe.h"

#include <string.h>
#include <stdint.h>

#define HISTORY_SAMPLES 0x100000
class LiveAudio;
class LiveAudioWindow;


class LiveAudioConfig
{
public:
	LiveAudioConfig();
};






class LiveAudioWindow : public BC_Window
{
public:
	LiveAudioWindow(LiveAudio *plugin, int x, int y);
	~LiveAudioWindow();

	void create_objects();
	int close_event();

	LiveAudio *plugin;
};


PLUGIN_THREAD_HEADER(LiveAudio, LiveAudioThread, LiveAudioWindow)



class LiveAudio : public PluginAClient
{
public:
	LiveAudio(PluginServer *server);
	~LiveAudio();


	PLUGIN_CLASS_MEMBERS(LiveAudioConfig, LiveAudioThread);

	int process_buffer(int64_t size, 
		double **buffer,
		int64_t start_position,
		int sample_rate);
	int is_realtime();
	int is_multichannel();
	int is_synthesis();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_stop();

	AudioDevice *adevice;
	double **history;
	int history_ptr;
	int history_channels;
	int64_t history_position;
	int history_size;
};












LiveAudioConfig::LiveAudioConfig()
{
}








LiveAudioWindow::LiveAudioWindow(LiveAudio *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	300, 
	160, 
	300, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

LiveAudioWindow::~LiveAudioWindow()
{
}

void LiveAudioWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, "Live audio"));
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(LiveAudioWindow)









PLUGIN_THREAD_OBJECT(LiveAudio, LiveAudioThread, LiveAudioWindow)










REGISTER_PLUGIN(LiveAudio)






LiveAudio::LiveAudio(PluginServer *server)
 : PluginAClient(server)
{
	adevice = 0;
	history = 0;
	history_channels = 0;
	history_ptr = 0;
	history_position = 0;
	history_size = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}


LiveAudio::~LiveAudio()
{
	if(adevice)
	{
		adevice->interrupt_crash();
		adevice->close_all();
	}
	delete adevice;
	if(history)
	{
		for(int i = 0; i < history_channels; i++)
			delete [] history[i];
		delete [] history;
	}
	PLUGIN_DESTRUCTOR_MACRO
}



int LiveAudio::process_buffer(int64_t size, 
	double **buffer,
	int64_t start_position,
	int sample_rate)
{
	load_configuration();
//printf("LiveAudio::process_buffer 10 start_position=%lld buffer_size=%d size=%d\n", 
//start_position, get_buffer_size(), size);
	int first_buffer = 0;

	if(!adevice)
	{
		EDLSession *session = PluginClient::get_edlsession();
		if(session)
		{
			adevice = new AudioDevice;
			adevice->open_input(session->aconfig_in, 
				session->vconfig_in, 
				get_project_samplerate(), 
				get_buffer_size(),
				get_total_buffers(),
				session->real_time_record);
			adevice->start_recording();
			first_buffer = 1;
			history_position = start_position;
		}
	}

	if(!history)
	{
		history_channels = get_total_buffers();
		history = new double*[history_channels];
		for(int i = 0; i < history_channels; i++)
		{
			history[i] = new double[HISTORY_SAMPLES];
			bzero(history[i], sizeof(double) * HISTORY_SAMPLES);
		}
	}

SET_TRACE
//	if(get_direction() == PLAY_FORWARD)
	{
// Reset history buffer to current position if before maximum history
		if(start_position < history_position - HISTORY_SAMPLES)
			history_position = start_position;



// Extend history buffer
		int64_t end_position = start_position + size;
// printf("LiveAudio::process_buffer %lld %lld %lld\n", 
// end_position, 
// history_position,
// end_position - history_position);
		if(end_position > history_position)
		{
// Reset history buffer to current position if after maximum history
			if(start_position >= history_position + HISTORY_SAMPLES)
				history_position = start_position;
// A delay seems required because ALSA playback may get ahead of
// ALSA recording and never recover.
			if(first_buffer) end_position += sample_rate;
			int done = 0;
			while(!done && history_position < end_position)
			{
// Reading in playback buffer sized fragments seems to help
// even though the sound driver abstracts this size.  Larger 
// fragments probably block unnecessarily long.
				int fragment = size;
				if(history_ptr + fragment  > HISTORY_SAMPLES)
				{
					fragment = HISTORY_SAMPLES - history_ptr;
					done = 1;
				}

// Read rest of buffer from sound driver
				if(adevice)
				{
					int over[get_total_buffers()];
					double max[get_total_buffers()];
					adevice->read_buffer(history, 
						fragment, 
						over, 
						max, 
						history_ptr);
				}
				history_ptr += fragment;
// wrap around buffer
				if(history_ptr >= HISTORY_SAMPLES)
					history_ptr = 0;
				history_position += fragment;
			}
		}

// Copy data from history buffer
		int buffer_position = 0;
		int history_buffer_ptr = history_ptr - history_position + start_position;
		while(history_buffer_ptr < 0)
			history_buffer_ptr += HISTORY_SAMPLES;
		while(buffer_position < size)
		{
			int fragment = size;
			if(history_buffer_ptr + fragment > HISTORY_SAMPLES)
				fragment = HISTORY_SAMPLES - history_buffer_ptr;
			if(buffer_position + fragment > size)
				fragment = size - buffer_position;
			for(int i = 0; i < get_total_buffers(); i++)
				memcpy(buffer[i] + buffer_position, 
					history[i] + history_buffer_ptr,
					sizeof(double) * fragment);
			history_buffer_ptr += fragment;
			if(history_buffer_ptr >= HISTORY_SAMPLES)
				history_buffer_ptr = 0;
			buffer_position += fragment;
		}

SET_TRACE
	}


	return 0;
}

void LiveAudio::render_stop()
{
	if(adevice)
	{
		adevice->interrupt_crash();
		adevice->close_all();
	}
	delete adevice;
	adevice = 0;
	history_ptr = 0;
	history_position = 0;
	history_size = 0;
}


char* LiveAudio::plugin_title() { return N_("Live Audio"); }
int LiveAudio::is_realtime() { return 1; }
int LiveAudio::is_multichannel() { return 1; }
int LiveAudio::is_synthesis() { return 1; }


NEW_PICON_MACRO(LiveAudio) 

SHOW_GUI_MACRO(LiveAudio, LiveAudioThread)

RAISE_WINDOW_MACRO(LiveAudio)

SET_STRING_MACRO(LiveAudio);

int LiveAudio::load_configuration()
{
	return 0;
}

int LiveAudio::load_defaults()
{
	return 0;
}

int LiveAudio::save_defaults()
{
	return 0;
}

void LiveAudio::save_data(KeyFrame *keyframe)
{
}

void LiveAudio::read_data(KeyFrame *keyframe)
{
}

void LiveAudio::update_gui()
{
}





