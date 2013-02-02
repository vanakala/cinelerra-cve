
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
#include "tracks.h"
#include "transportcommand.h"
#include "videodevice.h"
#include "vrender.h"
#include "workarounds.h"



RenderEngine::RenderEngine(PlaybackEngine *playback_engine,
	Preferences *preferences, 
	TransportCommand *command,
	Canvas *output,
	ArrayList<PluginServer*> *plugindb)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->playback_engine = playback_engine;
	this->output = output;
	this->plugindb = plugindb;
	audio = 0;
	video = 0;
	config = new PlaybackConfig;
	arender = 0;
	vrender = 0;
	do_audio = 0;
	do_video = 0;
	interrupted = 0;
	actual_frame_rate = 0;
	sync_basetime = 0;
	this->preferences = new Preferences;
	this->command = new TransportCommand;
	this->preferences->copy_from(preferences);
	this->command->copy_from(command);
	edl = new EDL;
	edl->create_objects();
// EDL only changed in construction.
// The EDL contained in later commands is ignored.
	edl->copy_all(command->get_edl());
	audio_cache = 0;
	video_cache = 0;
	if(playback_engine && playback_engine->mwindow)
		mwindow = playback_engine->mwindow;
	else
		mwindow = 0;

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
	delete edl;
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
	preferences->brender_asset->frame_rate = command->get_edl()->session->frame_rate;
	preferences->brender_asset->width = command->get_edl()->session->output_w;
	preferences->brender_asset->height = command->get_edl()->session->output_h;
	preferences->brender_asset->use_header = 0;
	preferences->brender_asset->layers = 1;
	preferences->brender_asset->video_data = 1;

	done = 0;
	interrupted = 0;

// Retool configuration for this node
	this->config->copy_from(command->get_edl()->session->playback_config);
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
		fragment_len = aconfig->get_fragment_size(command->get_edl()->session->sample_rate);
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
		edl->tracks->playable_audio_tracks() &&
		edl->session->audio_channels)
	{
		do_audio = 1;
	}

	if(edl->tracks->playable_video_tracks())
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
	return edl->session->output_w;
}

int RenderEngine::get_output_h()
{
	return edl->session->output_h;
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


ptstime RenderEngine::get_tracking_position()
{
	if(playback_engine) 
		return playback_engine->get_tracking_position();
	else
		return 0;
}

void RenderEngine::open_output()
{
	if(command->realtime)
	{
// Allocate devices
		if(do_audio)
		{
			audio = new AudioDevice;
			if (audio->open_output(config->aconfig, 
				edl->session->sample_rate, 
				fragment_len,
				edl->session->audio_channels))
			{
				do_audio = 0;
				delete audio;
				audio = 0;
			}
		}

		if(do_video)
		{
			video = new VideoDevice;
		}

// Initialize sharing

// Start playback
		if(do_audio && do_video)
		{
			video->set_adevice(audio);
			audio->set_vdevice(video);
		}

// Retool playback configuration
		if(do_audio)
		{
			audio->set_software_positioning(edl->session->playback_software_position);
			audio->start_playback();
		}

		if(do_video)
		{
			video->open_output(config->vconfig, 
				edl->session->frame_rate,
				get_output_w(),
				get_output_h(),
				output,
				command->single_frame());
			video->set_quality(80);
			video->set_cpus(preferences->processors);
		}
	}
}

void RenderEngine::reset_sync_postime(void)
{
	timer.update();
	if(do_audio)
		sync_basetime = audio->current_postime(command->get_speed());
}

ptstime RenderEngine::sync_postime(void)
{
// Use audio device
// No danger of race conditions because the output devices are closed after all
// threads join.
	if(do_audio)
	{
		return audio->current_postime(command->get_speed()) - sync_basetime;
	}

	if(do_video)
	{
		return (ptstime)timer.get_difference() / 1000;
	}
}

PluginServer* RenderEngine::scan_plugindb(char *title, 
	int data_type)
{
	for(int i = 0; i < plugindb->total; i++)
	{
		PluginServer *server = plugindb->values[i];
		if(!strcasecmp(server->title, title) &&
			((data_type == TRACK_AUDIO && server->audio) ||
			(data_type == TRACK_VIDEO && server->video)))
			return plugindb->values[i];
	}
	return 0;
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
// Synchronization timer.  Gets reset once again after the first video frame.
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
	playback_engine->mwindow->edl->session->actual_frame_rate = framerate;
	playback_engine->mwindow->preferences_thread->update_framerate();
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
	if(playback_engine)
		playback_engine->stop_tracking();
	interrupt_lock->unlock();
}

void RenderEngine::close_output()
{
	if(audio)
	{
		audio->close_all();
		delete audio;
		audio = 0;
	}

	if(video)
	{
		video->close_all();
		delete video;
		video = 0;
	}
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

// Fix the tracking position
	if(playback_engine)
	{
		if(command->single_frame())
		{
			if(command->command != CURRENT_FRAME)
				playback_engine->stop_tracking();
		}
		else
		{
// Make sure transport doesn't issue a pause command next
			if(!interrupted)
			{
				if(do_audio)
					playback_engine->tracking_position = 
						arender->current_postime;
				else
				if(do_video)
				{
					playback_engine->tracking_position = 
						vrender->current_postime;
				}
				playback_engine->stop_tracking();
			}

		}
		playback_engine->is_playing_back = 0;
	}

	input_lock->unlock();
	interrupt_lock->unlock();
}

int RenderEngine::start_video()
{
// start video for realtime
	if(video) video->start_playback();
	vrender->start_playback();
}

void RenderEngine::wait_another(const char *location)
{
	if(do_audio && do_video)
		render_start_lock->wait_another(location);
}
