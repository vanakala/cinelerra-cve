
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
#include "device1394output.h"
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


Device1394Output::Device1394Output(AudioDevice *adevice)
 : Thread(1, 0, 0)
{
	reset();
	this->adevice = adevice;

	set_ioctls();
}

Device1394Output::Device1394Output(VideoDevice *vdevice)
 : Thread(1, 0, 0)
{
	reset();
	this->vdevice = vdevice;

	set_ioctls();
}

Device1394Output::~Device1394Output()
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

	if(output_fd >= 0)
	{
        output_queue.buffer = (output_mmap.nb_buffers + output_queue.buffer - 1) % output_mmap.nb_buffers;

		if(get_dv1394())
		{
  			if(ioctl(output_fd, DV1394_IOC_WAIT_FRAMES, status.init.n_frames - 1) < 0)
  			{
  				fprintf(stderr,
  					"Device1394Output::close_all: DV1394_WAIT_FRAMES %i: %s",
  					output_mmap.nb_buffers,
  					strerror(errno));
  			}
  			munmap(output_buffer, status.init.n_frames *
				(is_pal ? DV1394_PAL_FRAME_SIZE : DV1394_NTSC_FRAME_SIZE));
  			if(ioctl(output_fd, DV1394_IOC_SHUTDOWN, NULL) < 0)
  			{
  				perror("Device1394Output::close_all: DV1394_SHUTDOWN");
  			}
		}
		else
		{
        	if(ioctl(output_fd, video1394_talk_wait_buffer, &output_queue) < 0) 
			{
            	fprintf(stderr, 
					"Device1394::close_all: VIDEO1394_TALK_WAIT_BUFFER %s: %s",
					output_queue,
					strerror(errno));
        	}
        	munmap(output_buffer, output_mmap.nb_buffers * output_mmap.buf_size);

        	if(ioctl(output_fd, video1394_untalk_channel, &output_mmap.channel) < 0)
			{
            	perror("Device1394::close_all: VIDEO1394_UNTALK_CHANNEL");
        	}
		}

        close(output_fd);

//		if(avc_handle)
//			raw1394_destroy_handle(avc_handle);
	}

	if(temp_frame) delete temp_frame;
	if(temp_frame2) delete temp_frame2;
	if(video_encoder) dv_delete(video_encoder);
	if(position_presented) delete [] position_presented;
	if(audio_encoder) dv_delete(audio_encoder);
	if(buffer_lock) delete buffer_lock;
	if(position_lock) delete position_lock;
}


void Device1394Output::reset()
{
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
	output_fd = -1;
//	avc_handle = 0;
	temp_frame = 0;
	temp_frame2 = 0;
	audio_position = 0;
	interrupted = 0;
	position_presented = 0;
	have_video = 0;
	adevice = 0;
	vdevice = 0;
	is_pal = 0;
}

int Device1394Output::get_dv1394()
{
	if(adevice) return adevice->out_config->driver == AUDIO_DV1394;
	if(vdevice) return vdevice->out_config->driver == PLAYBACK_DV1394;
}

