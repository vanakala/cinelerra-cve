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
 : Thread()
{
	this->mwindow = mwindow;
	this->output = output;
	is_playing_back = 0;
	tracking_position = 0;
	tracking_active = 0;
	audio_cache = 0;
	video_cache = 0;
	last_command = STOP;
	Thread::set_synchronous(1);
	tracking_lock = new Mutex("PlaybackEngine::tracking_lock");
	tracking_done = new Condition(1, "PlaybackEngine::tracking_done");
	pause_lock = new Condition(0, "PlaybackEngine::pause_lock");
	start_lock = new Condition(0, "PlaybackEngine::start_lock");
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
	delete_render_engines();
	if(audio_cache) delete audio_cache;
	if(video_cache) delete video_cache;
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

	*preferences = *mwindow->preferences;

	done = 0;
	Thread::start();
	start_lock->lock("PlaybackEngine::create_objects");
	return result;
}


int PlaybackEngine::create_render_engines()
{
//printf("PlaybackEngine::create_render_engines 1 %d\n", command->edl->session->playback_strategy);
// Fix playback configurations
	int playback_strategy = command->get_edl()->session->playback_strategy;
	int current_vchannel = 0;
	int current_achannel = 0;

	delete_render_engines();


//printf("PlaybackEngine::create_render_engines %d\n", command->get_edl()->session->playback_config[playback_strategy].total);
	for(int i = 0; 
		i < command->get_edl()->session->playback_config[playback_strategy].total; 
		i++)
	{
		RenderEngine *engine = new RenderEngine(this,
			preferences, 
			command, 
			output,
			mwindow->plugindb,
			&mwindow->channeldb_buz,
			i);  
		render_engines.append(engine);
	}
	return 0;
}

void PlaybackEngine::delete_render_engines()
{
	render_engines.remove_all_objects();
}

void PlaybackEngine::arm_render_engines()
{
	int current_achannel = 0, current_vchannel = 0;
	for(int i = 0; i < render_engines.total; i++)
	{
//printf("PlaybackEngine::arm_render_engines %ld\n", command->playbackstart);
		render_engines.values[i]->arm_command(command,
			current_achannel,
			current_vchannel);
	}
}

void PlaybackEngine::start_render_engines()
{
//printf("PlaybackEngine::start_render_engines 1 %d\n", render_engines.total);
	for(int i = 0; i < render_engines.total; i++)
	{
		render_engines.values[i]->start_command();
	}
}

void PlaybackEngine::wait_render_engines()
{
	if(command->realtime)
	{
		for(int i = 0; i < render_engines.total; i++)
		{
			render_engines.values[i]->join();
		}
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
//printf("PlaybackEngine::perform_change 1 %x\n", command->change_type);
	switch(command->change_type)
	{
		case CHANGE_ALL:
			create_cache();
		case CHANGE_EDL:
			audio_cache->set_edl(command->get_edl());
			video_cache->set_edl(command->get_edl());
//printf("PlaybackEngine::perform_change 1\n");
			create_render_engines();
		case CHANGE_PARAMS:
//printf("PlaybackEngine::perform_change 1\n");
			if(command->change_type != CHANGE_EDL &&
				command->change_type != CHANGE_ALL)
				for(int i = 0; i < render_engines.total; i++)
					render_engines.values[i]->edl->synchronize_params(command->get_edl());
		case CHANGE_NONE:
//printf("PlaybackEngine::perform_change 2\n");
			break;
	}
}

void PlaybackEngine::sync_parameters(EDL *edl)
{
// TODO: lock out render engine from keyframe deletions
	for(int i = 0; i < render_engines.total; i++)
		render_engines.values[i]->edl->synchronize_params(edl);
}


void PlaybackEngine::interrupt_playback(int wait_tracking)
{
	for(int i = 0; i < render_engines.total; i++)
		render_engines.values[i]->interrupt_playback();

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
	for(int j = 0; j < render_engines.total; j++)
	{
		if(render_engines.values[j]->do_audio)
		{
			result = 1;
			render_engines.values[j]->get_output_levels(levels, position);
		}
	}
	return result;
}


int PlaybackEngine::get_module_levels(ArrayList<double> *module_levels, long position)
{
	int result = 0;
	for(int j = 0; j < render_engines.total; j++)
	{
		if(render_engines.values[j]->do_audio)
		{
			result = 1;
			render_engines.values[j]->get_module_levels(module_levels, position);
			break;
		}
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
			render_engines.total &&
			render_engines.values[0]->do_video)
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

		wait_render_engines();


// Read the new command
		que->input_lock->lock("PlaybackEngine::run");
		if(done) return;

		*command = que->command;
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
				arm_render_engines();
// Dispatch the command
				start_render_engines();
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
				arm_render_engines();

// Start tracking after arming so the tracking position doesn't change.
// The tracking for a single frame command occurs during PAUSE
				init_tracking();

// Dispatch the command
				start_render_engines();
				break;
		}


//printf("PlaybackEngine::run 100\n");
	}while(!done);
}






















