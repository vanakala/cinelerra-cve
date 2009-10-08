
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

#include "assets.h"
#include "file.inc"
#include "language.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"
#include "strategies.inc"
#include "vdevicelml.h"
#include "vframe.h"
#include "videoconfig.h"
#include "videodevice.h"


#define SOI 0xffd8
#define APP3 0xffe3
#define APP1 0xffe1
#define APP0 0xffe0
#define EOI  0xffd9

VDeviceLML::VDeviceLML(VideoDevice *device)
 : VDeviceBase(device)
{
	reset_parameters();
	render_strategies.append(VRENDER_MJPG);
}

VDeviceLML::~VDeviceLML()
{
	close_all();
}

int VDeviceLML::reset_parameters()
{
	jvideo_fd = 0;
	input_buffer = 0;
	frame_buffer = 0;
	frame_size = 0;
	frame_allocated = 0;
	input_error = 0;
	input_position = INPUT_BUFFER_SIZE;
	last_frame_no = 0;
}

int VDeviceLML::open_input()
{
	jvideo_fd = fopen(device->in_config->lml_in_device, "rb");
	if(jvideo_fd)
	{
		return 0;
	}
	else
	{
		perror("VDeviceLML::open_input");
		jvideo_fd = 0;
		return 1;
	}
	return 0;
}

int VDeviceLML::open_output()
{
	jvideo_fd = fopen(device->out_config->lml_out_device, "wb");
	if(jvideo_fd)
	{
		return 0;
	}
	else
	{
		perror("VDeviceLML::open_output");
		jvideo_fd = 0;
		return 1;
	}
	return 0;
}

int VDeviceLML::close_all()
{
	if(device->r)
	{
		if(jvideo_fd) fclose(jvideo_fd);
	}
	if(device->w)
	{
		if(jvideo_fd) fclose(jvideo_fd);
	}
	if(input_buffer)
	{
		delete input_buffer;
	}
	if(frame_buffer)
	{
		delete frame_buffer;
	}
	reset_parameters();
	return 0;
}

int VDeviceLML::read_buffer(VFrame *frame)
{
	long first_field = 0, frame1_size = 0, frame2_size = 0, i;
	int result = 0, frame_no = 0, retries = 0;

	if(!jvideo_fd) return 1;

	input_error = 0;

retry:
	frame->set_compressed_size(0);
	retries++;
	if(retries > 5) return 1;

// Keep reading until the first field of a frame arrives.
	while(!input_error && !first_field)
	{
// Get the first marker of a frame
		while(!input_error && next_bytes(2) != SOI)
		{
			get_byte();
		}

// Store SOI marker
		frame_size = 0;
		write_byte(get_byte());
		write_byte(get_byte());

// Copy the first frame
		while(!input_error && next_bytes(2) != EOI)
		{
// Replace the LML header with a Quicktime header
			if(next_bytes(2) == APP3)
			{
				first_field = 1;
				write_fake_marker();
			
				get_byte(); // APP3
				get_byte();
				get_byte(); // LEN
				get_byte();
				get_byte(); // COMMENT
				get_byte();
				get_byte();
				get_byte();
				get_byte(); // Frame no
				get_byte();
				get_byte(); // sec
				get_byte();
				get_byte();
				get_byte();
				get_byte(); // usec
				get_byte();
				get_byte();
				get_byte();
				get_byte(); // framesize (useless since we have to swap frames)
				get_byte();
				get_byte();
				get_byte();
				frame_no = get_byte(); // frame seq no
				frame_no |= (long)get_byte() << 8;
				frame_no |= (long)get_byte() << 16;
				frame_no |= (long)get_byte() << 24;

				if(frame_no <= last_frame_no)
				{
					input_error = reopen_input();
					first_field = 0;
					goto retry;
				}
				else
				{
// Finish LML header
					last_frame_no = frame_no;
					while(next_bytes(2) != 0xffdb) get_byte();
				}
			}
			else
			{
				write_byte(get_byte());
			}
		}

// Store EOI marker
		write_byte(get_byte());
		write_byte(get_byte());
	}

	frame1_size = frame_size;

// Read the second field
	if(first_field)
	{
// Find next field
		while(!input_error && next_bytes(2) != SOI)
		{
			get_byte();
		}

// Store SOI marker
		write_byte(get_byte());
		write_byte(get_byte());

// Store Quicktime header
		write_fake_marker();

// Copy the second frame
		while(!input_error && next_bytes(2) != EOI)
		{
			write_byte(get_byte());
		}

// Store EOI marker
		write_byte(get_byte());
		write_byte(get_byte());
	}

	frame2_size = frame_size - frame1_size;

// Insert the required information
	if(!input_error)
	{
//		quicktime_fixmarker_jpeg(&jpeg_header, (char*)frame_buffer, frame1_size, !device->odd_field_first);
//		quicktime_fixmarker_jpeg(&jpeg_header, (char*)frame_buffer + frame1_size, frame2_size, device->odd_field_first);

// Store in the VFrame
		frame->allocate_compressed_data(frame_size);

// Quicktime expects the even field first
		if(device->odd_field_first)
		{
			memcpy(frame->get_data(), frame_buffer + frame1_size, frame2_size);
			memcpy(frame->get_data() + frame2_size, frame_buffer, frame1_size);
		}
		else
			memcpy(frame->get_data(), frame_buffer, frame_size);

		frame->set_compressed_size(frame_size);
	}
	else
	{
		input_error = 0;
		reopen_input();
		goto retry;
	}

	return input_error;
}

