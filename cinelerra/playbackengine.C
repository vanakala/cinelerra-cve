#include "cache.h"
#include "condition.h"
#include "defaults.h"
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
#include "videodevice.h"
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
	tracking_lock = new Mutex("PlaybackEngine::tracking_lock");
	tracking_done = new Condition(1, "PlaybackEngine::tracking_done");
	pause_lock = new Condition(0, "PlaybackEngine::pause_lock");
	start_lock = new Condition(0, "PlaybackEngine::start_lock");
	render_engine = 0;
	debug = 0;
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
	delete tracking_lock;
	delete tracking_done;
	delete pause_lock;
	delete start_lock;
}

int PlaybackEngine::create_objects()
{
	int result = 0;
	preferences = new Preferences;
	command = new TransportCommand;
	que = new TransportQue;
// Set the first change to maximum
	que->command.change_type = CHANGE_ALL;

	preferences->copy_from(mwindow->preferences);

	done = 0;
	Thread::start();
	start_lock->lock("PlaybackEngine::create_objects");
	return result;
}

ChannelDB* PlaybackEngine::get_channeldb()
{
	PlaybackConfig *config = command->get_edl()->session->playback_config;
	switch(config->vconfig->driver)
	{
		case VIDEO4LINUX2JPEG:
			return mwindow->channeldb_v4l2jpeg;
			break;
		case PLAYBACK_BUZ:
			return mwindow->channeldb_buz;
			break;
	}
	return 0;
}

int PlaybackEngine::create_render_engine()
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
		get_channeldb());  
	return 0;
}

void PlaybackEngine::delete_render_engine()
{
	delete render_engine;
	render_engine = 0;
}

void PlaybackEngine::arm_render_engine()
{
	int current_achannel = 0, current_vchannel = 0;
	if(render_engine)
		render_engine->arm_command(command,
			current_achannel,
			current_vchannel);
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
		audio_cache = new CICache(command->get_edl(), preferences, mwindow->plugindb);
	else
		audio_cache->set_edl(command->get_edl());

	if(!video_cache) 
		video_cache = new CICache(command->get_edl(), preferences, mwindow->plugindb);
	else
		video_cache->set_edl(command->get_edl());
}


void PlaybackEngine::perform_change()
{
	switch(command->change_type)
	{
		case CHANGE_ALL:
			create_cache();
		case CHANGE_EDL:
			audio_cache->set_edl(command->get_edl());
			video_cache->set_edl(command->get_edl());
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
int PlaybackEngine::get_output_levels(double *levels, long position)
{
	int result = 0;
	if(render_engine && render_engine->do_audio)
	{
		result = 1;
		render_engine->get_output_levels(levels, position);
	}
	return result;
}


int PlaybackEngine::get_module_levels(ArrayList<double> *module_levels, long position)
{
	int result = 0;
	if(render_engine && render_engine->do_audio)
	{
		result = 1;
		render_engine->get_module_levels(module_levels, position);
	}
	return result;
}

int PlaybackEngine::brender_available(long position)
{
	return 0;
}

void PlaybackEngine::init_cursor()
{
}

void PlaybackEngine::stop_cursor()
{
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

void PlaybackEngine::update_tracking(double position)
{
	tracking_lock->lock("PlaybackEngine::update_tracking");

	tracking_position = position;

// Signal that the timer is accurate.
	if(tracking_active) tracking_active = 2;
	tracking_timer.update();
	tracking_lock->unlock();
}

double PlaybackEngine::get_tracking_position()
{
	double result = 0;

	tracking_lock->lock("PlaybackEngine::get_tracking_position");


// Adjust for elapsed time since last update_tracking.
// But tracking timer isn't accurate until the first update_tracking
// so wait.
	if(tracking_active == 2)
	{
//printf("PlaybackEngine::get_tracking_position %d %d %d\n", command->get_direction(), tracking_position, tracking_timer.get_scaled_difference(command->get_edl()->session->sample_rate));


// Don't interpolate when every frame is played.
		if(command->get_edl()->session->video_every_frame &&
			render_engine &&
			render_engine->do_video)
		{
			result = tracking_position;
		}
		else
// Interpolate
		{
			double loop_start = command->get_edl()->local_session->loop_start;
			double loop_end = command->get_edl()->local_session->loop_end;
			double loop_size = loop_end - loop_start;

			if(command->get_direction() == PLAY_FORWARD)
			{
// Interpolate
				result = tracking_position + 
					command->get_speed() * 
					tracking_timer.get_difference() /
					1000.0;

// Compensate for loop
//printf("PlaybackEngine::get_tracking_position 1 %d\n", command->get_edl()->local_session->loop_playback);
				if(command->get_edl()->local_session->loop_playback)
				{
					while(result > loop_end) result -= loop_size;
				}
			}
			else
			{
// Interpolate
				result = tracking_position - 
					command->get_speed() * 
					tracking_timer.get_difference() /
					1000.0;

// Compensate for loop
				if(command->get_edl()->local_session->loop_playback)
				{
					while(result < loop_start) result += loop_size;
				}
			}

		}
	}
	else
		result = tracking_position;

	tracking_lock->unlock();
//printf("PlaybackEngine::get_tracking_position %f %f %d\n", result, tracking_position, tracking_active);

// Adjust for loop

	return result;
}

void PlaybackEngine::update_transport(int command, int paused)
{
//	mwindow->gui->lock_window();
//	mwindow->gui->mbuttons->transport->update_gui_state(command, paused);
//	mwindow->gui->unlock_window();
}

void PlaybackEngine::run()
{
	start_lock->unlock();

	do
	{
// Wait for current command to finish
		que->output_lock->lock("PlaybackEngine::run");

//printf("PlaybackEngine::run 1\n");
		wait_render_engine();
//printf("PlaybackEngine::run 2\n");


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
//				command->command = last_command;
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
 				if(command->command == SINGLE_FRAME_FWD ||
					command->command == SINGLE_FRAME_REWIND)
				{
 					command->playbackstart = get_tracking_position();
				}

				perform_change();
				arm_render_engine();

// Start tracking after arming so the tracking position doesn't change.
// The tracking for a single frame command occurs during PAUSE
				init_tracking();

// Dispatch the command
				start_render_engine();
				break;
		}


//printf("PlaybackEngine::run 100\n");
	}while(!done);
}



