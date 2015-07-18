
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

#include "asset.h"
#include "assets.h"
#include "bccapture.h"
#include "bcsignals.h"
#include "file.inc"
#include "mutex.h"
#include "mwindow.h"
#include "picture.h"
#include "playbackconfig.h"
#include "playbackengine.h"
#include "preferences.h"
#include "quicktime.h"
#include "recordconfig.h"
#include "vdevicex11.h"
#include "videodevice.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

KeepaliveThread::KeepaliveThread(VideoDevice *device)
 : Thread()
{
	still_alive = 1;
	failed = 0;
	interrupted = 0;
	set_synchronous(1);
	this->device = device;
	capturing = 0;
	startup_lock = new Mutex("KeepaliveThread::startup_lock");
}

KeepaliveThread::~KeepaliveThread()
{
	delete startup_lock;
}

int KeepaliveThread::start_keepalive()
{
	startup_lock->lock("KeepaliveThread::start_keepalive 1");
	start();
	startup_lock->lock("KeepaliveThread::start_keepalive 2");
	startup_lock->unlock();
}

void KeepaliveThread::run()
{
	startup_lock->unlock();
	while(!interrupted)
	{
		still_alive = 0;
// Give the capture a moment
// Should fix the delay in case users want slower frame rates.
		timer.delay((KEEPALIVE_DELAY * 1000));

// See if a capture happened
		if(still_alive == 0 && capturing)
		{
			failed++;
		}
		else
			failed = 0;
	}
}

int KeepaliveThread::reset_keepalive()
{
	still_alive = 1;
}

int KeepaliveThread::get_failed()
{
	if(failed) return 1; 
	else return 0;
}

int KeepaliveThread::stop()
{
	interrupted = 1;

// Force an immediate exit even if capture_frame worked.
	Thread::end();
	Thread::join();
}


VideoDevice::VideoDevice(MWindow *mwindow)
{
	this->mwindow = mwindow;
	in_config = new VideoInConfig;
	out_config = 0;
	picture = new PictureConfig(mwindow ? mwindow->defaults : 0);
	picture_lock = new Mutex("VideoDevice::picture_lock");
	irate = 0;
	out_w = out_h = 0;
	reading = 0;
	writing = 0;
	is_recording = 0;
	input_x = 0;
	input_y = 0;
	input_z = 1;
	frame_resized = 0;
	capturing = 0;
	keepalive = 0;
	input_base = 0;
	output_base = 0;
	output_format = 0;
	interrupt = 0;
	adevice = 0;
	quality = 80;
	cpus = 1;
	single_frame = 0;
	picture_changed = 0;
}

VideoDevice::~VideoDevice()
{
	close_all();
	delete in_config;
	delete picture;
	delete picture_lock;
}

void VideoDevice::open_input(VideoInConfig *config, 
	int input_x, 
	int input_y, 
	float input_z,
	double frame_rate)
{
	int result = 0;

	*this->in_config = *config;

	reading = 1;
	this->input_z = -1;   // Force initialization.
	this->frame_rate = frame_rate;

	switch(in_config->driver)
	{
#ifdef HAVE_VIDEO4LINUX
	case VIDEO4LINUX:
		keepalive = new KeepaliveThread(this);
		keepalive->start_keepalive();
		new_device_base();
		result = input_base->open_input();
		break;
#endif
#ifdef HAVE_VIDEO4LINUX2
	case VIDEO4LINUX2:
		new_device_base();
		result = input_base->open_input();
		break;
	case VIDEO4LINUX2JPEG:
		new_device_base();
		result = input_base->open_input();
		break;
#endif

	case SCREENCAPTURE:
		this->input_x = input_x;
		this->input_y = input_y;
		new_device_base();
		result = input_base->open_input();
		break;

	case CAPTURE_DVB:
		new_device_base();
		result = input_base->open_input();
		break;
	}

	if(!result) capturing = 1;
}

VDeviceBase* VideoDevice::new_device_base()
{
	switch(in_config->driver)
	{
	case SCREENCAPTURE:
		return input_base = new VDeviceX11(this, 0);
	}
	return 0;
}

VDeviceBase* VideoDevice::get_output_base()
{
	return output_base;
}

int VideoDevice::is_compressed(int driver, int use_file, int use_fixed)
{
// FileMOV needs to have write_frames called so the start codes get scanned.
	return ((driver == VIDEO4LINUX2JPEG && use_fixed));
}

int VideoDevice::is_compressed(int use_file, int use_fixed)
{
	return is_compressed(in_config->driver, use_file, use_fixed);
}

