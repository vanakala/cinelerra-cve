
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

#ifndef VIDEODEVICE_H
#define VIDEODEVICE_H

#include "asset.inc"
#include "assets.inc"
#include "audiodevice.inc"
#include "bccapture.inc"
#include "bctimer.h"
#include "canvas.inc"
#include "edl.inc"
#include "mwindow.inc"
#include "mutex.inc"
#include "preferences.inc"
#include "thread.h"
#include "vdevicebase.inc"
#include "vdevicex11.inc"


class VideoDevice
{
public:
// MWindow is required where picture settings are used, to get the defaults.
	VideoDevice(MWindow *mwindow = 0);
	~VideoDevice();

	void close_all();

// Used for calling OpenGL functions
	VDeviceBase* get_output_base();

	static const char* drivertostr(int driver);

// Specify the audio device opened concurrently with this video device
	void set_adevice(AudioDevice *adevice);

	BC_Bitmap* get_bitmap();

// Used by all devices to cause fd's to be not copied in fork operations.
	int set_cloexec_flag(int desc, int value);

// ================================== Playback
	int open_output(VideoOutConfig *config,
		int out_w,
		int out_h,
		Canvas *output,
		int single_frame);
	void set_cpus(int cpus);
	void interrupt_playback();
// Get output buffer for playback using colormodel.
// colormodel argument should be as close to best_colormodel as possible
	VFrame *new_output_buffer(int colormodel);
	int wait_for_startup();
	int wait_for_completion();
	int output_visible();     // Whether the output is visible or not.
	void stop_playback();

// absolute frame of last frame in buffer.
// The EDL parameter is passed to Canvas and can be 0.
	int write_buffer(VFrame *output, EDL *edl);

// Flag when output is interrupted
	int interrupt;

// Audio device to share data with
	AudioDevice *adevice;

// frame rate
	float irate;
// timer for displaying frames in the current buffer
	Timer buffer_timer;
// timer for getting frame rate
	Timer rate_timer;
// size of output frame being fed to device during playback
	int out_w, out_h;
// modes
	int writing;
// time from start of previous frame to start of next frame in ms
	int frame_delay;

// Single frame mode for playback
	int single_frame;

	VDeviceBase *output_base;
	VideoOutConfig *out_config;
	MWindow *mwindow;
};
#endif
