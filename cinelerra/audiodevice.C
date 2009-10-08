
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

#ifdef HAVE_FIREWIRE
#include "audio1394.h"
#endif
#include "audioalsa.h"
#include "audiocine.h"
#include "audiodevice.h"
#include "audiodvb.h"
#include "audioesound.h"
#include "audiooss.h"
#include "bctimer.h"
#include "condition.h"
#include "dcoffset.h"
#include "mutex.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"
#include "sema.h"


AudioLowLevel::AudioLowLevel(AudioDevice *device)
{
	this->device = device;
}

AudioLowLevel::~AudioLowLevel()
{
}






AudioDevice::AudioDevice(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	initialize();
	this->mwindow = mwindow;
	this->out_config = new AudioOutConfig(0);
	this->in_config = new AudioInConfig;
	this->vconfig = new VideoInConfig;
	startup_lock = new Condition(0, "AudioDevice::startup_lock");
	duplex_lock = new Condition(0, "AudioDevice::duplex_lock");
	timer_lock = new Mutex("AudioDevice::timer_lock");
	buffer_lock = new Mutex("AudioDevice::buffer_lock");
	polling_lock = new Condition(0, "AudioDevice::polling_lock");
	playback_timer = new Timer;
	record_timer = new Timer;
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		play_lock[i] = new Sema(0, "AudioDevice::play_lock");
		arm_lock[i] = new Sema(1, "AudioDevice::arm_lock");
	}
}

AudioDevice::~AudioDevice()
{
	delete out_config;
	delete in_config;
	delete vconfig;
	delete startup_lock;
	delete duplex_lock;
	delete timer_lock;
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		delete play_lock[i];
		delete arm_lock[i];
	}
	delete playback_timer;
	delete record_timer;
	delete buffer_lock;
	delete polling_lock;
}

int AudioDevice::initialize()
{
	record_before_play = 0;
	r = w = d = 0;

	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		output_buffer[i] = 0;
		input_buffer[i] = 0;
		buffer_size[i] = 0;
		last_buffer[i] = 0;
	}

	duplex_init = 0;
	rec_dither = play_dither = 0;
	software_position_info = 0;
	arm_buffer_num = 0;
	is_playing_back = 0;
	is_recording = 0;
	last_buffer_size = 0;
	total_samples = 0;
	position_correction = 0;
	last_position = 0;
	interrupt = 0;
	lowlevel_in = lowlevel_out = lowlevel_duplex = 0;
	vdevice = 0;
	sharing = 0;
	total_samples_read = 0;
	out_realtime = 0;
	duplex_realtime = 0;
	in_realtime = 0;
	read_waiting = 0; 
	return 0;
}

int AudioDevice::create_lowlevel(AudioLowLevel* &lowlevel, int driver)
{
	this->driver = driver;


	if(!lowlevel)
	{
		switch(driver)
		{
#ifdef HAVE_OSS
			case AUDIO_OSS:
			case AUDIO_OSS_ENVY24:
				lowlevel = new AudioOSS(this);
				break;
#endif

#ifdef HAVE_ESOUND
			case AUDIO_ESOUND:
				lowlevel = new AudioESound(this);
				break;
#endif
			case AUDIO_NAS:
				break;

#ifdef HAVE_ALSA
			case AUDIO_ALSA:
				lowlevel = new AudioALSA(this);
				break;
#endif

#ifdef HAVE_FIREWIRE	
			case AUDIO_1394:
			case AUDIO_DV1394:
			case AUDIO_IEC61883:
				lowlevel = new Audio1394(this);
				break;
#endif



			case AUDIO_DVB:
				lowlevel = new AudioDVB(this);
				break;



			case AUDIO_CINE:
				lowlevel = new AudioCine(this);
				break;
		}
	}
	return 0;
}

int AudioDevice::open_input(AudioInConfig *config, 
	VideoInConfig *vconfig, 
	int rate, 
	int samples,
	int channels,
	int realtime)
{
	r = 1;
	duplex_init = 0;
	this->in_config->copy_from(config);
	this->vconfig->copy_from(vconfig);
	in_samplerate = rate;
	in_samples = samples;
	in_realtime = realtime;
	in_channels = channels;
	create_lowlevel(lowlevel_in, config->driver);
	lowlevel_in->open_input();
	record_timer->update();
	return 0;
}

int AudioDevice::open_output(AudioOutConfig *config, 
	int rate, 
	int samples, 
	int channels,
	int realtime)
{
	w = 1;
	duplex_init = 0;
	*this->out_config = *config;
	out_samplerate = rate;
	out_samples = samples;
	out_channels = channels;
	out_realtime = realtime;
	create_lowlevel(lowlevel_out, config->driver);
	return lowlevel_out ? lowlevel_out->open_output() : 0;
}


int AudioDevice::interrupt_crash()
{
	if(lowlevel_in) return lowlevel_in->interrupt_crash();
	return 0;
}


int AudioDevice::close_all()
{
	if(is_recording)
	{
		is_recording = 0;
		read_waiting = 1;
		Thread::join();
	}
	

	if(lowlevel_in) lowlevel_in->close_all();
	if(lowlevel_out) lowlevel_out->close_all();
	if(lowlevel_duplex) lowlevel_duplex->close_all();

	reset_output();
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		delete [] input_buffer[i]; 
		input_buffer[i] = 0;
	}
	
	is_recording = 0;
	rec_dither = play_dither = 0;
	software_position_info = position_correction = last_buffer_size = 0;
	r = w = d = 0;
	duplex_init = 0;
	vdevice = 0;
	sharing = 0;

	if(lowlevel_in)
	{
		delete lowlevel_in;
		lowlevel_in = 0;
	}

	if(lowlevel_out)
	{
		delete lowlevel_out;
		lowlevel_out = 0;
	}

	if(lowlevel_duplex)
	{
		delete lowlevel_duplex;
		lowlevel_duplex = 0;
	}

	return 0;
}

int AudioDevice::set_vdevice(VideoDevice *vdevice)
{
	this->vdevice = vdevice;
	return 0;
}


int AudioDevice::get_ichannels()
{
	if(r) return in_channels;
	else if(d) return duplex_channels;
	else return 0;
}

int AudioDevice::get_ibits()
{
	if(r) return in_bits;
	else if(d) return duplex_bits;
	return 0;
}


int AudioDevice::get_obits()
{
	if(w) return out_bits;
	else if(d) return duplex_bits;
	return 0;
}

int AudioDevice::get_ochannels()
{
	if(w) return out_channels;
	else if(d) return duplex_channels;
	return 0;
}

AudioLowLevel* AudioDevice::get_lowlevel_out()
{
	if(w) return lowlevel_out;
	else if(d) return lowlevel_duplex;
	return 0;
}

AudioLowLevel* AudioDevice::get_lowlevel_in()
{
	if(r) return lowlevel_in;
	else if(d) return lowlevel_duplex;
	return 0;
}

int AudioDevice::get_irate()
{
	if(r) return in_samplerate;
	else
	if(d) return duplex_samplerate;
}

int AudioDevice::get_orealtime()
{
	if(w) return out_realtime;
	else 
	if(d) return duplex_realtime;
	return 0;
}

int AudioDevice::get_irealtime()
{
	if(r) return in_realtime;
	else 
	if(d) return duplex_realtime;
	return 0;
}


int AudioDevice::get_orate()
{
	if(w) return out_samplerate;
	else if(d) return duplex_samplerate;
	return 0;
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
	if(w)
		run_output();
	else
	if(r)
		run_input();
}

