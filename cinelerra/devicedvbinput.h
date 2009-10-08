
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

#ifndef DEVICEDVBINPUT_H
#define DEVICEDVBINPUT_H

#include "channel.inc"
#include "condition.inc"
#include "garbage.h"
#include "mwindow.inc"
#include "recordconfig.inc"
#include <stdint.h>
#include "thread.h"


// This is the common interface for the DVB recording devices.
// It handles the network connection to the DVB tuner.


class DeviceDVBInput : public GarbageObject
{
public:
	DeviceDVBInput(MWindow *mwindow, 
		VideoInConfig *config);
	~DeviceDVBInput();

	void reset();

	int close();
	int try_tuner();
	int reopen_tuner();
	int get_signal_strength(int channel, int tuner_id);

// Provides multiplexed data.  Each caller needs its own demuxer.
// Returns the number of bytes read.
// Initial opening and channel changes are handled in here.
	int read_stream(unsigned char *data, int size);

// Access the socket
	int read_tuner(unsigned char *data, int size);
	int write_tuner(unsigned char *data, int size);
	int write_int64(int64_t value);
	int64_t read_int64(int *error);

// Acquire the global DVB input thread
// by either referencing MWindow::dvb_input or creating a new one.
// Then try to connect to the tuner and begin streaming immediately.
// Return 0 if failure.
	static DeviceDVBInput* get_input_thread(MWindow *mwindow);
// Release the global DVB input thread by either deleting it or decreasing
// the reference counter.
	static void put_input_thread(MWindow *mwindow);


	VideoInConfig *config;
	int tuner_fd;
	int want_signal;
	Condition *signal_lock;
	int lock_result;
	int signal_result;
	int channel_changed;
	MWindow *mwindow;
	Channel *channel;
};




#endif

