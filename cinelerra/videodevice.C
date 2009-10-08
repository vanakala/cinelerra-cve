
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
#include "channel.h"
#include "channeldb.h"
#include "chantables.h"
#include "file.inc"
#include "mutex.h"
#include "mwindow.h"
#include "picture.h"
#include "playbackconfig.h"
#include "playbackengine.h"
#include "preferences.h"
#include "quicktime.h"
#include "recordconfig.h"
#include "recordmonitor.h"
#ifdef HAVE_FIREWIRE
#include "vdevice1394.h"
#endif
#include "vdevicebuz.h"
#include "vdevicedvb.h"
#include "vdevicev4l.h"
#include "vdevicev4l2.h"
#include "vdevicev4l2jpeg.h"
#include "vdevicex11.h"
#include "videoconfig.h"
#include "videodevice.h"
#include "videowindow.h"
#include "videowindowgui.h"
#include "vframe.h"

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
		timer.delay((long)(KEEPALIVE_DELAY * 1000));

// See if a capture happened
		if(still_alive == 0 && capturing)
		{
//			printf("KeepaliveThread::run: device crashed\n");
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
	if(failed) return 1; else return 0;
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
	out_config = new VideoOutConfig;
	channel = new Channel;
	picture = new PictureConfig(mwindow ? mwindow->defaults : 0);
	sharing_lock = new Mutex("VideoDevice::sharing_lock");
	channel_lock = new Mutex("VideoDevice::channel_lock");
	picture_lock = new Mutex("VideoDevice::picture_lock");
	initialize();
}


VideoDevice::~VideoDevice()
{
	input_sources.remove_all_objects();
	delete in_config;
	delete out_config;
	delete channel;
	delete picture;
	delete sharing_lock;
	delete channel_lock;
	delete picture_lock;
}

int VideoDevice::initialize()
{
	sharing = 0;
	done_sharing = 0;
	sharing_lock->reset();
	orate = irate = 0;
	out_w = out_h = 0;
	r = w = 0;
	is_playing_back = is_recording = 0;
	input_x = 0;
	input_y = 0;
	input_z = 1;
	frame_resized = 0;
	capturing = 0;
	keepalive = 0;
	swap_bytes = 0;
	input_base = 0;
	output_base = 0;
	output_format = 0;
	interrupt = 0;
	adevice = 0;
	quality = 80;
	cpus = 1;
	single_frame = 0;
	channel_changed = 0;
	picture_changed = 0;
}

int VideoDevice::open_input(VideoInConfig *config, 
	int input_x, 
	int input_y, 
	float input_z,
	double frame_rate)
{
	int result = 0;

	*this->in_config = *config;

	r = 1;
	this->input_z = -1;   // Force initialization.
	this->frame_rate = frame_rate;


	switch(in_config->driver)
	{
		case VIDEO4LINUX:
			keepalive = new KeepaliveThread(this);
			keepalive->start_keepalive();
			new_device_base();
			result = input_base->open_input();
			break;


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
		case CAPTURE_BUZ:
//printf("VideoDevice 1\n");
			keepalive = new KeepaliveThread(this);
			keepalive->start_keepalive();
			new_device_base();
			result = input_base->open_input();
			break;
#ifdef HAVE_FIREWIRE
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			new_device_base();
			result = input_base->open_input();
			break;
#endif

		case CAPTURE_DVB:
			new_device_base();
			result = input_base->open_input();
			break;
	}
	
	if(!result) capturing = 1;
	return 0;
}

VDeviceBase* VideoDevice::new_device_base()
{
	switch(in_config->driver)
	{
		case VIDEO4LINUX:
			return input_base = new VDeviceV4L(this);

#ifdef HAVE_VIDEO4LINUX2
		case VIDEO4LINUX2:
			return input_base = new VDeviceV4L2(this);

		case VIDEO4LINUX2JPEG:
			return input_base = new VDeviceV4L2JPEG(this);
#endif

		case SCREENCAPTURE:
			return input_base = new VDeviceX11(this, 0);

		case CAPTURE_BUZ:
			return input_base = new VDeviceBUZ(this);

#ifdef HAVE_FIREWIRE
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			return input_base = new VDevice1394(this);
#endif

		case CAPTURE_DVB:
			return input_base = new VDeviceDVB(this);
	}
	return 0;
}

