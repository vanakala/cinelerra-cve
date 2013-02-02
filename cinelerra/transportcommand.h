
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
	~TransportCommand();

	void reset();
// Get the direction based on the command
	int get_direction();
	float get_speed();
	void copy_from(TransportCommand *command);
	TransportCommand& operator=(TransportCommand &command);
// Get the range to play back from the EDL
	void set_playback_range(EDL *edl = 0, int use_inout = 0);

// Adjust playback range with in/out points for rendering
	void playback_range_adjust_inout();
// Set playback range to in/out points for rendering
	void playback_range_inout();
// Set playback range to whole project for rendering
	void playback_range_project();

	int single_frame();
	EDL* get_edl();
	void delete_edl();
	void new_edl();
	const char* commandstr(int cmd = -1);
	void dump_command();

	int command;
	int change_type;
// lowest numbered second in playback range
	ptstime start_position;
// highest numbered second in playback range
	ptstime end_position;
	int infinite;
// Position used when starting playback
	ptstime playbackstart;
// Send output to device
	int realtime;
private:
// Copied to render engines
	EDL *edl;
};

#endif