void VideoDevice::fix_asset(Asset *asset, int driver)
{
// Fix asset using legacy routine
	switch(driver)
	{
	case VIDEO4LINUX2JPEG:
		if(asset->format != FILE_AVI &&
			asset->format != FILE_MOV)
			asset->format = FILE_MOV;
		strcpy(asset->vcodec, QUICKTIME_MJPA);
		return;
	}

// Fix asset using inherited routine
	new_device_base();

	if(input_base) input_base->fix_asset(asset);
	delete input_base;
	input_base = 0;
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

int VideoDevice::get_best_colormodel(Asset *asset)
{
	if(input_base)
		return input_base->get_best_colormodel(asset);
	else
		return BC_RGB888;
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

	if(reading && capturing)
	{
		capturing = 0;
		if(input_base)
		{
			delete input_base;
			input_base = 0;
		}

		if(keepalive)
		{
			keepalive->stop();
			delete keepalive;
		}
		reading = 0;
	}
}

void VideoDevice::set_adevice(AudioDevice *adevice)
{
	this->adevice = adevice;
}

int VideoDevice::get_failed()
{
	if(keepalive)
		return keepalive->get_failed();
	else
		return 0;
}

int VideoDevice::interrupt_crash()
{
	if(input_base) return input_base->interrupt_crash();
	return 0;
}

void VideoDevice::set_translation(int input_x, int input_y)
{
	this->input_x = input_x;
	this->input_y = input_y;
}

void VideoDevice::set_field_order(int odd_field_first)
{
	this->odd_field_first = odd_field_first;
}

void VideoDevice::set_quality(int quality)
{
	this->quality = quality;
}

void VideoDevice::set_cpus(int cpus)
{
	this->cpus = cpus;
}

void VideoDevice::set_picture(PictureConfig *picture)
{
	if(picture)
	{
		picture_lock->lock("VideoDevice::set_picture");
		this->picture->copy_settings(picture);
		picture_changed = 1;
		picture_lock->unlock();

		if(input_base) input_base->set_picture(picture);
	}
}

void VideoDevice::update_translation()
{
	float frame_in_capture_x1f, frame_in_capture_x2f, frame_in_capture_y1f, frame_in_capture_y2f;
	float capture_in_frame_x1f, capture_in_frame_x2f, capture_in_frame_y1f, capture_in_frame_y2f;
	int z_changed = 0;

	if(frame_resized)
	{
		input_x = new_input_x;
		input_y = new_input_y;

		if(in_config->driver == VIDEO4LINUX || in_config->driver == VIDEO4LINUX2)
		{
			if(input_z != new_input_z)
			{
				input_z = new_input_z;
				z_changed = 1;

				capture_w = (int)((float)in_config->w * input_z + 0.5);
				capture_h = (int)((float)in_config->h * input_z + 0.5);

// Need to align to multiple of 4
				capture_w &= ~3;
				capture_h &= ~3;
			}

			frame_in_capture_x1f = (float)input_x * input_z + capture_w / 2 - in_config->w / 2;
			frame_in_capture_x2f = (float)input_x * input_z  + capture_w / 2 + in_config->w / 2;
			frame_in_capture_y1f = (float)input_y * input_z  + capture_h / 2 - in_config->h / 2;
			frame_in_capture_y2f = (float)input_y * input_z  + capture_h / 2 + in_config->h / 2;

			capture_in_frame_x1f = 0;
			capture_in_frame_y1f = 0;
			capture_in_frame_x2f = in_config->w;
			capture_in_frame_y2f = in_config->h;

			if(frame_in_capture_x1f < 0) 
			{ 
				capture_in_frame_x1f -= frame_in_capture_x1f;
				frame_in_capture_x1f = 0;
			}
			if(frame_in_capture_y1f < 0)
			{ 
				capture_in_frame_y1f -= frame_in_capture_y1f;
				frame_in_capture_y1f = 0;
			}
			if(frame_in_capture_x2f > capture_w)
			{
				capture_in_frame_x2f -= frame_in_capture_x2f - capture_w;
				frame_in_capture_x2f = capture_w;
			}
			if(frame_in_capture_y2f > capture_h)
			{
				capture_in_frame_y2f -= frame_in_capture_y2f - capture_h;
				frame_in_capture_y2f = capture_h;
			}

			frame_in_capture_x1 = (int)frame_in_capture_x1f;
			frame_in_capture_y1 = (int)frame_in_capture_y1f;
			frame_in_capture_x2 = (int)frame_in_capture_x2f;
			frame_in_capture_y2 = (int)frame_in_capture_y2f;

			capture_in_frame_x1 = (int)capture_in_frame_x1f;
			capture_in_frame_y1 = (int)capture_in_frame_y1f;
			capture_in_frame_x2 = (int)capture_in_frame_x2f;
			capture_in_frame_y2 = (int)capture_in_frame_y2f;

			frame_resized = 0;
		}
	}
}

void VideoDevice::set_latency_counter(int value)
{
	latency_counter = value;
}

int VideoDevice::has_signal()
{
	if(input_base) return input_base->has_signal();
	return 0;
}

int VideoDevice::read_buffer(VFrame *frame)
{
	int result = 0;
	if(!capturing) return 0;

	if(input_base)
	{
// Reset the keepalive thread
		if(keepalive) keepalive->capturing = 1;
		result = input_base->read_buffer(frame);
		if(keepalive)
		{
			keepalive->capturing = 0;
			keepalive->reset_keepalive();
		}
		return result;
	}

	return 0;
}


// ================================= OUTPUT ==========================================


int VideoDevice::open_output(VideoOutConfig *config,
			int out_w,
			int out_h,
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

	if(output_base->open_output())
	{
		delete output_base;
		output_base = 0;
	}

	if(output_base) 
		return 0;
	else
		return 1;
}

void VideoDevice::goose_input()
{
	if(input_base) input_base->goose_input();
}

void VideoDevice::new_output_buffer(VFrame **output, int colormodel)
{
	if(!output_base) return;
	output_base->new_output_buffer(output, colormodel);
}

void VideoDevice::interrupt_playback()
{
	interrupt = 1;
}

int VideoDevice::write_buffer(VFrame *output, EDL *edl)
{
	if(output_base) return output_base->write_buffer(output, edl);
	return 1;
}

int VideoDevice::output_visible()
{
	if(output_base) return output_base->output_visible();
	return 0;
}

BC_Bitmap* VideoDevice::get_bitmap()
{
	if(output_base) return output_base->get_bitmap();
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
