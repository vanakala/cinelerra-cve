
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

#include "amodule.h"
#include "arender.h"
#include "asset.h"
#include "audiodevice.h"
#include "bcsignals.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "levelhist.h"
#include "mutex.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "mainsession.h"
#include "transportcommand.h"
#include "videodevice.h"
#include "vrender.h"


RenderEngine::RenderEngine(PlaybackEngine *playback_engine,
	Preferences *preferences, 
	TransportCommand *command,
	Canvas *output)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->playback_engine = playback_engine;
	this->output = output;
	audio = 0;
	video = 0;
	config = new PlaybackConfig;
	arender = 0;
	vrender = 0;
	do_audio = 0;
	do_video = 0;
	interrupted = 0;
	actual_frame_rate = 0;
	audio_playing = 0;
	this->preferences = new Preferences;
	this->command = new TransportCommand;
	this->preferences->copy_from(preferences);
	this->command->copy_from(command);
	edl = command->get_edl();
// EDL only changed in construction.
// The EDL contained in later commands is ignored.
	audio_cache = 0;
	video_cache = 0;
	mwindow = mwindow_global;

	input_lock = new Condition(1, "RenderEngine::input_lock");
	start_lock = new Condition(1, "RenderEngine::start_lock");
	output_lock = new Condition(1, "RenderEngine::output_lock");
	interrupt_lock = new Mutex("RenderEngine::interrupt_lock");
	render_start_lock = new Condition(1, "RenderEngine::render_start", 1);
	do_audio = 0;
	do_video = 0;
	done = 0;
}

RenderEngine::~RenderEngine()
{
	close_output();
	delete command;
	delete preferences;
	if(arender) delete arender;
	if(vrender) delete vrender;
	if(audio) delete audio;
	if(video) delete video;
	delete input_lock;
	delete start_lock;
	delete output_lock;
	delete interrupt_lock;
	delete render_start_lock;
	delete config;
}

int RenderEngine::arm_command(TransportCommand *command)
{
// Prevent this renderengine from accepting another command until finished.
// Since the renderengine is often deleted after the input_lock command it must
// be locked here as well as in the calling routine.

	input_lock->lock("RenderEngine::arm_command");

	this->command->copy_from(command);

// Fix background rendering asset to use current dimensions and ignore
// headers.
	preferences->brender_asset->frame_rate = edlsession->frame_rate;
	preferences->brender_asset->width = edlsession->output_w;
	preferences->brender_asset->height = edlsession->output_h;
	preferences->brender_asset->use_header = 0;
	preferences->brender_asset->layers = 1;
	preferences->brender_asset->video_data = 1;

	done = 0;
	interrupted = 0;

// Retool configuration for this node
	this->config->copy_from(edlsession->playback_config);
	VideoOutConfig *vconfig = this->config->vconfig;
	AudioOutConfig *aconfig = this->config->aconfig;
	if(command->realtime)
	{
		if(command->single_frame() && vconfig->driver != PLAYBACK_X11_GL)
		{
			vconfig->driver = PLAYBACK_X11;
		}
	}
	else
	{
		vconfig->driver = PLAYBACK_X11;
	}

	get_duty();

	if(do_audio)
	{
		fragment_len = aconfig->get_fragment_size(edlsession->sample_rate);
	}

	open_output();
	create_render_threads();
	arm_render_threads();
}

void RenderEngine::get_duty()
{
	do_audio = 0;
	do_video = 0;

	if(!command->single_frame() &&
		edl->playable_tracks_of(TRACK_AUDIO) &&
		edlsession->audio_channels)
	{
		do_audio = 1;
	}

	if(edl->playable_tracks_of(TRACK_VIDEO))
	{
		do_video = 1;
	}
}

void RenderEngine::create_render_threads()
{
	if(do_video && !vrender)
	{
		vrender = new VRender(this);
	}

	if(do_audio && !arender)
	{
		arender = new ARender(this);
	}
}


