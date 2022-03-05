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
	PlayTransport(BC_WindowBase *subwindow, int x, int y);
	~PlayTransport();

	void reposition_buttons(int x, int y);

	virtual void goto_start();
	virtual void goto_end();

// Set the playback engine to control
	void set_engine(PlaybackEngine *engine);

	static int get_transport_width();

	int keypress_event();

// Abstract TransportQue::send_command.
// use_inout - causes the in/out points to determine the beginning and end 
// of playback
	void handle_transport(int command, int use_inout = 0);

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
	virtual ~PTransportButton() {};

	virtual void set_mode(int mode);

	int mode;
	PlayTransport *transport;
};

class RewindButton : public PTransportButton
{
public:
	RewindButton(PlayTransport *transport, int x, int y);

	int handle_event();
};

class FastReverseButton : public PTransportButton
{
public:
	FastReverseButton(PlayTransport *transport, int x, int y);

	int handle_event();
};

class ReverseButton : public PTransportButton
{
public:
	ReverseButton(PlayTransport *transport, int x, int y);

	int handle_event();
};

class FrameReverseButton : public PTransportButton
{
public:
	FrameReverseButton(PlayTransport *transport, int x, int y);

	int handle_event();
};

class PlayButton : public PTransportButton
{
public:
	PlayButton(PlayTransport *transport, int x, int y);

	int handle_event();
};

class FramePlayButton : public PTransportButton
{
public:
	FramePlayButton(PlayTransport *transport, int x, int y);

	int handle_event();
};

class FastPlayButton : public PTransportButton
{
public:
	FastPlayButton(PlayTransport *transport, int x, int y);

	int handle_event();
};

class EndButton : public PTransportButton
{
public:
	EndButton(PlayTransport *transport, int x, int y);

	int handle_event();
};

class StopButton : public PTransportButton
{
public:
	StopButton(PlayTransport *transport, int x, int y);

	int handle_event();
};

#endif
