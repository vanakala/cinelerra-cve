// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "audioalsa.h"
#include "audiodevice.h"
#include "bctimer.h"
#include "bcsignals.h"
#include "condition.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "sema.h"

AudioLowLevel::AudioLowLevel(AudioDevice *device)
{
	this->device = device;
}

AudioDevice::AudioDevice()
 : Thread(THREAD_SYNCHRONOUS)
{
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		output_buffer[i] = 0;
		buffer_size[i] = 0;
		last_buffer[i] = 0;
	}

	play_dither = 0;
	arm_buffer_num = 0;
	is_playing_back = 0;
	is_flushing = 0;
	last_buffer_size = 0;
	total_samples = 0;
	last_position = 0;
	interrupt = 0;
	lowlevel_out = 0;
	vdevice = 0;
	sharing = 0;
	total_samples_read = 0;
	read_waiting = 0;
	out_config = 0;
	startup_lock = new Condition(0, "AudioDevice::startup_lock");
	timer_lock = new Mutex("AudioDevice::timer_lock");
	buffer_lock = new Mutex("AudioDevice::buffer_lock");
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		play_lock[i] = new Sema(0, "AudioDevice::play_lock");
		arm_lock[i] = new Sema(1, "AudioDevice::arm_lock");
	}
}

AudioDevice::~AudioDevice()
{
	if(lowlevel_out)
	{
		delete lowlevel_out;
		lowlevel_out = 0;
	}

	delete startup_lock;
	delete timer_lock;
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		delete play_lock[i];
		delete arm_lock[i];
		delete [] output_buffer[i];
		output_buffer[i] = 0;
		buffer_size[i] = 0;
	}
	delete buffer_lock;
}

void AudioDevice::create_lowlevel(AudioLowLevel* &lowlevel, int driver)
{
	this->driver = driver;

	if(!lowlevel)
	{
		switch(driver)
		{
		case AUDIO_ALSA:
			lowlevel = new AudioALSA(this);
			break;
		default:
			errorbox(_("Can't configure requested audio device"));
			exit(1);
		}
	}
}

int AudioDevice::open_output(AudioOutConfig *config, 
	int rate, 
	int samples, 
	int channels)
{
	int ret = 0;

	out_config = config;
	out_samplerate = rate;
	out_samples = samples;
	out_channels = channels;
	create_lowlevel(lowlevel_out, config->driver);
	if(lowlevel_out)
		ret = lowlevel_out->open_output();
	return ret;
}

void AudioDevice::close_all()
{
	if(lowlevel_out) lowlevel_out->close_all();

	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		arm_lock[i]->reset();
		play_lock[i]->reset();
		last_buffer[i] = 0;
	}

	play_dither = 0;

	is_playing_back = 0;
	is_flushing = 0;
	last_buffer_size = 0;
	total_samples = 0;
	arm_buffer_num = 0;
	last_position = 0;
	interrupt = 0;

	vdevice = 0;
	sharing = 0;
}

void AudioDevice::set_vdevice(VideoDevice *vdevice)
{
	this->vdevice = vdevice;
}

int AudioDevice::get_interrupted()
{
	return interrupt;
}

int AudioDevice::get_device_buffer()
{
	return device_buffer;
}

void AudioDevice::run()
{
	if(lowlevel_out)
		run_output();
}