static char* get_channeldb_path(VideoInConfig *vconfig_in)
{
	char *path = "";
	switch(vconfig_in->driver)
	{
		case VIDEO4LINUX:
			path = "channels_v4l";
			break;
		case VIDEO4LINUX2:
			path = "channels_v4l2";
			break;
		case VIDEO4LINUX2JPEG:
			path = "channels_v4l2jpeg";
			break;
		case CAPTURE_BUZ:
			path = "channels_buz";
			break;
		case CAPTURE_DVB:
			path = "channels_dvb";
			break;
	}
	return path;
}

void VideoDevice::load_channeldb(ChannelDB *channeldb, VideoInConfig *vconfig_in)
{
	channeldb->load(get_channeldb_path(vconfig_in));
}

void VideoDevice::save_channeldb(ChannelDB *channeldb, VideoInConfig *vconfig_in)
{
	channeldb->save(get_channeldb_path(vconfig_in));
}


VDeviceBase* VideoDevice::get_output_base()
{
	return output_base;
}

int VideoDevice::is_compressed(int driver, int use_file, int use_fixed)
{
// FileMOV needs to have write_frames called so the start codes get scanned.
	return ((driver == CAPTURE_BUZ && use_fixed) ||
		(driver == VIDEO4LINUX2JPEG && use_fixed) || 
		driver == CAPTURE_LML || 
		driver == CAPTURE_FIREWIRE ||
		driver == CAPTURE_IEC61883);
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
		case CAPTURE_BUZ:
		case CAPTURE_LML:
		case VIDEO4LINUX2JPEG:
			if(asset->format != FILE_AVI &&
				asset->format != FILE_MOV)
				asset->format = FILE_MOV;
			strcpy(asset->vcodec, QUICKTIME_MJPA);
			return;
		
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			if(asset->format != FILE_AVI &&
				asset->format != FILE_MOV)
				asset->format = FILE_MOV;
			strcpy(asset->vcodec, QUICKTIME_DVSD);
			return;
	}

// Fix asset using inherited routine
	new_device_base();

	if(input_base) input_base->fix_asset(asset);
	delete input_base;
	input_base = 0;
}


