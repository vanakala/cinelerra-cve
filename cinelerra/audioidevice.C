
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
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "condition.h"
#include "dcoffset.h"
#include "mutex.h"

#include <string.h>


#define GET_PEAK_MACRO \
					input_channel[j] = sample;                          \
					if(sample > max[i]) max[i] = sample;           \
					else if(-sample > max[i]) max[i] = -sample;


#define GET_8BIT_SAMPLE_MACRO1 \
sample = input_buffer[k];      \
k += input_channels;           \
if(sample >= max_sample[i]) { sample = max_sample[i]; if(over_count < 3) over_count++; } \
else                           \
if(sample <= min_sample[i]) { sample = min_sample[i]; if(over_count < 3) over_count++; } \
else                           \
if(over_count < 3) over_count = 0; 

#define GET_8BIT_SAMPLE_MACRO2 \
sample /= 0x7f;                  



#define GET_16BIT_SAMPLE_MACRO1                            \
sample = input_buffer_16[k];                               \
k += input_channels;                                       \
if(sample >= max_sample[i]) { sample = max_sample[i]; if(over_count < 3) over_count++; } \
else                                                       \
if(sample <= min_sample[i]) { sample = min_sample[i]; if(over_count < 3) over_count++; } \
else                                                       \
if(over_count < 3) over_count = 0;                         \

#define GET_16BIT_SAMPLE_MACRO2                            \
sample /= 0x7fff;                  



#define GET_24BIT_SAMPLE_MACRO1 \
sample = (unsigned char)input_buffer[k] | \
			(((unsigned char)input_buffer[k + 1]) << 8) | \
			(((int)input_buffer[k + 2]) << 16); \
k += input_channels * 3; \
if(sample >= max_sample[i]) { sample = max_sample[i]; if(over_count < 3) over_count++; } \
else                                                 \
if(sample <= min_sample[i]) { sample = min_sample[i]; if(over_count < 3) over_count++; } \
else                                                 \
if(over_count < 3) over_count = 0; 

#define GET_24BIT_SAMPLE_MACRO2       \
sample /= 0x7fffff;



#define GET_32BIT_SAMPLE_MACRO1 \
sample = (unsigned char)input_buffer[k] | \
			(((unsigned char)input_buffer[k + 1]) << 8) | \
			(((unsigned char)input_buffer[k + 2]) << 16) | \
			(((int)input_buffer[k + 3]) << 24); \
k += input_channels * 4; \
if(sample >= max_sample[i]) { sample = max_sample[i]; if(over_count < 3) over_count++; } \
else \
if(sample <= min_sample[i]) { sample = min_sample[i]; if(over_count < 3) over_count++; } \
else \
if(over_count < 3) over_count = 0; 

#define GET_32BIT_SAMPLE_MACRO2       \
sample /= 0x7fffffff;

