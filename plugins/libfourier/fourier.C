
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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "aframe.h"
#include "bcsignals.h"
#include "clip.h"
#include "fourier.h"

#define HALF_WINDOW (window_size / 2)


// we need to do some trickery to get around of fftw thread unsafetyness
fftw_plan_desc *FFT::fftw_plans = 0;
Mutex FFT::plans_lock = Mutex();

FFT::FFT()
{
}

FFT::~FFT()
{
}

void FFT::do_fft(unsigned int samples,  // must be a power of 2
	int inverse,         // 0 = forward FFT, 1 = inverse
	double *real_in,     // array of input's real samples
	double *imag_in,     // array of input's imag samples
	double *real_out,    // array of output's reals
	double *imag_out)
{
	unsigned int num_bits;    // Number of bits needed to store indices
	unsigned int i, j, k, n;
	unsigned int block_size, block_end;

	double angle_numerator = 2.0 * M_PI;
	double tr, ti;     // temp real, temp imaginary

	if(inverse)
		angle_numerator = -angle_numerator;

	num_bits = samples_to_bits(samples);

// Do simultaneous data copy and bit-reversal ordering into outputs

	for(i = 0; i < samples; i++)
	{
		j = reverse_bits(i, num_bits);
		real_out[j] = real_in[i];
		imag_out[j] = (imag_in == 0) ? 0.0 : imag_in[i];
	}

// Do the FFT itself

	block_end = 1;
	double delta_angle;
	double sm2;
	double sm1;
	double cm2;
	double cm1;
	double w;
	double ar[3], ai[3];
	double temp;
	for(block_size = 2; block_size <= samples; block_size <<= 1)
	{
		delta_angle = angle_numerator / (double)block_size;
		sm2 = sin(-2 * delta_angle);
		sm1 = sin(-delta_angle);
		cm2 = cos(-2 * delta_angle);
		cm1 = cos(-delta_angle);
		w = 2 * cm1;

		for(i = 0; i < samples; i += block_size)
		{
			ar[2] = cm2;
			ar[1] = cm1;

			ai[2] = sm2;
			ai[1] = sm1;

			for(j = i, n = 0; n < block_end; j++, n++)
			{
				ar[0] = w * ar[1] - ar[2];
				ar[2] = ar[1];
				ar[1] = ar[0];

				ai[0] = w * ai[1] - ai[2];
				ai[2] = ai[1];
				ai[1] = ai[0];

				k = j + block_end;
				tr = ar[0] * real_out[k] - ai[0] * imag_out[k];
				ti = ar[0] * imag_out[k] + ai[0] * real_out[k];

				real_out[k] = real_out[j] - tr;
				imag_out[k] = imag_out[j] - ti;

				real_out[j] += tr;
				imag_out[j] += ti;
			}
		}

		block_end = block_size;
	}

// Normalize if inverse transform

	if(inverse)
	{
		double denom = (double)samples;

		for (i = 0; i < samples; i++)
		{
			real_out[i] /= denom;
			imag_out[i] /= denom;
		}
	}
}

unsigned int FFT::samples_to_bits(unsigned int samples)
{
	unsigned int i;

	for(i = 0; ; i++)
	{
		if(samples & (1 << i))
			return i;
	}
	return i;
}

unsigned int FFT::reverse_bits(unsigned int index, unsigned int bits)
{
	unsigned int i, rev;

	for(i = rev = 0; i < bits; i++)
	{
		rev = (rev << 1) | (index & 1);
		index >>= 1;
	}

	return rev;
}

void FFT::symmetry(int size, double *freq_real, double *freq_imag)
{
	int h = size / 2;
	for(int i = h + 1; i < size; i++)
	{
		freq_real[i] = freq_real[size - i];
		freq_imag[i] = -freq_imag[size - i];
	}
}

// Create a proper fftw plan to be used later
void FFT::ready_fftw(unsigned int samples)
{
// FFTW plan generation is not thread safe, so we have to take precausions
	FFT::plans_lock.lock();
	fftw_plan_desc *plan;

	my_fftw_plan = 0;

	for (plan = fftw_plans; plan; plan = plan->next)
		if (plan->samples == samples) 
		{
			my_fftw_plan = plan;
			break;
		}

	if (!my_fftw_plan)
	{
		fftw_complex *temp_data = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * samples);
		my_fftw_plan = new fftw_plan_desc;   // we never discard this, since they are static
		my_fftw_plan->samples = samples;
		my_fftw_plan->plan_forward = fftw_plan_dft_1d(samples, temp_data, temp_data, FFTW_FORWARD, FFTW_ESTIMATE);
		my_fftw_plan->plan_backward = fftw_plan_dft_1d(samples, temp_data, temp_data, FFTW_BACKWARD, FFTW_ESTIMATE);
		// We will use this plan only in guru mode so we can now discard the temp_data
		fftw_free(temp_data);

		// Put the plan into the linked list
		my_fftw_plan->next = fftw_plans;
		fftw_plans = my_fftw_plan;
	}

	FFT::plans_lock.unlock();
}

