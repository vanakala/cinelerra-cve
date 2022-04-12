// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "audiodevice.h"
#include "bctimer.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "mutex.h"
#include "playbackconfig.h"
#include "sema.h"

#include <string.h>
#include <unistd.h>

void AudioDevice::write_buffer(double **output, int samples)
{
// find free buffer to fill
	if(interrupt) return;
	arm_buffer(arm_buffer_num, output, samples);
	arm_buffer_num++;
	if(arm_buffer_num >= TOTAL_BUFFERS) arm_buffer_num = 0;
}

void AudioDevice::set_last_buffer(void)
{
	arm_lock[arm_buffer_num]->lock("AudioDevice::set_last_buffer");
	last_buffer[arm_buffer_num] = 1;
	play_lock[arm_buffer_num]->unlock();

	arm_buffer_num++;
	if(arm_buffer_num >= TOTAL_BUFFERS) arm_buffer_num = 0;
}


// must run before threading once to allocate buffers
// must send maximum size buffer the first time or risk reallocation while threaded
void AudioDevice::arm_buffer(int buffer_num, 
	double **output, 
	int samples)
{
	int new_size;
	int i, j;
	int input_offset;
	int output_offset;
	int output_advance;
	int channel, last_input_channel;
	double sample;
	int int_sample, int_sample2;
	int dither_value;
	int frame;
	int device_channels = out_channels;
	char *buffer_num_buffer;
	double *buffer_in_channel;

	frame = device_channels * (out_bits / 8);

	new_size = frame * samples;

	if(interrupt) return;

// wait for buffer to become available for writing
	arm_lock[buffer_num]->lock("AudioDevice::arm_buffer");
	if(interrupt) return;

	if(new_size > buffer_size[buffer_num])
	{
		delete [] output_buffer[buffer_num];
		output_buffer[buffer_num] = new char[new_size];
		buffer_size[buffer_num] = new_size;
	}

	buffer_size[buffer_num] = new_size;

	buffer_num_buffer = output_buffer[buffer_num];
	memset(buffer_num_buffer, 0, new_size);

	last_input_channel = device_channels - 1;
// copy data
// intel byte order only to correspond with bits_to_fmt

	for(channel = 0; channel < device_channels; channel++)
	{
		buffer_in_channel = output[channel];
		switch(out_bits)
		{
		case 8:
			output_advance = device_channels;
			if(play_dither)
			{
				for(output_offset = channel, input_offset = 0; input_offset < samples; output_offset += output_advance, input_offset++)
				{
					sample = buffer_in_channel[input_offset];
					CLAMP(sample, -1, 1);
					sample *= 0x7fff;
					int_sample = (int)sample;
					dither_value = rand() % 255;
					int_sample -= dither_value;
					int_sample /= 0x100;
					buffer_num_buffer[output_offset] = int_sample;
				}
			}
			else
			{
				for(output_offset = channel, input_offset = 0; input_offset < samples; output_offset += output_advance, input_offset++)
				{
					sample = buffer_in_channel[input_offset];
					CLAMP(sample, -1, 1);
					sample *= 0x7f;
					int_sample = (int)sample;
					buffer_num_buffer[output_offset] = int_sample;
				}
			}
			break;

		case 16:
			output_advance = device_channels * 2 - 1;
			if(play_dither)
			{
				for(output_offset = channel * 2, input_offset = 0; 
					input_offset < samples; 
					output_offset += output_advance, input_offset++)
				{
					sample = buffer_in_channel[input_offset];
					CLAMP(sample, -1, 1);
					sample *= 0x7fffff;
					int_sample = (int)sample;
					dither_value = rand() % 255;
					int_sample -= dither_value;
					int_sample /= 0x100;
					buffer_num_buffer[output_offset] = int_sample;
				}
			}
			else
			{
				for(output_offset = channel * 2, input_offset = 0; 
					input_offset < samples; 
					output_offset += output_advance, input_offset++)
				{
					sample = buffer_in_channel[input_offset];
					CLAMP(sample, -1, 1);
					sample *= 0x7fff;
					int_sample = (int)sample;
					buffer_num_buffer[output_offset++] = (int_sample & 0xff);
					buffer_num_buffer[output_offset] = (int_sample & 0xff00) >> 8;
				}
			}
			break;

		case 24:
			output_advance = (device_channels - 1) * 3;
			for(output_offset = channel * 3, input_offset = 0; 
				input_offset < samples; 
				output_offset += output_advance, input_offset++)
			{
				sample = buffer_in_channel[input_offset];
				CLAMP(sample, -1, 1);
				sample *= 0x7fffff;
				int_sample = (int)sample;
				buffer_num_buffer[output_offset++] = (int_sample & 0xff);
				buffer_num_buffer[output_offset++] = (int_sample & 0xff00) >> 8;
				buffer_num_buffer[output_offset++] = (int_sample & 0xff0000) >> 16;
			}
			break;

		case 32:
			output_advance = (device_channels - 1) * 4;
			for(output_offset = channel * 4, input_offset = 0; 
				input_offset < samples; 
				output_offset += output_advance, input_offset++)
			{
				sample = buffer_in_channel[input_offset];
				CLAMP(sample, -1, 1);
				sample *= 0x7fffffff;
				int_sample = (int)sample;
				buffer_num_buffer[output_offset++] = (int_sample & 0xff);
				buffer_num_buffer[output_offset++] = (int_sample & 0xff00) >> 8;
				buffer_num_buffer[output_offset++] = (int_sample & 0xff0000) >> 16;
				buffer_num_buffer[output_offset++] = (int_sample & 0xff000000) >> 24;
			}
			break;
		}
	}

// make buffer available for playback
	play_lock[buffer_num]->unlock();
}

