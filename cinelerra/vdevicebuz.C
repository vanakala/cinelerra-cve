
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

// ALPHA C++ can't compile 64 bit headers
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE
#undef _FILE_OFFSET_BITS

#include "assets.h"
#include "bcsignals.h"
#include "channel.h"
#include "chantables.h"
#include "condition.h"
#include "file.inc"
#include "mutex.h"
#include "picture.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"
#include "strategies.inc"
#include "vdevicebuz.h"
#include "vframe.h"
#include "videoconfig.h"
#include "videodevice.h"

#include <errno.h>
#include <stdint.h>
#include <linux/kernel.h>
//#include <linux/videodev2.h>
#include <linux/videodev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>



#define READ_TIMEOUT 5000000


VDeviceBUZInput::VDeviceBUZInput(VDeviceBUZ *device)
 : Thread(1, 1, 0)
{
	this->device = device;
	buffer = 0;
	buffer_size = 0;
	total_buffers = 0;
	current_inbuffer = 0;
	current_outbuffer = 0;
	done = 0;
	output_lock = new Condition(0, "VDeviceBUZInput::output_lock");
	buffer_lock = new Mutex("VDeviceBUZInput::buffer_lock");
}

VDeviceBUZInput::~VDeviceBUZInput()
{
	if(Thread::running())
	{
		done = 1;
		Thread::cancel();
		Thread::join();
	}

	if(buffer)
	{
		for(int i = 0; i < total_buffers; i++)
		{
			delete [] buffer[i];
		}
		delete [] buffer;
		delete [] buffer_size;
	}
	delete output_lock;
	delete buffer_lock;
}

void VDeviceBUZInput::start()
{
// Create buffers
	total_buffers = device->device->in_config->capture_length;
	buffer = new char*[total_buffers];
	buffer_size = new int[total_buffers];
	bzero(buffer_size, sizeof(int) * total_buffers);
	for(int i = 0; i < total_buffers; i++)
	{
		buffer[i] = new char[INPUT_BUFFER_SIZE];
	}

	Thread::start();
}

void VDeviceBUZInput::run()
{
    struct buz_sync bsync;

// Wait for frame
	while(1)
	{
		Thread::enable_cancel();
		if(ioctl(device->jvideo_fd, BUZIOC_SYNC, &bsync) < 0)
		{
			perror("VDeviceBUZInput::run BUZIOC_SYNC");
			if(done) return;
			Thread::disable_cancel();
		}
		else
		{
			Thread::disable_cancel();



			int new_buffer = 0;
			buffer_lock->lock("VDeviceBUZInput::run");
// Save only if the current buffer is free.
			if(!buffer_size[current_inbuffer])
			{
				new_buffer = 1;
// Copy to input buffer
				memcpy(buffer[current_inbuffer], 
					device->input_buffer + bsync.frame * device->breq.size,
					bsync.length);

// Advance input buffer number and decrease semaphore.
				buffer_size[current_inbuffer] = bsync.length;
				increment_counter(&current_inbuffer);
			}

			buffer_lock->unlock();

			if(ioctl(device->jvideo_fd, BUZIOC_QBUF_CAPT, &bsync.frame))
				perror("VDeviceBUZInput::run BUZIOC_QBUF_CAPT");

			if(new_buffer) output_lock->unlock();
		}
	}
}

void VDeviceBUZInput::get_buffer(char **ptr, int *size)
{
// Increase semaphore to wait for buffer.
	int result = output_lock->timed_lock(READ_TIMEOUT, "VDeviceBUZInput::get_buffer");


// The driver has its own timeout routine but it doesn't work because
// because the tuner lock is unlocked and relocked with no delay.
//	int result = 0;
//	output_lock->lock("VDeviceBUZInput::get_buffer");

	if(!result)
	{
// Take over buffer table
		buffer_lock->lock("VDeviceBUZInput::get_buffer");
		*ptr = buffer[current_outbuffer];
		*size = buffer_size[current_outbuffer];
		buffer_lock->unlock();
	}
	else
	{
//printf("VDeviceBUZInput::get_buffer 1\n");
		output_lock->unlock();
	}
}

void VDeviceBUZInput::put_buffer()
{
	buffer_lock->lock("VDeviceBUZInput::put_buffer");
	buffer_size[current_outbuffer] = 0;
	buffer_lock->unlock();
	increment_counter(&current_outbuffer);
}

void VDeviceBUZInput::increment_counter(int *counter)
{
	(*counter)++;
	if(*counter >= total_buffers) *counter = 0;
}

