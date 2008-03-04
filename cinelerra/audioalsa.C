#include "audiodevice.h"
#include "audioalsa.h"
#include "bcsignals.h"
#include "mutex.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"

#include <errno.h>

#ifdef HAVE_ALSA

AudioALSA::AudioALSA(AudioDevice *device)
 : AudioLowLevel(device)
{
	samples_written = 0;
	timer = new Timer;
	delay = 0;
	timer_lock = new Mutex("AudioALSA::timer_lock");
	interrupted = 0;
	dsp_out = 0;
}

AudioALSA::~AudioALSA()
{
	delete timer_lock;
	delete timer;
}

void AudioALSA::list_devices(ArrayList<char*> *devices, int pcm_title, int mode)
{
	snd_ctl_t *handle;
	int card, err, dev, idx;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	char string[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	int error;
	switch(mode)
	{
		case MODERECORD:
			stream = SND_PCM_STREAM_CAPTURE;
			break;
		case MODEPLAY:
			stream = SND_PCM_STREAM_PLAYBACK;
			break;
	}

	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	card = -1;
#define DEFAULT_DEVICE "default"
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
			printf("AudioALSA::list_devices card=%i: %s\n", card, snd_strerror(err));
			continue;
		}

		if((err = snd_ctl_card_info(handle, info)) < 0)
		{
			printf("AudioALSA::list_devices card=%i: %s\n", card, snd_strerror(err));
			snd_ctl_close(handle);
			continue;
		}

		dev = -1;

		while(1)
		{
			unsigned int count;
			if(snd_ctl_pcm_next_device(handle, &dev) < 0)
				printf("AudioALSA::list_devices: snd_ctl_pcm_next_device\n");

			if (dev < 0)
				break;

			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);

			if((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) 
			{
				if(err != -ENOENT)
					printf("AudioALSA::list_devices card=%i: %s\n", card, snd_strerror(err));
				continue;
			}

			if(pcm_title)
			{
				sprintf(string, "plughw:%d,%d", card, dev);
//				strcpy(string, "cards.pcm.front");
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

//	snd_ctl_card_info_free(info);
//	snd_pcm_info_free(pcminfo);
}

void AudioALSA::translate_name(char *output, char *input)
{
	ArrayList<char*> titles;
	ArrayList<char*> pcm_titles;
	int mode;
	if(device->r) mode = MODERECORD;
	else
	if(device->w) mode = MODEPLAY;
	
	list_devices(&titles, 0, mode);
	list_devices(&pcm_titles, 1, mode);

	sprintf(output, "default");	
	for(int i = 0; i < titles.total; i++)
	{
//printf("AudioALSA::translate_name %s %s\n", titles.values[i], pcm_titles.values[i]);
		if(!strcasecmp(titles.values[i], input))
		{
			strcpy(output, pcm_titles.values[i]);
			break;
		}
	}
	
	titles.remove_all_objects();
	pcm_titles.remove_all_objects();
}

snd_pcm_format_t AudioALSA::translate_format(int format)
{
	switch(format)
	{
		case 8:
			return SND_PCM_FORMAT_S8;
			break;
		case 16:
			return SND_PCM_FORMAT_S16_LE;
			break;
		case 24:
			return SND_PCM_FORMAT_S24_LE;
			break;
		case 32:
			return SND_PCM_FORMAT_S32_LE;
			break;
	}
}

void AudioALSA::set_params(snd_pcm_t *dsp, 
	int channels, 
	int bits,
	int samplerate,
	int samples)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	int err;

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(dsp, params);

	if (err < 0) 
	{
		printf("AudioALSA::set_params: no PCM configurations available\n");
		return;
	}

	snd_pcm_hw_params_set_access(dsp, 
		params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(dsp, 
		params, 
		translate_format(bits));
	snd_pcm_hw_params_set_channels(dsp, 
		params, 
		channels);
	snd_pcm_hw_params_set_rate_near(dsp, 
		params, 
		(unsigned int*)&samplerate, 
		(int*)0);

// Buffers written must be equal to period_time
	int buffer_time;
	int period_time;
	if(device->r)
	{
		buffer_time = 10000000;
		period_time = (int)((int64_t)samples * 1000000 / samplerate);
	}
	else
	{
		buffer_time = (int)((int64_t)samples * 1000000 * 2 / samplerate + 0.5);
		period_time = samples * samplerate / 1000000;
	}


//printf("AudioALSA::set_params 1 %d %d %d\n", samples, buffer_time, period_time);
	snd_pcm_hw_params_set_buffer_time_near(dsp, 
		params,
		(unsigned int*)&buffer_time, 
		(int*)0);
	snd_pcm_hw_params_set_period_time_near(dsp, 
		params,
		(unsigned int*)&period_time, 
		(int*)0);
//printf("AudioALSA::set_params 5 %d %d\n", buffer_time, period_time);
	err = snd_pcm_hw_params(dsp, params);
	if(err < 0)
	{
		printf("AudioALSA::set_params: hw_params failed\n");
		return;
	}

	snd_pcm_uframes_t chunk_size = 1024;
	snd_pcm_uframes_t buffer_size = 262144;
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
//printf("AudioALSA::set_params 10 %d %d\n", chunk_size, buffer_size);

	snd_pcm_sw_params_current(dsp, swparams);
	size_t xfer_align = 1 /* snd_pcm_sw_params_get_xfer_align(swparams) */;
	unsigned int sleep_min = 0;
	err = snd_pcm_sw_params_set_sleep_min(dsp, swparams, sleep_min);
	int n = chunk_size;
	err = snd_pcm_sw_params_set_avail_min(dsp, swparams, n);
	err = snd_pcm_sw_params_set_xfer_align(dsp, swparams, xfer_align);
	if(snd_pcm_sw_params(dsp, swparams) < 0)
	{
		printf("AudioALSA::set_params: snd_pcm_sw_params failed\n");
	}

	device->device_buffer = samples * bits / 8 * channels;

//printf("AudioALSA::set_params 100 %d %d\n", samples,  device->device_buffer);

//	snd_pcm_hw_params_free(params);
//	snd_pcm_sw_params_free(swparams);
}

int AudioALSA::open_input()
{
	char pcm_name[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
	int open_mode = 0;
	int err;

	device->in_channels = device->get_ichannels();
	device->in_bits = device->in_config->alsa_in_bits;

	translate_name(pcm_name, device->in_config->alsa_in_device);
//printf("AudioALSA::open_input %s\n", pcm_name);

	err = snd_pcm_open(&dsp_in, device->in_config->alsa_in_device, stream, open_mode);

	if(err < 0)
	{
		printf("AudioALSA::open_input: %s\n", snd_strerror(err));
		return 1;
	}

	set_params(dsp_in, 
		device->get_ichannels(), 
		device->in_config->alsa_in_bits,
		device->in_samplerate,
		device->in_samples);

	return 0;
}

int AudioALSA::open_output()
{
	char pcm_name[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	int open_mode = 0;
	int err;

	device->out_channels = device->get_ochannels();
	device->out_bits = device->out_config->alsa_out_bits;

	translate_name(pcm_name, device->out_config->alsa_out_device);

	err = snd_pcm_open(&dsp_out, device->out_config->alsa_out_device, stream, open_mode);

	if(err < 0)
	{
		dsp_out = 0;
		printf("AudioALSA::open_output %s: %s\n", pcm_name, snd_strerror(err));
		return 1;
	}

	set_params(dsp_out, 
		device->get_ochannels(), 
		device->out_config->alsa_out_bits,
		device->out_samplerate,
		device->out_samples);
	timer->update();
	return 0;
}

int AudioALSA::open_duplex()
{
// ALSA always opens 2 devices
	return 0;
}

int AudioALSA::close_output()
{
	if(device->w && dsp_out)
	{
		snd_pcm_close(dsp_out);
	}
	return 0;
}

int AudioALSA::close_input()
{
	if(device->r)
	{
//		snd_pcm_reset(dsp_in);
		snd_pcm_drop(dsp_in);
		snd_pcm_drain(dsp_in);
		snd_pcm_close(dsp_in);
	}
	return 0;
}

int AudioALSA::close_all()
{
	close_input();
	close_output();
	if(device->d)
	{
		snd_pcm_close(dsp_duplex);
	}
	samples_written = 0;
	delay = 0;
	interrupted = 0;
}

// Undocumented
int64_t AudioALSA::device_position()
{
	timer_lock->lock("AudioALSA::device_position");
	int64_t result = samples_written + 
		timer->get_scaled_difference(device->out_samplerate) - 
		delay;
// printf("AudioALSA::device_position 1 %lld %lld %d %lld\n", 
// samples_written,
// timer->get_scaled_difference(device->out_samplerate),
// delay,
// samples_written + timer->get_scaled_difference(device->out_samplerate) - delay);
	timer_lock->unlock();
	return result;
}

int AudioALSA::read_buffer(char *buffer, int size)
{
//printf("AudioALSA::read_buffer 1\n");
	int attempts = 0;
	int done = 0;

	if(!get_input())
	{
		sleep(1);
		return 0;
	}

	while(attempts < 1 && !done)
	{
		if(snd_pcm_readi(get_input(), 
			buffer, 
			size / (device->in_bits / 8) / device->get_ichannels()) < 0)
		{
			printf("AudioALSA::read_buffer overrun at sample %lld\n", 
				device->total_samples_read);
//			snd_pcm_resume(get_input());
			close_input();
			open_input();
			attempts++;
		}
		else
			done = 1;
	}
	return 0;
}

int AudioALSA::write_buffer(char *buffer, int size)
{
// Don't give up and drop the buffer on the first error.
	int attempts = 0;
	int done = 0;
	int samples = size / (device->out_bits / 8) / device->get_ochannels();

	if(!get_output()) return 0;

	while(attempts < 2 && !done && !interrupted)
	{
// Buffers written must be equal to period_time
// Update timing
 		snd_pcm_sframes_t delay;
 		snd_pcm_delay(get_output(), &delay);
		snd_pcm_avail_update(get_output());

		device->Thread::enable_cancel();
		if(snd_pcm_writei(get_output(), 
			buffer, 
			samples) < 0)
		{
			device->Thread::disable_cancel();
			printf("AudioALSA::write_buffer underrun at sample %lld\n",
				device->current_position());
//			snd_pcm_resume(get_output());
			close_output();
			open_output();
			attempts++;
		}
		else
		{
			device->Thread::disable_cancel();
			done = 1;
		}
	}

	if(done)
	{
		timer_lock->lock("AudioALSA::write_buffer");
		this->delay = delay;
		timer->update();
		samples_written += samples;
		timer_lock->unlock();
	}
	return 0;
}

int AudioALSA::flush_device()
{
	if(get_output()) snd_pcm_drain(get_output());
	return 0;
}

int AudioALSA::interrupt_playback()
{
	if(get_output()) 
	{
		interrupted = 1;
// Interrupts the playback but may not have caused snd_pcm_writei to exit.
// With some soundcards it causes snd_pcm_writei to freeze for a few seconds.
		if(!device->out_config->interrupt_workaround)
			snd_pcm_drop(get_output());

// Makes sure the current buffer finishes before stopping.
//		snd_pcm_drain(get_output());

// The only way to ensure snd_pcm_writei exits, but
// got a lot of crashes when doing this.
//		device->Thread::cancel();
	}
	return 0;
}


snd_pcm_t* AudioALSA::get_output()
{
	if(device->w) return dsp_out;
	else
	if(device->d) return dsp_duplex;
	return 0;
}

snd_pcm_t* AudioALSA::get_input()
{
	if(device->r) return dsp_in;
	else
	if(device->d) return dsp_duplex;
	return 0;
}

#endif