char* VideoDevice::drivertostr(int driver)
{
	switch(driver)
	{
		case PLAYBACK_X11:
			return PLAYBACK_X11_TITLE;
			break;
		case PLAYBACK_X11_XV:
			return PLAYBACK_X11_XV_TITLE;
			break;
		case PLAYBACK_X11_GL:
			return PLAYBACK_X11_GL_TITLE;
			break;
		case PLAYBACK_BUZ:
			return PLAYBACK_BUZ_TITLE;
			break;
		case VIDEO4LINUX:
			return VIDEO4LINUX_TITLE;
			break;
		case VIDEO4LINUX2:
			return VIDEO4LINUX2_TITLE;
			break;
		case VIDEO4LINUX2JPEG:
			return VIDEO4LINUX2JPEG_TITLE;
			break;
		case SCREENCAPTURE:
			return SCREENCAPTURE_TITLE;
			break;
		case CAPTURE_BUZ:
			return CAPTURE_BUZ_TITLE;
			break;
#ifdef HAVE_FIREWIRE
		case CAPTURE_FIREWIRE:
			return CAPTURE_FIREWIRE_TITLE;
			break;
		case CAPTURE_IEC61883:
			return CAPTURE_IEC61883_TITLE;
			break;
#endif
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

int VideoDevice::close_all()
{
	int i;

	if(w)
	{
		if(output_base)
		{
			output_base->close_all();
			delete output_base;
		}
	}

	if(r && capturing)
	{
		capturing = 0;
		if(input_base)
		{
			input_base->close_all();
			delete input_base;

			input_base = 0;
		}

		if(keepalive)
		{
			keepalive->stop();
			delete keepalive;
		}
	}


	input_sources.remove_all_objects();

	initialize();

	return 0;
}


int VideoDevice::set_adevice(AudioDevice *adevice)
{
	this->adevice = adevice;
	return 0;
}


ArrayList<Channel*>* VideoDevice::get_inputs()
{
	return &input_sources;
}

Channel* VideoDevice::new_input_source(char *device_name)
{
	for(int i = 0; i < input_sources.total; i++)
	{
		if(!strcmp(input_sources.values[i]->device_name, device_name))
			return input_sources.values[i];
	}
	Channel *item = new Channel;
	strcpy(item->device_name, device_name);
	input_sources.append(item);
	return item;
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

int VideoDevice::set_translation(int input_x, int input_y)
{
	this->input_x = input_x;
	this->input_y = input_y;
	return 0;
}

int VideoDevice::set_field_order(int odd_field_first)
{
	this->odd_field_first = odd_field_first;
	return 0;
}

int VideoDevice::set_channel(Channel *channel)
{
	if(channel)
	{
		channel_lock->lock("VideoDevice::set_channel");
		this->channel->copy_settings(channel);
		channel_changed = 1;
		channel_lock->unlock();

		if(input_base) return input_base->set_channel(channel);
		if(output_base) return output_base->set_channel(channel);
	}
}

void VideoDevice::set_quality(int quality)
{
	this->quality = quality;
}

void VideoDevice::set_cpus(int cpus)
{
	this->cpus = cpus;
}

int VideoDevice::set_picture(PictureConfig *picture)
{
	if(picture)
	{
		picture_lock->lock("VideoDevice::set_picture");
		this->picture->copy_settings(picture);
		picture_changed = 1;
		picture_lock->unlock();

		if(input_base) return input_base->set_picture(picture);
	}
}

int VideoDevice::update_translation()
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

			if(frame_in_capture_x1f < 0) { capture_in_frame_x1f -= frame_in_capture_x1f; frame_in_capture_x1f = 0; }
			if(frame_in_capture_y1f < 0) { capture_in_frame_y1f -= frame_in_capture_y1f; frame_in_capture_y1f = 0; }
			if(frame_in_capture_x2f > capture_w) { capture_in_frame_x2f -= frame_in_capture_x2f - capture_w; frame_in_capture_x2f = capture_w; }
			if(frame_in_capture_y2f > capture_h) { capture_in_frame_y2f -= frame_in_capture_y2f - capture_h; frame_in_capture_y2f = capture_h; }

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
	return 0;
}

int VideoDevice::set_latency_counter(int value)
{
	latency_counter = value;
	return 0;
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

//printf("VideoDevice::read_buffer %p %p\n", frame, input_base);
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
					float rate, 
					int out_w, 
					int out_h,
					Canvas *output,
					int single_frame)
{
	w = 1;
//printf("VideoDevice::open_output 1 %d\n", out_config->driver);
	*this->out_config = *config;
//printf("VideoDevice::open_output 1 %d\n", out_config->driver);
	this->out_w = out_w;
	this->out_h = out_h;
	this->orate = rate;
	this->single_frame = single_frame;

//printf("VideoDevice::open_output 1 %d\n", out_config->driver);
	switch(out_config->driver)
	{
		case PLAYBACK_BUZ:
			output_base = new VDeviceBUZ(this);
			break;
		case PLAYBACK_X11:
		case PLAYBACK_X11_XV:
		case PLAYBACK_X11_GL:
			output_base = new VDeviceX11(this, output);
			break;

#ifdef HAVE_FIREWIRE
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
		case PLAYBACK_IEC61883:
			output_base = new VDevice1394(this);
			break;
#endif
	}
//printf("VideoDevice::open_output 2 %d\n", out_config->driver);

	if(output_base->open_output())
	{
		delete output_base;
		output_base = 0;
	}
//printf("VideoDevice::open_output 3 %d\n", out_config->driver);

	if(output_base) 
		return 0;
	else
		return 1;
}



int VideoDevice::start_playback()
{
// arm buffer before doing this
	is_playing_back = 1;
	interrupt = 0;

	if(output_base) return output_base->start_playback();
	return 1;
}

int VideoDevice::stop_playback()
{
	if(output_base) output_base->stop_playback();
	is_playing_back = 0;
	interrupt = 0;
	return 0;
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


int VideoDevice::interrupt_playback()
{
	interrupt = 1;
	return 0;
}

int VideoDevice::write_buffer(VFrame *output, EDL *edl)
{
//printf("VideoDevice::write_buffer 1 %p\n", output_base);
	if(output_base) return output_base->write_buffer(output, edl);
	return 1;
}

int VideoDevice::output_visible()
{
	if(output_base) return output_base->output_visible();
}

BC_Bitmap* VideoDevice::get_bitmap()
{
	if(output_base) return output_base->get_bitmap();
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

