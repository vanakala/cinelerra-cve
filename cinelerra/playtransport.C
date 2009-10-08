
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

#include "edl.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "theme.h"
#include "transportque.h"
#include "vframe.h"
#include "localsession.h"



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
	return mwindow->theme->get_image_set("stop")[0]->get_w() * 7 +
		mwindow->theme->get_image_set("rewind")[0]->get_w() * 2;
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
	int result = 0;
	if(subwindow->shift_down())
	{
		switch(subwindow->get_keypress())
		{
			case END:
				subwindow->lock_window("PlayTransport::keypress_event 1");
				goto_end();                                   
				result = 1; 
				subwindow->unlock_window();
				break;
			case HOME:
				subwindow->lock_window("PlayTransport::keypress_event 2");
		    	goto_start();                                 
				result = 1; 
				subwindow->unlock_window();
				break;
		}
		return result;
	}

// Set playback range to in/out points if CTRL is down
	int use_inout = 0;
	if(subwindow->ctrl_down())
	{
		use_inout = 1;
	}
	subwindow->unlock_window();


	switch(subwindow->get_keypress())
	{
		case KPPLUS:        handle_transport(FAST_REWIND, 0, use_inout);                result = 1; break;
		case KP6:           handle_transport(NORMAL_REWIND, 0, use_inout);              result = 1; break;
		case KP5:           handle_transport(SLOW_REWIND, 0, use_inout);                result = 1; break;
		case KP4:           handle_transport(SINGLE_FRAME_REWIND, 0, use_inout);        result = 1; break;
		case KP1:           handle_transport(SINGLE_FRAME_FWD, 0, use_inout);   		  result = 1; break;
		case KP2:           handle_transport(SLOW_FWD, 0, use_inout);           		  result = 1; break;
		case KP3:           handle_transport(NORMAL_FWD, 0, use_inout);         		  result = 1; break;
		case KPENTER:       handle_transport(FAST_FWD, 0, use_inout);           		  result = 1; break;
		case KPINS:         handle_transport(STOP, 0, use_inout);                       result = 1; break;
		case ' ':           handle_transport(NORMAL_FWD, 0, use_inout);                 result = 1; break;
		case 'k':           handle_transport(STOP, 0, use_inout);   					  result = 1; break;
		case END:
			subwindow->lock_window("PlayTransport::keypress_event 3");
			goto_end();                                   
			result = 1; 
			subwindow->unlock_window();
			break;
		case HOME:
			subwindow->lock_window("PlayTransport::keypress_event 4");
		    goto_start();                                 
			result = 1; 
			subwindow->unlock_window();
			break;
	}

	subwindow->lock_window("PlayTransport::keypress_event 5");

	return result;
}


void PlayTransport::goto_start()
{
	handle_transport(REWIND, 1, 0);
}

void PlayTransport::goto_end()
{
	handle_transport(GOTO_END, 1, 0);
}



