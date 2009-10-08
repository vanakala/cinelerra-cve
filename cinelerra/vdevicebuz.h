
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

#ifndef VDEVICEBUZ_H
#define VDEVICEBUZ_H

#include "buz.h"
#include "channel.inc"
#include "condition.inc"
#include "guicast.h"
#include "libmjpeg.h"
#include "mutex.inc"
#include "thread.h"
#include "vdevicebase.h"
#include "vdevicebuz.inc"
#include "vframe.inc"


#define INPUT_BUFFER_SIZE 0x40000

// Let's get real.  The Buz driver doesn't work.  If the buffers overflow
// for enough time it locks up and can't be recovered except by a
// SIGINT and restart.  We need to cascade the buffer reading in another
// ring buffer thread, have it read continuously, and cancel it if it 
// dies.  How about if we do this in SCHED_RR and wait for it to die before 
// implementing cancellation?


class VDeviceBUZInput : public Thread
{
public:
	VDeviceBUZInput(VDeviceBUZ *device);
	~VDeviceBUZInput();
	void start();
	void run();
	void get_buffer(char **ptr, int *size);
	void put_buffer();
	void increment_counter(int *counter);
	void decrement_counter(int *counter);
	VDeviceBUZ *device;

	char **buffer;
	int *buffer_size;
	int total_buffers;
	int current_inbuffer;
	int current_outbuffer;
	Condition *output_lock;
	Mutex *buffer_lock;
	int done;
};


class VDeviceBUZ : public VDeviceBase
{
public:
	VDeviceBUZ(VideoDevice *device);
	~VDeviceBUZ();

	friend class VDeviceBUZInput;

	int open_input();
	int open_output();
	int close_all();
	int read_buffer(VFrame *frame);
	int write_buffer(VFrame *frames, EDL *edl);
	int reset_parameters();
	ArrayList<int>* get_render_strategies();
	int set_channel(Channel *channel);
	int get_norm(int norm);
	static void get_inputs(ArrayList<Channel*> *input_sources);
	int set_picture(PictureConfig *picture);
	int get_best_colormodel(int colormodel);
	void create_channeldb(ArrayList<Channel*> *channeldb);
	void new_output_buffer(VFrame *output, int colormodel);


private:
	int open_input_core(Channel *channel);
	int close_input_core();
	int open_output_core(Channel *channel);
	int close_output_core();

	int jvideo_fd;
	char *input_buffer, *frame_buffer, *output_buffer;
	long frame_size, frame_allocated;
	int input_error;
//	quicktime_mjpeg_hdr jpeg_header;
	long last_frame_no;
	ArrayList<int> render_strategies;
// Temporary frame for compressing output data
	VFrame *temp_frame;
// Frame given to user to acquire data
	VFrame *user_frame;
	mjpeg_t *mjpeg;
	Mutex *tuner_lock;
	VDeviceBUZInput *input_thread;

	struct buz_params bparm;
	struct buz_requestbuffers breq;
// Can't CSYNC the first loop
	int total_loops;
// Number of output frame to load
	int output_number;

	int brightness;
	int hue;
	int color;
	int contrast;
	int whiteness;
};

#endif
