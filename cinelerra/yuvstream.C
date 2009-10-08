
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

#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "guicast.h"
#include "pipe.h"
#include "yuvstream.h"
#include "interlacemodes.h"
#include "mainerror.h"

YUVStream::YUVStream() { 
	y4m_init_stream_info(&stream_info);
	y4m_init_frame_info(&frame_info);
	stream_fd = -1;
	stream_pipe= 0;
	frame_count = 0;
	frame_index = 0;
}

YUVStream::~YUVStream() {		
	y4m_fini_stream_info(&stream_info);
	y4m_fini_frame_info(&frame_info);
	if (frame_index) delete frame_index;
	close_fd();
}


int YUVStream::open_read(char *path) {
	// NOTE: reading from pipes would be very difficult without temp files
	stream_fd = open(path, O_RDONLY);

	if (stream_fd < 0) {
		eprintf("Error while opening \"%s\" for reading. \n%m\n", path);
		return 1;
	}

	int result = read_header();
	if (result != Y4M_OK) {
		eprintf("Bad YUV4MPEG2 header: %s\n", y4m_strerr(result));
		return 1;
	}

	// generate index to frame position if not done yet
	if (frame_index == 0) {
		if (make_index() != 0) {
			return 1;
		}
	}

	return 0;
}

// NOTE: path is opened as a pipe if contains '|'
int YUVStream::open_write(char *path, char *pipe) {
	if (pipe && *pipe) {
		// skip over the '|' if present
		if (char *p = strchr(path, '|')) {
			pipe = p + 1;
		}

		stream_pipe = new Pipe(pipe, path);
		if (stream_pipe->open_write() == 0) {
			stream_fd = stream_pipe->fd;
			return 0;
		}
		return 1;
	}

	stream_fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0666);
	if (stream_fd > 0) return 0;
	
	eprintf("Error while opening \"%s\" for writing. \n%m\n", path);
	return 1;
}


void YUVStream::close_fd() {
	if (stream_pipe) {
		stream_pipe->close();
		stream_pipe = 0;
		stream_fd = -1;
	}
		
	if (stream_fd >= 0) close(stream_fd);
	stream_fd = -1;
}

int YUVStream::read_frame(uint8_t *yuv[3]) {
	int result =  y4m_read_frame(stream_fd, &stream_info, 
				  &frame_info, yuv);
	if (result != Y4M_OK) {
		if (result != Y4M_ERR_EOF) {
			eprintf("read_frame() failed: %s\n", 
			       y4m_strerr(result));
		}
		return 1;
	}

	return 0;
}

int YUVStream::read_frame_raw(uint8_t *data, long frame_size) {
#if (MJPEGTOOLS_Y4M_WRITE_FRAME_HEADER__3ARGS)
	int result = y4m_read_frame_header(stream_fd, &stream_info, &frame_info);
#else
	int result = y4m_read_frame_header(stream_fd, &frame_info);
#endif
	if (result != Y4M_OK) {
		eprintf("y4m_read_frame_header() failed: %s\n", 
		       y4m_strerr(result));
		return 1;
	}
	result = y4m_read(stream_fd, data, frame_size);
	if (result != Y4M_OK) {
		printf("y4m_read(%d) failed: %s\n", 
		       frame_size, y4m_strerr(result));
		return 1;
	}
	return 0;
}
		
int YUVStream::write_frame(uint8_t *yuv[3]) {
	int result = y4m_write_frame(stream_fd, &stream_info, 
				     &frame_info, yuv);
	if (result != Y4M_OK) {
		eprintf("write_frame() failed: %s\n", y4m_strerr(result));
		return 1;
	}
	return 0;
}
int YUVStream::write_frame_raw(uint8_t *data, long frame_size) {
#if (MJPEGTOOLS_Y4M_WRITE_FRAME_HEADER__3ARGS)
	int result = y4m_write_frame_header(stream_fd, &stream_info, &frame_info);
#else
	int result = y4m_write_frame_header(stream_fd, &frame_info);
#endif
	if (result != Y4M_OK) {
		eprintf("y4m_write_frame_header() failed: %s\n", 
		       y4m_strerr(result));
		return 1;
	}
	result = y4m_write(stream_fd, data, frame_size);
	if (result != Y4M_OK) {
		eprintf("y4m_write(%d) failed: %s\n", 
		       frame_size, y4m_strerr(result));
		return 1;
	}
	return 0;
}		

