
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

#include "audiodevice.h"
#include "audioesound.h"
#include "mainerror.h"
#include "playbackconfig.h"
#include "preferences.h"

#ifdef HAVE_ESOUND
#include <esd.h>
#include <string.h>

AudioESound::AudioESound(AudioDevice *device) : AudioLowLevel(device)
{
	esd_out = 0;
	esd_out_fd = 0;
}

AudioESound::~AudioESound()
{
}


int AudioESound::get_bit_flag(int bits)
{
	switch(bits)
	{
	case 8:
		return ESD_BITS8;
		break;

	case 16:
		return ESD_BITS16;
		break;

	case 24:
		return ESD_BITS16;
		break;
	}
}

// No more than 2 channels in ESD
int AudioESound::get_channels_flag(int channels)
{
	switch(channels)
	{
	case 1:
		return ESD_MONO;
		break;

	case 2:
		return ESD_STEREO;
		break;

	default:
		return ESD_STEREO;
		break;
	}
}

char* AudioESound::translate_device_string(char *server, int port)
{
// ESD server
	if(port > 0 && strlen(server))
		sprintf(device_string, "%s:%d", server, port);
	else
		device_string[0] = 0;
	return device_string;
}

int AudioESound::open_output()
{
	esd_format_t format = ESD_STREAM | ESD_PLAY;

	device->out_channels = 2;
	device->out_bits = 16;

	format |= get_channels_flag(device->out_channels);
	format |= get_bit_flag(device->out_bits);

	if((esd_out = esd_open_sound(translate_device_string(device->out_config->esound_out_server, device->out_config->esound_out_port))) <= 0)
	{
		errorbox("Failed to open ESound output");
		return 1;
	};
	esd_out_fd = esd_play_stream_fallback(format, device->out_samplerate, 
					translate_device_string(device->out_config->esound_out_server, device->out_config->esound_out_port),
					"Cinelerra");

	device->device_buffer = esd_get_latency(esd_out);
	device->device_buffer *= device->out_bits / 8 * device->out_channels;
	return 0;
}

void AudioESound::close_all()
{
	if(esd_out > 0)
	{
		close(esd_out_fd);
		esd_close(esd_out);
	}
}

// No position on ESD
samplenum AudioESound::device_position()
{
	return -1;
}

int AudioESound::write_buffer(char *buffer, int size)
{
	if(esd_out_fd > 0)
		return write(esd_out_fd, buffer, size);
	else
		return 0;
}
#endif // HAVE_ESOUND
