
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

#include "audio1394.h"
#include "playbackconfig.h"
#include "device1394input.h"
#include "device1394output.h"
#include "iec61883input.h"
#include "iec61883output.h"
#include "preferences.h"
#include "recordconfig.h"
#include "videoconfig.h"
#include "videodevice.h"

#define SAMPLES_PER_FRAME 2048

Audio1394::Audio1394(AudioDevice *device)
 : AudioLowLevel(device)
{
	initialize();
}


Audio1394::~Audio1394()
{
	close_all();
}

int Audio1394::initialize()
{
	input_thread = 0;
	output_thread = 0;
	input_iec = 0;
	output_iec = 0;
}

int Audio1394::open_input()
{
	int result = 0;
	if(!input_thread && !input_iec)
	{
// Lock the channels for the DV format
		device->in_channels = 2;
		device->in_bits = 16;
		bytes_per_sample = device->in_channels * device->in_bits / 8;


		if(device->driver == AUDIO_DV1394 ||
			device->driver == AUDIO_1394)
		{
			input_thread = new Device1394Input;
			result = input_thread->open(device->in_config->firewire_path,
				device->in_config->firewire_port, 
				device->in_config->firewire_channel, 
				30,
				device->in_channels,
				device->in_samplerate,
				device->in_bits,
				device->vconfig->w,
				device->vconfig->h);
		}
		else
		{
			input_iec = new IEC61883Input;
			result = input_iec->open(device->in_config->firewire_port, 
				device->in_config->firewire_channel, 
				30,
				device->in_channels,
				device->in_samplerate,
				device->in_bits,
				device->vconfig->w,
				device->vconfig->h);
		}




		if(result)
		{
			delete input_thread;
			input_thread = 0;
			delete input_iec;
			input_iec = 0;
		}
	}

	return result;
}

int Audio1394::open_output()
{
	if(!output_thread && !output_iec)
	{
// Lock the channels for the DV format
		device->out_channels = 2;
		device->out_bits = 16;
		bytes_per_sample = device->out_channels * device->out_bits / 8;


		if(device->driver == AUDIO_DV1394)
		{
			output_thread = new Device1394Output(device);
			output_thread->open(device->out_config->dv1394_path,
				device->out_config->dv1394_port,
				device->out_config->dv1394_channel,
				30,
				device->out_channels, 
				device->out_bits, 
				device->out_samplerate,
				device->out_config->dv1394_syt);
		}
		else
		if(device->driver == AUDIO_1394)
		{
			output_thread = new Device1394Output(device);
			output_thread->open(device->out_config->firewire_path,
				device->out_config->firewire_port,
				device->out_config->firewire_channel,
				30,
				device->out_channels, 
				device->out_bits, 
				device->out_samplerate,
				device->out_config->firewire_syt);
		}
		else
		{
			output_iec = new IEC61883Output(device);
			output_iec->open(device->out_config->firewire_port,
				device->out_config->firewire_channel,
				30,
				device->out_channels, 
				device->out_bits, 
				device->out_samplerate,
				device->out_config->firewire_syt);
		}
	}
	return 0;
}

int Audio1394::close_all()
{
	if(input_thread)
	{
		delete input_thread;
	}

	if(output_thread)
	{
		delete output_thread;
	}
	delete input_iec;
	delete output_iec;

	initialize();
	return 0;
}


int Audio1394::read_buffer(char *buffer, int bytes)
{
	if(input_thread)
	{
		input_thread->read_audio(buffer, bytes / bytes_per_sample);
	}
	else
	if(input_iec)
	{
		input_iec->read_audio(buffer, bytes / bytes_per_sample);
	}

	return 0;
}

int Audio1394::write_buffer(char *buffer, int bytes)
{
	if(output_thread)
		output_thread->write_samples(buffer, bytes / bytes_per_sample);
	else
	if(output_iec)
		output_iec->write_samples(buffer, bytes / bytes_per_sample);
	return 0;
}

int64_t Audio1394::device_position()
{
	if(output_thread)
		return output_thread->get_audio_position();
	else
	if(output_iec)
		return output_iec->get_audio_position();
	else
		return 0;
}


int Audio1394::flush_device()
{
	if(output_thread)
		output_thread->flush();
	else
	if(output_iec)
		output_iec->flush();
	return 0;
}

int Audio1394::interrupt_playback()
{
	if(output_thread)
		output_thread->interrupt();
	else
	if(output_iec)
		output_iec->interrupt();
	return 0;
}
