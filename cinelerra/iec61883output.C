
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

#ifdef HAVE_FIREWIRE



#include "audiodevice.h"
#include "condition.h"
#include "iec61883output.h"
#include "mutex.h"
#include "playbackconfig.h"
#include "bctimer.h"
#include "vframe.h"
#include "videodevice.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/utsname.h>





// Crazy DV internals
#define CIP_N_NTSC 2436
#define CIP_D_NTSC 38400
#define CIP_N_PAL 1
#define CIP_D_PAL 16
#define OUTPUT_SAMPLES 262144
#define BUFFER_TIMEOUT 500000


IEC61883Output::IEC61883Output(AudioDevice *adevice)
 : Thread(1, 0, 0)
{
	reset();
	this->adevice = adevice;
}

IEC61883Output::IEC61883Output(VideoDevice *vdevice)
 : Thread(1, 0, 0)
{
	reset();
	this->vdevice = vdevice;
}

IEC61883Output::~IEC61883Output()
{
	if(Thread::running())
	{
		done = 1;
		start_lock->unlock();
		Thread::cancel();
		Thread::join();
	}

	if(buffer)
	{
		for(int i = 0; i < total_buffers; i++)
		{
			if(buffer[i]) delete [] buffer[i];
		}
		delete [] buffer;
		delete [] buffer_size;
		delete [] buffer_valid;
	}

	if(audio_lock) delete audio_lock;
	if(video_lock) delete video_lock;
	if(start_lock) delete start_lock;
	if(audio_buffer) delete [] audio_buffer;

	if(temp_frame) delete temp_frame;
	if(temp_frame2) delete temp_frame2;
	if(video_encoder) dv_delete(video_encoder);
	if(audio_encoder) dv_delete(audio_encoder);
	if(buffer_lock) delete buffer_lock;
	if(position_lock) delete position_lock;
	if(frame) iec61883_dv_close(frame);
	if(handle) raw1394_destroy_handle(handle);
}


void IEC61883Output::reset()
{
	handle = 0;
	fd = 0;
	frame = 0;
	out_position = 0;
	out_buffer = 0;
	out_size = 0;

	buffer = 0;
	buffer_size = 0;
	total_buffers = 0;
	current_inbuffer = 0;
	current_outbuffer = 0;
	done = 0;
	audio_lock = 0;
	video_lock = 0;
	start_lock = 0;
	buffer_lock = 0;
	position_lock = 0;
	video_encoder = 0;
	audio_encoder = 0;
	audio_buffer = 0;
	audio_samples = 0;
	temp_frame = 0;
	temp_frame2 = 0;
	audio_position = 0;
	interrupted = 0;
	have_video = 0;
	adevice = 0;
	vdevice = 0;
	is_pal = 0;
}



static int read_frame_static(unsigned char *data, int n, unsigned int dropped, void *ptr)
{
	IEC61883Output *output = (IEC61883Output*)ptr;
	return output->read_frame(data, n, dropped);
}





int IEC61883Output::open(int port,
	int channel,
	int length,
	int channels, 
	int bits, 
	int samplerate,
	int syt)
{
	this->channels = channels;
	this->bits = bits;
	this->samplerate = samplerate;
	this->total_buffers = length;
	this->syt = syt;

// Set PAL mode based on frame height
	if(vdevice) is_pal = (vdevice->out_h == 576);




	if(!handle)
	{
		handle = raw1394_new_handle_on_port(port);
		if(handle)
		{
			frame = iec61883_dv_xmit_init(handle, 
				is_pal, 
				read_frame_static, 
				(void *)this);
			if(frame)
			{
				if(!iec61883_dv_xmit_start(frame, channel))
				{
					fd = raw1394_get_fd(handle);
				}
			}
		}

// Create buffers
		buffer = new char*[total_buffers];
		for(int i = 0; i < length; i++)
			buffer[i] = new char[DV_PAL_SIZE];
		buffer_size = new int[total_buffers];
		buffer_valid = new int[total_buffers];
		bzero(buffer_size, sizeof(int) * total_buffers);
		bzero(buffer_valid, sizeof(int) * total_buffers);
		bzero(buffer, sizeof(char*) * total_buffers);
		video_lock = new Condition(0, "IEC61883Output::video_lock");
		audio_lock = new Condition(0, "IEC61883Output::audio_lock");
		start_lock = new Condition(0, "IEC61883Output::start_lock");
		buffer_lock = new Mutex("IEC61883Output::buffer_lock");
		position_lock = new Mutex("IEC61883Output::position_lock");
		encoder = dv_new();
		audio_buffer = new char[OUTPUT_SAMPLES * channels * bits / 8];
		Thread::start();
	}
	return 0;
}

