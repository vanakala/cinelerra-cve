#include "edl.h"
#include "keys.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "theme.h"
#include "transportque.h"
#include "vframe.h"


PlayTransport::PlayTransport(MWindow *mwindow, 
	BC_WindowBase *subwindow, 
	int x, 
	int y)
{
	this->subwindow = subwindow;
	this->mwindow = mwindow;
	this->x = x;
	this->y = y;
	this->slider = 0;
	this->engine = 0;
}


PlayTransport::~PlayTransport()
{
	delete forward_play;
	delete frame_forward_play;
	delete reverse_play;
	delete frame_reverse_play;
	delete fast_reverse;
	delete fast_play;
	delete rewind_button;
	delete stop_button;
	delete end_button;
}

void PlayTransport::set_engine(PlaybackEngine *engine)
{
	this->engine = engine;
}

void PlayTransport::set_slider(BC_Slider *slider)
{
	this->slider = slider;
}

int PlayTransport::get_transport_width(MWindow *mwindow)
{
//printf("PlayTransport::get_transport_width %p\n", mwindow->theme->rewind_data);
	return mwindow->theme->stop_data[0]->get_w() * 7 +
		mwindow->theme->rewind_data[0]->get_w() * 2;
}

int PlayTransport::create_objects()
{
	int x = this->x, y = this->y;
	subwindow->add_subwindow(rewind_button = new RewindButton(mwindow, this, x, y));
	x += rewind_button->get_w();
	subwindow->add_subwindow(fast_reverse = new FastReverseButton(mwindow, this, x, y)); 
	x += fast_reverse->get_w();
	subwindow->add_subwindow(reverse_play = new ReverseButton(mwindow, this, x, y)); 
	x += reverse_play->get_w();
	subwindow->add_subwindow(frame_reverse_play = new FrameReverseButton(mwindow, this, x, y)); 
	x += frame_reverse_play->get_w();
	subwindow->add_subwindow(stop_button = new StopButton(mwindow, this, x, y)); 
	x += stop_button->get_w();
	subwindow->add_subwindow(frame_forward_play = new FramePlayButton(mwindow, this, x, y)); 
	x += frame_forward_play->get_w();
	subwindow->add_subwindow(forward_play = new PlayButton(mwindow, this, x, y)); 
	x += forward_play->get_w();
	subwindow->add_subwindow(fast_play = new FastPlayButton(mwindow, this, x, y)); 
	x += fast_play->get_w();
	subwindow->add_subwindow(end_button = new EndButton(mwindow, this, x, y)); 
	x += end_button->get_w();

	reverse = 0;
	speed = 0;

	return 0;
}

void PlayTransport::reposition_buttons(int x, int y)
{
	this->x = x;
	this->y = y;
	rewind_button->reposition_window(x, y);
	x += rewind_button->get_w();
	fast_reverse->reposition_window(x, y);
	x += fast_reverse->get_w();
	reverse_play->reposition_window(x, y);
	x += reverse_play->get_w();
	frame_reverse_play->reposition_window(x, y);
	x += frame_reverse_play->get_w();
	stop_button->reposition_window(x, y);
	x += stop_button->get_w();
	frame_forward_play->reposition_window(x, y);
	x += frame_forward_play->get_w();
	forward_play->reposition_window(x, y);
	x += forward_play->get_w();
	fast_play->reposition_window(x, y);
	x += fast_play->get_w();
	end_button->reposition_window(x, y);
	x += end_button->get_w();
}

int PlayTransport::get_w()
{
	return end_button->get_x() + end_button->get_w() - rewind_button->get_x();
}