int VDeviceLML::reopen_input()
{
	int input_error = 0;
	Timer timer;
	fprintf(stderr, _("VDeviceLML::read_buffer: driver crash\n"));
	fclose(jvideo_fd);
	timer.delay(100);
	input_error = open_input();
	if(!input_error) fprintf(stderr, _("VDeviceLML::read_buffer: reopened\n"));
	last_frame_no = 0;
	input_position = INPUT_BUFFER_SIZE;
	return input_error;
}


int VDeviceLML::write_fake_marker()
{
// Marker
	write_byte(0xff);
	write_byte(0xe1);
// Size
	write_byte(0x00);
	write_byte(0x2a);
// Blank space
	for(int i = 0; i < 0x28; i++)
	{
		write_byte(0x00);
	}
	return 0;
}

int VDeviceLML::refill_input()
{
// Shift remaining data up.
	memcpy(input_buffer, input_buffer + input_position, INPUT_BUFFER_SIZE - input_position);

// Append new data
	input_error = !fread(input_buffer + INPUT_BUFFER_SIZE - input_position, 
					INPUT_BUFFER_SIZE - (INPUT_BUFFER_SIZE - input_position), 
					1,
					jvideo_fd);

	input_position = 0;
	return input_error;
}


int VDeviceLML::write_buffer(VFrame *frame, EDL *edl)
{
	int result = 0, i, frame1size, j, size_qword, real_size, skip;
	unsigned long size = frame->get_compressed_size();
	unsigned char *data = frame->get_data();
	unsigned char *data1;
	int even_field_first = 1;

#if 0
	if(!jvideo_fd || frame->get_color_model() != VFRAME_COMPRESSED) return 1;
#endif

	if(frame_allocated < size * 2)
	{
		delete frame_buffer;
		frame_buffer = 0;
	}
	
	if(!frame_buffer)
	{
		frame_buffer = new unsigned char[size * 2];
	}

	for(data1 = data + 1, i = 0; i < size - 1; i++)
		if(data[i] == ((EOI & 0xff00) >> 8) && data1[i] == (EOI & 0xff)) break;

	i += 2;
	frame1size = i;
	j = 0;
	if(even_field_first) i = 0;

// SOI
	frame_buffer[j++] = data[i++];
	frame_buffer[j++] = data[i++];

// APP3 for LML driver
	frame_buffer[j++] = (APP3 & 0xff00) >> 8;
	frame_buffer[j++] = APP3 & 0xff;
	frame_buffer[j++] = 0;       // Marker size
	frame_buffer[j++] = 0x2c;
	frame_buffer[j++] = 'L';     // nm
	frame_buffer[j++] = 'M';
	frame_buffer[j++] = 'L';
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;       // frameNo
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;       // sec
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;       // usec
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
// Frame size eventually goes here
	size_qword = j;      
	frame_buffer[j++] = 0;		 
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
// Frame Seq No
	frame_buffer[j++] = 0;		 
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
// Color Encoding
	frame_buffer[j++] = 1;		 
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
// Video Stream
	frame_buffer[j++] = 1;		 
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
// Time Decimation
	frame_buffer[j++] = 1;		 
	frame_buffer[j++] = 0;
// Filler
	frame_buffer[j++] = 0;		 
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;		 
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;
	frame_buffer[j++] = 0;

// Copy rest of first field
	data1 = data + 1;
	
	while(i < size)
	{
		frame_buffer[j++] = data[i++];
	}

// Copy second field
	if(!even_field_first)
	{
		for(i = 0; i < frame1size; )
		{
			frame_buffer[j++] = data[i++];
		}
	}

	real_size = j;
// frameSize in little endian
	frame_buffer[size_qword++] = (real_size & 0xff);
	frame_buffer[size_qword++] = ((real_size & 0xff00) >> 8);
	frame_buffer[size_qword++] = ((real_size & 0xff0000) >> 16);
	frame_buffer[size_qword++] = ((real_size & 0xff000000) >> 24);

//fwrite(frame_buffer, real_size, 1, stdout);
	result = !fwrite(frame_buffer, 
		real_size, 
		1, 
		jvideo_fd);
	if(result) perror("VDeviceLML::write_buffer");

	return result;
}

ArrayList<int>* VDeviceLML::get_render_strategies()
{
	return &render_strategies;
}
