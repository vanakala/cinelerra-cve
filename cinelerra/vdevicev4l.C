
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

// V4L2 is incompatible with large file support
// ALPHA C++ can't compile 64 bit headers
#undef _FILE_OFFSET_BITS
#undef _LARGEFILE_SOURCE
#undef _LARGEFILE64_SOURCE


#include "assets.h"
#include "bcsignals.h"
#include "channel.h"
#include "chantables.h"
#include "clip.h"
#include "file.h"
#include "picture.h"
#include "preferences.h"
#include "quicktime.h"
#include "recordconfig.h"
#include "vdevicev4l.h"
#include "vframe.h"
#include "videodevice.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

VDeviceV4L::VDeviceV4L(VideoDevice *device)
 : VDeviceBase(device)
{
	initialize();
}

VDeviceV4L::~VDeviceV4L()
{
}

int VDeviceV4L::initialize()
{
	capture_buffer = 0;
	capture_frame_number = 0;
	read_frame_number = 0;
	shared_memory = 0;
	initialization_complete = 0;
	return 0;
}

int VDeviceV4L::open_input()
{
	device->channel->use_frequency = 1;
	device->channel->use_fine = 1;
	device->channel->use_norm = 1;
	device->channel->use_input = 1;


	device->picture->use_brightness = 1;
	device->picture->use_contrast = 1;
	device->picture->use_color = 1;
	device->picture->use_hue = 1;
	device->picture->use_whiteness = 1;

	if((input_fd = open(device->in_config->v4l_in_device, O_RDWR)) < 0)
	{
		perror("VDeviceV4L::open_input");
		return 1;
	}
	else
	{
		v4l1_get_inputs();
		close(input_fd);
	}
	return 0;
}

int VDeviceV4L::close_all()
{
	close_v4l();
	return 0;
}

int VDeviceV4L::close_v4l()
{
	unmap_v4l_shmem();
	if(input_fd != -1) close(input_fd);
	return 0;
}

int VDeviceV4L::unmap_v4l_shmem()
{
	if(capture_buffer)
	{
		if(shared_memory)
			munmap(capture_buffer, capture_params.size);
		else
			delete capture_buffer;
		capture_buffer = 0;
	}
	return 0;
}

int VDeviceV4L::v4l_init()
{
	int i;

	input_fd = open(device->in_config->v4l_in_device, O_RDWR);

	if(input_fd < 0)
		perror("VDeviceV4L::v4l_init");
	else
	{
		set_cloexec_flag(input_fd, 1);
		set_mute(0);
		if(ioctl(input_fd, VIDIOCGWIN, &window_params) < 0)
			perror("VDeviceV4L::v4l_init VIDIOCGWIN");
		window_params.x = 0;
		window_params.y = 0;
		window_params.width = device->in_config->w;
		window_params.height = device->in_config->h;
		window_params.chromakey = 0;
		window_params.flags = 0;
		window_params.clipcount = 0;
		if(ioctl(input_fd, VIDIOCSWIN, &window_params) < 0)
			perror("VDeviceV4L::v4l_init VIDIOCSWIN");
		if(ioctl(input_fd, VIDIOCGWIN, &window_params) < 0)
			perror("VDeviceV4L::v4l_init VIDIOCGWIN");

		device->in_config->w = window_params.width;
		device->in_config->h = window_params.height;

		PictureConfig picture(0);
		set_picture(&picture);

		if(ioctl(input_fd, VIDIOCGMBUF, &capture_params) < 0)
			perror("VDeviceV4L::v4l_init VIDIOCGMBUF");

		capture_buffer = (char*)mmap(0, 
			capture_params.size, 
			PROT_READ|PROT_WRITE, 
			MAP_SHARED, 
			input_fd, 
			0);

		capture_frame_number = 0;

		if(capture_buffer == MAP_FAILED)
		{
// Use read instead.
			perror("VDeviceV4L::v4l_init mmap");
			shared_memory = 0;
			capture_buffer = new char[capture_params.size];
		}
		else
		{
// Get all frames capturing
			shared_memory = 1;
		}
	}
	got_first_frame = 0;
	return 0;
}

