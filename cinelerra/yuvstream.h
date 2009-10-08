
/*
 * CINELERRA
 * Copyright (C) 2004 Nathan Kurz
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

#include <stdio.h>

#include "guicast.h"
#include "pipe.h"

#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H
#endif /* HAVE_STDINT_H */

#include <mjpegtools/yuv4mpeg.h>
#include <mjpegtools/mpegconsts.h>

class YUVStream {
 public:
	YUVStream();
	~YUVStream();

	int open_read(char *path);
	int open_write(char *path, char *pipe);
	void close_fd(); 

	int read_frame(uint8_t *yuv[3]);
	int read_frame_raw(uint8_t *data, long frame_size);
		
	int write_frame(uint8_t *yuv[3]);
	int write_frame_raw(uint8_t *data, long frame_size);

	int make_index();
	int seek_frame(int64_t frame_number);

	int read_header();
	int write_header();

	int get_interlace();
	void set_interlace(int width);

	int get_width();
	void set_width(int width);

	int get_height();
	void set_height(int height);

	double get_frame_rate();
	void set_frame_rate(double frame_rate);

	double get_aspect_ratio();
	void set_aspect_ratio(double aspect_ratio);

	int64_t frame_count;

	int stream_fd;  // used for files
 private: 
	y4m_stream_info_t stream_info;
	y4m_frame_info_t frame_info;
	Pipe *stream_pipe; // used for pipes
	ArrayList<off_t> *frame_index;
};