void VDeviceBUZInput::decrement_counter(int *counter)
{
	(*counter)--;
	if(*counter < 0) *counter = total_buffers - 1;
}















VDeviceBUZ::VDeviceBUZ(VideoDevice *device)
 : VDeviceBase(device)
{
	reset_parameters();
	render_strategies.append(VRENDER_MJPG);
	tuner_lock = new Mutex("VDeviceBUZ::tuner_lock");
}

VDeviceBUZ::~VDeviceBUZ()
{
	close_all();
	delete tuner_lock;
}

int VDeviceBUZ::reset_parameters()
{
	jvideo_fd = 0;
	input_buffer = 0;
	output_buffer = 0;
	frame_buffer = 0;
	frame_size = 0;
	frame_allocated = 0;
	input_error = 0;
	last_frame_no = 0;
	temp_frame = 0;
	user_frame = 0;
	mjpeg = 0;
	total_loops = 0;
	output_number = 0;
	input_thread = 0;
	brightness = 32768;
	hue = 32768;
	color = 32768;
	contrast = 32768;
	whiteness = 32768;
}

int VDeviceBUZ::close_input_core()
{
	if(input_thread)
	{
		delete input_thread;
		input_thread = 0;
	}


	if(device->r)
	{
		if(jvideo_fd) close(jvideo_fd);
		jvideo_fd = 0;
	}

	if(input_buffer)
	{
		if(input_buffer > 0)
			munmap(input_buffer, breq.count * breq.size);
		input_buffer = 0;
	}
}

int VDeviceBUZ::close_output_core()
{
//printf("VDeviceBUZ::close_output_core 1\n");
	if(device->w)
	{
		int n = -1;
// 		if(ioctl(jvideo_fd, BUZIOC_QBUF_PLAY, &n) < 0)
// 			perror("VDeviceBUZ::close_output_core BUZIOC_QBUF_PLAY");
		if(jvideo_fd) close(jvideo_fd);
		jvideo_fd = 0;
	}
	if(output_buffer)
	{
		if(output_buffer > 0)
			munmap(output_buffer, breq.count * breq.size);
		output_buffer = 0;
	}
	if(temp_frame)
	{
		delete temp_frame;
		temp_frame = 0;
	}
	if(mjpeg)
	{
		mjpeg_delete(mjpeg);
		mjpeg = 0;
	}
	if(user_frame)
	{
		delete user_frame;
		user_frame = 0;
	}
//printf("VDeviceBUZ::close_output_core 2\n");
	return 0;
}


int VDeviceBUZ::close_all()
{
//printf("VDeviceBUZ::close_all 1\n");
	close_input_core();
//printf("VDeviceBUZ::close_all 1\n");
	close_output_core();
//printf("VDeviceBUZ::close_all 1\n");
	if(frame_buffer) delete frame_buffer;
//printf("VDeviceBUZ::close_all 1\n");
	reset_parameters();
//printf("VDeviceBUZ::close_all 2\n");
	return 0;
}

#define COMPOSITE_TEXT "Composite"
#define SVIDEO_TEXT "S-Video"
#define BUZ_COMPOSITE 0
#define BUZ_SVIDEO 1

void VDeviceBUZ::get_inputs(ArrayList<Channel*> *input_sources)
{
	Channel *new_source = new Channel;

	new_source = new Channel;
	strcpy(new_source->device_name, COMPOSITE_TEXT);
	input_sources->append(new_source);

	new_source = new Channel;
	strcpy(new_source->device_name, SVIDEO_TEXT);
	input_sources->append(new_source);
}

int VDeviceBUZ::open_input()
{
	device->channel->use_norm = 1;
	device->channel->use_input = 1;

	device->picture->use_brightness = 1;
	device->picture->use_contrast = 1;
	device->picture->use_color = 1;
	device->picture->use_hue = 1;
	device->picture->use_whiteness = 1;

// Can't open input until after the channel is set
	return 0;
}

int VDeviceBUZ::open_output()
{
// Can't open output until after the channel is set
	return 0;
}

int VDeviceBUZ::set_channel(Channel *channel)
{
	if(!channel) return 0;

	tuner_lock->lock("VDeviceBUZ::set_channel");

	if(device->r)
	{
		close_input_core();
		open_input_core(channel);
	}
	else
	{
		close_output_core();
		open_output_core(channel);
	}

	tuner_lock->unlock();


	return 0;
}

void VDeviceBUZ::create_channeldb(ArrayList<Channel*> *channeldb)
{
	;
}