void IEC61883Output::run()
{
	Thread::enable_cancel();
	start_lock->lock("IEC61883Output::run");
	Thread::disable_cancel();

	while(!done)
	{
		struct timeval tv;
		fd_set rfds;
		FD_ZERO (&rfds);
		FD_SET (fd, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 20000;
		if(select(fd + 1, &rfds, 0, 0, &tv) > 0)
			raw1394_loop_iterate (handle);
	}
}



int IEC61883Output::read_frame(unsigned char *data, int n, unsigned int dropped)
{
// Next frame
	if(!out_buffer || out_position + 480 > out_size)
	{
		buffer_lock->lock("IEC61883Output read_frame 1");

		out_buffer = buffer[current_outbuffer];
		out_size = buffer_size[current_outbuffer];
		out_position = 0;


// No video.  Put in a fake frame for audio only
		if(!have_video)
		{
#include "data/fake_ntsc_dv.h"
			out_size = sizeof(fake_ntsc_dv) - 4;
			out_buffer = (char*)fake_ntsc_dv + 4;
		}








// Calculate number of samples needed based on given pattern for 
// norm.
		int samples_per_frame = 2048;

// Encode audio
		if(audio_samples > samples_per_frame)
		{
			int samples_written = dv_write_audio(encoder,
				(unsigned char*)out_buffer,
				(unsigned char*)audio_buffer,
				samples_per_frame,
				channels,
				bits,
				samplerate,
				is_pal ? DV_PAL : DV_NTSC);
			memcpy(audio_buffer, 
				audio_buffer + samples_written * bits * channels / 8,
				(audio_samples - samples_written) * bits * channels / 8);
			audio_samples -= samples_written;
			position_lock->lock("IEC61883Output::run");
			audio_position += samples_written;
			position_lock->unlock();


			audio_lock->unlock();
		}


		buffer_lock->unlock();
	}




// Write next chunk of current frame
	if(out_buffer && out_position + 480 <= out_size)
	{
		memcpy(data, out_buffer + out_position, 480);
		out_position += 480;



		if(out_position >= out_size)
		{
			buffer_lock->lock("IEC61883Output read_frame 2");
			buffer_valid[current_outbuffer] = 0;

// Advance buffer number if possible
			increment_counter(&current_outbuffer);

// Reuse same buffer next time
			if(!buffer_valid[current_outbuffer])
			{
				decrement_counter(&current_outbuffer);
			}
			else
// Wait for user to reach current buffer before unlocking any more.
			{
				video_lock->unlock();
			}

			buffer_lock->unlock();
		}
	}
}




void IEC61883Output::write_frame(VFrame *input)
{
	VFrame *ptr = 0;
	int result = 0;

//printf("IEC61883Output::write_frame 1\n");

	if(fd <= 0) return;
	if(interrupted) return;

// Encode frame to DV
	if(input->get_color_model() != BC_COMPRESSED)
	{
		if(!temp_frame) temp_frame = new VFrame;
		if(!encoder) encoder = dv_new();
		ptr = temp_frame;

// Exact resolution match.  Don't do colorspace conversion
		if(input->get_color_model() == BC_YUV422 &&
			input->get_w() == 720 &&
			(input->get_h() == 480 ||
			input->get_h() == 576))
		{
			int norm = is_pal ? DV_PAL : DV_NTSC;
			int data_size = is_pal ? DV_PAL_SIZE : DV_NTSC_SIZE;
			temp_frame->allocate_compressed_data(data_size);
			temp_frame->set_compressed_size(data_size);

			dv_write_video(encoder,
				temp_frame->get_data(),
				input->get_rows(),
				BC_YUV422,
				norm);
			ptr = temp_frame;
		}
		else
// Convert resolution and color model before compressing
		{
			if(!temp_frame2)
			{
				int h = input->get_h();
// Default to NTSC if unknown
				if(h != 480 && h != 576) h = 480;

				temp_frame2 = new VFrame(0,
					720,
					h,
					BC_YUV422);
				
			}

			int norm = is_pal ? DV_PAL : DV_NTSC;
			int data_size = is_pal ? DV_PAL_SIZE : DV_NTSC_SIZE;
			temp_frame->allocate_compressed_data(data_size);
			temp_frame->set_compressed_size(data_size);


			cmodel_transfer(temp_frame2->get_rows(), /* Leave NULL if non existent */
				input->get_rows(),
				temp_frame2->get_y(), /* Leave NULL if non existent */
				temp_frame2->get_u(),
				temp_frame2->get_v(),
				input->get_y(), /* Leave NULL if non existent */
				input->get_u(),
				input->get_v(),
				0,        /* Dimensions to capture from input frame */
				0, 
				MIN(temp_frame2->get_w(), input->get_w()),
				MIN(temp_frame2->get_h(), input->get_h()),
				0,       /* Dimensions to project on output frame */
				0, 
				MIN(temp_frame2->get_w(), input->get_w()),
				MIN(temp_frame2->get_h(), input->get_h()),
				input->get_color_model(), 
				BC_YUV422,
				0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
				input->get_bytes_per_line(),       /* For planar use the luma rowspan */
				temp_frame2->get_bytes_per_line());     /* For planar use the luma rowspan */

			dv_write_video(encoder,
				temp_frame->get_data(),
				temp_frame2->get_rows(),
				BC_YUV422,
				norm);



			ptr = temp_frame;
		}
	}
	else
		ptr = input;











// Take over buffer table
	buffer_lock->lock("IEC61883Output::write_frame 1");
	have_video = 1;
// Wait for buffer to become available with timeout
	while(buffer_valid[current_inbuffer] && !result && !interrupted)
	{
		buffer_lock->unlock();
		result = video_lock->timed_lock(BUFFER_TIMEOUT);
		buffer_lock->lock("IEC61883Output::write_frame 2");
	}



// Write buffer if there's room
	if(!buffer_valid[current_inbuffer])
	{
		if(!buffer[current_inbuffer])
		{
			buffer[current_inbuffer] = new char[ptr->get_compressed_size()];
			buffer_size[current_inbuffer] = ptr->get_compressed_size();
		}
		memcpy(buffer[current_inbuffer], ptr->get_data(), ptr->get_compressed_size());
		buffer_valid[current_inbuffer] = 1;
		increment_counter(&current_inbuffer);
	}
	else
// Ignore it if there isn't room.
	{
		;
	}

	buffer_lock->unlock();
	start_lock->unlock();
//printf("IEC61883Output::write_frame 100\n");
}

void IEC61883Output::write_samples(char *data, int samples)
{
//printf("IEC61883Output::write_samples 1\n");
	int result = 0;
	int timeout = (int64_t)samples * 
		(int64_t)1000000 * 
		(int64_t)2 / 
		(int64_t)samplerate;
	if(interrupted) return;

//printf("IEC61883Output::write_samples 2\n");

// Check for maximum sample count exceeded
	if(samples > OUTPUT_SAMPLES)
	{
		printf("IEC61883Output::write_samples samples=%d > OUTPUT_SAMPLES=%d\n",
			samples,
			OUTPUT_SAMPLES);
		return;
	}

//printf("IEC61883Output::write_samples 3\n");
// Take over buffer table
	buffer_lock->lock("IEC61883Output::write_samples 1");
// Wait for buffer to become available with timeout
	while(audio_samples > OUTPUT_SAMPLES - samples && !result && !interrupted)
	{
		buffer_lock->unlock();
		result = audio_lock->timed_lock(BUFFER_TIMEOUT);
		buffer_lock->lock("IEC61883Output::write_samples 2");
	}

	if(!interrupted && audio_samples <= OUTPUT_SAMPLES - samples)
	{
//printf("IEC61883Output::write_samples 4 %d\n", audio_samples);
		memcpy(audio_buffer + audio_samples * channels * bits / 8,
			data,
			samples * channels * bits / 8);
		audio_samples += samples;
	}
	buffer_lock->unlock();
	start_lock->unlock();
//printf("IEC61883Output::write_samples 100\n");
}

long IEC61883Output::get_audio_position()
{
	position_lock->lock("IEC61883Output::get_audio_position");
	long result = audio_position;
	position_lock->unlock();
	return result;
}

void IEC61883Output::interrupt()
{
	interrupted = 1;
// Break write_samples out of a lock
	video_lock->unlock();
	audio_lock->unlock();
// Playback should stop when the object is deleted.
}

void IEC61883Output::flush()
{
	
}

void IEC61883Output::increment_counter(int *counter)
{
	(*counter)++;
	if(*counter >= total_buffers) *counter = 0;
}

void IEC61883Output::decrement_counter(int *counter)
{
	(*counter)--;
	if(*counter < 0) *counter = total_buffers - 1;
}











#endif // HAVE_FIREWIRE






