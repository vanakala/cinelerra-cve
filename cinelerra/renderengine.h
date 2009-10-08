
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

#ifndef RENDERENGINE_H
#define RENDERENGINE_H


class RenderEngine;

#include "arender.inc"
#include "audiodevice.inc"
#include "cache.inc"
#include "canvas.inc"
#include "channel.inc"
#include "channeldb.inc"
#include "condition.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "playbackengine.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "thread.h"
#include "transportque.inc"
#include "videodevice.inc"
#include "vrender.inc"

class RenderEngine : public Thread
{
public:
	RenderEngine(PlaybackEngine *playback_engine,
		Preferences *preferences, 
		TransportCommand *command, 
		Canvas *output,
		ArrayList<PluginServer*> *plugindb,
		ChannelDB *channeldb);
	~RenderEngine();

	void get_duty();
	void create_render_threads();
	void arm_render_threads();
	void start_render_threads();
	void wait_render_threads();
	void interrupt_playback();
	int get_output_w();
	int get_output_h();
	int brender_available(int position, int direction);
// Get current channel for the BUZ output
	Channel* get_current_channel();
	double get_tracking_position();
// Find the plugin whose title matches title and return it
	PluginServer* scan_plugindb(char *title,
		int data_type);
	CICache* get_acache();
	CICache* get_vcache();
	void set_acache(CICache *cache);
	void set_vcache(CICache *cache);
// Get levels for tracking
	void get_output_levels(double *levels, int64_t position);
	void get_module_levels(ArrayList<double> *module_levels, int64_t position);

	void run();
// Sends the command sequence, compensating for network latency
	int arm_command(TransportCommand *command,
		int &current_vchannel, 
		int &current_achannel);
// Start the command
	int start_command();

	int open_output();
	int close_output();
// return position to synchronize video against
	int64_t sync_position();
// Called by VRender to reset the timers once the first frame is done.
	void reset_sync_position();
// return samples since start of playback
	int64_t session_position();

// Update preferences window
	void update_framerate(float framerate);

// Copy of command
	TransportCommand *command;
// EDL to be used by renderengine since not all commands involve an EDL change
	EDL *edl;
// Pointer to playback config for one head
	PlaybackConfig *config;
// Defined only for the master render engine
	PlaybackEngine *playback_engine;
// Copy of preferences
	Preferences *preferences;
// Canvas if being used for CWindow
	Canvas *output;


// Lock out new commands until completion
	Condition *input_lock;
// Lock out interrupts until started
	Condition *start_lock;
	Condition *output_lock;
// Lock out audio and synchronization timers until first frame is done
	Condition *first_frame_lock;
// Lock out interrupts before and after renderengine is active
	Mutex *interrupt_lock;

// Show the timecode on the video
	int show_tc;

	int done;
	AudioDevice *audio;
	VideoDevice *video;
	ARender *arender;
	VRender *vrender;
	int do_audio;
	int do_video;
// Timer for synchronization without audio
	Timer timer;
	float actual_frame_rate;
// If the termination came from interrupt or end of selection
	int interrupted;

	ArrayList<PluginServer*> *plugindb;
// Channels for the BUZ output
	ChannelDB *channeldb;

// Samples in audio buffer to process
	int64_t fragment_len;
// Samples to send to audio device after speed adjustment
	int64_t adjusted_fragment_len;              
// CICaches for use if no playbackengine exists
	CICache *audio_cache, *video_cache;
















// constructing with an audio device forces output buffer allocation
// constructing without an audio device puts in one buffer at a time mode
	RenderEngine(MWindow *mwindow, 
		AudioDevice *audio = 0, 
		VideoDevice *video = 0,
		PlaybackEngine *playbackengine = 0);

// buffersize is in samples
	int reset_parameters();

	int arm_playback_audio(int64_t input_length, 
			int64_t amodule_render_fragment, 
			int64_t playback_buffer, 
			int64_t output_length);

	int arm_playback_video(int every_frame, 
			int64_t read_length, 
			int64_t output_length,
			int track_w,
			int track_h,
			int output_w,
			int output_h);

	int64_t get_correction_factor(int reset);     // calling it resets the correction factor

// start video since vrender is the master
	int start_video();


// information for playback
	int follow_loop;       // loop if mwindow is looped
	int infinite;          // don't stop rendering at the end of the range or loops

	int64_t start_position;      // lowest numbered sample in playback range
	int64_t end_position;        // highest numbered sample in playback range
	int64_t current_sample;
	int every_frame;

	MWindow *mwindow;
};








#endif
