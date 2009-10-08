
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
#include "channeldb.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "renderengine.h"
#include "mainsession.h"
#include "tracks.h"
#include "transportque.h"
#include "videodevice.h"
#include "vrender.h"
#include "workarounds.h"



RenderEngine::RenderEngine(PlaybackEngine *playback_engine,
 	Preferences *preferences, 
	TransportCommand *command,
	Canvas *output,
	ArrayList<PluginServer*> *plugindb,
	ChannelDB *channeldb)
 : Thread(1, 0, 0)
{
	this->playback_engine = playback_engine;
	this->output = output;
	this->plugindb = plugindb;
	this->channeldb = channeldb;
	audio = 0;
	video = 0;
	config = new PlaybackConfig;
	arender = 0;
	vrender = 0;
	do_audio = 0;
	do_video = 0;
	interrupted = 0;
	actual_frame_rate = 0;
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
	show_tc = 0;


	input_lock = new Condition(1, "RenderEngine::input_lock");
	start_lock = new Condition(1, "RenderEngine::start_lock");
	output_lock = new Condition(1, "RenderEngine::output_lock");
	interrupt_lock = new Mutex("RenderEngine::interrupt_lock");
	first_frame_lock = new Condition(1, "RenderEngine::first_frame_lock");
	reset_parameters();
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
	delete first_frame_lock;
	delete config;
}

int RenderEngine::arm_command(TransportCommand *command,
	int &current_vchannel, 
	int &current_achannel)
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
		fragment_len = aconfig->fragment_size;
// Larger of audio_module_fragment and fragment length adjusted for speed
// Extra memory must be allocated for rendering slow motion.
		adjusted_fragment_len = (int64_t)((float)aconfig->fragment_size / 
			command->get_speed() + 0.5);
		if(adjusted_fragment_len < aconfig->fragment_size)
			adjusted_fragment_len = aconfig->fragment_size;
	}

// Set lock so audio doesn't start until video has started.
	if(do_video)
	{
		while(first_frame_lock->get_value() > 0) 
			first_frame_lock->lock("RenderEngine::arm_command");
	}
	else
// Set lock so audio doesn't wait for video which is never to come.
	{
		while(first_frame_lock->get_value() <= 0)
			first_frame_lock->unlock();
	}

	open_output();
	create_render_threads();
	arm_render_threads();

	return 0;
}

