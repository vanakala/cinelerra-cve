
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

#ifndef AFRAME_H
#define AFRAME_H

#include "datatype.h"

class AFrame
{
public:
	AFrame(int buffer_length = 0, int float_data = 0);
	~AFrame();

// Set shared buffer (AFrame does not delete it)
	void set_buffer(double *buffer, int length);
	void set_buffer(float *buffer, int length);

// Clears unused part of buffer
	void clear_buffer(void);
// Reallocates buffer to size at least length
	void check_buffer(int length);
// Makes frame empty
	void reset_buffer(void);
// Initializes frame pts, checks buffer length, clears buffer
	void init_aframe(ptstime postime, int length);
// Copy parameters
	void copy_pts(AFrame *that);
// Copy buffer
	void copy(AFrame *that);
// Copy parameters and length only
	void copy_of(AFrame *that);
// Mark frame filled to length
	void set_filled(int length);
	void set_filled(ptstime duration);
// Increase buffer size (keeps data in buffer)
	void extend_buffer(int length);

// Samples/duration conversions
	samplenum to_samples(ptstime duration);
	ptstime to_duration(samplenum samples);

// Calculates source_duration
	ptstime get_source_duration();

// Calculates fill position in samples
	samplenum fill_position(int srcpos = 0);

// Calculates fill length, avoids buffer overflow
	int fill_length();

// Sets filled length of buffer
	void set_filled_length();

// Clears buffer, sets fill request
	void set_fill_request(ptstime pts, ptstime duration);
	void set_fill_request(ptstime pts, int length);
	void set_fill_request(samplenum pos, int length);

// Return true if parameter difference is less-equal of the duration
//  of a sample
	int ptsequ(ptstime t1, ptstime t2);

// Round pts to nearest sample
	ptstime round_to_sample(ptstime pts);

// Set frame pts rounded to sample
	ptstime set_pts(ptstime t);

	void dump(int indent, int dumpdata = 0);

// Buffer start in source
	ptstime source_pts;
	ptstime source_duration;
	int source_length;
// Buffer start in project
	ptstime pts;
// Buffer duration
	ptstime duration;
// Sample position in source
	samplenum position;
// Audio channel
	int channel;
// Buffer length in samples
	int length;

	int samplerate;
	double *buffer;
	float *float_buffer;

// Length of allocated buffer
	int buffer_length;

private:
	int shared;
	int float_data;
};


#endif
