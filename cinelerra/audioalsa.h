// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AUDIOALSA_H
#define AUDIOALSA_H

#include "adeviceprefs.inc"
#include "arraylist.h"
#include "audiodevice.h"

#include <alsa/asoundlib.h>

class AudioALSA : public AudioLowLevel
{
public:
	AudioALSA(AudioDevice *device);
	~AudioALSA();

	static void list_devices(ArrayList<char*> *devices, int pcm_title = 0);
	static void list_hw_devices(ArrayList<char*> *devices, int pcm_title = 0);
	static void list_pcm_devices(ArrayList<char*> *devices, int pcm_title = 0);
	int open_output();
	int write_buffer(char *buffer, int size);
	void close_all();
	posnum device_position();
	void flush_device();
	void interrupt_playback();
	void dump_params(snd_pcm_t* dsp);

private:
	void close_output();
	snd_pcm_format_t translate_format(int format);
	int set_params(snd_pcm_t *dsp, 
		int channels, 
		int bits,
		int samplerate,
		int samples);
	int create_format(snd_pcm_format_t *format, int bits, int channels, int rate);
	snd_pcm_t* get_output();
	snd_pcm_t* get_input();
	snd_pcm_t *dsp_in, *dsp_out;
	samplenum samples_written;
	snd_pcm_sframes_t delay;
	int sleep_delay;
	Mutex *timing_lock;
	int interrupted;
};

#endif