void RenderEngine::get_duty()
{
	do_audio = 0;
	do_video = 0;

//edl->dump();
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

int RenderEngine::brender_available(int position, int direction)
{
	if(playback_engine)
	{
		int64_t corrected_position = position;
		if(direction == PLAY_REVERSE)
			corrected_position--;
		return playback_engine->brender_available(corrected_position);
	}
	else
		return 0;
}

Channel* RenderEngine::get_current_channel()
{
	if(channeldb)
	{
		switch(config->vconfig->driver)
		{
			case PLAYBACK_BUZ:
				if(config->vconfig->buz_out_channel >= 0 && 
					config->vconfig->buz_out_channel < channeldb->size())
				{
					return channeldb->get(config->vconfig->buz_out_channel);
				}
				break;
			case VIDEO4LINUX2JPEG:
				
				break;
		}
	}
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


double RenderEngine::get_tracking_position()
{
	if(playback_engine) 
		return playback_engine->get_tracking_position();
	else
		return 0;
}

int RenderEngine::open_output()
{
	if(command->realtime)
	{
// Allocate devices
		if(do_audio)
		{
			audio = new AudioDevice;
			if (audio->open_output(config->aconfig, 
				edl->session->sample_rate, 
				adjusted_fragment_len,
				edl->session->audio_channels,
				edl->session->real_time_playback))
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
			Channel *channel = get_current_channel();
			if(channel) video->set_channel(channel);
			video->set_quality(80);
			video->set_cpus(preferences->processors);
		}
	}

	return 0;
}

int64_t RenderEngine::session_position()
{
	if(do_audio)
	{
		return audio->current_position();
	}

	if(do_video)
	{
		return (int64_t)((double)vrender->session_frame / 
				edl->session->frame_rate * 
				edl->session->sample_rate /
				command->get_speed() + 0.5);
	}
}

void RenderEngine::reset_sync_position()
{
	timer.update();
}

int64_t RenderEngine::sync_position()
{
// Use audio device
// No danger of race conditions because the output devices are closed after all
// threads join.
	if(do_audio)
	{
		return audio->current_position();
	}

	if(do_video)
	{
		int64_t result = timer.get_scaled_difference(
			edl->session->sample_rate);
		return result;
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

int RenderEngine::start_command()
{
	if(command->realtime)
	{
		interrupt_lock->lock("RenderEngine::start_command");
		start_lock->lock("RenderEngine::start_command 1");
		Thread::start();
		start_lock->lock("RenderEngine::start_command 2");
		start_lock->unlock();
	}
	return 0;
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
//printf("RenderEngine::interrupt_playback 3 %p\n", this);
		video->interrupt_playback();
//printf("RenderEngine::interrupt_playback 4 %p\n", this);
	}
	interrupt_lock->unlock();
}

int RenderEngine::close_output()
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
	return 0;
}

void RenderEngine::get_output_levels(double *levels, int64_t position)
{
	if(do_audio)
	{
		int history_entry = arender->get_history_number(arender->level_samples, 
			position);
		for(int i = 0; i < MAXCHANNELS; i++)
		{
			if(arender->audio_out[i])
				levels[i] = arender->level_history[i][history_entry];
		}
	}
}

void RenderEngine::get_module_levels(ArrayList<double> *module_levels, int64_t position)
{
	if(do_audio)
	{
		for(int i = 0; i < arender->total_modules; i++)
		{
//printf("RenderEngine::get_module_levels %p %p\n", ((AModule*)arender->modules[i]), ((AModule*)arender->modules[i])->level_samples);
			int history_entry = arender->get_history_number(((AModule*)arender->modules[i])->level_samples, position);

			module_levels->append(((AModule*)arender->modules[i])->level_history[history_entry]);
		}
	}
}





void RenderEngine::run()
{
	start_render_threads();
	start_lock->unlock();
	interrupt_lock->unlock();

	wait_render_threads();

	interrupt_lock->lock("RenderEngine::run");


	if(interrupted)
	{
		playback_engine->tracking_position = playback_engine->get_tracking_position();
	}

	close_output();

// Fix the tracking position
	if(playback_engine)
	{
		if(command->command == CURRENT_FRAME)
		{
//printf("RenderEngine::run 4.1 %d\n", playback_engine->tracking_position);
			playback_engine->tracking_position = command->playbackstart;
		}
		else
		{
// Make sure transport doesn't issue a pause command next
//printf("RenderEngine::run 4.1 %d\n", playback_engine->tracking_position);
			if(!interrupted)
			{
				if(do_audio)
					playback_engine->tracking_position = 
						(double)arender->current_position / 
							command->get_edl()->session->sample_rate;
				else
				if(do_video)
				{
					playback_engine->tracking_position = 
						(double)vrender->current_position / 
							command->get_edl()->session->frame_rate;
				}
			}

			if(!interrupted) playback_engine->command->command = STOP;
			playback_engine->stop_tracking();

		}
		playback_engine->is_playing_back = 0;
	}

	input_lock->unlock();
	interrupt_lock->unlock();
}

























int RenderEngine::reset_parameters()
{
	start_position = 0;
	follow_loop = 0;
	end_position = 0;
	infinite = 0;
	start_position = 0;
	do_audio = 0;
	do_video = 0;
	done = 0;
}

int RenderEngine::arm_playback_audio(int64_t input_length, 
			int64_t amodule_render_fragment, 
			int64_t playback_buffer, 
			int64_t output_length)
{

	do_audio = 1;

	arender = new ARender(this);
	arender->arm_playback(current_sample, 
							input_length, 
							amodule_render_fragment, 
							playback_buffer, 
							output_length);
}

int RenderEngine::arm_playback_video(int every_frame, 
			int64_t read_length, 
			int64_t output_length,
			int track_w,
			int track_h,
			int output_w,
			int output_h)
{
	do_video = 1;
	this->every_frame = every_frame;

	vrender = new VRender(this);
// 	vrender->arm_playback(current_sample, 
// 							read_length, 
// 							output_length, 
// 							output_length, 
// 							track_w,
// 							track_h,
// 							output_w,
// 							output_h);
}

int RenderEngine::start_video()
{
// start video for realtime
	if(video) video->start_playback();
	vrender->start_playback();
}


int64_t RenderEngine::get_correction_factor(int reset)
{
	if(!every_frame)
	{
		int64_t x;
//		x = playbackengine->correction_factor;
//		if(reset) playbackengine->correction_factor = 0;
		return x;
	}
	else
		return 0;
}

