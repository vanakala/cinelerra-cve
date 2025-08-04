// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "audiodevice.h"
#include "audioalsa.h"
#include "bcsignals.h"
#include "mainerror.h"
#include "mutex.h"
#include "playbackconfig.h"
#include "preferences.h"

#include <errno.h>

#define DEFAULT_DEVICE "default"
#define NULL_DEVICE "null"

AudioALSA::AudioALSA(AudioDevice *device)
 : AudioLowLevel(device)
{
	samples_written = 0;
	delay = 0;
	timing_lock = new Mutex("AudioALSA::timing_lock");
	interrupted = 0;
	dsp_out = 0;
	dsp_in = 0;
}

AudioALSA::~AudioALSA()
{
	delete timing_lock;
}

void AudioALSA::list_devices(ArrayList<char*> *devices, int pcm_title)
{
	AudioALSA::list_pcm_devices(devices, pcm_title);
}

void AudioALSA::list_pcm_devices(ArrayList<char*> *devices, int pcm_title)
{
	snd_pcm_stream_t stream;
	snd_pcm_t *handle;
	char *s, *name, *io;
	const char *filter;
	void **hints, **n;
	snd_pcm_hw_params_t *hwparams;

	s = new char[strlen(DEFAULT_DEVICE) + 1];
	strcpy(s, DEFAULT_DEVICE);
	devices->append(s);
	devices->set_array_delete();

	if(snd_device_name_hint(-1, "pcm", &hints) < 0)
		return;

	filter = "Output";
	stream = SND_PCM_STREAM_PLAYBACK;

	snd_pcm_hw_params_alloca(&hwparams);
	name = 0;
	io = 0;
	for(n = hints; *n; n++)
	{
		name = snd_device_name_get_hint(*n, "NAME");
		// Skip null device
		if(strcmp(name, NULL_DEVICE) == 0)
			goto do_free;
		io = snd_device_name_get_hint(*n, "IOID");
		if(io != NULL && strcmp(io, filter) != 0)
			goto do_free;
		// Check if device is usable
		if(snd_pcm_open(&handle, name, stream, 0) == 0)
		{
			if(snd_pcm_hw_params_any(handle, hwparams) == 0)
			{
				unsigned int val;
				if(snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) == 0
					&& snd_pcm_hw_params_get_channels_max(hwparams, &val) == 0
					&& val > 0)
				{
					s = new char[strlen(name) + 1];
					strcpy(s, name);
					devices->append(s);
				}
			}
			snd_pcm_close(handle);
		}
do_free:
		if(name)
			free(name);
		if(io)
			free(io);
	}
	snd_device_name_free_hint(hints);
}