int Device1394Output::open(char *path,
	int port,
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
	if (get_dv1394())
	{
// dv1394 syt is given in frames and limited to 2 or 3, default is 3
// Needs to be accurate to calculate presentation time
		if (syt < 2 || syt > 3)
			syt = 3;
	}
	this->syt = syt;

// Set PAL mode based on frame height
	if(vdevice) is_pal = (vdevice->out_h == 576);

    struct dv1394_init setup = 
	{
       api_version: DV1394_API_VERSION,
       channel:     channel,
  	   n_frames:    length,
       format:      is_pal ? DV1394_PAL : DV1394_NTSC,
       cip_n:       0,
       cip_d:       0,
       syt_offset:  syt
    };

	

//printf("Device1394::open_output 2 %s %d %d %d %d\n", path, port, channel, length, syt);
	if(output_fd < 0)
	{
    	output_fd = ::open(path, O_RDWR);

		if(output_fd <= 0)
		{
			fprintf(stderr, 
				"Device1394Output::open path=%s: %s\n", 
				path,
				strerror(errno));
			return 1;
		}
		else
		{
        	output_mmap.channel = channel;
        	output_queue.channel = channel;
        	output_mmap.sync_tag = 0;
        	output_mmap.nb_buffers = total_buffers;
        	output_mmap.buf_size = 320 * 512;
        	output_mmap.packet_size = 512;
// Shouldn't this be handled by the video1394 driver?
// dvgrab originally used 19000
// JVC DVL300 -> 30000
        	output_mmap.syt_offset = syt;
        	output_mmap.flags = VIDEO1394_VARIABLE_PACKET_SIZE;



			if(get_dv1394())
			{
  				if(ioctl(output_fd, DV1394_IOC_INIT, &setup) < 0)
  				{
  					perror("Device1394Output::open DV1394_INIT");
  				}

  				if(ioctl(output_fd, DV1394_IOC_GET_STATUS, &status) < 0)
  				{
  					perror("Device1394Output::open DV1394_GET_STATUS");
  				}

           		output_buffer = (unsigned char*)mmap(0,
              		output_mmap.nb_buffers * (is_pal ? DV1394_PAL_FRAME_SIZE : DV1394_NTSC_FRAME_SIZE),
                 	PROT_READ | PROT_WRITE,
              		MAP_SHARED,
              		output_fd,
              		0);

				if(position_presented) delete [] position_presented;
				position_presented = new long[length];
				for (int i = 0; i < length; i++)
					position_presented[i] = 0;
			}
			else
			{
        		if(ioctl(output_fd, video1394_talk_channel, &output_mmap) < 0)
				{
            		perror("Device1394Output::open VIDEO1394_TALK_CHANNEL:");
        		}

        		output_buffer = (unsigned char*)mmap(0, 
					output_mmap.nb_buffers * output_mmap.buf_size,
            		PROT_READ | PROT_WRITE, 
					MAP_SHARED, 
					output_fd, 
					0);
			}

        	if(output_buffer <= 0)
			{
            	perror("Device1394Output::open mmap");
        	}

			unused_buffers = output_mmap.nb_buffers;
        	output_queue.buffer = 0;
        	output_queue.packet_sizes = packet_sizes;
        	continuity_counter = 0;
        	cip_counter = 0;

// Create buffers
			buffer = new char*[total_buffers];
			for(int i = 0; i < length; i++)
				buffer[i] = new char[get_dv1394() ?
					(is_pal ? DV1394_PAL_FRAME_SIZE : DV1394_NTSC_FRAME_SIZE) :
					DV_PAL_SIZE];
			buffer_size = new int[total_buffers];
			buffer_valid = new int[total_buffers];
			bzero(buffer_size, sizeof(int) * total_buffers);
			bzero(buffer_valid, sizeof(int) * total_buffers);
			bzero(buffer, sizeof(char*) * total_buffers);
			video_lock = new Condition(0, "Device1394Output::video_lock");
			audio_lock = new Condition(0, "Device1394Output::audio_lock");
			start_lock = new Condition(0, "Device1394Output::start_lock");
			buffer_lock = new Mutex("Device1394Output::buffer_lock");
			position_lock = new Mutex("Device1394Output::position_lock");
			encoder = dv_new();
			audio_buffer = new char[OUTPUT_SAMPLES * channels * bits / 8];
			Thread::start();
    	}
	}
	return 0;
}

void Device1394Output::run()
{
  	unsigned char *output;
  	char *out_buffer;
  	int out_size;

	Thread::enable_cancel();
	start_lock->lock("Device1394Output::run");
	Thread::disable_cancel();

//Timer timer;
// Write buffers continuously
	while(!done)
	{
// Get current buffer to play
		if(done) return;

		buffer_lock->lock("Device1394Output::run 1");

		out_buffer = buffer[current_outbuffer];
		out_size = buffer_size[current_outbuffer];





// No video.  Put in a fake frame for audio only
		if(!have_video)
		{
#include "data/fake_ntsc_dv.h"
			out_size = sizeof(fake_ntsc_dv) - 4;
			out_buffer = (char*)fake_ntsc_dv + 4;
		}




  		if(get_dv1394())
  		{
  			output = output_buffer + 
  				out_size *
  				status.first_clear_frame;
  		}
  		else
  		{
			output = output_buffer + 
				output_queue.buffer * 
				output_mmap.buf_size;
		}








// Got a buffer
		if(out_buffer && out_size)
		{
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
				position_lock->lock("Device1394Output::run");

				if (get_dv1394())
				{
// When this frame is being uploaded to the 1394 device,
// the frame actually playing on the device will be the one
// uploaded syt frames before.
					position_presented[status.first_clear_frame] = 
						audio_position - syt * samples_per_frame;
					if (position_presented[status.first_clear_frame] < 0)
						position_presented[status.first_clear_frame] = 0;
				}

				audio_position += samples_written;
				position_lock->unlock();


				audio_lock->unlock();
			}

// Copy from current buffer to mmap buffer with firewire encryption
			if(get_dv1394())
			{
				memcpy(output, out_buffer, out_size);
			}
			else
			{
				encrypt((unsigned char*)output, 
					(unsigned char*)out_buffer, 
					out_size);
			}
			buffer_valid[current_outbuffer] = 0;
		}

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
//printf("Device1394Output::run 100\n");



		if(out_size > 0)
		{

// Write mmap to device
			Thread::enable_cancel();
			unused_buffers--;


			if(get_dv1394())
			{
  				if(ioctl(output_fd, DV1394_IOC_SUBMIT_FRAMES, 1) < 0)
  				{
  					perror("Device1394Output::run DV1394_SUBMIT_FRAMES");
  				}
 				if(ioctl(output_fd, DV1394_IOC_WAIT_FRAMES, 1) < 0)
				{
					perror("Device1394Output::run DV1394_WAIT_FRAMES");
				}
  				if(ioctl(output_fd, DV1394_IOC_GET_STATUS, &status) < 0)
  				{
  					perror("Device1394Output::run DV1394_GET_STATUS");
  				}
			}
			else
			{
				if(ioctl(output_fd, video1394_talk_queue_buffer, &output_queue) < 0)
				{
        			perror("Device1394Output::run VIDEO1394_TALK_QUEUE_BUFFER");
    			}
			}

    		output_queue.buffer++;
			if(output_queue.buffer >= output_mmap.nb_buffers) 
				output_queue.buffer = 0;

			if(unused_buffers <= 0)
			{
				if(!get_dv1394())
				{
    				if(ioctl(output_fd, video1394_talk_wait_buffer, &output_queue) < 0) 
					{
        				perror("Device1394::run VIDEO1394_TALK_WAIT_BUFFER");
    				}
				}
				unused_buffers++;
			}


			Thread::disable_cancel();
		}
		else
		{
// 			Thread::enable_cancel();
// 			start_lock->lock();
// 			Thread::disable_cancel();
		}

//printf("Device1394Output::run %lld\n", timer.get_difference());
	}
}