void FFT::do_fftw_inplace(unsigned int samples,
		int inverse,
		fftw_complex *data)
{
	if (inverse == 0)
		fftw_execute_dft(my_fftw_plan->plan_forward, data, data);
	else
		fftw_execute_dft(my_fftw_plan->plan_backward, data, data);
}


CrossfadeFFT::CrossfadeFFT(int winsize) 
 : FFT()
{
	input_buffer = 0;
	output_buffer = 0;
	freq_real = 0;
	freq_imag = 0;
	temp_real = 0;
	temp_imag = 0;
	first_window = 1;
// samples in input_buffer and output_buffer
	input_size = 0;
	output_size = 0;
	input_allocation = 0;
	output_allocation = 0;
	output_sample = 0;
	input_sample = 0;
	samples_ready = 0;
	oversample = 0;
	pre_window = 0;
	post_window = 0;
	fftw_data = 0;

// window size must be a power of 2
	window_size = 16;
	while(window_size < winsize) window_size *= 2;
	window_size = MIN(131072, window_size);
}

CrossfadeFFT::~CrossfadeFFT()
{
	if(input_buffer) delete [] input_buffer;
	if(output_buffer) delete [] output_buffer;
	if(freq_real) delete [] freq_real;
	if(freq_imag) delete [] freq_imag;
	if(temp_real) delete [] temp_real;
	if(temp_imag) delete [] temp_imag;
	if(pre_window) delete [] pre_window;
	if(post_window) delete [] post_window;
	if(fftw_data) fftw_free(fftw_data);
}

void CrossfadeFFT::process_frame(AFrame *aframe)
{
	int size = aframe->source_length;

	if(!PTSEQU(aframe->pts, aframe->to_duration(output_sample)) || first_window)
	{
		output_size = 0;
		input_size = 0;
		first_window = 1;
		input_sample = output_sample = aframe->to_samples(aframe->pts);
		input_frame.samplerate = aframe->samplerate;
	}

// Fill output buffer half a window at a time until size samples are available
	while(output_size < size)
	{
		if(!input_buffer) input_buffer = new double[window_size];
		if(!freq_real) freq_real = new double[window_size];
		if(!freq_imag) freq_imag = new double[window_size];
		if(!temp_real) temp_real = new double[window_size];
		if(!temp_imag) temp_imag = new double[window_size];

// Fill enough input to make a window starting at output_sample
		if(first_window)
		{
			input_frame.set_buffer(input_buffer, window_size);
			input_frame.set_fill_request(input_sample, window_size);
		}
		else
		{
			input_frame.set_buffer(input_buffer + HALF_WINDOW, HALF_WINDOW);
			input_frame.set_fill_request(input_sample + HALF_WINDOW, HALF_WINDOW);
		}
		get_frame(&input_frame);

		input_size = window_size;

		do_fft(window_size,   // must be a power of 2
			0,                // 0 = forward FFT, 1 = inverse
			input_buffer,     // array of input's real samples
			0,                // array of input's imag samples
			freq_real,        // array of output's reals
			freq_imag);
		signal_process();
		do_fft(window_size,  // must be a power of 2
			1,               // 0 = forward FFT, 1 = inverse
			freq_real,       // array of input's real samples
			freq_imag,       // array of input's imag samples
			temp_real,       // array of output's reals
			temp_imag);      // array of output's imaginaries

// Allocate output buffer
		int new_allocation = output_size + window_size;
		if(new_allocation > output_allocation)
		{
			double *new_output = new double[new_allocation];
			if(output_buffer)
			{
				memcpy(new_output, 
					output_buffer, 
					sizeof(double) * (output_size + HALF_WINDOW));
				delete [] output_buffer;
			}
			output_buffer = new_output;
			output_allocation = new_allocation;
		}
// Overlay processed buffer
		if(first_window)
		{
			memcpy(output_buffer + output_size,
				temp_real,
				sizeof(double) * window_size);
			first_window = 0;
		}
		else
		{
			for(int i = 0, j = output_size; i < HALF_WINDOW; i++, j++)
			{
				double src_level = (double)i / HALF_WINDOW;
				double dst_level = (double)(HALF_WINDOW - i) / HALF_WINDOW;
				output_buffer[j] = output_buffer[j] * dst_level +
					temp_real[i] * src_level;
			}

			memcpy(output_buffer + output_size + HALF_WINDOW,
				temp_real + HALF_WINDOW,
				sizeof(double) * HALF_WINDOW);
		}

		output_size += HALF_WINDOW;

// Shift input buffer
		memmove(input_buffer,
			&input_buffer[window_size - HALF_WINDOW],
			(input_size - (window_size - HALF_WINDOW)) * sizeof(double));
		input_size = HALF_WINDOW;
		input_sample += HALF_WINDOW;
	}

// Transfer output buffer
	if(aframe->buffer)
	{
		aframe->set_filled(size);
		memcpy(aframe->buffer, output_buffer, sizeof(double) * size);
	}
	memmove(output_buffer, output_buffer + size, (output_size + HALF_WINDOW - size) * sizeof(double));
	output_sample += size;
	output_size -= size;
}

