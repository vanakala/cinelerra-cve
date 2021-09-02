// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2011 Einar Rünkaru <einarrunkaru@gmail dot com>

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
// Make empty frame with start and duration
	void clear_frame(ptstime pts, ptstime duration);
// Reallocates buffer to size at least length
	void check_buffer(int length);
// Makes frame empty
	void reset_buffer(void);
// Initializes frame pts, checks buffer length, clears buffer
	void init_aframe(ptstime postime, int length, int samplerate);
// Copy parameters
	void copy_pts(AFrame *that);
// Copy buffer
	void copy(AFrame *that);
// Copy parameters and length only
	void copy_of(AFrame *that);
// Mark frame filled to length
	void set_filled(int length);
	void set_filled(ptstime duration);

// Samples/duration conversions
	samplenum to_samples(ptstime duration);
	ptstime to_duration(samplenum samples);

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
// Set filled length
	void set_length(int length);
// Set filled duration
	ptstime set_duration(ptstime pts);
// Get frame pts
	inline ptstime get_pts() { return pts; };
// Get frame duration
	inline ptstime get_duration() { return duration; };
// Get end pts of frame
	inline ptstime get_end_pts() { return pts + duration; };
// Get frame length in samples
	inline int get_length() { return length; };
// Set frame source pts
	ptstime set_source_pts(ptstime pts);
// Set frame source duration
	ptstime set_source_duration(ptstime duration);
	void set_source_length(int length);
// Get frame source_pts
	inline ptstime get_source_pts() { return source_pts; };
// Get source duration
	inline ptstime get_source_duration() { return source_duration; };
	inline int get_source_length() { return source_length; };
// Set samplerate
	inline void set_samplerate(int rate) { samplerate = rate; };
// Get buffer length
	inline int get_buffer_length() { return buffer_length; };
// Get samplerate
	inline int get_samplerate() { return samplerate; };
// Make frame empty
	void set_empty();

// Track number
	void set_track(int number);
	int get_track();
// Data size of the object
	size_t get_data_size();

	void dump(int indent, int dumpdata = 0);

// Audio channel
	int channel;
// Stream of asset
	int stream;
// Sample position in source
	samplenum position;

// Buffers of samples
	double *buffer;
	float *float_buffer;

private:
// Buffer start in source
	ptstime source_pts;
	ptstime source_duration;
	int source_length;
// Buffer start in project
	ptstime pts;
// Buffer duration
	ptstime duration;
// Filled length in samples
	int length;

	int samplerate;

// Length of allocated buffer
	int buffer_length;

	int trackno;
	int shared;
	int float_data;
};


#endif
