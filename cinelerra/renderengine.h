
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

#include "audiorender.inc"
#include "bctimer.h"
#include "audiodevice.inc"
#include "canvas.inc"
#include "condition.inc"
#include "mutex.inc"
#include "playbackengine.inc"
#include "playbackconfig.inc"
#include "pluginserver.inc"
#include "renderengine.inc"
#include "thread.h"
#include "transportcommand.h"
#include "videodevice.inc"
#include "videorender.inc"

class RenderEngine : public Thread
{
public:
	RenderEngine(PlaybackEngine *playback_engine,
		Canvas *output);
	~RenderEngine();

	void get_duty();
	void create_render_threads();
	void arm_render_threads();
	void start_render_threads();
	void wait_render_threads();
	void interrupt_playback();
	void stop_tracking(ptstime position, int type);
	void set_tracking_position(ptstime pts, int type);
	int brender_available(ptstime position);

// Get levels for tracking
	int get_output_levels(double *levels, ptstime pts);
	int get_module_levels(double *levels, ptstime pts);

	void run();
// Synchronizes Playcackconfig with edlsession
	void copy_playbackconfig();
// Sends the command sequence, compensating for network latency
	int arm_command(TransportCommand *command);
// Start the command
	void start_command();

	void open_output();
	void close_output();
// return position to synchronize video against
	ptstime sync_postime(void);
// Update preferences window
	void update_framerate(float framerate);
// Update statistics
	void update_playstatistics(int frames, int late, int delay);
// Wait for other thread
	void wait_another(const char *location, int type);

// Copy of command
	TransportCommand command;
// EDL to be used by renderengine since not all commands involve an EDL change
	EDL *edl;
// Pointer to playback config for one head
	PlaybackConfig *config;
// Defined only for the master render engine
	PlaybackEngine *playback_engine;
// Canvas if being used for CWindow
	Canvas *output;

// Lock out new commands until completion
	Condition *input_lock;
// Lock out interrupts until started
	Condition *start_lock;
	Condition *output_lock;
// Audio-video syncronization lock
	Condition *render_start_lock;
// Lock out interrupts before and after renderengine is active
	Mutex *interrupt_lock;

	int done;
	AudioDevice *audio;
	VideoDevice *video;
	AudioRender *arender;
	VideoRender *vrender;
	int do_audio;
	int do_video;
	float actual_frame_rate;
// If the termination came from interrupt or end of selection
	int interrupted;

// Samples in audio buffer to process
	int fragment_len;

private:
	void reset_sync_postime(void);
// Timer for synchronization without audio
	Timer timer;
	int audio_playing;
};

#endif
