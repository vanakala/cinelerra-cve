
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

#ifndef VDEVICELML_H
#define VDEVICELML_H

#include "guicast.h"
#include "vdevicebase.h"

// ./quicktime
#include "jpeg.h"
#include "quicktime.h"

#define INPUT_BUFFER_SIZE 65536

class VDeviceLML : public VDeviceBase
{
public:
	VDeviceLML(VideoDevice *device);
	~VDeviceLML();

	int open_input();
	int open_output();
	int close_all();
	int read_buffer(VFrame *frame);
	int write_buffer(VFrame *frame, EDL *edl);
	int reset_parameters();
	ArrayList<int>* get_render_strategies();

private:
	int reopen_input();

	inline unsigned char get_byte()
	{
		if(!input_buffer) input_buffer = new unsigned char[INPUT_BUFFER_SIZE];
		if(input_position >= INPUT_BUFFER_SIZE) refill_input();
		return input_buffer[input_position++];
	};

	inline unsigned long next_bytes(int total)
	{
		unsigned long result = 0;
		int i;

		if(!input_buffer) input_buffer = new unsigned char[INPUT_BUFFER_SIZE];
		if(input_position + total > INPUT_BUFFER_SIZE) refill_input();

		for(i = 0; i < total; i++)
		{
			result <<= 8;
			result |= input_buffer[input_position + i];
		}
		return result;
	};

	int refill_input();
	inline int write_byte(unsigned char byte)
	{
		if(!frame_buffer)
		{
			frame_buffer = new unsigned char[256000];
			frame_allocated = 256000;
		}

		if(frame_size >= frame_allocated)
		{
			unsigned char *new_frame = new unsigned char[frame_allocated * 2];
			memcpy(new_frame, frame_buffer, frame_size);
			delete frame_buffer;
			frame_buffer = new_frame;
			frame_allocated *= 2;
		}

		frame_buffer[frame_size++] = byte;
		return 0;
	};

	int write_fake_marker();

	FILE *jvideo_fd;
	unsigned char *input_buffer, *frame_buffer;
	long input_position;
	long frame_size, frame_allocated;
	int input_error;
//	quicktime_mjpeg_hdr jpeg_header;
	long last_frame_no;
	ArrayList<int> render_strategies;
};

#endif
