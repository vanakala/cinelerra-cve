
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
#include "bctimer.h"
#include "clip.h"
#include "condition.h"
#include "mutex.h"
#include "playbackconfig.h"
#include "sema.h"

#include <string.h>

int AudioDevice::write_buffer(double **output, int samples)
{
// find free buffer to fill
	if(interrupt) return 0;
	arm_buffer(arm_buffer_num, output, samples);
	arm_buffer_num++;
	if(arm_buffer_num >= TOTAL_BUFFERS) arm_buffer_num = 0;
	return 0;
}

int AudioDevice::set_last_buffer()
{
	arm_lock[arm_buffer_num]->lock("AudioDevice::set_last_buffer");
	last_buffer[arm_buffer_num] = 1;
	play_lock[arm_buffer_num]->unlock();


	arm_buffer_num++;
	if(arm_buffer_num >= TOTAL_BUFFERS) arm_buffer_num = 0;
	return 0;
}


// must run before threading once to allocate buffers
// must send maximum size buffer the first time or risk reallocation while threaded
int AudioDevice::arm_buffer(int buffer_num, 
	double **output, 
	int samples)
{
	int bits;
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
	int device_channels = get_ochannels();
	char *buffer_num_buffer;
	double *buffer_in_channel;

	bits = get_obits();

	frame = device_channels * (bits / 8);

	new_size = frame * samples;

	if(interrupt) return 1;

// wait for buffer to become available for writing
	arm_lock[buffer_num]->lock("AudioDevice::arm_buffer");
	if(interrupt) return 1;

	if(new_size > buffer_size[buffer_num])
	{
		delete [] output_buffer[buffer_num];
		output_buffer[buffer_num] = new char[new_size];
		buffer_size[buffer_num] = new_size;
	}

	buffer_size[buffer_num] = new_size;

	buffer_num_buffer = output_buffer[buffer_num];
	bzero(buffer_num_buffer, new_size);
	
	last_input_channel = device_channels - 1;
// copy data
// intel byte order only to correspond with bits_to_fmt

	for(channel = 0; channel < device_channels; channel++)
	{
		buffer_in_channel = output[channel];
		switch(bits)
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
	return 0;
}

int AudioDevice::reset_output()
{
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		delete [] output_buffer[i];
		output_buffer[i] = 0;
		buffer_size[i] = 0;
		arm_lock[i]->reset();
		play_lock[i]->reset(); 
		last_buffer[i] = 0;
	}

	is_playing_back = 0;
	software_position_info = 0;
	position_correction = 0;
	last_buffer_size = 0;
	total_samples = 0;
	play_dither == 0;
	arm_buffer_num = 0;
	last_position = 0;
	interrupt = 0;
	return 0;
}


int AudioDevice::set_play_dither(int status)
{
	play_dither = status;
	return 0;
}

int AudioDevice::set_software_positioning(int status)
{
	software_position_info = status;
	return 0;
}

int AudioDevice::start_playback()
{
// arm buffer before doing this
	is_playing_back = 1;
	interrupt = 0;
// zero timers
	playback_timer->update();
	last_position = 0;

	Thread::set_realtime(get_orealtime());
	Thread::start();                  // synchronize threads by starting playback here and blocking
}

int AudioDevice::interrupt_playback()
{
	interrupt = 1;

	if(is_playing_back)
	{
// cancel thread
		is_playing_back = 0;
		get_lowlevel_out()->interrupt_playback();
// Completion is waited for in arender
	}

// unlock everything
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
// When TRACE_LOCKS is enabled, this
// locks up when run() is waiting on it at just the right time.
// Seems there may be a cancel after the trace lock is locked.
		play_lock[i]->unlock();  
		arm_lock[i]->unlock();
	}

	return 0;
}

int AudioDevice::wait_for_startup()
{
	startup_lock->lock("AudioDevice::wait_for_startup");
	return 0;
}

int AudioDevice::wait_for_completion()
{
	Thread::join();
	return 0;
}



int64_t AudioDevice::current_position()
{
// try to get OSS position
	int64_t hardware_result = 0, software_result = 0, frame;

	if(w)
	{
		frame = get_obits() / 8;

// get hardware position
		if(!software_position_info)
		{
			hardware_result = get_lowlevel_out()->device_position();
		}

// get software position
		if(hardware_result < 0 || software_position_info)
		{
			timer_lock->lock("AudioDevice::current_position");
			software_result = total_samples - last_buffer_size - 
				device_buffer / frame / get_ochannels();
			software_result += playback_timer->get_scaled_difference(get_orate());
			timer_lock->unlock();

			if(software_result < last_position) 
				software_result = last_position;
			else
				last_position = software_result;
		}

		int64_t offset_samples = -(int64_t)(get_orate() * 
			out_config->audio_offset);

		if(hardware_result < 0 || software_position_info) 
			return software_result + offset_samples;
		else
			return hardware_result + offset_samples;
	}
	else
	if(r)
	{
		int64_t result = total_samples_read + 
			record_timer->get_scaled_difference(get_irate());
		return result;
	}

	return 0;
}

void AudioDevice::run_output()
{
	thread_buffer_num = 0;

	startup_lock->unlock();
	playback_timer->update();


//printf("AudioDevice::run 1 %d\n", Thread::calculate_realtime());
	while(is_playing_back && !interrupt && !last_buffer[thread_buffer_num])
	{
// wait for buffer to become available
		play_lock[thread_buffer_num]->lock("AudioDevice::run 1");

		if(is_playing_back && !last_buffer[thread_buffer_num])
		{
			if(duplex_init)
			{
				if(record_before_play)
				{
// block until recording starts
					duplex_lock->lock("AudioDevice::run 2");
				}
				else
				{
// allow recording to start
					duplex_lock->unlock();
				}
				duplex_init = 0;
			}

// get size for position information
			timer_lock->lock("AudioDevice::run 3");
			last_buffer_size = buffer_size[thread_buffer_num] / (get_obits() / 8) / get_ochannels();
			total_samples += last_buffer_size;
			playback_timer->update();
			timer_lock->unlock();


// write converted buffer synchronously
			thread_result = get_lowlevel_out()->write_buffer(
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


//printf("AudioDevice::run 1 %d %d\n", interrupt, last_buffer[thread_buffer_num]);
// test for last buffer
		if(!interrupt && last_buffer[thread_buffer_num])
		{
// no more buffers
			is_playing_back = 0;
// flush the audio device
			get_lowlevel_out()->flush_device();
		}
	}
}








