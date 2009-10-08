
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

#include "condition.h"
#include "device1394input.h"
#include "ieee1394-ioctl.h"
#include "mutex.h"
#include "vframe.h"
#include "video1394.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define INPUT_SAMPLES 131072
#define BUFFER_TIMEOUT 500000


Device1394Input::Device1394Input()
 : Thread(1, 1, 0)
{
	buffer = 0;
	buffer_valid = 0;
	input_buffer = 0;
	done = 0;
	total_buffers = 0;
	current_inbuffer = 0;
	current_outbuffer = 0;
	buffer_size = 0;
	audio_buffer = 0;
	audio_samples = 0;
	video_lock = 0;
	audio_lock = 0;
	buffer_lock = 0;
	decoder = 0;
	fd = -1;
}

Device1394Input::~Device1394Input()
{
// Driver crashes if it isn't stopped before cancelling the thread.
// May still crash during the cancel though.

	if(Thread::running())
	{
		done = 1;
		Thread::cancel();
		Thread::join();
	}

	if(buffer)
	{
		for(int i = 0; i < total_buffers; i++)
			delete [] buffer[i];
		delete [] buffer;
		delete [] buffer_valid;
	}

	if(input_buffer)
		munmap(input_buffer, total_buffers * buffer_size);

	if(audio_buffer)
	{
		delete [] audio_buffer;
	}

	if(decoder)
	{
		dv_delete(decoder);
	}

	if(video_lock) delete video_lock;
	if(audio_lock) delete audio_lock;
	if(buffer_lock) delete buffer_lock;
	if(fd > 0)
	{
		close(fd);
	}
}

int Device1394Input::open(const char *path,
	int port,
	int channel,
	int length,
	int channels,
	int samplerate,
	int bits,
	int w,
	int h)
{
	int result = 0;
	this->channel = channel;
	this->length = length;
	this->channels = channels;
	this->samplerate = samplerate;
	this->bits = bits;
	this->w = w;
	this->h = h;
	is_pal = (h == 576);
	buffer_size = is_pal ? DV_PAL_SIZE : DV_NTSC_SIZE;
	total_buffers = length;


// Initialize grabbing
	if(fd < 0)
	{
		if((fd = ::open(path, O_RDWR)) < 0)
		{
			printf("Device1394Input::open %s: %s\n", path, strerror(errno));
		}
		else
		{
#define CIP_N_NTSC   68000000
#define CIP_D_NTSC 1068000000

#define CIP_N_PAL  1
#define CIP_D_PAL 16

			struct dv1394_init init = 
			{
				api_version: DV1394_API_VERSION,
				channel: channel,
				n_frames: length,
				format: is_pal ? DV1394_PAL: DV1394_NTSC,
				cip_n: 0,
				cip_d: 0,
				syt_offset: 0
			};
			if(ioctl(fd, DV1394_IOC_INIT, &init) < 0)
			{
				printf("Device1394Input::open DV1394_IOC_INIT: %s\n", strerror(errno));
			}

			input_buffer = (unsigned char*)mmap(0,
              	length * buffer_size,
                PROT_READ | PROT_WRITE,
              	MAP_SHARED,
              	fd,
              	0);

			if(ioctl(fd, DV1394_IOC_START_RECEIVE, 0) < 0)
			{
				perror("Device1394Input::open DV1394_START_RECEIVE");
			}
		}



		buffer = new char*[total_buffers];
		buffer_valid = new int[total_buffers];
		bzero(buffer_valid, sizeof(int) * total_buffers);
		for(int i = 0; i < total_buffers; i++)
		{
			buffer[i] = new char[DV_PAL_SIZE];
		}


		audio_buffer = new char[INPUT_SAMPLES * 2 * channels];

		audio_lock = new Condition(0, "Device1394Input::audio_lock");
		video_lock = new Condition(0, "Device1394Input::video_lock");
		buffer_lock = new Mutex("Device1394Input::buffer_lock");

		decoder = dv_new();

		Thread::start();
	}
	return result;
}