void Device1394Output::encrypt(unsigned char *output, 
	unsigned char *data, 
	int data_size)
{
// Encode in IEEE1394 video encryption
	int output_size = 320;
	int packets_per_frame = (is_pal ? 300 : 250);
	int min_packet_size = output_mmap.packet_size;
	unsigned long frame_size = packets_per_frame * 480;
	unsigned long vdata = 0;
	unsigned int *packet_sizes = this->packet_sizes;

    if(cip_counter == 0) 
	{
        if(!is_pal) 
		{
            cip_n = CIP_N_NTSC;
            cip_d = CIP_D_NTSC;
            f50_60 = 0x00;
        }
		else 
		{
            cip_n = CIP_N_PAL;
            cip_d = CIP_D_PAL;
            f50_60 = 0x80;
        }
        cip_counter = cip_n;
    }




	for(int i = 0; i < output_size && vdata < frame_size; i++)
	{
        unsigned char *p = output;
        int want_sync = (cip_counter > cip_d);

/* Source node ID ! */
       	*p++ = 0x01; 
/* Packet size in quadlets (480 / 4) - this stays the same even for empty packets */
      	*p++ = 0x78; 
      	*p++ = 0x00;
     	*p++ = continuity_counter;

/* const */
        *p++ = 0x80; 
/* high bit = 50/60 indicator */
        *p++ = f50_60; 

/* timestamp - generated in driver */
        *p++ = 0xff; 
/* timestamp */
        *p++ = 0xff; 

/* video data */
        if(!want_sync)
		{
            continuity_counter++;
            cip_counter += cip_n;

            memcpy(p, data + vdata, 480);
            p += 480;
            vdata += 480;
        }
        else
            cip_counter -= cip_d;

        *packet_sizes++ = p - output;
        output += min_packet_size;
	}
   	*packet_sizes++ = 0;
}




