
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

#ifndef AUDIOALSA_H
#define AUDIOALSA_H

#include "adeviceprefs.inc"
#include "arraylist.h"
#include "audiodevice.h"

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>

class AudioALSA : public AudioLowLevel
{
public:
	AudioALSA(AudioDevice *device);
	~AudioALSA();

	static void list_devices(ArrayList<char*> *devices, int pcm_title = 0, int mode = MODEPLAY);
	int open_input();
	int open_output();
	int open_duplex();
	int write_buffer(char *buffer, int size);
	int read_buffer(char *buffer, int size);
	void close_all();
	void close_input();
	posnum device_position();
	void flush_device();
	void interrupt_playback();

private:
	void close_output();
	void translate_name(char *output, char *input);
	snd_pcm_format_t translate_format(int format);
	int set_params(snd_pcm_t *dsp, 
		int channels, 
		int bits,
		int samplerate,
		int samples);
	int create_format(snd_pcm_format_t *format, int bits, int channels, int rate);
	snd_pcm_t* get_output();
	snd_pcm_t* get_input();
	snd_pcm_t *dsp_in, *dsp_out, *dsp_duplex;
	samplenum samples_written;
	Timer *timer;
	int delay;
	Mutex *timer_lock;
	int interrupted;
};

#endif
#endif