void VDeviceV4L::v4l1_start_capture()
{
	for(int i = 0; i < MIN(capture_params.frames, device->in_config->capture_length); i++)
		capture_frame(i);
}








int VDeviceV4L::v4l1_get_inputs()
{
	struct video_channel channel_struct;
	int i = 0, done = 0;
	char *new_source;

	while(!done && i < 20)
	{
		channel_struct.channel = i;
		if(ioctl(input_fd, VIDIOCGCHAN, &channel_struct) < 0)
		{
// Finished
			done = 1;
		}
		else
		{
			Channel *channel = new Channel;
			strcpy(channel->device_name, channel_struct.name);
			device->input_sources.append(channel);
		}
		i++;
	}
	return 0;
}

int VDeviceV4L::set_mute(int muted)
{
// Open audio, which obviously is controlled by the video driver.
// and apparently resets the input source.
	v4l1_set_mute(muted);
}

int VDeviceV4L::v4l1_set_mute(int muted)
{
	struct video_audio audio;

    if(ioctl(input_fd, VIDIOCGAUDIO, &audio))
	if(ioctl(input_fd, VIDIOCGAUDIO, &audio) < 0)
	    perror("VDeviceV4L::ioctl VIDIOCGAUDIO");

	audio.volume = 65535;
	audio.bass = 65535;
	audio.treble = 65535;
	if(muted)
		audio.flags |= VIDEO_AUDIO_MUTE | VIDEO_AUDIO_VOLUME;
	else
		audio.flags &= ~VIDEO_AUDIO_MUTE;

    if(ioctl(input_fd, VIDIOCSAUDIO, &audio) < 0)
		perror("VDeviceV4L::ioctl VIDIOCSAUDIO");
	return 0;
}


int VDeviceV4L::set_cloexec_flag(int desc, int value)
{
	int oldflags = fcntl(desc, F_GETFD, 0);
	if(oldflags < 0) return oldflags;
	if(value != 0) 
		oldflags |= FD_CLOEXEC;
	else
		oldflags &= ~FD_CLOEXEC;
	return fcntl(desc, F_SETFD, oldflags);
}





int VDeviceV4L::get_best_colormodel(Asset *asset)
{
	int result = BC_RGB888;

// Get best colormodel for hardware acceleration

	result = File::get_best_colormodel(asset, device->in_config->driver);


// Need to get color model before opening device but don't call this
// unless you want to open the device either.
	if(!initialization_complete)
	{
		device_colormodel = translate_colormodel(result);
		this->colormodel = result;
		v4l_init();
		initialization_complete = 1;
	}
// printf("VDeviceV4L::get_best_colormodel %c%c%c%c\n", 
// 	((char*)&device_colormodel)[0],
// 	((char*)&device_colormodel)[1],
// 	((char*)&device_colormodel)[2],
// 	((char*)&device_colormodel)[3]);
	return result;
}

unsigned long VDeviceV4L::translate_colormodel(int colormodel)
{
	unsigned long result = 0;
	switch(colormodel)
	{
		case BC_YUV422:      result = VIDEO_PALETTE_YUV422;      break;
		case BC_YUV420P:     result = VIDEO_PALETTE_YUV420P;     break;
		case BC_YUV422P:     result = VIDEO_PALETTE_YUV422P;     break;
		case BC_YUV411P:     result = VIDEO_PALETTE_YUV411P;     break;
		case BC_RGB888:      result = VIDEO_PALETTE_RGB24;       break;
		default: result = VIDEO_PALETTE_RGB24; break;
	}
//printf("VDeviceV4L::translate_colormodel %d\n", result);
	return result;
}

int VDeviceV4L::set_channel(Channel *channel)
{
	return v4l1_set_channel(channel);
}