void AudioDevice::set_play_dither(int status)
{
	play_dither = status;
}

void AudioDevice::start_playback()
{
// arm buffer before doing this
	is_playing_back = 1;
	interrupt = 0;
	last_position = 0;
	Thread::start();                  // synchronize threads by starting playback here and blocking
}

void AudioDevice::interrupt_playback()
{
	interrupt = 1;

	if(is_playing_back || is_flushing)
	{
// cancel thread
		is_playing_back = 0;
		lowlevel_out->interrupt_playback();
// Completion is waited for in arender
	}
}

void AudioDevice::wait_for_startup()
{
	startup_lock->lock("AudioDevice::wait_for_startup");
}

void AudioDevice::wait_for_completion()
{
	Thread::join();
}

ptstime AudioDevice::current_postime(double speed)
{
	samplenum hardware_result = 0;

	if(lowlevel_out)
	{
		hardware_result = lowlevel_out->device_position();

		if(hardware_result > 0)
		{
			last_position = hardware_result;
			return (ptstime)hardware_result / out_samplerate - (out_config->audio_offset / speed);
		}
	}
	if(last_position > 0)
		return (ptstime)last_position / out_samplerate - (out_config->audio_offset/ speed);
	return -1;
}

void AudioDevice::run_output()
{
	thread_buffer_num = 0;

	startup_lock->unlock();

	while(is_playing_back && !interrupt && !last_buffer[thread_buffer_num])
	{
// wait for buffer to become available
		play_lock[thread_buffer_num]->lock("AudioDevice::run 1");

		if(is_playing_back && !last_buffer[thread_buffer_num])
		{
// get size for position information
			timer_lock->lock("AudioDevice::run 3");
			last_buffer_size = buffer_size[thread_buffer_num] / (out_bits / 8) / out_channels;
			total_samples += last_buffer_size;
			timer_lock->unlock();

// write converted buffer synchronously
			thread_result = lowlevel_out->write_buffer(
				output_buffer[thread_buffer_num], 
				buffer_size[thread_buffer_num]);

// allow writing to the buffer
			arm_lock[thread_buffer_num]->unlock();

// inform user if the buffer write failed
			if(thread_result < 0)
			{
				perror("AudioDevice::write_buffer");
				sleep(1);
			}

			thread_buffer_num++;
			if(thread_buffer_num >= TOTAL_BUFFERS) thread_buffer_num = 0;
		}

// test for last buffer
		if(!interrupt && last_buffer[thread_buffer_num])
		{
// no more buffers
// flush the audio device
			is_flushing = 1;
			is_playing_back = 0;
			lowlevel_out->flush_device();
			is_flushing = 0;
		}
	}
}
