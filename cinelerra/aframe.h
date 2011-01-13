
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
	AFrame(int buffer_length = 0);
	~AFrame();

// Set shared buffer (AFrame does not delete it)
	void set_buffer(double *buffer, int length);

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

	void dump(int dumpdata = 0);

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

// Length of allocated buffer
	int buffer_length;

private:
	int shared;
};


#endif
