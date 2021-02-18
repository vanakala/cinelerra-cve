// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLAYTRANSPORT_H
#define PLAYTRANSPORT_H

#include "bcbutton.h"
#include "edl.inc"
#include "mwindow.inc"
#include "playbackengine.inc"
#include "playtransport.inc"

#define PLAY_MODE 0
#define PAUSE_MODE 1
#define PAUSEDOWN_MODE 2

class PlayTransport
{
public:
	PlayTransport(MWindow *mwindow, BC_WindowBase *subwindow, int x, int y);
	~PlayTransport();

	void reposition_buttons(int x, int y);

	virtual void goto_start();
	virtual void goto_end();

// Set the playback engine to control
	void set_engine(PlaybackEngine *engine);

	static int get_transport_width(MWindow *mwindow);

	int keypress_event();

// Abstract TransportQue::send_command.
// wait_tracking - causes stop to wail until the final tracking position 
// is updated before returning
// use_inout - causes the in/out points to determine the beginning and end 
// of playback
	void handle_transport(int command, 
		int wait_tracking = 0, 
		int use_inout = 0);

	int get_w();

// playback parameters
	int reverse;
	float speed;

	PTransportButton *active_button;
	PlayButton *forward_play;
	FramePlayButton *frame_forward_play;
	ReverseButton *reverse_play;
	FrameReverseButton *frame_reverse_play;
	FastReverseButton *fast_reverse;
	FastPlayButton *fast_play;
	RewindButton *rewind_button;
	StopButton *stop_button;
	EndButton *end_button;
	MWindow *mwindow;
	BC_WindowBase *subwindow;
	PlaybackEngine *engine;
	int status;
	int x, y;
};

// Should be toggle

class PTransportButton : public BC_Button
{
public:
	PTransportButton(PlayTransport *transport, int x, int y, VFrame **data);
	virtual ~PTransportButton();

	virtual void set_mode(int mode);

	int mode;
	PlayTransport *transport;
};

class RewindButton : public PTransportButton
{
public:
	RewindButton(MWindow *mwindow, PlayTransport *transport, int x, int y);

	int handle_event();
};

class FastReverseButton : public PTransportButton
{
public:
	FastReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y);

	int handle_event();
};

class ReverseButton : public PTransportButton
{
public:
	ReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y);

	int handle_event();
};

class FrameReverseButton : public PTransportButton
{
public:
	FrameReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y);

	int handle_event();
};

class PlayButton : public PTransportButton
{
public:
	PlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y);

	int handle_event();
};

class FramePlayButton : public PTransportButton
{
public:
	FramePlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y);

	int handle_event();
};

class FastPlayButton : public PTransportButton
{
public:
	FastPlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y);

	int handle_event();
};

class EndButton : public PTransportButton
{
public:
	EndButton(MWindow *mwindow, PlayTransport *transport, int x, int y);

	int handle_event();
};

class StopButton : public PTransportButton
{
public:
	StopButton(MWindow *mwindow, PlayTransport *transport, int x, int y);

	int handle_event();
};

#endif