int VDeviceV4L::v4l1_set_channel(Channel *channel)
{
	struct video_channel channel_struct;
	struct video_tuner tuner_struct;
	unsigned long new_freq;

// Mute changed the input to TV
//	set_mute(1);

//printf("VDeviceV4L::v4l1_set_channel 1 %d\n", channel->input);
// Read norm/input defaults
	channel_struct.channel = channel->input;
	if(ioctl(input_fd, VIDIOCGCHAN, &channel_struct) < 0)
		perror("VDeviceV4L::v4l1_set_channel VIDIOCGCHAN");

// Set norm/input
	channel_struct.channel = channel->input;
	channel_struct.norm = v4l1_get_norm(channel->norm);
	if(ioctl(input_fd, VIDIOCSCHAN, &channel_struct) < 0)
		perror("VDeviceV4L::v4l1_set_channel VIDIOCSCHAN");

	if(channel_struct.flags & VIDEO_VC_TUNER)
	{
// Read tuner defaults
		tuner_struct.tuner = channel->input;
		if(ioctl(input_fd, VIDIOCGTUNER, &tuner_struct) < 0)
			perror("VDeviceV4L::v4l1_set_channel VIDIOCGTUNER");

// Set tuner
		tuner_struct.mode = v4l1_get_norm(channel->norm);
		if(ioctl(input_fd, VIDIOCSTUNER, &tuner_struct) < 0)
			perror("VDeviceV4L::v4l1_set_channel VIDIOCSTUNER");

		new_freq = chanlists[channel->freqtable].list[channel->entry].freq;
		new_freq = (int)(new_freq * 0.016);
		new_freq += channel->fine_tune;

		if(ioctl(input_fd, VIDIOCSFREQ, &new_freq) < 0)
			perror("VDeviceV4L::v4l1_set_channel VIDIOCSFREQ");
	}
//	set_mute(0);
	return 0;
}

int VDeviceV4L::v4l1_get_norm(int norm)
{
	switch(norm)
	{
		case NTSC:         return VIDEO_MODE_NTSC;         break;
		case PAL:          return VIDEO_MODE_PAL;          break;
		case SECAM:        return VIDEO_MODE_SECAM;        break;
	}
	return 0;
}

int VDeviceV4L::set_picture(PictureConfig *picture)
{
	v4l1_set_picture(picture);
}


int VDeviceV4L::v4l1_set_picture(PictureConfig *picture)
{
	int brightness = (int)((float)picture->brightness / 100 * 32767 + 32768);
	int hue = (int)((float)picture->hue / 100 * 32767 + 32768);
	int color = (int)((float)picture->color / 100 * 32767 + 32768);
	int contrast = (int)((float)picture->contrast / 100 * 32767 + 32768);
	int whiteness = (int)((float)picture->whiteness / 100 * 32767 + 32768);

	if(ioctl(input_fd, VIDIOCGPICT, &picture_params) < 0)
		perror("VDeviceV4L::v4l1_set_picture VIDIOCGPICT");
	picture_params.brightness = brightness;
	picture_params.hue = hue;
	picture_params.colour = color;
	picture_params.contrast = contrast;
	picture_params.whiteness = whiteness;
// Bogus.  Values are only set in the capture routine.
	picture_params.depth = 3;
	picture_params.palette = device_colormodel;
	if(ioctl(input_fd, VIDIOCSPICT, &picture_params) < 0)
		perror("VDeviceV4L::v4l1_set_picture VIDIOCSPICT");
	if(ioctl(input_fd, VIDIOCGPICT, &picture_params) < 0)
		perror("VDeviceV4L::v4l1_set_picture VIDIOCGPICT");
	return 0;
}


int VDeviceV4L::capture_frame(int capture_frame_number)
{
	struct video_mmap params;
	params.frame = capture_frame_number;
	params.width = device->in_config->w;
	params.height = device->in_config->h;
// Required to actually set the palette.
	params.format = device_colormodel;
// Tells the driver the buffer is available for writing
	if(ioctl(input_fd, VIDIOCMCAPTURE, &params) < 0)
		perror("VDeviceV4L::capture_frame VIDIOCMCAPTURE");
	return 0;
}

