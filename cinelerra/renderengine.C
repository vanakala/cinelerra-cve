#include "amodule.h"
#include "arender.h"
#include "asset.h"
#include "audiodevice.h"
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
	ArrayList<Channel*> *channeldb,
	int head_number)
 : Thread()
{
	this->playback_engine = playback_engine;
	this->output = output;
	this->plugindb = plugindb;
	this->channeldb = channeldb;
	this->head_number = head_number;
	set_synchronous(1);
	audio = 0;
	video = 0;
	arender = 0;
	vrender = 0;
	do_audio = 0;
	do_video = 0;
	interrupted = 0;
	actual_frame_rate = 0;
 	this->preferences = new Preferences;
 	this->command = new TransportCommand;
 	*this->preferences = *preferences;
 	*this->command = *command;
	edl = new EDL;
	edl->create_objects();
// EDL only changed in construction.
// The EDL contained in later commands is ignored.
	*this->edl = *command->get_edl();
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
}

int RenderEngine::arm_command(TransportCommand *command,
	int &current_vchannel, 
	int &current_achannel)
{
// Prevent this renderengine from accepting another command until finished.
// Since the renderengine is often deleted after the input_lock command it must
// be locked here as well as in the calling routine.


//printf("RenderEngine::arm_command 1\n");
	input_lock->lock("RenderEngine::arm_command");
//printf("RenderEngine::arm_command 1\n");


	*this->command = *command;
	int playback_strategy = command->get_edl()->session->playback_strategy;
	this->config = command->get_edl()->session->playback_config[playback_strategy].values[head_number];

//printf("RenderEngine::arm_command 1\n");
// Fix background rendering asset to use current dimensions and ignore
// headers.
	preferences->brender_asset->frame_rate = command->get_edl()->session->frame_rate;
	preferences->brender_asset->width = command->get_edl()->session->output_w;
	preferences->brender_asset->height = command->get_edl()->session->output_h;
	preferences->brender_asset->use_header = 0;
	preferences->brender_asset->layers = 1;
	preferences->brender_asset->video_data = 1;

//printf("RenderEngine::arm_command 1\n");
	done = 0;
	interrupted = 0;

// Retool configuration for this node
	VideoOutConfig *vconfig = this->config->vconfig;
	AudioOutConfig *aconfig = this->config->aconfig;
	if(command->realtime)
	{
		int device_channels = 0;
		int edl_channels = 0;
		if(command->single_frame())
		{
			vconfig->driver = PLAYBACK_X11;
			device_channels = 1;
			edl_channels = command->get_edl()->session->video_channels;
		}
		else
		{
			device_channels = 1;
			edl_channels = command->get_edl()->session->video_channels;
		}

		for(int i = 0; i < MAX_CHANNELS; i++)
		{
			vconfig->do_channel[i] = 
				((i == current_vchannel) && 
					device_channels &&
					edl_channels);

// GCC 3.2 optimization error causes do_channel[0] to always be 0 unless
// we do this.
Workarounds::clamp(vconfig->do_channel[i], 0, 1);

			if(vconfig->do_channel[i])
			{
				current_vchannel++;
				device_channels--;
				edl_channels--;
			}
		}

		device_channels = aconfig->total_output_channels();
		edl_channels = command->get_edl()->session->audio_channels;

		for(int i = 0; i < MAX_CHANNELS; i++)
		{

			aconfig->do_channel[i] = 
				(i == current_achannel && 
					device_channels &&
					edl_channels);
			if(aconfig->do_channel[i])
			{
				current_achannel++;
				device_channels--;
				edl_channels--;
			}
		}

	}
	else
	{
		this->config->playback_strategy = PLAYBACK_LOCALHOST;
		vconfig->driver = PLAYBACK_X11;
		vconfig->playback_strategy = PLAYBACK_LOCALHOST;
		aconfig->playback_strategy = PLAYBACK_LOCALHOST;
		for(int i = 0; i < MAX_CHANNELS; i++)
		{
			vconfig->do_channel[i] = (i < command->get_edl()->session->video_channels);
			vconfig->do_channel[i] = (i < command->get_edl()->session->video_channels);
			aconfig->do_channel[i] = (i < command->get_edl()->session->audio_channels);
		}
	}

//printf("RenderEngine::arm_command 1\n");


	get_duty();

//printf("RenderEngine::arm_command 1 %d %d\n", do_audio, do_video);
	if(do_audio)
	{
// Larger of audio_module_fragment and fragment length adjusted for speed
// Extra memory must be allocated for rendering slow motion.
		adjusted_fragment_len = (int64_t)((float)edl->session->audio_module_fragment / 
			command->get_speed() + 0.5);
		if(adjusted_fragment_len < edl->session->audio_module_fragment)
			adjusted_fragment_len = edl->session->audio_module_fragment;
	}

//printf("RenderEngine::arm_command 1\n");
	open_output();
//printf("RenderEngine::arm_command 1\n");
	create_render_threads();
//printf("RenderEngine::arm_command 1\n");
	arm_render_threads();
//printf("RenderEngine::arm_command 10\n");

	return 0;
}