int PlayTransport::flip_vertical(int vertical, int &x, int &y)
{
	if(vertical)
	{
		rewind_button->reposition_window(x, y); 
		y += rewind_button->get_h();
		fast_reverse->reposition_window(x, y); 
		y += fast_reverse->get_h();
		reverse_play->reposition_window(x, y); 
		y += reverse_play->get_h();
		frame_reverse_play->reposition_window(x, y); 
		y += frame_reverse_play->get_h();
		stop_button->reposition_window(x, y); 
		y += stop_button->get_h();
		frame_forward_play->reposition_window(x, y); 
		y += frame_forward_play->get_h();
		forward_play->reposition_window(x, y); 
		y += forward_play->get_h();
		fast_play->reposition_window(x, y); 
		y += fast_play->get_h();
		end_button->reposition_window(x, y); 
		y += end_button->get_h();
	}
	else
	{
		rewind_button->reposition_window(x, y - 2); 
		x += rewind_button->get_w();
		fast_reverse->reposition_window(x, y - 2); 
		x += fast_reverse->get_w();
		reverse_play->reposition_window(x, y - 2); 
		x += reverse_play->get_w();
		frame_reverse_play->reposition_window(x, y - 2); 
		x += frame_reverse_play->get_w();
		stop_button->reposition_window(x, y - 2); 
		x += stop_button->get_w();
		frame_forward_play->reposition_window(x, y - 2); 
		x += frame_forward_play->get_w();
		forward_play->reposition_window(x, y - 2); 
		x += forward_play->get_w();
		fast_play->reposition_window(x, y - 2); 
		x += fast_play->get_w();
		end_button->reposition_window(x, y - 2); 
		x += end_button->get_w();
	}

	return 0;
}

int PlayTransport::keypress_event()
{
//printf("PlayTransport::keypress_event 1\n");
	if(subwindow->shift_down()) return  0;
	int result = 0;

//printf("PlayTransport::keypress_event %x\n", subwindow->get_keypress());
	subwindow->unlock_window();
	switch(subwindow->get_keypress())
	{
		case KPPLUS:        handle_transport(FAST_REWIND);                result = 1; break;
		case KP6:           handle_transport(NORMAL_REWIND);              result = 1; break;
		case KP5:           handle_transport(SLOW_REWIND);                result = 1; break;
		case KP4:           handle_transport(SINGLE_FRAME_REWIND);        result = 1; break;
		case KP1:           handle_transport(SINGLE_FRAME_FWD);   		  result = 1; break;
		case KP2:           handle_transport(SLOW_FWD);           		  result = 1; break;
		case KP3:           handle_transport(NORMAL_FWD);         		  result = 1; break;
		case KPENTER:       handle_transport(FAST_FWD);           		  result = 1; break;
		case KPINS:         handle_transport(STOP);                       result = 1; break;
		case ' ':           handle_transport(NORMAL_FWD);                 result = 1; break;
		case 'k':           handle_transport(STOP);   					  result = 1; break;
	}

	subwindow->lock_window();

	return result;
}


void PlayTransport::goto_start()
{
	handle_transport(REWIND, 1);
}

void PlayTransport::goto_end()
{
	handle_transport(END, 1);
}



void PlayTransport::handle_transport(int command, int wait_tracking)
{
//printf("PlayTransport::handle_transport 1\n");
	if(!get_edl()) return;
//printf("PlayTransport::handle_transport 2 %d\n", command);

// Dispatch command
	switch(command)
	{
		case FAST_REWIND:
		case NORMAL_REWIND:
		case SLOW_REWIND:
		case SINGLE_FRAME_REWIND:
		case SINGLE_FRAME_FWD:
		case SLOW_FWD:
		case NORMAL_FWD:
		case FAST_FWD:
// Stop
//printf("PlayTransport::handle_transport 3 %d %d\n", engine->command->command, command);
			if(engine->command->command == command && 
				!engine->command->single_frame())
			{
//printf("PlayTransport::handle_transport 4 %d\n", command);
				engine->que->send_command(STOP,
					CHANGE_NONE, 
					0,
					0);
				engine->interrupt_playback(wait_tracking);
			}
			else
// Resume or change direction
			if(engine->command->command != STOP &&
				engine->command->command != COMMAND_NONE &&
				engine->command->command != SINGLE_FRAME_FWD &&
				engine->command->command != SINGLE_FRAME_REWIND)
			{
//printf("PlayTransport::handle_transport 5 %d\n", command);
				engine->que->send_command(STOP,
					CHANGE_NONE, 
					0,
					0);
//printf("PlayTransport::handle_transport 3\n");
				engine->interrupt_playback(wait_tracking);
//printf("PlayTransport::handle_transport 4\n");
				engine->que->send_command(command,
					CHANGE_NONE, 
					get_edl(),
					1,
					1);
//printf("PlayTransport::handle_transport 5\n");
			}
			else
// Start from scratch
			{
//printf("PlayTransport::handle_transport 6 %d\n", command);
				engine->que->send_command(command,
					CHANGE_NONE, 
					get_edl(),
					1,
					0);
			}
			break;

		case STOP:
		case REWIND:
		case GOTO_END:
//printf("PlayTransport 1\n");
			engine->que->send_command(STOP,
				CHANGE_NONE, 
				0,
				0);
//printf("PlayTransport 1\n");
			engine->interrupt_playback(wait_tracking);
//printf("PlayTransport 2\n");
			break;
	}
}