int RenderEngine::get_output_w()
{
	return edlsession->output_w;
}

int RenderEngine::get_output_h()
{
	return edlsession->output_h;
}

int RenderEngine::brender_available(ptstime position)
{
	if(playback_engine)
	{
		return playback_engine->brender_available(position);
	}
	else
		return 0;
}

CICache* RenderEngine::get_acache()
{
	if(playback_engine)
		return playback_engine->audio_cache;
	else
		return audio_cache;
}

CICache* RenderEngine::get_vcache()
{
	if(playback_engine)
		return playback_engine->video_cache;
	else
		return video_cache;
}

void RenderEngine::set_acache(CICache *cache)
{
	this->audio_cache = cache;
}

void RenderEngine::set_vcache(CICache *cache)
{
	this->video_cache = cache;
}

void RenderEngine::open_output()
{
	if(command->realtime)
	{
// Allocate devices
		if(do_audio)
		{
			if(!audio)
				audio = new AudioDevice;
			if (audio->open_output(config->aconfig, 
				edlsession->sample_rate,
				fragment_len,
				edlsession->audio_channels))
			{
				do_audio = 0;
				delete audio;
				audio = 0;
			}
		}

		if(do_video && !video)
			video = new VideoDevice;

// Start playback
		if(do_audio && do_video)
		{
			video->set_adevice(audio);
			audio->set_vdevice(video);
		}

// Retool playback configuration
		if(do_audio)
		{
			audio->start_playback();
		}

		if(do_video)
		{
			video->open_output(config->vconfig,
				get_output_w(),
				get_output_h(),
				output,
				command->single_frame());
		}
	}
}

void RenderEngine::reset_sync_postime(void)
{
	timer.update();
	if(do_audio)
	{
		if(!edlsession->playback_software_position)
			audio_playing = 1;
	}
}

ptstime RenderEngine::sync_postime(void)
{
	if(audio_playing)
		return audio->current_postime(command->get_speed());

	return (ptstime)timer.get_difference() / 1000;
}

void RenderEngine::start_command()
{
	if(command->realtime)
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
	if(do_audio)
	{
		arender->arm_command();
	}

	if(do_video)
	{
		vrender->arm_command();
	}
}


void RenderEngine::start_render_threads()
{
	timer.update();

	if(do_audio)
	{
		arender->start_command();
	}

	if(do_video)
	{
		vrender->start_command();
	}
}

void RenderEngine::update_framerate(float framerate)
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
	if(do_audio)
	{
		arender->Thread::join();
	}
	if(do_video)
	{
		vrender->Thread::join();
	}
}

void RenderEngine::interrupt_playback()
{
	interrupt_lock->lock("RenderEngine::interrupt_playback");
	interrupted = 1;
	if(audio)
	{
		audio->interrupt_playback();
	}
	if(video)
	{
		video->interrupt_playback();
	}
	interrupt_lock->unlock();
}

void RenderEngine::stop_tracking(ptstime position, int type)
{
	if(type == TRACK_AUDIO)
	{
		audio_playing = 0;
		if(do_video)
			return;
	}

	if(playback_engine)
		playback_engine->stop_tracking(position);
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
	if(do_audio)
	{
		return arender->output_levels->get_levels(levels, pts);
	}
	return 0;
}

int RenderEngine::get_module_levels(double *levels, ptstime pts)
{
	int i;

	if(do_audio)
	{
		for(i = 0; i < arender->total_modules; i++)
			((AModule*)arender->modules[i])->module_levels->get_levels(&levels[i], pts);
		return i;
	}
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
	interrupt_lock->unlock();
}

void RenderEngine::wait_another(const char *location, int type)
{
	if(do_audio && do_video)
	{
		render_start_lock->wait_another(location);
		if(type == TRACK_VIDEO)
			reset_sync_postime();
	} else
		reset_sync_postime();
}
