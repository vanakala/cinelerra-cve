// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "playbackconfig.h"
#include "playbackengine.h"
#include "vdevicex11.h"
#include "videodevice.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

VideoDevice::VideoDevice()
{
	out_config = 0;
	out_w = out_h = 0;
	writing = 0;
	output_base = 0;
	interrupt = 0;
	adevice = 0;
	single_frame = 0;
}

VideoDevice::~VideoDevice()
{
	close_all();
}


VDeviceX11 *VideoDevice::get_output_base()
{
	return output_base;
}

const char* VideoDevice::drivertostr(int driver)
{
	switch(driver)
	{
	case PLAYBACK_X11:
		return PLAYBACK_X11_TITLE;
	case PLAYBACK_X11_XV:
		return PLAYBACK_X11_XV_TITLE;
	case PLAYBACK_X11_GL:
		return PLAYBACK_X11_GL_TITLE;
	case VIDEO4LINUX:
		return VIDEO4LINUX_TITLE;
	case VIDEO4LINUX2:
		return VIDEO4LINUX2_TITLE;
	case VIDEO4LINUX2JPEG:
		return VIDEO4LINUX2JPEG_TITLE;
	case SCREENCAPTURE:
		return SCREENCAPTURE_TITLE;
	}
	return "";
}

void VideoDevice::close_all()
{
	if(writing)
	{
		if(output_base)
		{
			delete output_base;
			output_base = 0;
		}
		writing = 0;
	}
}

void VideoDevice::set_adevice(AudioDevice *adevice)
{
	this->adevice = adevice;
}

// ================================= OUTPUT ==========================================


int VideoDevice::open_output(VideoOutConfig *config,
			int out_w,
			int out_h,
			int colormodel,
			Canvas *output,
			int single_frame)
{
	writing = 1;
	out_config = config;
	this->out_w = out_w;
	this->out_h = out_h;
	this->single_frame = single_frame;
	interrupt = 0;

	switch(config->driver)
	{
	case PLAYBACK_X11:
	case PLAYBACK_X11_XV:
	case PLAYBACK_X11_GL:
		output_base = new VDeviceX11(this, output);
		break;
	}

	if(output_base->open_output(colormodel))
	{
		delete output_base;
		output_base = 0;
	}

	if(output_base) 
		return 0;
	else
		return 1;
}

void VideoDevice::interrupt_playback()
{
	interrupt = 1;
}

int VideoDevice::write_buffer(VFrame *output)
{
	if(output_base)
		return output_base->write_buffer(output);
	return 1;
}

int VideoDevice::output_visible()
{
	if(output_base) return output_base->output_visible();
	return 0;
}

int VideoDevice::set_cloexec_flag(int desc, int value)
{
	int oldflags = fcntl(desc, F_GETFD, 0);
	if(oldflags < 0) return oldflags;
	if(value != 0) 
		oldflags |= FD_CLOEXEC;
	else
		oldflags &= ~FD_CLOEXEC;
	return fcntl(desc, F_SETFD, oldflags);
}
