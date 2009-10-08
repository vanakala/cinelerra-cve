
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

#ifndef RESAMPLE_H
#define RESAMPLE_H

#define BPC 160
#define BLACKSIZE 25

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
		long in_len,
		int in_rate,
		int out_rate,
		int channel);
// Resample from the file handler and store in *output.
// Returns the total samples read from the file handler.
	int resample(double *output, 
		long out_len,
		int in_rate,
		int out_rate,
		int channel,
		long in_position,      // Starting sample in input samplerate
		long out_position);      // Starting sample in output samplerate
	virtual void read_chunk(double *input, 
		long len, 
		int &reseek, 
		int iteration);   // True once for every resample call

// History buffer for resampling.
	double **old;
	double *itime;



// Unaligned resampled output
	double **output_temp;


// Total samples in unaligned output
// Tied to each channel independantly
	long *output_size;


// Sample start of output_temp in the resampled domain.
	long *output_temp_start;
// Allocation of unaligned output
	long output_allocation;
// input chunk
	double *input;
// Sample end of input chunks in the input domain.
	long *input_chunk_end;
	long input_size;
	int channels;
	int *resample_init;
// Last sample ratio configured to
	double last_ratio;
	double blackfilt[2 * BPC + 1][BLACKSIZE];
	File *file;
// Determine whether to reset after a seek
// Sample end of last buffer read for each channel
	long *last_out_end;
};

class Resample_float
{
public:
	Resample_float(File *file, int channels);
	~Resample_float();

// Reset after seeking
	void reset(int channel = -1);
	float blackman(int i, float offset, float fcn, int l);
// Query output temp
	int get_output_size(int channel);
	void read_output(double *output, int channel, int size);
// Resamples input and dumps it to output_temp
	void resample_chunk(float *input,
		long in_len,
		int in_rate,
		int out_rate,
		int channel);
// Resample from the file handler and store in *output.
// Returns the total samples read from the file handler.
	int resample(double *output, 
		long out_len,
		int in_rate,
		int out_rate,
		int channel,
		long in_position,      // Starting sample in input samplerate
		long out_position);      // Starting sample in output samplerate
	virtual void read_chunk(float *input, 
		long len, 
		int &reseek, 
		int iteration);   // True once for every resample call

// History buffer for resampling.
	float **old;
	float *itime;



// Unaligned resampled output
	double **output_temp;


// Total samples in unaligned output
// Tied to each channel independantly
	long *output_size;


// Sample start of output_temp in the resampled domain.
	long *output_temp_start;
// Allocation of unaligned output
	long output_allocation;
// input chunk
	float *input;
// Sample end of input chunks in the input domain.
	long *input_chunk_end;
	long input_size;
	int channels;
	int *resample_init;
// Last sample ratio configured to
	float last_ratio;
	float blackfilt[2 * BPC + 1][BLACKSIZE];
	File *file;
// Determine whether to reset after a seek
// Sample end of last buffer read for each channel
	long *last_out_end;
};

#endif
