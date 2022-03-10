// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "audiodevice.h"
#include "audiorender.h"
#include "bcsignals.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "levelhist.h"
#include "mutex.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "transportcommand.h"
#include "videodevice.h"
#include "videorender.h"


RenderEngine::RenderEngine(PlaybackEngine *playback_engine,
	Canvas *output)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->playback_engine = playback_engine;
	this->output = output;
	audio = 0;
	video = 0;
	config = new PlaybackConfig;
	config->copy_from(edlsession->playback_config);
	arender = 0;
	vrender = 0;
	interrupted = 0;
	audio_playing = 0;
	edl = 0;
	do_reset = 0;

	input_lock = new Condition(1, "RenderEngine::input_lock");
	start_lock = new Condition(1, "RenderEngine::start_lock");
	output_lock = new Condition(1, "RenderEngine::output_lock");
	interrupt_lock = new Mutex("RenderEngine::interrupt_lock");
	render_start_lock = new Condition(1, "RenderEngine::render_start", 1);
}

RenderEngine::~RenderEngine()
{
	close_output();
	delete arender;
	delete vrender;
	delete audio;
	delete video;
	delete input_lock;
	delete start_lock;
	delete output_lock;
	delete interrupt_lock;
	delete render_start_lock;
	delete config;
}

void RenderEngine::reset_engines()
{
	do_reset = 1;
	interrupt_playback();
}

void RenderEngine::reset_brender()
{
	if(vrender)
		vrender->reset_brender();
}

void RenderEngine::copy_playbackconfig()
{
	config->copy_from(edlsession->playback_config);
}

void RenderEngine::arm_command(TransportCommand *new_command)
{
// Prevent this renderengine from accepting another command until finished.
// Since the renderengine is often deleted after the input_lock command it must
// be locked here as well as in the calling routine.
	input_lock->lock("RenderEngine::arm_command");

	edl = new_command->get_edl();

	command.copy_from(new_command);

	interrupted = 0;
	if(playback_engine)
	{
		if(command.realtime)
		{
			if(command.single_frame() && config->vconfig->driver != PLAYBACK_X11_GL)
				config->vconfig->driver = PLAYBACK_X11;
		}
		else
			config->vconfig->driver = PLAYBACK_X11;
	}

	fragment_len = AUDIO_BUFFER_SIZE;

	create_render_threads();

	if(playback_engine)
		open_output();
	arm_render_threads();
}

void RenderEngine::create_render_threads()
{
	if(do_reset)
	{
		do_reset = 0;
		delete arender;
		arender = 0;
		delete vrender;
		vrender = 0;
	}
	if(!command.single_frame() &&
			edl->playable_tracks_of(TRACK_AUDIO) &&
			edlsession->audio_channels && !arender)
		arender = new AudioRender(this, edl);

	if(edl->playable_tracks_of(TRACK_VIDEO) && !vrender)
		vrender = new VideoRender(this, edl);
}

void RenderEngine::release_asset(Asset *asset)
{
	if(vrender)
		vrender->release_asset(asset);
	if(arender)
		arender->release_asset(asset);
}

int RenderEngine::brender_available(ptstime position)
{
	if(playback_engine)
		return playback_engine->brender_available(position);
	else
		return 0;
}

void RenderEngine::open_output()
{
	if(command.realtime)
	{
// Allocate devices
		if(arender)
		{
			if(!audio)
				audio = new AudioDevice;
			if(audio->open_output(config->aconfig,
				edlsession->sample_rate,
				fragment_len,
				edlsession->audio_channels))
			{
				delete audio;
				audio = 0;
				delete arender;
				arender = 0;
			}
		}

		if(vrender && !video)
			video = new VideoDevice;

// Start playback
		if(arender && vrender)
		{
			video->set_adevice(audio);
			audio->set_vdevice(video);
		}

// Retool playback configuration
		if(arender)
			audio->start_playback();

		if(vrender)
		{
			video->open_output(config->vconfig,
				edl->this_edlsession->output_w,
				edl->this_edlsession->output_h,
				edl->this_edlsession->color_model,
				output,
				command.single_frame());
		}
	}
}

void RenderEngine::reset_sync_postime(void)
{
	timer.update();
	if(arender)
	{
		if(!edlsession->playback_software_position)
			audio_playing = 1;
	}
}

ptstime RenderEngine::sync_postime(void)
{
	if(audio_playing)
		return audio->current_postime(command.get_speed());

	return (ptstime)timer.get_difference() / 1000;
}

void RenderEngine::start_command()
{
	if(command.realtime)
	{
		interrupt_lock->lock("RenderEngine::start_command");
		start_lock->lock("RenderEngine::start_command 1");
		Thread::start();
		start_lock->lock("RenderEngine::start_command 2");
		start_lock->unlock();
	}
}

void RenderEngine::arm_render_threads()
{
	if(arender)
		arender->arm_command();

	if(vrender)
		vrender->arm_command();
}


void RenderEngine::start_render_threads()
{
	timer.update();

	if(arender)
		arender->start_command();

	if(vrender)
		vrender->start_command();
}

void RenderEngine::update_framerate(double framerate)
{
	edlsession->actual_frame_rate = framerate;
	mwindow_global->preferences_thread->update_framerate();
}

void RenderEngine::update_playstatistics(int frames, int late, int delay)
{
	edlsession->frame_count = frames;
	edlsession->frames_late = late;
	edlsession->avg_delay = delay;
	mwindow_global->preferences_thread->update_playstatistics();
}

void RenderEngine::wait_render_threads()
{
	if(arender)
		arender->Thread::join();

	if(vrender)
		vrender->Thread::join();
}

void RenderEngine::interrupt_playback()
{
	interrupt_lock->lock("RenderEngine::interrupt_playback");
	interrupted = 1;

	if(audio)
		audio->interrupt_playback();

	if(video)
		video->interrupt_playback();
	interrupt_lock->unlock();
}

void RenderEngine::stop_tracking(ptstime position, int type)
{
	if(type == TRACK_AUDIO)
	{
		audio_playing = 0;
		if(vrender)
			return;
	}

	if(playback_engine)
		playback_engine->stop_tracking(position);
}

void RenderEngine::set_tracking_position(ptstime pts, int type)
{
	if(type == TRACK_AUDIO)
	{
		if(vrender)
			return;
	}

	if(playback_engine)
		playback_engine->set_tracking_position(pts);
}

void RenderEngine::close_output()
{
	if(audio)
		audio->close_all();

	if(video)
		video->close_all();
}

int RenderEngine::get_output_levels(double *levels, ptstime pts)
{
	if(arender)
		return arender->get_output_levels(levels, pts);
	return 0;
}

int RenderEngine::get_module_levels(double *levels, ptstime pts)
{
	if(arender)
		return arender->get_track_levels(levels, pts);
	return 0;
}

void RenderEngine::run()
{
	start_render_threads();
	start_lock->unlock();
	interrupt_lock->unlock();

	wait_render_threads();

	interrupt_lock->lock("RenderEngine::run");

	close_output();

	if(playback_engine)
		playback_engine->is_playing_back = 0;

	input_lock->unlock();

	if(do_reset)
	{
		delete arender;
		arender = 0;
		delete vrender;
		vrender = 0;
		do_reset = 0;
	}
	interrupt_lock->unlock();
}

void RenderEngine::wait_another(const char *location, int type)
{
	if(arender && vrender)
	{
		render_start_lock->wait_another(location);
		if(type == TRACK_VIDEO)
			reset_sync_postime();
	} else
		reset_sync_postime();
}
