// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "bcsignals.h"
#include "clip.h"
#include "file.h"
#include "resample.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define REPOSITION(x, y) (llabs((x) - (y)) > 1)

// Resampling from Lame

Resample::Resample(File *file, int channels)
{
	this->file = file;
	this->channels = channels;
	for(int i = 0; i < channels; i++)
	{
		old[i] = new double[BLACKSIZE];
	}
	memset(resample_init, 0, sizeof(int) * channels);
	last_ratio = 0;
	output_temp = 0;
	memset(output_size, 0, sizeof(int) * channels);
	output_allocation = 0;
	input_size = RESAMPLE_CHUNKSIZE;
	memset(input_chunk_end, 0, sizeof(samplenum) * channels);
	inframe = new AFrame(input_size);
	memset(last_out_end, 0, sizeof(samplenum) * channels);
}


Resample::~Resample()
{
	for(int i = 0; i < channels; i++)
	{
		delete [] old[i];
	}
	if(output_temp)
	{
		for(int i = 0; i < channels; i++)
		{
			delete [] output_temp[i];
		}
		delete [] output_temp;
	}
	delete inframe;
}

void Resample::reset(int channel)
{
	if(channel < 0)
	{
		memset(resample_init, 0, sizeof(int) * channels);
		memset(output_size, 0, sizeof(int) * channels);
		memset(last_out_end, 0, sizeof(samplenum) * channels);
		memset(input_chunk_end, 0, sizeof(samplenum) * channels);
	}
	else
	{
		resample_init[channel] = 0;
		output_size[channel] = 0;
		last_out_end[channel] = 0;
		input_chunk_end[channel] = 0;
	}
}

double Resample::blackman(int i, double offset, double fcn, int l)
{
  /* This algorithm from:
SIGNAL PROCESSING ALGORITHMS IN FORTRAN AND C
S.D. Stearns and R.A. David, Prentice-Hall, 1992
  */

	double bkwn;
	double wcn = (M_PI * fcn);
	double dly = l / 2.0;
	double x = i-offset;
	if(x < 0) x = 0;
	else
	if(x > l) x = l;

	bkwn = 0.42 - 0.5 * cos((x * 2) * M_PI /l) + 0.08 * cos((x * 4) * M_PI /l);
	if(fabs(x - dly) < 1e-9) 
		return wcn / M_PI;
	else
		return (sin((wcn * (x - dly))) / (M_PI * (x - dly)) * bkwn);
}

int Resample::get_output_size(int channel)
{
	return output_size[channel];
}

void Resample::read_output(double *output, int channel, int size)
{
	memcpy(output, output_temp[channel], size * sizeof(double));
// Shift leftover forward
	for(int i = size; i < output_size[channel]; i++)
		output_temp[channel][i - size] = output_temp[channel][i];
	output_size[channel] -= size;
}

void Resample::resample_chunk(double *input,
	int in_len,
	int in_rate,
	int out_rate,
	int channel)
{
	double resample_ratio = (double)in_rate / out_rate;
	int filter_l;
	double fcn, intratio;
	double offset, xvalue;
	int num_used;
	int i, j, k;

	intratio = (fabs(resample_ratio - floor(.5 + resample_ratio)) < .0001);
	fcn = .90 / resample_ratio;
	if(fcn > .90) fcn = .90;
	filter_l = BLACKSIZE - 6;  
/* must be odd */
	if(0 == filter_l % 2 ) --filter_l;

/* if resample_ratio = int, filter_l should be even */
	filter_l += (int)intratio;

// Blackman filter initialization must be called whenever there is a 
// sampling ratio change
	if(!resample_init[channel] || last_ratio != resample_ratio)
	{
		resample_init[channel] = 1;
		itime[channel] = 0;
		memset(old[channel], 0, sizeof(double) * BLACKSIZE);

// precompute blackman filter coefficients
		for (j = 0; j <= 2 * BPC; ++j) 
		{
			for(j = 0; j <= 2 * BPC; j++)
			{
				offset = (double)(j - BPC) / (2 * BPC);
				for(i = 0; i <= filter_l; i++)
				{
					blackfilt[j][i] = blackman(i, offset, fcn, filter_l);
				}
			}
		}
	}

// Main loop
	double *inbuf_old = old[channel];
	for(k = 0; 1; k++)
	{
		double time0;
		int joff;
		
		time0 = k * resample_ratio;
		j = (int)floor(time0 - itime[channel]);

		if(j + (filter_l + 1) / 2 >= in_len) break;

/* blackman filter.  by default, window centered at j+.5(filter_l%2) */
/* but we want a window centered at time0.   */
		offset = (time0 - itime[channel] - (j + .5 * (filter_l % 2)));
		joff = (int)floor((offset * 2 * BPC) + BPC + .5);
		xvalue = 0;

		for(i = 0; i <= filter_l; i++)
		{
			int j2 = i + j - filter_l / 2;
			double y = ((j2 < 0) ? inbuf_old[BLACKSIZE + j2] : input[j2]);

			xvalue += y * blackfilt[joff][i];
		}

		if(output_allocation <= output_size[channel])
		{
			double **new_output = new double*[channels];
			long new_allocation = output_allocation ? (output_allocation * 2) : 16384;
			for(int l = 0; l < channels; l++)
			{
				new_output[l] = new double[new_allocation];
				if(output_temp) 
				{
					memcpy(new_output[l], output_temp[l], output_allocation * sizeof(double));
					delete [] output_temp[l];
				}
			}

			if(output_temp) delete [] output_temp;
			output_temp = new_output;
			output_allocation = new_allocation;
		}

		output_temp[channel][output_size[channel]++] = xvalue;
	}

	num_used = MIN(in_len, j + filter_l / 2);
	itime[channel] += num_used - k * resample_ratio;
	memcpy(inbuf_old, input + num_used - BLACKSIZE, BLACKSIZE * sizeof(double));

	last_ratio = resample_ratio;
}

int Resample::resample(double *output, 
	int out_len,
	int in_rate,
	int out_rate,
	int channel,
	samplenum out_position)
{
	int total_output = 0;
	int reseek = 0;

	if(REPOSITION(last_out_end[channel], out_position))
	{
		reseek = 1;
		reset(channel);
	}

	last_out_end[channel] = out_position + out_len;

	int i = 0;
	while(out_len > 0)
	{
// Drain output buffer
		if(output_size[channel])
		{
			int fragment_len = output_size[channel];
			if(fragment_len > out_len) fragment_len = out_len;

			memcpy(output, output_temp[channel], fragment_len * sizeof(double));

			memmove(output_temp[channel], output_temp[channel] + fragment_len, (output_size[channel] - fragment_len) * sizeof(double)); 

			output_size[channel] -= fragment_len;
			out_len -= fragment_len;
			output += fragment_len;
			out_position += fragment_len;
			total_output += fragment_len;
		}

// Import new samples
		if(out_len > 0)
		{
			inframe->set_samplerate(in_rate);
			if(reseek)
			{
				inframe->set_fill_request((double)out_position / out_rate, input_size);
				reseek = 0;
			}
			else
				inframe->set_fill_request(input_chunk_end[channel], input_size);

			inframe->channel = channel;
			file->get_samples(inframe);

			input_chunk_end[channel] = inframe->position + inframe->get_length();

			resample_chunk(inframe->buffer,
				input_size,
				in_rate,
				out_rate,
				channel);
		}

		i++;
	}

	return total_output;
}