void Device1394Input::run()
{
	while(!done)
	{
// Wait for frame to arrive
		struct dv1394_status status;

		Thread::enable_cancel();
		if(ioctl(fd, DV1394_IOC_WAIT_FRAMES, 1))
		{
			perror("Device1394Input::run DV1394_IOC_WAIT_FRAMES");
			sleep(1);
		}
		else
		if(ioctl(fd, DV1394_IOC_GET_STATUS, &status))
		{
			perror("Device1394Input::run DV1394_IOC_GET_STATUS");
		}
		Thread::disable_cancel();



		buffer_lock->lock("Device1394Input::run 1");

		for(int i = 0; i < status.n_clear_frames; i++)
		{
// Get a buffer to transfer to
			char *dst = 0;
			int is_overflow = 0;
			if(!buffer_valid[current_inbuffer])
				dst = buffer[current_inbuffer];
			else
				is_overflow = 1;

			char *src = (char*)(input_buffer + buffer_size * status.first_clear_frame);
// static FILE *test = 0;
// if(!test) test = fopen("/tmp/test", "w");
// fwrite(src, buffer_size, 1, test);

// Export the video
			if(dst)
			{
				memcpy(dst, src, buffer_size);
				buffer_valid[current_inbuffer] = 1;
				video_lock->unlock();
			}


// Extract the audio
			if(audio_samples < INPUT_SAMPLES - 2048)
			{
				int audio_result = dv_read_audio(decoder, 
					(unsigned char*)audio_buffer + 
						audio_samples * 2 * 2,
					(unsigned char*)src,
					buffer_size,
 					channels,
					bits);
				int real_freq = decoder->decoder->audio->frequency;
 				if (real_freq == 32000) 
 				{
// do in-place _FAST_ && _SIMPLE_ upsampling to 48khz
// i also think user should get a warning that his material is effectively 32khz
// we take 16bit samples for both channels in one 32bit int
 					int *twosample = (int*) (audio_buffer + audio_samples * 2 * 2);
 					int from = audio_result - 1;
 					int new_result = audio_result * 48000 / real_freq;
 					for (int to = new_result - 1; to >=0; to--)
 					{	
 						if ((to % 3) == 0 || (to % 3) == 1) from --;
 						twosample[to] = twosample[from];
 					}
 					audio_result = new_result;
 				}


				audio_samples += audio_result;

// Export the audio
				audio_lock->unlock();
			}

// Advance buffer
			if(!is_overflow)
				increment_counter(&current_inbuffer);


			Thread::enable_cancel();
			if(ioctl(fd, DV1394_IOC_RECEIVE_FRAMES, 1))
			{
				perror("Device1394Input::run DV1394_IOC_RECEIVE_FRAMES");
			}

			if(ioctl(fd, DV1394_IOC_GET_STATUS, &status))
			{
				perror("Device1394Input::run DV1394_IOC_GET_STATUS");
			}
			Thread::disable_cancel();
		}

		buffer_lock->unlock();
	}
}

void Device1394Input::increment_counter(int *counter)
{
	(*counter)++;
	if(*counter >= total_buffers) *counter = 0;
}

void Device1394Input::decrement_counter(int *counter)
{
	(*counter)--;
	if(*counter < 0) *counter = total_buffers - 1;
}



int Device1394Input::read_video(VFrame *data)
{
	int result = 0;

// Take over buffer table
	buffer_lock->lock("Device1394Input::read_video 1");
// Wait for buffer with timeout
	while(!buffer_valid[current_outbuffer] && !result)
	{
		buffer_lock->unlock();
		result = video_lock->timed_lock(BUFFER_TIMEOUT, "Device1394Input::read_video 2");
		buffer_lock->lock("Device1394Input::read_video 3");
	}

// Copy frame
	if(buffer_valid[current_outbuffer])
	{
		data->allocate_compressed_data(buffer_size);
		data->set_compressed_size(buffer_size);
		memcpy(data->get_data(), buffer[current_outbuffer], buffer_size);
		buffer_valid[current_outbuffer] = 0;
		increment_counter(&current_outbuffer);
	}

	buffer_lock->unlock();
	return result;
}




int Device1394Input::read_audio(char *data, int samples)
{
	int result = 0;
	int timeout = (int64_t)samples * (int64_t)1000000 * (int64_t)2 / (int64_t)samplerate;
	if(timeout < 500000) timeout = 500000;

// Take over buffer table
	buffer_lock->lock("Device1394Input::read_audio 1");
// Wait for buffer with timeout
	while(audio_samples < samples && !result)
	{
		buffer_lock->unlock();
		result = audio_lock->timed_lock(timeout, "Device1394Input::read_audio 2");
		buffer_lock->lock("Device1394Input::read_audio 3");
	}
//printf("Device1394Input::read_audio 1 %d %d\n", result, timeout);

	if(audio_samples >= samples)
	{
		memcpy(data, audio_buffer, samples * bits * channels / 8);
		memcpy(audio_buffer, 
			audio_buffer + samples * bits * channels / 8,
			(audio_samples - samples) * bits * channels / 8);
		audio_samples -= samples;
	}
//printf("Device1394Input::read_audio 100\n");
	buffer_lock->unlock();
	return result;
}