void AudioALSA::list_hw_devices(ArrayList<char*> *devices, int pcm_title)
{
	snd_ctl_t *handle;
	int card, err, dev, idx;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	char string[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	int error;

	stream = SND_PCM_STREAM_PLAYBACK;

	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	card = -1;
	char *result = new char[strlen(DEFAULT_DEVICE) + 1];
	devices->append(result);
	devices->set_array_delete();     // since we are allocating by new[]
	strcpy(result, DEFAULT_DEVICE);

	while(snd_card_next(&card) >= 0)
	{
		char name[BCTEXTLEN];
		if(card < 0) break;
		sprintf(name, "hw:%i", card);

		if((err = snd_ctl_open(&handle, name, 0)) < 0)
		{
			errorbox("Failed to open ALSA card=%i: %s", card, snd_strerror(err));
			continue;
		}

		if((err = snd_ctl_card_info(handle, info)) < 0)
		{
			errorbox("Failed to get ALSA info card=%i: %s", card, snd_strerror(err));
			snd_ctl_close(handle);
			continue;
		}

		dev = -1;

		while(1)
		{
			unsigned int count;
			if(snd_ctl_pcm_next_device(handle, &dev) < 0)
				errorbox("Failed to get next ALSA device");

			if (dev < 0)
				break;

			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);

			if((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) 
			{
				if(err != -ENOENT)
					errorbox("ALSA card=%i: %s\n", card, snd_strerror(err));
				continue;
			}

			if(pcm_title)
			{
				sprintf(string, "plughw:%d,%d", card, dev);
			}
			else
			{
				sprintf(string, "%s #%d", 
					snd_ctl_card_info_get_name(info), 
					dev);
			}

			char *result = devices->append(new char[strlen(string) + 1]);
			strcpy(result, string);
		}

		snd_ctl_close(handle);
	}
}

snd_pcm_format_t AudioALSA::translate_format(int format)
{
	switch(format)
	{
	case 8:
		return SND_PCM_FORMAT_S8;

	case 16:
		return SND_PCM_FORMAT_S16_LE;

	case 24:
		return SND_PCM_FORMAT_S24_LE;

	case 32:
		return SND_PCM_FORMAT_S32_LE;
	}
	return SND_PCM_FORMAT_UNKNOWN;
}

int AudioALSA::set_params(snd_pcm_t *dsp, 
	int channels, 
	int bits,
	int samplerate,
	int samples)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	unsigned int ratemin;
	int dir;

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);

	if(snd_pcm_hw_params_any(dsp, params) < 0)
	{
		errormsg("ALSA: no PCM configurations available");
		return 1;
	}

	dir = 0;
	snd_pcm_hw_params_get_rate_min(params, &ratemin, &dir);

	if(snd_pcm_hw_params_set_access(dsp, 
		params,
		SND_PCM_ACCESS_RW_INTERLEAVED))
	{
		errormsg("ALSA: failed to set up interleaved device access.");
		return 1;
	}

	if(snd_pcm_hw_params_set_rate(dsp, params, samplerate, 0))
	{
		errormsg("ALSA: Configured ALSA device does not support %u Hz playback.", samplerate);
		return 1;
	}

	if(snd_pcm_hw_params_set_format(dsp, 
		params, 
		translate_format(bits)))
	{
		errormsg("ALSA: failed to set output format.");
		return 1;
	}

	if(snd_pcm_hw_params_set_channels(dsp, 
		params, 
		channels))
	{
		errormsg("ALSA: Configured ALSA device does not support %d channel operation.", channels);
		return 1;
	}

	snd_pcm_uframes_t bufmin, bufmax, buffer_size;

	snd_pcm_hw_params_get_buffer_size_min(params, &bufmin);
	snd_pcm_hw_params_get_buffer_size_max(params, &bufmax);

	if(ratemin < 8000)
	// Assume pulse
		buffer_size = samples / 4;
	else
		buffer_size = samples;
	if(buffer_size < bufmin)
		buffer_size = bufmin;
	else if(buffer_size > bufmax)
		buffer_size = bufmax;

	snd_pcm_hw_params_set_buffer_size_near (dsp, params, 
		&buffer_size);

	dir = 0;
	snd_pcm_hw_params_set_period_size(dsp, params,
				buffer_size / 4, dir);

	if(snd_pcm_hw_params(dsp, params) < 0)
	{
		errormsg("Failed to set ALSA hw_params");
		return 1;
	}
	device->device_buffer = samples * bits / 8 * channels;
	return 0;
}

int AudioALSA::open_output()
{
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	int open_mode = 0;
	int err;

	device->out_bits = device->out_config->alsa_out_bits;

	if((err = snd_pcm_open(&dsp_out, device->out_config->alsa_out_device, stream, open_mode)) < 0)
	{
		dsp_out = 0;
		errorbox("Failed to open ALSA output %s: %s", device->out_config->alsa_out_device, snd_strerror(err));
		return 1;
	}

	if(set_params(dsp_out, 
		device->out_channels,
		device->out_config->alsa_out_bits,
		device->out_samplerate,
		device->out_samples))
	{
		errormsg("Aborting audio playback.");
		close_output();
		return 1;
	}
	samples_written = 0;
	delay = 0;
	interrupted = 0;
	return 0;
}

void AudioALSA::close_output()
{
	if(dsp_out)
	{
		timing_lock->lock("AudioALSA::close_output");
		snd_pcm_close(dsp_out);
		dsp_out = 0;
		timing_lock->unlock();
	}
}

void AudioALSA::close_all()
{
	close_output();
}