int AudioDevice::read_buffer(double **input, 
	int samples, 
	int *over, 
	double *max, 
	int input_offset)
{
	int i, j, k, frame, bits;
	double sample, denominator;
	double min_sample[MAXCHANNELS], max_sample[MAXCHANNELS];
	int sample_int;
	int over_count;
	int input_channels;
	int result = 0;
	double *input_channel;



	record_timer->update();

	bits = get_ibits();
	input_channels = get_ichannels();
	frame = input_channels * bits / 8;

	total_samples_read += samples;

	switch(bits)
	{
		case 8:       
			denominator = 0x7f;          
			break;
		case 16:      
			denominator = 0x7fff;        
			break;
		case 24:      
			denominator = 0x7fffff;      
			break;
		case 32:      
			denominator = 0x7fffffff;      
			break;
	}


	for(i = 0; i < get_ichannels(); i++)
	{
		min_sample[i] = -denominator; 
		max_sample[i] = denominator; 
		max[i] = 0; 
		over_count = 0;
	}



	int got_it = 0;
	int fragment_size = samples * frame;
	while(fragment_size > 0 && is_recording)
	{

// Get next buffer
		polling_lock->lock("AudioDevice::read_buffer");


		int output_buffer_num = thread_buffer_num - 1;
		if(output_buffer_num < 0) output_buffer_num = TOTAL_BUFFERS - 1;


// Test previously written buffer for data
		char *input_buffer = this->input_buffer[output_buffer_num];

		int *input_buffer_size = &this->buffer_size[output_buffer_num];


// No data.  Test current buffer for data
		if(!*input_buffer_size)
		{
			read_waiting = 1;
			buffer_lock->lock("AudioDevice::read_buffer 1");
			read_waiting = 0;
			input_buffer = this->input_buffer[thread_buffer_num];

			input_buffer_size = &this->buffer_size[thread_buffer_num];


// Data in current buffer.  Advance thread buffer.
			if(*input_buffer_size >= fragment_size)
			{
				thread_buffer_num++;
				if(thread_buffer_num >= TOTAL_BUFFERS)
					thread_buffer_num = 0;
			}
			else
// Not enough data.
			{
				input_buffer = 0;
				input_buffer_size = 0;
			}

			buffer_lock->unlock();

		}


// Transfer data
		if(input_buffer_size && *input_buffer_size)
		{
			int subfragment_size = fragment_size;
			if(subfragment_size > *input_buffer_size)
				subfragment_size = *input_buffer_size;
			int subfragment_samples = subfragment_size / frame;


			for(i = 0; i < get_ichannels(); i++)
			{
				input_channel = &input[i][input_offset];

// device is set to little endian
				switch(bits)
				{
					case 8:
						for(j = 0, k = i; j < subfragment_samples; j++)
						{
							GET_8BIT_SAMPLE_MACRO1
							GET_8BIT_SAMPLE_MACRO2
							GET_PEAK_MACRO
						}
						break;

					case 16:
						{
							int16_t *input_buffer_16;
							input_buffer_16 = (int16_t *)input_buffer;

							{
								for(j = 0, k = i; j < subfragment_samples; j++)
								{
									GET_16BIT_SAMPLE_MACRO1
									GET_16BIT_SAMPLE_MACRO2
									GET_PEAK_MACRO
								}
							}
						}
						break;

					case 24:
						for(j = 0, k = i * 3; j < subfragment_samples; j++)
						{
							GET_24BIT_SAMPLE_MACRO1
							GET_24BIT_SAMPLE_MACRO2
							GET_PEAK_MACRO
						}

					case 32:
						for(j = 0, k = i * 4; j < subfragment_samples; j++)
						{
							GET_32BIT_SAMPLE_MACRO1
							GET_32BIT_SAMPLE_MACRO2
							GET_PEAK_MACRO
						}
						break;
				}

				if(over_count >= 3) 
					over[i] = 1; 
				else 
					over[i] = 0;
			}




			input_offset += subfragment_size / frame;

// Shift input buffer
			if(*input_buffer_size > subfragment_size)
			{
				memcpy(input_buffer, 
					input_buffer + subfragment_size, 
					*input_buffer_size - subfragment_size);
			}

			fragment_size -= subfragment_size;
			*input_buffer_size -= subfragment_size;
		}
	}







	return result < 0;
}


void AudioDevice::run_input()
{
	int frame = get_ichannels() * get_ibits() / 8;
	int fragment_size = in_samples * frame;

	while(is_recording)
	{
		if(read_waiting) usleep(1);
		buffer_lock->lock("AudioDevice::run_input 1");

// Get available input buffer
		char *input_buffer = this->input_buffer[thread_buffer_num];
		int *input_buffer_size = &this->buffer_size[thread_buffer_num];

		if(*input_buffer_size + fragment_size > INPUT_BUFFER_BYTES)
		{
			*input_buffer_size = INPUT_BUFFER_BYTES - fragment_size;
			*input_buffer_size -= *input_buffer_size % fragment_size;
			printf("AudioDevice::run_input: buffer overflow\n");
		}

		int result = get_lowlevel_in()->read_buffer(
			input_buffer + *input_buffer_size, 
			fragment_size);

		if(result < 0)
		{
			perror("AudioDevice::run_input");
			sleep(1);
		}
		else
		{
			*input_buffer_size += fragment_size;

		}
		buffer_lock->unlock();
		polling_lock->unlock();
	}
}

void AudioDevice::start_recording()
{
	is_recording = 1;
	interrupt = 0;
	thread_buffer_num = 0;

	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		input_buffer[i] = new char[INPUT_BUFFER_BYTES];
	}
	record_timer->update();

	Thread::set_realtime(get_irealtime());
	Thread::start();
}













