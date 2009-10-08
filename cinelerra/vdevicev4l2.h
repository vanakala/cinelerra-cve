
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

#ifndef VDEVICEV4L2_H
#define VDEVICEV4L2_H


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_VIDEO4LINUX2

#include "vdevicebase.h"
#include <linux/types.h>
#include <linux/videodev2.h>
#include "videodevice.inc"
#include "vdevicev4l2.inc"


// Short delay is necessary whenn capturing a lousy source.
//#define BUFFER_TIMEOUT 250000

// Long delay is necessary to avoid losing synchronization due to spurrious
// resets.
#define BUFFER_TIMEOUT 10000000


// Isolate the application from the grabbing operation.
// Used by VDeviceV4L2 and VDeviceV4L2JPEG
// Color_model determines whether it uses compressed mode or not.
class VDeviceV4L2Thread : public Thread
{
public:
	VDeviceV4L2Thread(VideoDevice *device, int color_model);
	~VDeviceV4L2Thread();

	void start();
	void run();
	VFrame* get_buffer(int *timed_out);
	void put_buffer();
	void allocate_buffers(int number);

	Mutex *buffer_lock;
// Some of the drivers in 2.6.7 can't handle simultaneous QBUF and DQBUF calls.
	Mutex *ioctl_lock;
	Condition *video_lock;
	VideoDevice *device;
	VFrame **device_buffers;
	int *buffer_valid;
	int total_valid;
	int total_buffers;
	int current_inbuffer;
	int current_outbuffer;
// Don't block if first frame not recieved yet.
// This frees up the GUI during driver initialization.
	int first_frame;
	int done;
	int input_fd;
// COMPRESSED or another color model the device should use.
	int color_model;
	VDeviceV4L2Put *put_thread;
};


// Another thread which puts back buffers asynchronously of the buffer
// grabber.  Because 2.6.7 drivers block the buffer enqueuer.
class VDeviceV4L2Put : public Thread
{
public:
	VDeviceV4L2Put(VDeviceV4L2Thread *thread);
	~VDeviceV4L2Put();
	void run();
// Release buffer for capturing.
	void put_buffer(int number);
	VDeviceV4L2Thread *thread;
// List of buffers to requeue
	Mutex *lock;
	Condition *more_buffers;
	int *putbuffers;
	int total;
	int done;
};



class VDeviceV4L2 : public VDeviceBase
{
public:
	VDeviceV4L2(VideoDevice *device);
	~VDeviceV4L2();

	int close_all();
	int open_input();
	int initialize();
	int get_best_colormodel(Asset *asset);
	int read_buffer(VFrame *frame);
	int has_signal();
	static int cmodel_to_device(int color_model);
	static int get_sources(VideoDevice *device,
		char *path);

	VDeviceV4L2Thread *thread;
};

#endif
#endif