int PlaybackEngine::reset_parameters()
{
// called before every playback
	is_playing_back = 0;
	follow_loop = 0;
	speed = 1;
	reverse = 0;
	cursor = 0;
	last_position = 0;
	playback_start = playback_end = 0;
	infinite = 0;
	use_buttons = 0;
	audio = 0;
	video = 0;
	shared_audio = 0;
	return 0;
}

int PlaybackEngine::init_parameters()
{
//	is_playing_back = 1;
	update_button = 1;
	correction_factor = 0;

// correct playback buffer sizes
	input_length = 
		playback_buffer = 
		output_length = 
		audio_module_fragment = 
		command->get_edl()->session->audio_module_fragment;

// get maximum actual buffer size written to device plus padding
	if(speed != 1) output_length = (long)(output_length / speed) + 16;   
	if(output_length < playback_buffer) output_length = playback_buffer;

// samples to read at a time is a multiple of the playback buffer greater than read_length
	while(input_length < command->get_edl()->session->audio_read_length)
		input_length += playback_buffer;
	return 0;
}

int PlaybackEngine::init_audio_device()
{
	return 0;
}

int PlaybackEngine::init_video_device()
{
	return 0;
}



int PlaybackEngine::start_reconfigure()
{
	reconfigure_status = is_playing_back;
//	if(is_playing_back) stop_playback(0);
	return 0;
}

int PlaybackEngine::stop_reconfigure()
{
	return 0;
}

int PlaybackEngine::reset_buttons()
{
	return 0;
}


long PlaybackEngine::absolute_position(int sync_time)
{
	return 0;
}

long PlaybackEngine::get_position(int sync_time)
{
	long result;

	switch(is_playing_back)
	{
		case 2:
// playing back
			result = absolute_position(sync_time);
// adjust for speed
			result = (long)(result * speed);

// adjust direction and initial position
			if(reverse)
			{
				result = playback_end - result;
				result += loop_adjustment;

// adjust for looping
// 				while(mwindow->session->loop_playback && follow_loop && 
// 					result < mwindow->session->loop_start)
// 				{
// 					result += mwindow->session->loop_end - mwindow->session->loop_start;
// 					loop_adjustment += mwindow->session->loop_end - mwindow->session->loop_start;
// 				}
			}
			else
			{
				result += playback_start;
				result -= loop_adjustment;
				
// 				while(mwindow->session->loop_playback && follow_loop && 
// 					result > mwindow->session->loop_end)
// 				{
// 					result -= mwindow->session->loop_end - mwindow->session->loop_start;
// 					loop_adjustment += mwindow->session->loop_end - mwindow->session->loop_start;
// 				}
			}
			break;

		case 1:
// paused
			result = last_position;
			break;

		default:
// no value
			result = -1;
			break;
	}

	return result;
}


int PlaybackEngine::move_right(long distance) 
{ 
	mwindow->move_right(distance); 
	return 0;
}

