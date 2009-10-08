
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
#include "iec61883input.h"
#include "mutex.h"
#include "vframe.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define INPUT_SAMPLES 131072
#define BUFFER_TIMEOUT 500000


IEC61883Input::IEC61883Input()
 : Thread(1, 1, 0)
{
	frame = 0;
	handle = 0;
	fd = 0;
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
}

IEC61883Input::~IEC61883Input()
{
// Driver crashes if it isn't stopped before cancelling the thread.
// May still crash during the cancel though.

	if(Thread::running())
	{
		done = 1;
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
	if(frame) iec61883_dv_fb_close(frame);
	if(handle) raw1394_destroy_handle(handle);
}

static int write_frame_static(unsigned char *data, int len, int complete, void *ptr)
{
	IEC61883Input *input = (IEC61883Input*)ptr;
	return input->write_frame(data, len, complete);
}



int IEC61883Input::open(int port,
	int channel,
	int length,
	int channels,
	int samplerate,
	int bits,
	int w,
	int h)
{
	this->port = port;
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
	if(!handle)
	{
		handle = raw1394_new_handle_on_port(port);
		if(handle)
		{
			frame = iec61883_dv_fb_init(handle, write_frame_static, (void *)this);
			if(frame)
			{
				if(!iec61883_dv_fb_start(frame, channel))
				{
					fd = raw1394_get_fd(handle);
				}
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

		audio_lock = new Condition(0, "IEC61883Input::audio_lock");
		video_lock = new Condition(0, "IEC61883Input::video_lock");
		buffer_lock = new Mutex("IEC61883Input::buffer_lock");

		decoder = dv_new();

		Thread::start();
	}

	if(!handle || !frame || !fd) return 1;

	return 0;
}


void IEC61883Input::run()
{
	while(!done && handle)
	{
		struct timeval tv;
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 20000;
		if(select(fd + 1, &rfds, 0, 0, &tv) > 0)
		{
			raw1394_loop_iterate(handle);
		}
	}

}








int IEC61883Input::write_frame(unsigned char *data, int len, int complete)
{
	if(!complete) printf("write_frame: incomplete frame received.\n");

	buffer_lock->lock("IEC61883Input write_frame 1");

// Get a buffer to transfer to
	char *dst = 0;
	int is_overflow = 0;
	if(!buffer_valid[current_inbuffer])
		dst = buffer[current_inbuffer];
	else
		is_overflow = 1;

	char *src = (char*)(data);
// static FILE *test = 0;
// if(!test) test = fopen("/tmp/test", "w");
// fwrite(src, buffer_size, 1, test);

// Export the video
	if(dst)
	{
		memcpy(dst, src, len);
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
			len,
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


	buffer_lock->unlock();
	return 0;
}







void IEC61883Input::increment_counter(int *counter)
{
	(*counter)++;
	if(*counter >= total_buffers) *counter = 0;
}

void IEC61883Input::decrement_counter(int *counter)
{
	(*counter)--;
	if(*counter < 0) *counter = total_buffers - 1;
}



int IEC61883Input::read_video(VFrame *data)
{
	int result = 0;

// Take over buffer table
	buffer_lock->lock("IEC61883Input::read_video 1");
// Wait for buffer with timeout
	while(!buffer_valid[current_outbuffer] && !result)
	{
		buffer_lock->unlock();
		result = video_lock->timed_lock(BUFFER_TIMEOUT, "IEC61883Input::read_video 2");
		buffer_lock->lock("IEC61883Input::read_video 3");
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




int IEC61883Input::read_audio(char *data, int samples)
{
	int result = 0;
	int timeout = (int64_t)samples * (int64_t)1000000 * (int64_t)2 / (int64_t)samplerate;
	if(timeout < 500000) timeout = 500000;

// Take over buffer table
	buffer_lock->lock("IEC61883Input::read_audio 1");
// Wait for buffer with timeout
	while(audio_samples < samples && !result)
	{
		buffer_lock->unlock();
		result = audio_lock->timed_lock(timeout, "IEC61883Input::read_audio 2");
		buffer_lock->lock("IEC61883Input::read_audio 3");
	}

	if(audio_samples >= samples)
	{
		memcpy(data, audio_buffer, samples * bits * channels / 8);
		memcpy(audio_buffer, 
			audio_buffer + samples * bits * channels / 8,
			(audio_samples - samples) * bits * channels / 8);
		audio_samples -= samples;
	}

	buffer_lock->unlock();
	return result;
}