void PlayTransport::handle_transport(int command, 
	int wait_tracking,
	int use_inout,
	int update_refresh)
{
	if(!get_edl()) return;

// Stop requires transferring the output buffer to a refresh buffer.
	int do_stop = 0;
//printf("PlayTransport::handle_transport 1 %d\n", command);
	int prev_command = engine->command->command;
	int prev_direction = engine->command->get_direction();
	int prev_single_frame = engine->command->single_frame();

// Dispatch command
	switch(command)
	{
// Commands that play back
		case FAST_REWIND:
		case NORMAL_REWIND:
		case SLOW_REWIND:
		case SINGLE_FRAME_REWIND:
		case SINGLE_FRAME_FWD:
		case SLOW_FWD:
		case NORMAL_FWD:
		case FAST_FWD:
// Same direction pressed twice.  Stop
			if(prev_command == command && 
				!prev_single_frame)
			{
				do_stop = 1;
			}
			else
// Resume or change direction
			if(prev_command != STOP &&
				prev_command != COMMAND_NONE &&
				prev_command != SINGLE_FRAME_FWD &&
				prev_command != SINGLE_FRAME_REWIND)
			{
				engine->que->send_command(STOP,
					CHANGE_NONE, 
					0,
					0,
					0,
					0);
				engine->interrupt_playback(wait_tracking);
				engine->que->send_command(command,
					CHANGE_NONE, 
					get_edl(),
					1,
					1,
					use_inout);
			}
			else
// Start from scratch
			{
				engine->que->send_command(command,
					CHANGE_NONE, 
					get_edl(),
					1,
					0,
					use_inout);
			}
			break;

// Commands that stop
		case STOP:
			do_stop = 1;
			break;

		case REWIND:
		case GOTO_END:
			engine->que->send_command(STOP,
				CHANGE_NONE, 
				0,
				0,
				0,
				0);
			engine->interrupt_playback(wait_tracking);
			break;
	}

	if(do_stop)
	{
		engine->que->send_command(STOP,
			CHANGE_NONE, 
			0,
			0,
			0,
			0);
		engine->interrupt_playback(wait_tracking);
// This is necessary to get an OpenGL output buffer
// printf("PlayTransport::handle_transport 2 update_refresh=%d prev_command=%d prev_direction=%d\n", 
// update_refresh, prev_command, prev_direction);
		if(!prev_single_frame && 
			update_refresh &&
			prev_command != STOP &&
			prev_command != COMMAND_NONE)
		{
			engine->que->send_command(
				(prev_direction == PLAY_FORWARD) ? SINGLE_FRAME_REWIND : SINGLE_FRAME_FWD,
				CHANGE_NONE, 
				get_edl(),
				1,
				0,
				0);
		}
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
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("rewind"))
{
	set_tooltip(_("Rewind ( Home )"));
}
int RewindButton::handle_event()
{
//	unlock_window();
	transport->goto_start();
//	lock_window();
	return 1;
}

FastReverseButton::FastReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("fastrev")) 
{
	set_tooltip(_("Fast reverse ( + )"));
}
int FastReverseButton::handle_event() 
{
	unlock_window();
	transport->handle_transport(FAST_REWIND, 0, ctrl_down());
	lock_window("FastReverseButton::handle_event");
	return 1;
}

// Reverse playback normal speed

ReverseButton::ReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("reverse")) 
{
	set_tooltip(_("Normal reverse ( 6 )"));
}
int ReverseButton::handle_event()
{
	unlock_window();
	transport->handle_transport(NORMAL_REWIND, 0, ctrl_down());
	lock_window("ReverseButton::handle_event");
	return 1;
}

// Reverse playback one frame

FrameReverseButton::FrameReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("framerev"))
{
	set_tooltip(_("Frame reverse ( 4 )"));
}
int FrameReverseButton::handle_event()
{
	unlock_window();
	transport->handle_transport(SINGLE_FRAME_REWIND, 0, ctrl_down());
	lock_window("FrameReverseButton::handle_event");
	return 1;
}

// forward playback normal speed

PlayButton::PlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("play")) 
{
	set_tooltip(_("Normal forward ( 3 )"));
}
int PlayButton::handle_event()
{
	unlock_window();
	transport->handle_transport(NORMAL_FWD, 0, ctrl_down());
	lock_window("PlayButton::handle_event");
	return 1;
}



// forward playback one frame

FramePlayButton::FramePlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("framefwd")) 
{
	set_tooltip(_("Frame forward ( 1 )"));
}
int FramePlayButton::handle_event()
{
	unlock_window();
	transport->handle_transport(SINGLE_FRAME_FWD, 0, ctrl_down());
	lock_window("FramePlayButton::handle_event");
	return 1;
}



FastPlayButton::FastPlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("fastfwd")) 
{
	set_tooltip(_("Fast forward ( Enter )"));
}
int FastPlayButton::handle_event() 
{
	unlock_window();
	transport->handle_transport(FAST_FWD, 0, ctrl_down());
	lock_window("FastPlayButton::handle_event");
	return 1;
}

EndButton::EndButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("end")) 
{
	set_tooltip(_("Jump to end ( End )"));
}
int EndButton::handle_event()
{	
//	unlock_window();
	transport->goto_end();
//	lock_window();
	return 1;
}

StopButton::StopButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("stop")) 
{
	set_tooltip(_("Stop ( 0 )"));
}
int StopButton::handle_event()
{
	unlock_window();
	transport->handle_transport(STOP, 0, 0);
	lock_window("StopButton::handle_event");
	return 1;
}
