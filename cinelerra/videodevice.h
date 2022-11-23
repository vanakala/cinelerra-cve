// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VIDEODEVICE_H
#define VIDEODEVICE_H

#include "audiodevice.inc"
#include "canvas.inc"
#include "videodevice.inc"
#include "vdevicex11.inc"
#include "vframe.inc"

class VideoDevice
{
public:
// MWindow is required where picture settings are used, to get the defaults.
	VideoDevice();
	~VideoDevice();

	void close_all();

// Used for calling OpenGL functions
	VDeviceX11 *get_output_base();

	static const char* drivertostr(int driver);

// Specify the audio device opened concurrently with this video device
	void set_adevice(AudioDevice *adevice);

// Used by all devices to cause fd's to be not copied in fork operations.
	int set_cloexec_flag(int desc, int value);

// ================================== Playback
	int open_output(VideoOutConfig *config,
		int out_w,
		int out_h,
		int colormodel,
		Canvas *output,
		int single_frame);
	void set_cpus(int cpus);
	void interrupt_playback();
	int wait_for_startup();
	int wait_for_completion();
	int output_visible();     // Whether the output is visible or not.
	void stop_playback();

// absolute frame of last frame in buffer.
	int write_buffer(VFrame *output);

// Flag when output is interrupted
	int interrupt;

// Audio device to share data with
	AudioDevice *adevice;

// size of output frame being fed to device during playback
	int out_w, out_h;
// modes
	int writing;

// Single frame mode for playback
	int single_frame;

	VDeviceX11 *output_base;
	VideoOutConfig *out_config;
};
#endif
