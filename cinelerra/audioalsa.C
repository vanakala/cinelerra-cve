#include "audiodevice.h"
#include "audioalsa.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"

#include <errno.h>

#ifdef HAVE_ALSA

AudioALSA::AudioALSA(AudioDevice *device) : AudioLowLevel(device)
{
}

AudioALSA::~AudioALSA()
{
}

void AudioALSA::list_devices(ArrayList<char*> *devices, int pcm_title)
{
	snd_ctl_t *handle;
	int card, err, dev, idx;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	char string[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	int error;

	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	card = -1;

	if((error = snd_card_next(&card)) < 0)
	{
		printf("AudioALSA::list_devices: %s\n", snd_strerror(error));
		return;
	}

	while(card >= 0) 
	{
		char name[BCTEXTLEN];
		sprintf(name, "hw:%d", card);

		if((err = snd_ctl_open(&handle, name, 0)) < 0) 
		{
			printf("AudioALSA::list_devices (%i): %s", card, snd_strerror(err));
			continue;
		}

		if((err = snd_ctl_card_info(handle, info)) < 0)
		{
			printf("AudioALSA::list_devices (%i): %s", card, snd_strerror(err));
			snd_ctl_close(handle);
			continue;
		}

		dev = -1;

		while(1)
		{
			unsigned int count;
			if(snd_ctl_pcm_next_device(handle, &dev) < 0)
				printf("AudioALSA::list_devices: snd_ctl_pcm_next_device");

			if (dev < 0)
				break;

			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);

			if((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) 
			{
				if(err != -ENOENT)
					printf("AudioALSA::list_devices (%i): %s", card, snd_strerror(err));
				continue;
			}

			if(pcm_title)
			{
//				sprintf(string, "plug:%d,%d", card, dev);
printf("AudioALSA::list_devices: %s\n", snd_ctl_card_info_get_name(info));
				strcpy(string, "cards.pcm.front");
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

		if(snd_card_next(&card) < 0) 
		{
			printf("AudioALSA::list_devices: snd_card_next");
			break;
		}
	}

//	snd_ctl_card_info_free(info);
//	snd_pcm_info_free(pcminfo);
}

void AudioALSA::translate_name(char *output, char *input)
{
	ArrayList<char*> titles;
	ArrayList<char*> pcm_titles;
	
	list_devices(&titles, 0);
	list_devices(&pcm_titles, 1);

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

printf("AudioALSA::set_params 1\n");
	snd_pcm_hw_params_alloca(&params);
printf("AudioALSA::set_params 1\n");
	snd_pcm_sw_params_alloca(&swparams);
printf("AudioALSA::set_params 1 %p %p\n", dsp, params);
	err = snd_pcm_hw_params_any(dsp, params);

printf("AudioALSA::set_params 1\n");
	if (err < 0) 
	{
		printf("AudioALSA::set_params: no PCM configurations available\n");
		return;
	}

printf("AudioALSA::set_params 1\n");
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
		samplerate, 
		0);

printf("AudioALSA::set_params 1\n");
// Buffers written must be equal to period_time
	int buffer_time = (int)((double)samples / samplerate * 1000000 * 2 + 0.5);
	int period_time = buffer_time / 2;
	buffer_time = snd_pcm_hw_params_set_buffer_time_near(dsp, 
		params,
		buffer_time, 
		0);
	period_time = snd_pcm_hw_params_set_period_time_near(dsp, 
		params,
		period_time, 
		0);
	err = snd_pcm_hw_params(dsp, params);
	if(err < 0) 
	{
		printf("AudioALSA::set_params: hw_params failed\n");
		return;
	}

printf("AudioALSA::set_params 1\n");
	int chunk_size = snd_pcm_hw_params_get_period_size(params, 0);
	int buffer_size = snd_pcm_hw_params_get_buffer_size(params);

printf("AudioALSA::set_params 1\n");
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

printf("AudioALSA::set_params 1\n");
	device->device_buffer = buffer_size / 
		(bits / 8) / 
		channels;

printf("AudioALSA::set_params 2 %d %d\n", samples,  device->device_buffer);

//	snd_pcm_hw_params_free(params);
//	snd_pcm_sw_params_free(swparams);
}

int AudioALSA::open_input()
{
	char pcm_name[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
	int open_mode = 0;
	int err;

//printf("AudioALSA::open_input 1\n");
	device->in_channels = device->in_config->alsa_in_channels;
	device->in_bits = device->in_config->alsa_in_bits;
//printf("AudioALSA::open_input 1\n");

	translate_name(pcm_name, device->in_config->alsa_in_device);
//printf("AudioALSA::open_input 1\n");

	err = snd_pcm_open(&dsp_in, pcm_name, stream, open_mode);
//printf("AudioALSA::open_input 1\n");

	if(err < 0)
	{
		printf("AudioALSA::open_input: %s\n", snd_strerror(err));
		return 1;
	}
//printf("AudioALSA::open_input 1\n");

	set_params(dsp_in, 
		device->in_config->alsa_in_channels, 
		device->in_config->alsa_in_bits,
		device->in_samplerate,
		device->in_samples);
//printf("AudioALSA::open_input 2\n");
	return 0;
}

int AudioALSA::open_output()
{
	char pcm_name[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	int open_mode = 0;
	int err;

	device->out_channels = device->out_config->alsa_out_channels;
	device->out_bits = device->out_config->alsa_out_bits;

	translate_name(pcm_name, device->out_config->alsa_out_device);

	err = snd_pcm_open(&dsp_out, pcm_name, stream, open_mode);

	if(err < 0)
	{
		printf("AudioALSA::open_output %s: %s\n", pcm_name, snd_strerror(err));
		return 1;
	}

	set_params(dsp_out, 
		device->out_config->alsa_out_channels, 
		device->out_config->alsa_out_bits,
		device->out_samplerate,
		device->out_samples);
	return 0;
}

int AudioALSA::open_duplex()
{
// ALSA always opens 2 devices
	return 0;
}


int AudioALSA::close_all()
{
	if(device->r)
	{
		snd_pcm_reset(dsp_in);
		snd_pcm_drop(dsp_in);
		snd_pcm_drain(dsp_in);
		snd_pcm_close(dsp_in);
	}
	if(device->w)
	{
		snd_pcm_close(dsp_out);
	}
	if(device->d)
	{
		snd_pcm_close(dsp_duplex);
	}
}

// Undocumented
int64_t AudioALSA::device_position()
{
	return -1;
}

int AudioALSA::read_buffer(char *buffer, int size)
{
//printf("AudioALSA::read_buffer 1\n");
	snd_pcm_readi(get_input(), 
		buffer, 
		size / (device->in_bits / 8) / device->in_channels);
//printf("AudioALSA::read_buffer 2\n");
	return 0;
}

int AudioALSA::write_buffer(char *buffer, int size)
{
// Buffers written must be equal to period_time
	if(snd_pcm_writei(get_output(), 
		buffer, 
		size / (device->out_bits / 8) / device->out_channels) < 0)
	{
		printf("AudioALSA::write_buffer: failed\n");
	}
	return 0;
}

int AudioALSA::flush_device()
{
	snd_pcm_drain(get_output());
	return 0;
}

int AudioALSA::interrupt_playback()
{
	
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