void CrossfadeFFT::set_oversample(int oversample) 
{
// Only powers of two can be used for oversample
	int oversample_fix = 2;
	while(oversample_fix < oversample) oversample_fix *= 2;
	this->oversample = oversample = oversample_fix;

// Precalculate the pre-envelope hanning window
	pre_window = new double[window_size];
	for (int i = 0; i< window_size; i++) 
		pre_window[i] = 0.5 - 0.5 *cos(2 * M_PI * i / window_size); 

// Precalculate the post-envelope hanning window also, we could have triangle here also
	post_window = new double[window_size];

	for (int i = 0; i< window_size; i++) 
		post_window[i] = (0.5 - 0.5 *cos(2 * M_PI * i / window_size)) * 6/ oversample / window_size; 

	ready_fftw(window_size);
}

void CrossfadeFFT::process_frame_oversample(AFrame *aframe)
{
	if(oversample <= 0)
	{
		// set_oversample() has to be called to use process_buffer_oversample
		return;
	}

	int overlap_size = window_size / oversample;
	int total_size;
	int start_skip;
	int size = aframe->source_length;

	if(!PTSEQU(aframe->pts, aframe->to_duration(output_sample)) || first_window)
	{
		input_size = 0;
		first_window = 1;
		output_sample = aframe->to_samples(aframe->pts);
		input_frame.samplerate = aframe->samplerate;
		samples_ready = 0;
		start_skip = window_size - overlap_size;
		total_size = size + start_skip;
// signal_process() will always be able to know which window it has by looking at input_sample
		input_sample = output_sample - start_skip;
	} 
	else
	{
		start_skip = 0;
		total_size = size;
		first_window = 0;
	}

// Find out how big output buffer we will need, take overlapping into account
	int new_allocation = total_size + window_size;
	if(new_allocation > output_allocation)
	{
		double *new_output = new double[new_allocation];
		if(output_buffer)
		{
			memcpy(new_output, 
				output_buffer, 
				sizeof(double) * (samples_ready + window_size - overlap_size));
			delete [] output_buffer;
		}
		output_buffer = new_output;
		output_allocation = new_allocation;
	}
// Fill output buffer by overlap_size at a time until size samples are available
	while(samples_ready < total_size)
	{
		if(!input_buffer) input_buffer = new double[window_size];
		if(!fftw_data) fftw_data = (fftw_complex *)fftw_malloc(window_size * sizeof(fftw_complex));

// Fill enough input to make a window starting at output_sample
		samplenum read_start;
		int write_pos;
		int read_len;

		if(first_window)
		{
			read_start = this->input_sample;
			write_pos = 0;
			read_len = window_size;
		}
		else
		{
			read_start = this->input_sample + window_size - overlap_size;
			write_pos = window_size - overlap_size;
			read_len = overlap_size;
		}

		if (read_start + read_len < 0)
		{
// completely outside the track
			memset (input_buffer + write_pos, 0, read_len * sizeof(double));
		}
		else
		if (read_start < 0)
		{
// special case for reading before the track - in general it would be sensible that this behaviour is done by read_samples()
			memset (input_buffer + write_pos, 0, - read_start * sizeof(double));
			input_frame.set_buffer(input_buffer - read_start + write_pos, read_start + read_len);
			input_frame.set_fill_request((samplenum)0, read_start + read_len);
		}
		else
		{
			input_frame.set_buffer(input_buffer + write_pos, read_len);
			input_frame.set_fill_request(read_start, read_len);
		}
		get_frame(&input_frame);

// apply Hanning window to input samples
		for (int i = 0; i< window_size; i++) 
		{
			fftw_data[i][0] = input_buffer[i] * pre_window[i];
			fftw_data[i][1] = 0;
		}

		do_fftw_inplace(window_size, 0, fftw_data);
		signal_process_oversample(first_window);
		do_fftw_inplace(window_size, 1, fftw_data);

// Overlay over existing output - overlap processing
		for (int i = 0; i < window_size - overlap_size; i++)
			output_buffer[i + samples_ready] += fftw_data[i][0] * post_window[i]; 
		for (int i = window_size - overlap_size; i < window_size; i++)
			output_buffer[i + samples_ready] = fftw_data[i][0] * post_window[i];

// Shift input buffer
		memmove(input_buffer, input_buffer + overlap_size, (window_size - overlap_size) * sizeof(double));

		input_sample += overlap_size;

		samples_ready += overlap_size;
		first_window = 0;
	}

	memcpy(aframe->buffer, output_buffer + start_skip , aframe->source_length * sizeof(double));
	aframe->set_filled(aframe->source_length);
	samples_ready -= total_size;

	memmove(output_buffer, 
		output_buffer + total_size, 
		(samples_ready + window_size - overlap_size) * sizeof(double));
	this->output_sample += size;
}