int VDeviceBUZ::set_picture(PictureConfig *picture)
{
	this->brightness = (int)((float)picture->brightness / 100 * 32767 + 32768);
	this->hue = (int)((float)picture->hue / 100 * 32767 + 32768);
	this->color = (int)((float)picture->color / 100 * 32767 + 32768);
	this->contrast = (int)((float)picture->contrast / 100 * 32767 + 32768);
	this->whiteness = (int)((float)picture->whiteness / 100 * 32767 + 32768);


 	tuner_lock->lock("VDeviceBUZ::set_picture");
	if(device->r)
	{
		close_input_core();
		open_input_core(0);
	}
	else
	{
		close_output_core();
		open_output_core(0);
	}
 	tuner_lock->unlock();
// 
// 
// TRACE("VDeviceBUZ::set_picture 1");
// 	tuner_lock->lock("VDeviceBUZ::set_picture");
// TRACE("VDeviceBUZ::set_picture 2");
// 
// 
// 
// 	struct video_picture picture_params;
// // This call takes a long time in 2.4.22
// 	if(ioctl(jvideo_fd, VIDIOCGPICT, &picture_params) < 0)
// 		perror("VDeviceBUZ::set_picture VIDIOCGPICT");
// 	picture_params.brightness = brightness;
// 	picture_params.hue = hue;
// 	picture_params.colour = color;
// 	picture_params.contrast = contrast;
// 	picture_params.whiteness = whiteness;
// // This call takes a long time in 2.4.22
// 	if(ioctl(jvideo_fd, VIDIOCSPICT, &picture_params) < 0)
// 		perror("VDeviceBUZ::set_picture VIDIOCSPICT");
// 	if(ioctl(jvideo_fd, VIDIOCGPICT, &picture_params) < 0)
// 		perror("VDeviceBUZ::set_picture VIDIOCGPICT");
// 
// 
// TRACE("VDeviceBUZ::set_picture 10");
// 
// 
// 	tuner_lock->unlock();

	return 0;
}

int VDeviceBUZ::get_norm(int norm)
{
	switch(norm)
	{
		case NTSC:          return VIDEO_MODE_NTSC;      break;
		case PAL:           return VIDEO_MODE_PAL;       break;
		case SECAM:         return VIDEO_MODE_SECAM;     break;
	}
}

int VDeviceBUZ::read_buffer(VFrame *frame)
{
	tuner_lock->lock("VDeviceBUZ::read_buffer");
	if(!jvideo_fd) open_input_core(0);

// Get buffer from thread
	char *buffer = 0;
	int buffer_size = 0;
	if(input_thread) 
		input_thread->get_buffer(&buffer, &buffer_size);

	if(buffer)
	{
		frame->allocate_compressed_data(buffer_size);
		frame->set_compressed_size(buffer_size);

// Transfer fields to frame
		if(device->odd_field_first)
		{
			long field2_offset = mjpeg_get_field2((unsigned char*)buffer, buffer_size);
			long field1_len = field2_offset;
			long field2_len = buffer_size - field2_offset;

			memcpy(frame->get_data(), buffer + field2_offset, field2_len);
			memcpy(frame->get_data() + field2_len, buffer, field1_len);
		}
		else
		{
			bcopy(buffer, frame->get_data(), buffer_size);
		}

		input_thread->put_buffer();
		tuner_lock->unlock();
	}
	else
	{
		tuner_lock->unlock();
		Timer timer;
// Allow other threads to lock the tuner_lock under NPTL.
		timer.delay(100);
	}


	return 0;
}

