
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

#include "bcsignals.h"
#include "cache.h"
#include "condition.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mbuttons.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "tracking.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "renderengine.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vrender.h"


PlaybackEngine::PlaybackEngine(MWindow *mwindow, Canvas *output)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->output = output;
	is_playing_back = 0;
	tracking_position = 0;
	tracking_active = 0;
	audio_cache = 0;
	video_cache = 0;
	last_command = STOP;
	tracking_done = new Condition(1, "PlaybackEngine::tracking_done");
	pause_lock = new Condition(0, "PlaybackEngine::pause_lock");
	start_lock = new Condition(0, "PlaybackEngine::start_lock");
	render_engine = 0;
	debug = 0;

	preferences = new Preferences;
	command = new TransportCommand;
	que = new TransportQue;
// Set the first change to maximum
	que->command.change_type = CHANGE_ALL;

	preferences->copy_from(mwindow->preferences);

	done = 0;
	Thread::start();
	start_lock->lock("PlaybackEngine::create_objects");
}

PlaybackEngine::~PlaybackEngine()
{
	done = 1;
	que->send_command(STOP,
		CHANGE_NONE, 
		0,
		0);
	interrupt_playback();

	Thread::join();
	delete preferences;
	delete command;
	delete que;
	delete_render_engine();
	delete audio_cache;
	delete video_cache;
	delete tracking_done;
	delete pause_lock;
	delete start_lock;
}

void PlaybackEngine::create_render_engine()
{
// Fix playback configurations
	int current_vchannel = 0;
	int current_achannel = 0;

	delete_render_engine();

	render_engine = new RenderEngine(this,
		preferences, 
		command, 
		output,
		mwindow->plugindb,
		0);
}

void PlaybackEngine::delete_render_engine()
{
	delete render_engine;
	render_engine = 0;
}

void PlaybackEngine::arm_render_engine()
{
	if(render_engine)
		render_engine->arm_command(command);
}

void PlaybackEngine::start_render_engine()
{
	if(render_engine) render_engine->start_command();
}

void PlaybackEngine::wait_render_engine()
{
	if(command->realtime && render_engine)
	{
		render_engine->join();
	}
}

void PlaybackEngine::create_cache()
{
	if(audio_cache) delete audio_cache;
	audio_cache = 0;
	if(video_cache) delete video_cache;
	video_cache = 0;
	if(!audio_cache) 
		audio_cache = new CICache(preferences, mwindow->plugindb);
	if(!video_cache) 
		video_cache = new CICache(preferences, mwindow->plugindb);
}

void PlaybackEngine::perform_change()
{
	switch(command->change_type)
	{
		case CHANGE_ALL:
			create_cache();
		case CHANGE_EDL:
			create_render_engine();
		case CHANGE_PARAMS:
			if(command->change_type != CHANGE_EDL &&
				command->change_type != CHANGE_ALL)
				render_engine->edl->synchronize_params(command->get_edl());
		case CHANGE_NONE:
			break;
	}
}

void PlaybackEngine::sync_parameters(EDL *edl)
{
// TODO: lock out render engine from keyframe deletions
	command->get_edl()->synchronize_params(edl);
	if(render_engine) render_engine->edl->synchronize_params(edl);
}

void PlaybackEngine::interrupt_playback(int wait_tracking)
{
	if(render_engine)
		render_engine->interrupt_playback();

// Stop pausing
	pause_lock->unlock();

// Wait for tracking to finish if it is running
	if(wait_tracking)
	{
		tracking_done->lock("PlaybackEngine::interrupt_playback");
		tracking_done->unlock();
	}
}

// Return 1 if levels exist
int PlaybackEngine::get_output_levels(double *levels, samplenum position)
{
	if(render_engine && render_engine->do_audio)
	{
		render_engine->get_output_levels(levels, position);
		return 1;
	}
	return 0;
}

int PlaybackEngine::get_module_levels(double **module_levels, samplenum position)
{
	if(render_engine && render_engine->do_audio)
	{
		int n;
		*module_levels = render_engine->get_module_levels(&n, position);
		return n;
	}
	return 0;
}

void PlaybackEngine::init_tracking()
{
	if(!command->single_frame()) 
		tracking_active = 1;
	else
		tracking_active = 0;

	tracking_position = command->playbackstart;
	tracking_done->lock("PlaybackEngine::init_tracking");
	init_cursor();
}

void PlaybackEngine::stop_tracking()
{
	tracking_active = 0;
	stop_cursor();
	tracking_done->unlock();
}

void PlaybackEngine::update_tracking(ptstime position)
{
	tracking_position = position;
}

ptstime PlaybackEngine::get_tracking_position()
{
	return tracking_position;
}

void PlaybackEngine::run()
{
	start_lock->unlock();

	do
	{
// Wait for current command to finish
		que->output_lock->lock("PlaybackEngine::run");

		wait_render_engine();

// Read the new command
		que->input_lock->lock("PlaybackEngine::run");
		if(done) return;

		command->copy_from(&que->command);
		que->command.reset();
		que->input_lock->unlock();

		switch(command->command)
		{
// Parameter change only
			case COMMAND_NONE:
				perform_change();
				break;

			case PAUSE:
				init_cursor();
				pause_lock->lock("PlaybackEngine::run");
				stop_cursor();
				break;

			case STOP:
// No changing
				break;

			case CURRENT_FRAME:
				last_command = command->command;
				perform_change();
				arm_render_engine();
// Dispatch the command
				start_render_engine();
				break;

			default:
				last_command = command->command;
				is_playing_back = 1;

				double frame_len = 1.0 / command->get_edl()->session->frame_rate;
				if(command->command == SINGLE_FRAME_FWD)
					command->playbackstart = get_tracking_position() + frame_len;
				if(command->command == SINGLE_FRAME_REWIND)
					command->playbackstart = get_tracking_position() - frame_len;
				if(command->playbackstart < 0.0)
					command->playbackstart = 0.0;

				perform_change();
				arm_render_engine();

// Start tracking after arming so the tracking position doesn't change.
// The tracking for a single frame command occurs during PAUSE
				init_tracking();

// Dispatch the command
				start_render_engine();
				break;
		}
	}while(!done);
}