void RenderEngine::get_duty()
{
	do_audio = 0;
	do_video = 0;

//edl->dump();
//printf("RenderEngine::get_duty 1 %d %d\n", edl->tracks->playable_audio_tracks(), config->vconfig->total_playable_channels());
	if(!command->single_frame() &&
		edl->tracks->playable_audio_tracks() &&
		config->aconfig->total_playable_channels())
	{
		do_audio = 1;
	}

//printf("RenderEngine::get_duty 2 %d %d\n", edl->tracks->playable_video_tracks(), config->vconfig->total_playable_channels());
	if(edl->tracks->playable_video_tracks() &&
		config->vconfig->total_playable_channels())
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
	return edl->calculate_output_w(config->playback_strategy != PLAYBACK_LOCALHOST);
}

int RenderEngine::get_output_h()
{
	return edl->calculate_output_h(config->playback_strategy != PLAYBACK_LOCALHOST);
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
		if(config->vconfig->buz_out_channel >= 0 && 
			config->vconfig->buz_out_channel < channeldb->total)
		{
			return channeldb->values[config->vconfig->buz_out_channel];
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
//printf("RenderEngine::open_output 1\n");
	if(command->realtime)
	{
// Allocate devices
		if(do_audio)
		{
			audio = new AudioDevice;
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
			audio->open_output(config->aconfig, 
				edl->session->sample_rate, 
				adjusted_fragment_len,
				edl->session->real_time_playback);
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

int64_t RenderEngine::sync_position()
{
	switch(edl->session->playback_strategy)
	{
		case PLAYBACK_LOCALHOST:
		case PLAYBACK_MULTIHEAD:
// Use audio device
// No danger of race conditions because the output devices are closed after all
// threads join.
//printf("RenderEngine::sync_position 1\n");
			if(do_audio)
			{
				return audio->current_position();
			}

			if(do_video)
			{
				int64_t result = timer.get_scaled_difference(
					edl->session->sample_rate);
//printf("RenderEngine::sync_position 2 %lld\n", timer.get_difference());
				return result;
			}
			break;

		case PLAYBACK_BLONDSYMPHONY:
			break;
	}
}

PluginServer* RenderEngine::scan_plugindb(char *title)
{
	for(int i = 0; i < plugindb->total; i++)
	{
		if(!strcasecmp(plugindb->values[i]->title, title))
			return plugindb->values[i];
	}
	return 0;
}

int RenderEngine::start_command()
{
//printf("RenderEngine::start_command 1 %d\n", command->realtime);
	if(command->realtime)
	{
		interrupt_lock->lock("RenderEngine::start_command");
		start_lock->lock("RenderEngine::start_command 1");
		Thread::start();
		start_lock->lock("RenderEngine::start_command 2");
		start_lock->unlock();
//printf("RenderEngine::start_command 2 %p %d\n", this, Thread::get_tid());
	}
	return 0;
}

void RenderEngine::arm_render_threads()
{
//printf("RenderEngine::arm_render_threads 1 %d %d\n", do_audio, do_video);
	if(do_audio)
	{
		arender->arm_command();
	}

	if(do_video)
	{
//printf("RenderEngine::arm_render_threads 1 %d %d\n", do_audio, do_video);
		vrender->arm_command();
//printf("RenderEngine::arm_render_threads 2 %d %d\n", do_audio, do_video);
	}
}


void RenderEngine::start_render_threads()
{
// Synchronization timer
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
//printf("RenderEngine::wait_render_threads 1\n");
	if(do_video)
	{
		vrender->Thread::join();
	}
//printf("RenderEngine::wait_render_threads 2\n");
}

void RenderEngine::interrupt_playback()
{
//printf("RenderEngine::interrupt_playback 0 %p\n", this);
	interrupt_lock->lock("RenderEngine::interrupt_playback");
	interrupted = 1;
	if(audio)
	{
//printf("RenderEngine::interrupt_playback 1 %p\n", this);
		audio->interrupt_playback();
//printf("RenderEngine::interrupt_playback 2 %p\n", this);
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
//printf("RenderEngine::close_output 1\n");
	if(audio)
	{
//printf("RenderEngine::close_output 2\n");
		audio->close_all();
		delete audio;
		audio = 0;
	}



	if(video)
	{
//printf("RenderEngine::close_output 1\n");
		video->close_all();
//printf("RenderEngine::close_output 2\n");
		delete video;
//printf("RenderEngine::close_output 3\n");
		video = 0;
	}
//printf("RenderEngine::close_output 4\n");
	return 0;
}

void RenderEngine::get_output_levels(double *levels, int64_t position)
{
	if(do_audio)
	{
		int history_entry = arender->get_history_number(arender->level_samples, position);
//printf("RenderEngine::get_output_levels %d\n", history_entry);
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
//printf("RenderEngine::run 1 %p\n", this);
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
	reverse = 0;
	start_position = 0;
	audio_channels = 0;
	do_audio = 0;
	do_video = 0;
	done = 0;
}

int RenderEngine::arm_playback_common(int64_t start_sample, 
			int64_t end_sample,
			int64_t current_sample,
			int reverse, 
			float speed, 
			int follow_loop,
			int infinite)
{
	this->follow_loop = follow_loop;
	this->start_position = start_sample;
	this->end_position = end_sample;
	this->reverse = reverse;
	this->infinite = infinite;
	this->current_sample = current_sample;
	if(infinite) this->follow_loop = 0;
}

int RenderEngine::arm_playback_audio(int64_t input_length, 
			int64_t amodule_render_fragment, 
			int64_t playback_buffer, 
			int64_t output_length, 
			int audio_channels)
{
	this->audio_channels = audio_channels;

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

