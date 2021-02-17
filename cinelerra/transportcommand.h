// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef TRANSPORTCOMMAND_H
#define TRANSPORTCOMMAND_H

#include "canvas.inc"
#include "condition.inc"
#include "edl.inc"
#include "preferences.inc"
#include "transportcommand.inc"

class TransportCommand
{
public:
	TransportCommand();

	void reset();
// Get the direction based on the command
	int get_direction();
	double get_speed();
	void copy_from(TransportCommand *command);
	TransportCommand& operator=(TransportCommand &command);
// Get the range to play back from the EDL
	void set_playback_range(int use_inout = 0);

// Set playback range to in/out points for rendering
	void playback_range_inout();
// Set playback range to whole project for rendering
	void playback_range_project();

	int single_frame();
	EDL* get_edl();
	void set_edl(EDL *edl);
	const char* commandstr(int cmd = -1);
	void dump(int indent = 0);

	int command;
	int change_type;
// lowest numbered second in playback range
	ptstime start_position;
// highest numbered second in playback range
	ptstime end_position;
// Position used when starting playback
	ptstime playbackstart;
// Send output to device
	int realtime;
// Loop playback
	int loop_playback;
private:
// Pointer to current edl
	EDL *edl;
};

#endif