int VDeviceBUZ::open_input_core(Channel *channel)
{
	jvideo_fd = open(device->in_config->buz_in_device, O_RDONLY);

	if(jvideo_fd <= 0)
	{
		fprintf(stderr, "VDeviceBUZ::open_input %s: %s\n", 
			device->in_config->buz_in_device,
			strerror(errno));
		jvideo_fd = 0;
		return 1;
	}

// Create input sources
	get_inputs(&device->input_sources);

// Set current input source
	if(channel)
	{
		for(int i = 0; i < 2; i++)
		{
			struct video_channel vch;
			vch.channel = channel->input;
			vch.norm = get_norm(channel->norm);

//printf("VDeviceBUZ::open_input_core 2 %d %d\n", vch.channel, vch.norm);
			if(ioctl(jvideo_fd, VIDIOCSCHAN, &vch) < 0)
				perror("VDeviceBUZ::open_input_core VIDIOCSCHAN ");
		}
	}


// Throw away
//     struct video_capability vc;
// 	if(ioctl(jvideo_fd, VIDIOCGCAP, &vc) < 0)
// 		perror("VDeviceBUZ::open_input VIDIOCGCAP");

// API dependant initialization
	if(ioctl(jvideo_fd, BUZIOC_G_PARAMS, &bparm) < 0)
		perror("VDeviceBUZ::open_input BUZIOC_G_PARAMS");

	bparm.HorDcm = 1;
	bparm.VerDcm = 1;
	bparm.TmpDcm = 1;
	bparm.field_per_buff = 2;
	bparm.img_width = device->in_config->w;
	bparm.img_height = device->in_config->h / bparm.field_per_buff;
	bparm.img_x = 0;
	bparm.img_y = 0;
//	bparm.APPn = 0;
//	bparm.APP_len = 14;
	bparm.APP_len = 0;
	bparm.odd_even = 0;
    bparm.decimation = 0;
    bparm.quality = device->quality;
    bzero(bparm.APP_data, sizeof(bparm.APP_data));

	if(ioctl(jvideo_fd, BUZIOC_S_PARAMS, &bparm) < 0)
		perror("VDeviceBUZ::open_input BUZIOC_S_PARAMS");

// printf("open_input %d %d %d %d %d %d %d %d %d %d %d %d\n", 
// 		bparm.HorDcm,
// 		bparm.VerDcm,
// 		bparm.TmpDcm,
// 		bparm.field_per_buff,
// 		bparm.img_width,
// 		bparm.img_height,
// 		bparm.img_x,
// 		bparm.img_y,
// 		bparm.APP_len,
// 		bparm.odd_even,
//     	bparm.decimation,
//     	bparm.quality);

	breq.count = device->in_config->capture_length;
	breq.size = INPUT_BUFFER_SIZE;
	if(ioctl(jvideo_fd, BUZIOC_REQBUFS, &breq) < 0)
		perror("VDeviceBUZ::open_input BUZIOC_REQBUFS");

//printf("open_input %s %d %d %d %d\n", device->in_config->buz_in_device, breq.count, breq.size, bparm.img_width, bparm.img_height);
	if((input_buffer = (char*)mmap(0, 
		breq.count * breq.size, 
		PROT_READ, 
		MAP_SHARED, 
		jvideo_fd, 
		0)) <= 0)
		perror("VDeviceBUZ::open_input mmap");


// Set picture quality
	struct video_picture picture_params;
// This call takes a long time in 2.4.22
	if(ioctl(jvideo_fd, VIDIOCGPICT, &picture_params) < 0)
		perror("VDeviceBUZ::set_picture VIDIOCGPICT");
	picture_params.brightness = brightness;
	picture_params.hue = hue;
	picture_params.colour = color;
	picture_params.contrast = contrast;
	picture_params.whiteness = whiteness;
// This call takes a long time in 2.4.22
	if(ioctl(jvideo_fd, VIDIOCSPICT, &picture_params) < 0)
		perror("VDeviceBUZ::set_picture VIDIOCSPICT");
	if(ioctl(jvideo_fd, VIDIOCGPICT, &picture_params) < 0)
		perror("VDeviceBUZ::set_picture VIDIOCGPICT");




// Start capturing
	for(int i = 0; i < breq.count; i++)
	{
        if(ioctl(jvideo_fd, BUZIOC_QBUF_CAPT, &i) < 0)
			perror("VDeviceBUZ::open_input BUZIOC_QBUF_CAPT");
	}


	input_thread = new VDeviceBUZInput(this);
	input_thread->start();
//printf("VDeviceBUZ::open_input_core 2\n");
	return 0;
}

