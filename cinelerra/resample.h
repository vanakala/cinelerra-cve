// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RESAMPLE_H
#define RESAMPLE_H

#define BPC 160
#define BLACKSIZE 25

#include "aframe.inc"
#include "cinelerra.h"
#include "datatype.h"
#include "file.inc"

class Resample
{
public:
	Resample(File *file, int channels);
	~Resample();

// Reset after seeking
	void reset(int channel = -1);
	double blackman(int i, double offset, double fcn, int l);
// Query output temp
	int get_output_size(int channel);
	void read_output(double *output, int channel, int size);
// Resamples input and dumps it to output_temp
	void resample_chunk(double *input,
		int in_len,
		int in_rate,
		int out_rate,
		int channel);
// Resample from the file handler and store in *output.
// Returns the total samples read from the file handler.
	int resample(double *output, 
		int out_len,
		int in_rate,
		int out_rate,
		int channel,
		samplenum out_position);      // Starting sample in output samplerate

// History buffer for resampling.
	double *old[MAX_CHANNELS];
	double itime[MAX_CHANNELS];

// Unaligned resampled output
	double **output_temp;

// Total samples in unaligned output
// Tied to each channel independantly
	int output_size[MAX_CHANNELS];

// Allocation of unaligned output
	int output_allocation;
// input chunk
	AFrame *inframe;
// Sample end of input chunks in the input domain.
	samplenum input_chunk_end[MAX_CHANNELS];
	int input_size;
	int channels;
	int resample_init[MAX_CHANNELS];
// Last sample ratio configured to
	double last_ratio;
	double blackfilt[2 * BPC + 1][BLACKSIZE];
	File *file;
// Determine whether to reset after a seek
// Sample end of last buffer read for each channel
	samplenum last_out_end[MAX_CHANNELS];
};

#endif