samplenum AudioALSA::device_position()
{
	snd_pcm_sframes_t delay;
	samplenum result;

	timing_lock->lock("AudioALSA::device_position");
	if(dsp_out)
	{
		snd_pcm_delay(dsp_out, &delay);
		result = samples_written - delay;
	}
	else
		result = samples_written;
	timing_lock->unlock();
	return result;
}

#ifndef SND_PCM_WAIT_IO
#define SND_PCM_WAIT_IO (-1)
#endif

int AudioALSA::write_buffer(char *buffer, int size)
{
// Don't give up and drop the buffer on the first error.
	int attempts = 0;
	int samplesize = (device->out_bits / 8) * device->out_channels;
	snd_pcm_sframes_t samples = size / samplesize;
	snd_pcm_sframes_t written, avail;
	int rc;
	char *write_pos = buffer;

	if(!dsp_out)
		return 0;

	while(samples > 0 && attempts < 2 && !interrupted)
	{
		snd_pcm_wait(dsp_out, SND_PCM_WAIT_IO);

		if(interrupted)
			break;

		timing_lock->lock("AudioALSA::write_buffer");
		avail = snd_pcm_avail(dsp_out);

		if(avail > samples)
			avail = samples;

		if((written = snd_pcm_writei(dsp_out, write_pos, avail)) < 0)
		{
			if(written == -EPIPE)
			{
// Underrun
				if((rc = snd_pcm_prepare(dsp_out)) < 0 && attempts == 0)
				{
					errorbox("ALSA write underrun at %.3f: %s. Recovery failed.",
						device->current_postime(), snd_strerror(rc));
				}
				attempts++;
			}
			else
			{
				timing_lock->unlock();
				errorbox("ALSA write failed: %s", snd_strerror(written));
				return -1;
			}
		}
		else
		{
			write_pos += written * samplesize;
			samples -= written;
			samples_written += written;
		}
		timing_lock->unlock();
	}
	if(attempts >= 2)
		return -1;
	return 0;
}

void AudioALSA::flush_device()
{
	timing_lock->lock("AudioALSA::flush_device");
	if(dsp_out)
		snd_pcm_drain(dsp_out);
	timing_lock->unlock();
}

void AudioALSA::interrupt_playback()
{
	interrupted = 1;
// Interrupts the playback but may not have caused snd_pcm_writei to exit.
// With some soundcards it causes snd_pcm_writei to freeze for a few seconds.
// Use lock to avoid snd_pcm_drop called during snd_pcm_writei
	timing_lock->lock("AudioALSA::interrupt_playback");
	if(dsp_out)
		snd_pcm_drop(dsp_out);
	timing_lock->unlock();
}

void AudioALSA::dump_params(snd_pcm_t* dsp)
{
	snd_pcm_hw_params_t *hwparams;
	unsigned int val, val1;
	snd_pcm_format_t fmt;
	int dir;
	snd_pcm_uframes_t frames, frames1;

	snd_pcm_hw_params_alloca(&hwparams);

	printf("PCM handle '%s' status dump\n", snd_pcm_name(dsp));
	printf("    state = %s\n", snd_pcm_state_name(snd_pcm_state(dsp)));
	if(snd_pcm_hw_params_current(dsp, hwparams))
	{
		printf("    Failed to get parameters\n");
		return;
	}
	snd_pcm_hw_params_get_access(hwparams, (snd_pcm_access_t *) &val);
	printf("    access type = %s\n", snd_pcm_access_name((snd_pcm_access_t)val));
	snd_pcm_hw_params_get_format(hwparams, (snd_pcm_format_t *)&val);
	printf("    format = '%s' (%s)\n",
	snd_pcm_format_name((snd_pcm_format_t)val), 
		snd_pcm_format_description((snd_pcm_format_t)val));
	snd_pcm_hw_params_get_channels(hwparams, &val);
	dir = 0;
	snd_pcm_hw_params_get_rate(hwparams, &val1, &dir);
	printf("    samplerate = %d bps channels = %d\n", val1, val);
	snd_pcm_hw_params_get_buffer_size(hwparams, &frames1);
	dir = 0;
	snd_pcm_hw_params_get_period_size(hwparams, &frames, &dir);
	printf("    buffer = %ld,  period = %ld frames\n", frames1, frames);
}
