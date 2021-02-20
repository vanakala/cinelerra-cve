// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bcwindowbase.h"
#include "edl.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "theme.h"
#include "transportcommand.h"
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
	this->engine = 0;

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

int PlayTransport::get_transport_width(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("stop")[0]->get_w() * 7 +
		mwindow->theme->get_image_set("rewind")[0]->get_w() * 2;
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

int PlayTransport::keypress_event()
{
	if(subwindow->shift_down())
	{
		switch(subwindow->get_keypress())
		{
		case END:
			goto_end();
			break;
		case HOME:
			goto_start();
			break;
		default:
			return 0;
		}
		return 1;
	}

// Set playback range to in/out points if CTRL is down
	int use_inout = 0;
	if(subwindow->ctrl_down())
	{
		use_inout = 1;
	}

	switch(subwindow->get_keypress())
	{
	case KPPLUS:
		handle_transport(FAST_REWIND, use_inout);
		break;
	case KP6:
		handle_transport(NORMAL_REWIND, use_inout);
		break;
	case KP5:
		handle_transport(SLOW_REWIND, use_inout);
		break;
	case KP4:
		handle_transport(SINGLE_FRAME_REWIND, use_inout);
		break;
	case KP1:
		handle_transport(SINGLE_FRAME_FWD, use_inout);
		break;
	case KP2:
		handle_transport(SLOW_FWD, use_inout);
		break;
	case KP3:
		handle_transport(NORMAL_FWD, use_inout);
		break;
	case KPENTER:
		handle_transport(FAST_FWD, use_inout);
		break;
	case KPINS:
		handle_transport(STOP, use_inout);
		break;
	case ' ':
		handle_transport(NORMAL_FWD, use_inout);
		break;
	case 'k':
		handle_transport(STOP, use_inout);
		break;
	case END:
		goto_end();
		break;
	case HOME:
		goto_start();
		break;
	default:
		return 0;
	}

	return 1;
}

void PlayTransport::goto_start()
{
	handle_transport(REWIND);
}

void PlayTransport::goto_end()
{
	handle_transport(GOTO_END);
}


void PlayTransport::handle_transport(int command, int use_inout)
{
// Stop requires transferring the output buffer to a refresh buffer.
	int do_stop = 0;
	int prev_command = engine->command->command;
	int prev_single_frame = engine->command->single_frame();
	int options = 0;

	if(use_inout)
		options |= CMDOPT_USEINOUT;

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
			engine->send_command(STOP);
			engine->send_command(command | options);
		}
		else
// Start from scratch
		{
			engine->send_command(command | options);
		}
		break;

// Commands that stop
	case STOP:
	case REWIND:
	case GOTO_END:
		do_stop = 1;
		break;
	}

	if(do_stop)
		engine->send_command(STOP);
}


PTransportButton::PTransportButton(PlayTransport *transport, int x, int y, VFrame **data)
 : BC_Button(x, y, data)
{
	this->transport = transport;
	mode = PLAY_MODE;
}

PTransportButton::~PTransportButton()
{
}

void PTransportButton::set_mode(int mode)
{
	this->mode = mode;
}


RewindButton::RewindButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(transport, x, y, mwindow->theme->get_image_set("rewind"))
{
	set_tooltip(_("Rewind ( Home )"));
}

int RewindButton::handle_event()
{
	transport->goto_start();
	return 1;
}

FastReverseButton::FastReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(transport, x, y, mwindow->theme->get_image_set("fastrev")) 
{
	set_tooltip(_("Fast reverse ( + )"));
}

int FastReverseButton::handle_event() 
{
	transport->handle_transport(FAST_REWIND, ctrl_down());
	return 1;
}

// Reverse playback normal speed

ReverseButton::ReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(transport, x, y, mwindow->theme->get_image_set("reverse")) 
{
	set_tooltip(_("Normal reverse ( 6 )"));
}

int ReverseButton::handle_event()
{
	transport->handle_transport(NORMAL_REWIND, ctrl_down());
	return 1;
}

// Reverse playback one frame

FrameReverseButton::FrameReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(transport, x, y, mwindow->theme->get_image_set("framerev"))
{
	set_tooltip(_("Frame reverse ( 4 )"));
}

int FrameReverseButton::handle_event()
{
	transport->handle_transport(SINGLE_FRAME_REWIND, ctrl_down());
	return 1;
}

// forward playback normal speed

PlayButton::PlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(transport, x, y, mwindow->theme->get_image_set("play")) 
{
	set_tooltip(_("Normal forward ( 3 )"));
}

int PlayButton::handle_event()
{
	transport->handle_transport(NORMAL_FWD, ctrl_down());
	return 1;
}

// forward playback one frame

FramePlayButton::FramePlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(transport, x, y, mwindow->theme->get_image_set("framefwd")) 
{
	set_tooltip(_("Frame forward ( 1 )"));
}

int FramePlayButton::handle_event()
{
	transport->handle_transport(SINGLE_FRAME_FWD, ctrl_down());
	return 1;
}


FastPlayButton::FastPlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(transport, x, y, mwindow->theme->get_image_set("fastfwd")) 
{
	set_tooltip(_("Fast forward ( Enter )"));
}

int FastPlayButton::handle_event() 
{
	transport->handle_transport(FAST_FWD, ctrl_down());
	return 1;
}

EndButton::EndButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(transport, x, y, mwindow->theme->get_image_set("end")) 
{
	set_tooltip(_("Jump to end ( End )"));
}

int EndButton::handle_event()
{
	transport->goto_end();
	return 1;
}

StopButton::StopButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(transport, x, y, mwindow->theme->get_image_set("stop")) 
{
	set_tooltip(_("Stop ( 0 )"));
}

int StopButton::handle_event()
{
	transport->handle_transport(STOP);
	return 1;
}
