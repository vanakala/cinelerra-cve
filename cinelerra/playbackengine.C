
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
#include "transportcommand.h"
#include "vrender.h"


PlaybackEngine::PlaybackEngine(MWindow *mwindow, Canvas *output)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->mwindow = mwindow;
	this->output = output;
	is_playing_back = 0;
	tracking_position = 0;
	tracking_active = 0;
	audio_cache = 0;
	video_cache = 0;
	tracking_done = new Condition(1, "PlaybackEngine::tracking_done");
	start_lock = new Condition(0, "PlaybackEngine::start_lock");
	playback_lock = new Condition(0, "PlaybackEngine::playback_lock");
	cmds_lock = new Mutex("PlaybackEngine::cmds_lock");
	render_engine = 0;
	preferences = new Preferences;
	command = new TransportCommand;
	used_cmds = 0;
	memset(cmds, 0, sizeof(cmds));
	preferences->copy_from(mwindow->preferences);

	done = 0;
	Thread::start();
	start_lock->lock("PlaybackEngine::PlaybackEngine");
}

PlaybackEngine::~PlaybackEngine()
{
	done = 1;
	send_command(STOP);

	Thread::join();
	delete preferences;
	delete command;
	for(int i = 0; i < MAX_COMMAND_QUEUE; i++)
		if(cmds[i])
			delete cmds[i];
	delete_render_engine();
	delete audio_cache;
	delete video_cache;
	delete tracking_done;
	delete start_lock;
	delete cmds_lock;
	delete playback_lock;
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
		output);
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
		audio_cache = new CICache(preferences, FILE_OPEN_AUDIO);
	if(!video_cache) 
		video_cache = new CICache(preferences, FILE_OPEN_VIDEO);
}

void PlaybackEngine::perform_change()
{
	if(command->change_type == CHANGE_ALL)
		create_cache();
	if(command->change_type & CHANGE_EDL)
		create_render_engine();
	if(render_engine && (command->change_type & (CHANGE_PARAMS | CHANGE_EDL)) == CHANGE_PARAMS)
			render_engine->edl->synchronize_params(command->get_edl());
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

// Wait for tracking to finish if it is running
	if(wait_tracking)
	{
		tracking_done->lock("PlaybackEngine::interrupt_playback");
		tracking_done->unlock();
	}
}

// Return number of existing channels
int PlaybackEngine::get_output_levels(double *levels, ptstime pts)
{
	if(render_engine && render_engine->do_audio)
		return render_engine->get_output_levels(levels, pts);
	return 0;
}

int PlaybackEngine::get_module_levels(double *levels, ptstime pts)
{
	if(render_engine && render_engine->do_audio)
		return render_engine->get_module_levels(levels, pts);
	return 0;
}

void PlaybackEngine::init_tracking()
{
	if(!command->single_frame()) 
		tracking_active = 1;
	else
		tracking_active = 0;

	video_pts = -1;
	tracking_position = tracking_start = command->playbackstart;

	tracking_done->lock("PlaybackEngine::init_tracking");
	init_cursor();
}

void PlaybackEngine::stop_tracking(ptstime position)
{
	tracking_active = 0;
	if(position >= 0)
		tracking_position = position;
	stop_cursor();
	tracking_done->unlock();
	if(!command->single_frame())
		command->command = STOP;
}

ptstime PlaybackEngine::get_tracking_position()
{
	if(render_engine && tracking_active)
	{
		ptstime tpts = render_engine->sync_postime() *
			render_engine->command->get_speed();
		if(render_engine->command->get_direction() == PLAY_FORWARD)
			tpts += tracking_start;
		else
			tpts = tracking_start - tpts;
		tracking_position = tpts;
	}
	return tracking_position;
}

void PlaybackEngine::run()
{
	TransportCommand *tmp_cmd;

	start_lock->unlock();

	do
	{
		playback_lock->lock("PlaybackEngine::run");
// Read the new command
		cmds_lock->lock("PlaybackEngine::run");

		if(used_cmds < 1)
		{
			cmds_lock->unlock();
			continue;
		}
		tmp_cmd = command;
		command = cmds[0];
		for(int i = 1; i < used_cmds; i++)
			cmds[i - 1] = cmds[i];
		tmp_cmd->reset();
		cmds[--used_cmds] = tmp_cmd;
		cmds_lock->unlock();

// Wait for current command to finish
		wait_render_engine();

		if(done) return;

		switch(command->command)
		{
// Parameter change only
		case COMMAND_NONE:
			perform_change();
			break;

		case STOP:
// No changing
			break;

		case CURRENT_FRAME:
			perform_change();
			arm_render_engine();
// Dispatch the command
			start_render_engine();
			break;

		default:
			is_playing_back = 1;
			double frame_len = 1.0 / edlsession->frame_rate;

			if(command->command == SINGLE_FRAME_FWD)
				command->playbackstart = get_tracking_position() + frame_len;
			if(command->command == SINGLE_FRAME_REWIND)
				command->playbackstart = get_tracking_position() - frame_len;
			if(command->playbackstart < 0.0)
				command->playbackstart = 0.0;

			perform_change();
			arm_render_engine();

// Start tracking after arming so the tracking position doesn't change.
			init_tracking();

// Dispatch the command
			start_render_engine();
			break;
		}
	} while(!done);
}

void PlaybackEngine::send_command(int cmd, EDL *new_edl, int options)
{
	TransportCommand *new_cmd;

	cmds_lock->lock("PlaybackEngine:send_command");

	if(cmd == STOP)
	{
		// STOP resets the que
		if(!cmds[0])
			cmds[0] = new_cmd = new TransportCommand;
		else
			new_cmd = cmds[0];
		used_cmds = 1;
		interrupt_playback(options & CMDOPT_WAITTRACKING);
	}
	else
	{
		if(!(new_cmd = cmds[used_cmds]))
		{
			if(++used_cmds >= MAX_COMMAND_QUEUE)
			{
				used_cmds = MAX_COMMAND_QUEUE - 1;
				new_cmd = cmds[used_cmds - 1];
			}
			else
				cmds[used_cmds - 1] = new_cmd = new TransportCommand;
		} else
			used_cmds++;
	}

	new_cmd->command = cmd;
	new_cmd->realtime = cmd != STOP;
	new_cmd->change_type = options & CHANGE_ALL;

	if(new_edl)
	{
		new_cmd->set_edl(new_edl);
		new_cmd->set_playback_range(options & CMDOPT_USEINOUT);
	}

	// Drop previous CURRENT_FRAMEs
	if(new_cmd->command == CURRENT_FRAME && used_cmds > 1)
	{
		if(cmds[used_cmds - 2]->command == CURRENT_FRAME)
		{
			if(new_cmd->change_type == CHANGE_NONE)
				used_cmds--;
			else
			if(new_cmd->change_type >= cmds[used_cmds - 2]->change_type)
			{
				cmds[used_cmds - 1] = cmds[used_cmds -2];
				cmds[used_cmds - 2] = new_cmd;
				used_cmds--;
			}
		}
	}
	cmds_lock->unlock();
	playback_lock->unlock();
}
