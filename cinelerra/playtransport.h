
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

#ifndef PLAYTRANSPORT_H
#define PLAYTRANSPORT_H

#include "edl.inc"
#include "guicast.h"
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

	int create_objects();
	void reposition_buttons(int x, int y);

	virtual void goto_start();
	virtual void goto_end();
// Set a slider to control if there is one
	void set_slider(BC_Slider *slider);
// Set the playback engine to control
	void set_engine(PlaybackEngine *engine);
	static int get_transport_width(MWindow *mwindow);
	int flip_vertical(int vertical, int &x, int &y);
	int keypress_event();
// Abstract TransportQue::send_command.
// wait_tracking - causes stop to wail until the final tracking position 
// is updated before returning
// use_inout - causes the in/out points to determine the beginning and end 
// of playback
// update_refresh - causes a frame advance to be issued after STOP to update
// the refresh frame.
	void handle_transport(int command, 
		int wait_tracking = 0, 
		int use_inout = 0,
		int update_refresh = 1);

	int pause_transport();
	int reset_transport();
	int get_w();
// Get the EDL to play back with default to mwindow->edl
	virtual EDL* get_edl();

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
	BC_Slider *slider;
	PlaybackEngine *engine;
	int status;
	int x, y;
};

// Should be toggle

class PTransportButton : public BC_Button
{
public:
	PTransportButton(MWindow *mwindow, PlayTransport *transport, int x, int y, VFrame **data);
	virtual ~PTransportButton();
	virtual int set_mode(int mode);

	int mode;
	MWindow *mwindow;
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
