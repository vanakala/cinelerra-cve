#include "clip.h"
#include "file.h"
#include "resample.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

// Resampling from Lame

Resample::Resample(File *file, int channels)
{
//printf("Resample::Resample 1 %d\n", channels);
	this->file = file;
	this->channels = channels;

	old = new double*[channels];
	for(int i = 0; i < channels; i++)
	{
		old[i] = new double[BLACKSIZE];
	}
	itime = new double[channels];
	output_temp_start = new long[channels];
	bzero(output_temp_start, sizeof(long) * channels);
	resample_init = new int[channels];
	bzero(resample_init, sizeof(int) * channels);
	last_ratio = 0;
	output_temp = 0;
	output_size = new long[channels];
	bzero(output_size, sizeof(long) * channels);
	output_allocation = 0;
	input_size = RESAMPLE_CHUNKSIZE;
	input_chunk_end = new long[channels];
	bzero(input_chunk_end, sizeof(long) * channels);
	input = new double[input_size];
	last_out_end = new long[channels];
	bzero(last_out_end, sizeof(long) * channels);
//printf("Resample::Resample 2 %d\n", channels);
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

	delete [] input_chunk_end;
	delete [] input;
	delete [] old;
	delete [] itime;
	delete [] output_temp_start;
	delete [] output_size;
	delete [] last_out_end;
}

void Resample::reset(int channel)
{
//printf("Resample::reset 1 channel=%d normalized_sample_rate=%d\n", channel, file->normalized_sample_rate);
	if(channel < 0)
	{
		bzero(resample_init, sizeof(int) * channels);
		bzero(output_size, sizeof(long) * channels);
		bzero(last_out_end, sizeof(long) * channels);
		bzero(input_chunk_end, sizeof(long) * channels);
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
	long in_len,
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

//printf("Resample::resample_chunk 1\n");
  	intratio = (fabs(resample_ratio - floor(.5 + resample_ratio)) < .0001);
	fcn = .90 / resample_ratio;
	if(fcn > .90) fcn = .90;
	filter_l = BLACKSIZE - 6;  
/* must be odd */
	if(0 == filter_l % 2 ) --filter_l;  

//printf("Resample::resample_chunk 2\n");
/* if resample_ratio = int, filter_l should be even */
  	filter_l += (int)intratio;

//printf("Resample::resample_chunk 3\n");
// Blackman filter initialization must be called whenever there is a 
// sampling ratio change
	if(!resample_init[channel] || last_ratio != resample_ratio)
	{
		resample_init[channel] = 1;
		itime[channel] = 0;
		bzero(old[channel], sizeof(double) * BLACKSIZE);

//printf("Resample::resample_chunk 4\n");
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

//printf("Resample::resample_chunk 5\n");
// Main loop
	double *inbuf_old = old[channel];
	for(k = 0; 1; k++)
	{
		double time0;
		int joff;
		
//printf("Resample::resample_chunk 6\n");
		time0 = k * resample_ratio;
		j = (int)floor(time0 - itime[channel]);

//		if(j + filter_l / 2 >= input_size) break;
		if(j + filter_l / 2 >= in_len) break;

//printf("Resample::resample_chunk 7\n");
/* blackman filter.  by default, window centered at j+.5(filter_l%2) */
/* but we want a window centered at time0.   */
		offset = (time0 - itime[channel] - (j + .5 * (filter_l % 2)));
		joff = (int)floor((offset * 2 * BPC) + BPC + .5);
		xvalue = 0;

//printf("Resample::resample_chunk 8\n");
		for(i = 0; i <= filter_l; i++)
		{
			int j2 = i + j - filter_l / 2;
			double y = ((j2 < 0) ? inbuf_old[BLACKSIZE + j2] : input[j2]);

//printf("Resample::resample_chunk 9\n");
			xvalue += y * blackfilt[joff][i];
		}
		
//printf("Resample::resample_chunk 10\n");
		if(output_allocation <= output_size[channel])
		{
			double **new_output = new double*[channels];
			long new_allocation = output_allocation ? (output_allocation * 2) : 16384;
			for(int l = 0; l < channels; l++)
			{
				new_output[l] = new double[new_allocation];
				if(output_temp) 
				{
					bcopy(output_temp[l], new_output[l], output_allocation * sizeof(double));
					delete [] output_temp[l];
				}
			}

			if(output_temp) delete [] output_temp;
			output_temp = new_output;
			output_allocation = new_allocation;
		}

//printf("Resample::resample_chunk 11 %d %d\n", output_size[channel], output_allocation);
		output_temp[channel][output_size[channel]++] = xvalue;
	}

//printf("Resample::resample_chunk 12\n");
	num_used = MIN(in_len, j + filter_l / 2);
	itime[channel] += num_used - k * resample_ratio;
	for(i = 0; i < BLACKSIZE; i++)
		inbuf_old[i] = input[num_used + i - BLACKSIZE];

//printf("Resample::resample_chunk 13\n");
	last_ratio = resample_ratio;
}

void Resample::read_chunk(double *input, long len, int &reseek, int iteration)
{
//printf("Resample::read_chunk 1\n");
	if(reseek)
	{
		file->set_audio_position(file->current_sample, 0);
		reseek= 0;
	}
	else
	if(iteration == 0)
	{
// Resume at the end of the last resample call
		file->set_audio_position(input_chunk_end[file->current_channel], 0);
	}

	file->read_samples(input, len, 0);
	input_chunk_end[file->current_channel] = file->current_sample;

//printf("Resample::read_chunk 2\n");
}

int Resample::resample(double *output, 
	long out_len,
	int in_rate,
	int out_rate,
	int channel,
	long in_position,
	long out_position)
{
	int total_input = 0;
	int reseek = 0;

#define REPOSITION(x, y) \
	(labs((x) - (y)) > 1)



//printf("Resample::resample 1 last_out_end=%d out_position=%d\n", last_out_end[channel], out_position);

	if(REPOSITION(last_out_end[channel], out_position))
	{
		reseek = 1;
		reset(channel);
	}






	output_temp_start[channel] = file->get_audio_position(out_rate) + out_len;
	last_out_end[channel] = out_position + out_len;

	int i = 0;
	while(out_len > 0)
	{
// Drain output buffer
		if(output_size[channel])
		{
			int fragment_len = output_size[channel];
			if(fragment_len > out_len) fragment_len = out_len;

//printf("Resample::resample 1 %d %d %d\n", out_len, output_size[channel], channel);
			bcopy(output_temp[channel], output, fragment_len * sizeof(double));

// Shift leftover forward
			for(int i = fragment_len; i < output_size[channel]; i++)
				output_temp[channel][i - fragment_len] = output_temp[channel][i];

			output_size[channel] -= fragment_len;
			out_len -= fragment_len;
			output += fragment_len;
		}

// Import new samples
//printf("Resample::resample 2 %d %d\n", out_len, channel);
		if(out_len > 0)
		{
//printf("Resample::resample 3 input_size=%d reseek=%d out_position=%d channel=%d\n", input_size, reseek, out_position, channel);
			read_chunk(input, input_size, reseek, i);
			resample_chunk(input,
				input_size,
				in_rate,
				out_rate,
				channel);
			total_input += input_size;
		}

		i++;
	}
//printf("Resample::resample 2 %d %d\n", last_out_end[channel], out_position);
//printf("Resample::resample 2 %d %d %d\n", out_len, output_size[channel], channel);

//printf("Resample::resample 2 %d %d\n", channel, output_size[channel]);

	return total_input;
}
