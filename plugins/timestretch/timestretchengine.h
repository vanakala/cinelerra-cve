
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

#ifndef TIMESTRETCHENGINE_H
#define TIMESTRETCHENGINE_H


#include <stdint.h>


class TimeStretchEngine
{
public:
// scale = out length / in length
	TimeStretchEngine(double scale, int sample_rate);
	~TimeStretchEngine();

	void overlay(double *out, double *in, int size, int skirt);
// Returns the number of samples in the output buffer
	int process(double *in_buffer, int in_size);
// Return the output buffer
	double* get_samples();
// Increment output buffer counters and shift output
	void read_output(double *buffer, int size);

private:
// ms length of average window
	int window_time;
	int sample_rate;
	int window_size;
// Queer eye for the straight buffer
	int window_skirt;
	double *output;
	int output_allocation;
	int output_size;
// Sample at beginning of output buffer
	int64_t output_sample;
	double *input;
	int input_allocation;
	int input_size;
// Sample at beginning of input buffer
	int64_t input_sample;
	double scale;
};





#endif