int YUVStream::make_index() {
	off_t position;
	uint8_t *yuv[3];

	// NOTE: make_index() must be called after read_header().

	// NOTE: storing frame_index locally means it is destroyed too often.
	//       make_index() will be called 3 times per file.  If this
	//       becomes a performance problem, the index should be cached.
	//       Storing in 'asset' helps some, but still is done twice.
	if (frame_index) delete frame_index;
	frame_index = new ArrayList<off_t>;

	VFrame *frame = new VFrame(0, get_width(), get_height(), BC_YUV420P);
	yuv[0] = frame->get_y();
	yuv[1] = frame->get_u();
	yuv[2] = frame->get_v();

	// note the start of the first frame
	position = lseek(stream_fd, 0, SEEK_CUR);

	// reset the frame count
	frame_count = 0;

	while (read_frame(yuv) == 0) {
		// index is start position of each frame
		frame_index->append(position);
		position = lseek(stream_fd, 0, SEEK_CUR);
		frame_count++;
	} 

	// rewind to the start of the first frame
	lseek(stream_fd, frame_index->values[0], SEEK_SET);

	delete frame;

	return 0;
}

int YUVStream::seek_frame(int64_t frame_number) {
	if (frame_number > frame_count ||
	    frame_number < 0 ||
	    frame_index == 0) {
		eprintf("seek_frame(%d) failed (frame_count=%d)\n", 
		       frame_number, frame_count);
		return 1;
	}

	off_t position = frame_index->values[frame_number];
	if (position == 0) {
		// because of header, position should never be zero
		eprintf("seek_frame(%d): position was zero\n", frame_number);
	}

	if (lseek(stream_fd, position, SEEK_SET) < 0) {
		eprintf("lseek(%d) failed: %s\n", position, strerror(errno));
		return 1;
	}

	return 0;
}

int YUVStream::read_header() {
	int result = y4m_read_stream_header(stream_fd, &stream_info);
	if (result != Y4M_OK) {
		eprintf("y4m_read_stream_header() failed: %s\n", 
		       y4m_strerr(result));
		return 1;
	}
	return 0;
}
int YUVStream::write_header() {
	int result = y4m_write_stream_header(stream_fd, &stream_info);
	if (result != Y4M_OK) {
		eprintf("y4m_write_stream_header() failed: %s\n", 
		       y4m_strerr(result));
		return 1;
	}
	return 0;
}

int YUVStream::get_interlace() {
	return ilace_yuv4mpeg_to_bc(y4m_si_get_interlace(&stream_info));
}

void YUVStream::set_interlace(int imode) {
	y4m_si_set_interlace(&stream_info, ilace_bc_to_yuv4mpeg(imode));
}

int YUVStream::get_width() {
	return y4m_si_get_width(&stream_info);
}
void YUVStream::set_width(int width) {
	y4m_si_set_width(&stream_info, width);
}

int YUVStream::get_height() {
	return y4m_si_get_height(&stream_info);
}
void YUVStream::set_height(int height) {
	y4m_si_set_height(&stream_info, height);
}

double YUVStream::get_frame_rate() {
	y4m_ratio_t ratio = y4m_si_get_framerate(&stream_info);
	double frame_rate = (double) ratio.n / (double) ratio.d;
	return frame_rate;
}
void YUVStream::set_frame_rate(double frame_rate) {
	y4m_ratio_t ratio = mpeg_conform_framerate(frame_rate);
	y4m_si_set_framerate(&stream_info, ratio);
}

// FUTURE: these seem like a mess, possibly because I don't 
//         properly understand "display aspect" vs "pixel aspect"
double YUVStream::get_aspect_ratio() {
	y4m_ratio_t sar = y4m_si_get_sampleaspect(&stream_info);
	mpeg_aspect_code_t code = 
		mpeg_guess_mpeg_aspect_code(2, sar, 
					    get_width(),
					    get_height());
	y4m_ratio_t aspect_ratio =  mpeg_framerate(code);
	if (aspect_ratio.d == 0) return 0;
	return (double) aspect_ratio.n / (double) aspect_ratio.d;
}		
void YUVStream::set_aspect_ratio(double aspect_ratio) {
	y4m_ratio_t ratio;
	ratio.n = (int)(aspect_ratio * 10000);
	ratio.d = 10000;
	y4m_ratio_t sar = y4m_guess_sar(get_width(), get_height(),
					ratio);
	y4m_si_set_sampleaspect(&stream_info, sar);
}