int VDeviceBUZ::open_output_core(Channel *channel)
{
//printf("VDeviceBUZ::open_output 1\n");
	total_loops = 0;
	output_number = 0;
	jvideo_fd = open(device->out_config->buz_out_device, O_RDWR);
	if(jvideo_fd <= 0)
	{
		perror("VDeviceBUZ::open_output");
		return 1;
	}


// Set current input source
	if(channel)
	{
		struct video_channel vch;
		vch.channel = channel->input;
		vch.norm = get_norm(channel->norm);

		if(ioctl(jvideo_fd, VIDIOCSCHAN, &vch) < 0)
			perror("VDeviceBUZ::open_output_core VIDIOCSCHAN ");
	}

	breq.count = 10;
	breq.size = INPUT_BUFFER_SIZE;
	if(ioctl(jvideo_fd, BUZIOC_REQBUFS, &breq) < 0)
		perror("VDeviceBUZ::open_output BUZIOC_REQBUFS");
	if((output_buffer = (char*)mmap(0, 
		breq.count * breq.size, 
		PROT_READ | PROT_WRITE, 
		MAP_SHARED, 
		jvideo_fd, 
		0)) <= 0)
		perror("VDeviceBUZ::open_output mmap");

	if(ioctl(jvideo_fd, BUZIOC_G_PARAMS, &bparm) < 0)
		perror("VDeviceBUZ::open_output BUZIOC_G_PARAMS");

	bparm.decimation = 1;
	bparm.HorDcm = 1;
	bparm.field_per_buff = 2;
	bparm.TmpDcm = 1;
	bparm.VerDcm = 1;
	bparm.img_width = device->out_w;
	bparm.img_height = device->out_h / bparm.field_per_buff;
	bparm.img_x = 0;
	bparm.img_y = 0;
	bparm.odd_even = 0;

	if(ioctl(jvideo_fd, BUZIOC_S_PARAMS, &bparm) < 0)
		perror("VDeviceBUZ::open_output BUZIOC_S_PARAMS");
//printf("VDeviceBUZ::open_output 2\n");
	return 0;
}



int VDeviceBUZ::write_buffer(VFrame *frame, EDL *edl)
{
//printf("VDeviceBUZ::write_buffer 1\n");
	tuner_lock->lock("VDeviceBUZ::write_buffer");

	if(!jvideo_fd) open_output_core(0);

	VFrame *ptr = 0;
	if(frame->get_color_model() != BC_COMPRESSED)
	{
		if(!temp_frame) temp_frame = new VFrame;
		if(!mjpeg)
		{
			mjpeg = mjpeg_new(device->out_w, device->out_h, 2);
			mjpeg_set_quality(mjpeg, device->quality);
			mjpeg_set_float(mjpeg, 0);
		}
		ptr = temp_frame;
		mjpeg_compress(mjpeg, 
			frame->get_rows(), 
			frame->get_y(), 
			frame->get_u(), 
			frame->get_v(),
			frame->get_color_model(),
			device->cpus);
		temp_frame->allocate_compressed_data(mjpeg_output_size(mjpeg));
		temp_frame->set_compressed_size(mjpeg_output_size(mjpeg));
		bcopy(mjpeg_output_buffer(mjpeg), temp_frame->get_data(), mjpeg_output_size(mjpeg));
	}
	else
		ptr = frame;

// Wait for frame to become available
// Caused close_output_core to lock up.
// 	if(total_loops >= 1)
// 	{
// 		if(ioctl(jvideo_fd, BUZIOC_SYNC, &output_number) < 0)
// 			perror("VDeviceBUZ::write_buffer BUZIOC_SYNC");
// 	}

	if(device->out_config->buz_swap_fields)
	{
		long field2_offset = mjpeg_get_field2((unsigned char*)ptr->get_data(), 
			ptr->get_compressed_size());
		long field2_len = ptr->get_compressed_size() - field2_offset;
		memcpy(output_buffer + output_number * breq.size, 
			ptr->get_data() + field2_offset, 
			field2_len);
		memcpy(output_buffer + output_number * breq.size +field2_len,
			ptr->get_data(), 
			field2_offset);
	}
	else
	{
		bcopy(ptr->get_data(), 
			output_buffer + output_number * breq.size, 
			ptr->get_compressed_size());
	}

	if(ioctl(jvideo_fd, BUZIOC_QBUF_PLAY, &output_number) < 0)
		perror("VDeviceBUZ::write_buffer BUZIOC_QBUF_PLAY");

	output_number++;
	if(output_number >= breq.count)
	{
		output_number = 0;
		total_loops++;
	}
	tuner_lock->unlock();
//printf("VDeviceBUZ::write_buffer 2\n");

	return 0;
}

void VDeviceBUZ::new_output_buffer(VFrame *output,
	int colormodel)
{
//printf("VDeviceBUZ::new_output_buffer 1 %d\n", colormodel);
	if(user_frame)
	{
		if(colormodel != user_frame->get_color_model())
		{
			delete user_frame;
			user_frame = 0;
		}
	}

	if(!user_frame)
	{
		switch(colormodel)
		{
			case BC_COMPRESSED:
				user_frame = new VFrame;
				break;
			default:
				user_frame = new VFrame(0,
					device->out_w,
					device->out_h,
					colormodel,
					-1);
				break;
		}
	}
	user_frame->set_shm_offset(0);
	output = user_frame;
//printf("VDeviceBUZ::new_output_buffer 2\n");
}


ArrayList<int>* VDeviceBUZ::get_render_strategies()
{
	return &render_strategies;
}