EDL* PlayTransport::get_edl()
{
	return mwindow->edl;
}

int PlayTransport::pause_transport()
{
	if(active_button) active_button->set_mode(PLAY_MODE);
	return 0;
}


int PlayTransport::reset_transport()
{
	fast_reverse->set_mode(PLAY_MODE);
	reverse_play->set_mode(PLAY_MODE);
	forward_play->set_mode(PLAY_MODE);
	frame_reverse_play->set_mode(PLAY_MODE);
	frame_forward_play->set_mode(PLAY_MODE);
	fast_play->set_mode(PLAY_MODE);
	return 0;
}

PTransportButton::PTransportButton(MWindow *mwindow, PlayTransport *transport, int x, int y, VFrame **data)
 : BC_Button(x, y, data)
{
	this->mwindow = mwindow;
	this->transport = transport;
	mode = PLAY_MODE;
}
PTransportButton::~PTransportButton()
{
}

int PTransportButton::set_mode(int mode)
{
	this->mode = mode;
	return 0;
}


RewindButton::RewindButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->rewind_data)
{
	set_tooltip("Rewind");
}
int RewindButton::handle_event()
{
//	unlock_window();
	transport->goto_start();
//	lock_window();
	return 1;
}

FastReverseButton::FastReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->fastrev_data) 
{
	set_tooltip("Fast reverse ( + )");
}
int FastReverseButton::handle_event() 
{
	unlock_window();
	transport->handle_transport(FAST_REWIND);
	lock_window();
	return 1;
}

// Reverse playback normal speed

ReverseButton::ReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->reverse_data) 
{
	set_tooltip("Normal reverse ( 6 )");
}
int ReverseButton::handle_event()
{
	unlock_window();
	transport->handle_transport(NORMAL_REWIND);
	lock_window();
	return 1;
}

// Reverse playback one frame

FrameReverseButton::FrameReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->framerev_data)
{
	set_tooltip("Frame reverse ( 4 )");
}
int FrameReverseButton::handle_event()
{
	unlock_window();
	transport->handle_transport(SINGLE_FRAME_REWIND);
	lock_window();
	return 1;
}

// forward playback normal speed

PlayButton::PlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->forward_data) 
{
	set_tooltip("Normal forward ( 3 )");
}
int PlayButton::handle_event()
{
//printf("PlayButton::handle_event 1\n");
	unlock_window();
	transport->handle_transport(NORMAL_FWD);	
	lock_window();
//printf("PlayButton::handle_event 2\n");
	return 1;
}



// forward playback one frame

FramePlayButton::FramePlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->framefwd_data) 
{
	set_tooltip("Frame forward ( 1 )");
}
int FramePlayButton::handle_event()
{
	unlock_window();
	transport->handle_transport(SINGLE_FRAME_FWD);
	lock_window();
	return 1;
}



FastPlayButton::FastPlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->fastfwd_data) 
{
	set_tooltip("Fast forward ( Enter )");
}
int FastPlayButton::handle_event() 
{
	unlock_window();
	transport->handle_transport(FAST_FWD);
	lock_window();
	return 1;
}

EndButton::EndButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->end_data) 
{
	set_tooltip("Jump to end");
}
int EndButton::handle_event()
{	
//	unlock_window();
	transport->goto_end();
//	lock_window();
	return 1;
}

StopButton::StopButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->stop_data) 
{
	set_tooltip("Stop ( 0 )");
}
int StopButton::handle_event()
{
//printf("StopButton::handle_event 1\n");
	unlock_window();
//printf("StopButton::handle_event 1\n");
	transport->handle_transport(STOP);
//printf("StopButton::handle_event 1\n");
	lock_window();
//printf("StopButton::handle_event 2\n");
	return 1;
}