int VDeviceV4L::wait_v4l_frame()
{
//printf("VDeviceV4L::wait_v4l_frame 1 %d\n", capture_frame_number);
	if(ioctl(input_fd, VIDIOCSYNC, &capture_frame_number))
		perror("VDeviceV4L::wait_v4l_frame VIDIOCSYNC");
//printf("VDeviceV4L::wait_v4l_frame 2 %d\n", capture_frame_number);
	return 0;
}

int VDeviceV4L::read_v4l_frame(VFrame *frame)
{
	frame_to_vframe(frame, (unsigned char*)capture_buffer + capture_params.offsets[capture_frame_number]);
	return 0;
}

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

int VDeviceV4L::frame_to_vframe(VFrame *frame, unsigned char *input)
{
	int inwidth, inheight;
	int width, height;

	inwidth = window_params.width;
	inheight = window_params.height;

	width = MIN(inwidth, frame->get_w());
	height = MIN(inheight, frame->get_h());
//printf("VDeviceV4L::frame_to_vframe %d %d\n", colormodel, frame->get_color_model());

	if(frame->get_color_model() == colormodel)
	{
		switch(frame->get_color_model())
		{
			case BC_RGB888:
			{
				unsigned char *row_in;
				unsigned char *row_out_start, *row_out_end;
				int bytes_per_inrow = inwidth * 3;
				int bytes_per_outrow = frame->get_bytes_per_line();
				unsigned char **rows_out = frame->get_rows();

				for(int i = 0; i < frame->get_h(); i++)
				{
					row_in = input + bytes_per_inrow * i;
					row_out_start = rows_out[i];
					row_out_end = row_out_start + 
						MIN(bytes_per_outrow, bytes_per_inrow);

					while(row_out_start < row_out_end)
					{
						*row_out_start++ = row_in[2];
						*row_out_start++ = row_in[1];
						*row_out_start++ = row_in[0];
						row_in += 3;
					}
				}
				break;
			}

			case BC_YUV420P:
			case BC_YUV411P:
				memcpy(frame->get_y(), input, width * height);
				memcpy(frame->get_u(), input + width * height, width * height / 4);
				memcpy(frame->get_v(), input + width * height + width * height / 4, width * height / 4);
				break;

			case BC_YUV422P:
				memcpy(frame->get_y(), input, width * height);
				memcpy(frame->get_u(), input + width * height, width * height / 2);
				memcpy(frame->get_v(), input + width * height + width * height / 2, width * height / 2);
				break;

			case BC_YUV422:
				memcpy(frame->get_data(), 
					input, 
					VFrame::calculate_data_size(width, 
						height, 
						-1, 
						frame->get_color_model()));
				break;
		}
	}
	else
	{
		VFrame *in_frame = new VFrame(input, 
			inwidth, 
			inheight, 
			colormodel, 
			-1);
		cmodel_transfer(frame->get_rows(), 
			in_frame->get_rows(),
			frame->get_y(),
			frame->get_u(),
			frame->get_v(),
			in_frame->get_y(),
			in_frame->get_u(),
			in_frame->get_v(),
			0, 
			0, 
			inwidth, 
			inheight,
			0, 
			0, 
			frame->get_w(), 
			frame->get_h(),
			colormodel, 
			frame->get_color_model(),
			0,
			inwidth,
			inheight);
	}
	return 0;
}



int VDeviceV4L::next_frame(int previous_frame)
{
	int result = previous_frame + 1;

	if(result >= MIN(capture_params.frames, device->in_config->capture_length)) result = 0;
	return result;
}

int VDeviceV4L::read_buffer(VFrame *frame)
{
	int result = 0;

SET_TRACE
	if(shared_memory)
	{
// Read the current frame
		if(!got_first_frame) v4l1_start_capture();
		wait_v4l_frame();
		read_v4l_frame(frame);
// Free this frame up for capturing
		capture_frame(capture_frame_number);
// Advance the frame to capture.
		capture_frame_number = next_frame(capture_frame_number);
	}
	else
	{
		read(input_fd, capture_buffer, capture_params.size);
	}
	got_first_frame = 1;
SET_TRACE

	return 0;
}