void Device1394Output::write_frame(VFrame *input)
{
	VFrame *ptr = 0;
	int result = 0;

//printf("Device1394Output::write_frame 1\n");

	if(output_fd <= 0) return;
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
	buffer_lock->lock("Device1394Output::write_frame 1");
	have_video = 1;
// Wait for buffer to become available with timeout
	while(buffer_valid[current_inbuffer] && !result && !interrupted)
	{
		buffer_lock->unlock();
		result = video_lock->timed_lock(BUFFER_TIMEOUT);
		buffer_lock->lock("Device1394Output::write_frame 2");
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
//printf("Device1394Output::write_frame 100\n");
}

void Device1394Output::write_samples(char *data, int samples)
{
//printf("Device1394Output::write_samples 1\n");
	int result = 0;
	int timeout = (int64_t)samples * 
		(int64_t)1000000 * 
		(int64_t)2 / 
		(int64_t)samplerate;
	if(interrupted) return;

//printf("Device1394Output::write_samples 2\n");

// Check for maximum sample count exceeded
	if(samples > OUTPUT_SAMPLES)
	{
		printf("Device1394Output::write_samples samples=%d > OUTPUT_SAMPLES=%d\n",
			samples,
			OUTPUT_SAMPLES);
		return;
	}

//printf("Device1394Output::write_samples 3\n");
// Take over buffer table
	buffer_lock->lock("Device1394Output::write_samples 1");
// Wait for buffer to become available with timeout
	while(audio_samples > OUTPUT_SAMPLES - samples && !result && !interrupted)
	{
		buffer_lock->unlock();
		result = audio_lock->timed_lock(BUFFER_TIMEOUT);
		buffer_lock->lock("Device1394Output::write_samples 2");
	}

	if(!interrupted && audio_samples <= OUTPUT_SAMPLES - samples)
	{
//printf("Device1394Output::write_samples 4 %d\n", audio_samples);
		memcpy(audio_buffer + audio_samples * channels * bits / 8,
			data,
			samples * channels * bits / 8);
		audio_samples += samples;
	}
	buffer_lock->unlock();
	start_lock->unlock();
//printf("Device1394Output::write_samples 100\n");
}

long Device1394Output::get_audio_position()
{
	position_lock->lock("Device1394Output::get_audio_position");
	long result = audio_position;
	if (get_dv1394())
	{
// Take delay between placing in buffer and presentation 
// on device into account for dv1394
		result = position_presented[status.active_frame];
	}
	position_lock->unlock();
	return result;
}

void Device1394Output::interrupt()
{
	interrupted = 1;
// Break write_samples out of a lock
	video_lock->unlock();
	audio_lock->unlock();
// Playback should stop when the object is deleted.
}

void Device1394Output::flush()
{
	
}

void Device1394Output::increment_counter(int *counter)
{
	(*counter)++;
	if(*counter >= total_buffers) *counter = 0;
}

void Device1394Output::decrement_counter(int *counter)
{
	(*counter)--;
	if(*counter < 0) *counter = total_buffers - 1;
}

void Device1394Output::set_ioctls()
{
	// It would make sense to simply change the file that is included in
	// order to change the IOCTLs, but it isn't reasonable to think that
	// people will rebuild their software every time the update their
	// kernel, hence this fix.

	struct utsname buf;

	// Get the kernel version so we can set the right ioctls
	uname(&buf);

	char *ptr = strchr(buf.release, '.');
	int major = 2;
	int minor = 6;
	int point = 7;
	if(ptr)
	{
		*ptr++ = 0;
		major = atoi(buf.release);
		char *ptr2 = strchr(ptr, '.');
		if(ptr2)
		{
			*ptr2++ = 0;
			minor = atoi(ptr);
			point = atoi(ptr2);
		}
	}

	if(major >= 2 && minor >= 6 && point >= 0 ||
		major >= 2 && minor >= 4 && point >= 23)
	{
		// video1394
		video1394_listen_channel = VIDEO1394_IOC_LISTEN_CHANNEL;
		video1394_unlisten_channel = VIDEO1394_IOC_UNLISTEN_CHANNEL;
		video1394_listen_queue_buffer = VIDEO1394_IOC_LISTEN_QUEUE_BUFFER;
		video1394_listen_wait_buffer = VIDEO1394_IOC_LISTEN_WAIT_BUFFER;
		video1394_talk_channel = VIDEO1394_IOC_TALK_CHANNEL;
		video1394_untalk_channel = VIDEO1394_IOC_UNTALK_CHANNEL;
		video1394_talk_queue_buffer = VIDEO1394_IOC_TALK_QUEUE_BUFFER;
		video1394_talk_wait_buffer = VIDEO1394_IOC_TALK_WAIT_BUFFER;
		video1394_listen_poll_buffer = VIDEO1394_IOC_LISTEN_POLL_BUFFER;

		// raw1394
		// Nothing uses this right now, so I didn't include it.
	}
	else	 // we are using an older kernel
	{
      // video1394
      video1394_listen_channel = VIDEO1394_LISTEN_CHANNEL;
      video1394_unlisten_channel = VIDEO1394_UNLISTEN_CHANNEL;
      video1394_listen_queue_buffer = VIDEO1394_LISTEN_QUEUE_BUFFER;
      video1394_listen_wait_buffer = VIDEO1394_LISTEN_WAIT_BUFFER;
      video1394_talk_channel = VIDEO1394_TALK_CHANNEL;
      video1394_untalk_channel = VIDEO1394_UNTALK_CHANNEL;
      video1394_talk_queue_buffer = VIDEO1394_TALK_QUEUE_BUFFER;
      video1394_talk_wait_buffer = VIDEO1394_TALK_WAIT_BUFFER;
      video1394_listen_poll_buffer = VIDEO1394_LISTEN_POLL_BUFFER;
	}
}












#endif // HAVE_FIREWIRE






