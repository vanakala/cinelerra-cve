#include "arender.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "localsession.h"
#include "mainclock.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "tracking.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "renderengine.h"
#include "mainsession.h"
#include "trackcanvas.h"



// States
#define PLAYING 0
#define PAUSED 1
#define DONE 2



Tracking::Tracking(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->gui = mwindow->gui;
	this->mwindow = mwindow; 
	follow_loop = 0; 
	visible = 0;
	pixel = 0;
	state = PAUSED;
}

Tracking::~Tracking()
{
	state = DONE;
	pause_lock.unlock();
	Thread::join();
}

void Tracking::create_objects()
{
	startup_lock.lock();
	pause_lock.lock();
	start();
}

int Tracking::start_playback(double new_position)
{
	last_position = new_position;
	state = PLAYING;
	draw();
	pause_lock.unlock();
	return 0;
}

int Tracking::stop_playback()
{
// Wait to change state
	loop_lock.lock();
// Stop loop
	pause_lock.lock();
	state = PAUSED;
	loop_lock.unlock();

// Final position is updated continuously during playback
// Get final position
	double position = get_tracking_position();
// Update cursor
	update_tracker(position);
//printf("Tracking::stop_playback %ld\n", position);
	
	stop_meters();
	return 0;
}

PlaybackEngine* Tracking::get_playback_engine()
{
	return mwindow->cwindow->playback_engine;
}

double Tracking::get_tracking_position()
{
//printf("Tracking::get_tracking_position %ld\n", get_playback_engine()->get_tracking_position());
	return get_playback_engine()->get_tracking_position();
}

int Tracking::get_pixel(double position)
{
	return (long)((position - mwindow->edl->local_session->view_start) *
		mwindow->edl->session->sample_rate / 
		mwindow->edl->local_session->zoom_sample + 
		0.5);
}


void Tracking::update_meters(long position)
{
	double output_levels[MAXCHANNELS];
//printf("Tracking::update_meters 1\n");
	int do_audio = get_playback_engine()->get_output_levels(output_levels, position);

	if(do_audio)
	{
		module_levels.remove_all();
		get_playback_engine()->get_module_levels(&module_levels, position);

		mwindow->cwindow->gui->lock_window();
		mwindow->cwindow->gui->meters->update(output_levels);
		mwindow->cwindow->gui->unlock_window();

		mwindow->lwindow->gui->lock_window();
		mwindow->lwindow->gui->panel->update(output_levels);
		mwindow->lwindow->gui->unlock_window();

//for(int i = 0; i < module_levels.total; i++)
//	printf("Tracking::update_meters 2 %f\n", module_levels.values[i]);

		mwindow->gui->lock_window();
		mwindow->gui->patchbay->update_meters(&module_levels);
		mwindow->gui->unlock_window();
	}
//printf("Tracking::update_meters 3\n");
}

void Tracking::stop_meters()
{
//printf("Tracking::stop_meters 1\n");
	mwindow->cwindow->gui->lock_window();
	mwindow->cwindow->gui->meters->stop_meters();
	mwindow->cwindow->gui->unlock_window();

	mwindow->gui->lock_window();
	mwindow->gui->patchbay->stop_meters();
	mwindow->gui->unlock_window();

	mwindow->lwindow->gui->lock_window();
	mwindow->lwindow->gui->panel->stop_meters();
	mwindow->lwindow->gui->unlock_window();
//printf("Tracking::stop_meters 2\n");
}




void Tracking::update_tracker(double position)
{
}

void Tracking::draw()
{
	gui->lock_window();
	if(!visible)
	{
		pixel = get_pixel(last_position);
	}

	gui->canvas->set_color(GREEN);
	gui->canvas->set_inverse();
	gui->canvas->draw_line(pixel, 0, pixel, gui->canvas->get_h());
	gui->canvas->set_opaque();
//printf("Tracking::draw %d %d\n", pixel, gui->canvas->get_h());
	gui->canvas->flash(pixel, 0, pixel + 1, gui->canvas->get_h());
	visible ^= 1;
	gui->unlock_window();
}


int Tracking::wait_for_startup()
{
	startup_lock.lock();
	startup_lock.unlock();
	return 0;
}

void Tracking::run()
{
	double position;
	long last_peak = -1;
	long last_peak_number = 0; // for zeroing peaks
	long *peak_samples;
	long current_peak = 0, starting_peak;
	int total_peaks;
	int i, j, pass;
	int audio_on = 0;

	startup_lock.unlock();

//printf("Tracking::run 1\n");
	while(state != DONE)
	{
		pause_lock.lock();
		pause_lock.unlock();

//printf("Tracking::run 2\n");
		loop_lock.lock();
		if(state != PAUSED && state != DONE)
			timer.delay(100);

		if(state != PAUSED && state != DONE)
		{

//printf("Tracking::run 3\n");
// can be stopped during wait
			if(get_playback_engine()->tracking_active)   
			{
// Get position of cursor
				position = get_tracking_position();

//printf("Tracking::run 4 %ld\n", position);
// Update cursor
				update_tracker(position);

//printf("Tracking::run 5\n");
			}
		}
		loop_lock.unlock();
//printf("Tracking::run 6\n");
	}
}





