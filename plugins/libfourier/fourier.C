#include <math.h>
#include <stdio.h>
#include <string.h>

#include "clip.h"
#include "fourier.h"
#include "transportque.inc"

#define HALF_WINDOW (window_size / 2)


FFT::FFT()
{
}

FFT::~FFT()
{
}

int FFT::do_fft(unsigned int samples,  // must be a power of 2
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
	return 0;
}

int FFT::update_progress(int current_position)
{
	return 0;
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

int FFT::symmetry(int size, double *freq_real, double *freq_imag)
{
    int h = size / 2;
    for(int i = h + 1; i < size; i++)
    {
        freq_real[i] = freq_real[size - i];
        freq_imag[i] = -freq_imag[size - i];
    }
	return 0;
}





CrossfadeFFT::CrossfadeFFT() : FFT()
{
	reset();
	window_size = 4096;
}

CrossfadeFFT::~CrossfadeFFT()
{
	delete_fft();
}

int CrossfadeFFT::reset()
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
	return 0;
}

int CrossfadeFFT::delete_fft()
{
	if(input_buffer) delete [] input_buffer;
	if(output_buffer) delete [] output_buffer;
	if(freq_real) delete [] freq_real;
	if(freq_imag) delete [] freq_imag;
	if(temp_real) delete [] temp_real;
	if(temp_imag) delete [] temp_imag;
	reset();
	return 0;
}

int CrossfadeFFT::fix_window_size()
{
// fix the window size
// window size must be a power of 2
	int new_size = 16;
	while(new_size < window_size) new_size *= 2;
	window_size = MIN(131072, window_size);
	window_size = new_size;
	return 0;
}

int CrossfadeFFT::initialize(int window_size)
{
	this->window_size = window_size;
	first_window = 1;
	reconfigure();
	return 0;
}

long CrossfadeFFT::get_delay()
{
	return window_size + HALF_WINDOW;
}

int CrossfadeFFT::reconfigure()
{
	delete_fft();
	fix_window_size();
	
	
	
	return 0;
}

// int CrossfadeFFT::process_fifo(long size, 
// 	double *input_ptr, 
// 	double *output_ptr)
// {
// // Load next input buffer
// 	if(input_size + size > input_allocation)
// 	{
// 		double *new_input = new double[input_size + size];
// 		if(input_buffer)
// 		{
// 			memcpy(new_input, input_buffer, sizeof(double) * input_size);
// 			delete [] input_buffer;
// 		}
// 		input_buffer = new_input;
// 		input_allocation = input_size + size;
// 	}
// 
// 	memcpy(input_buffer + input_size, 
// 		input_ptr, 
// 		size * sizeof(double));
// 	input_size += size;
// 
// 
// 
// 
// 
// 
// 
// // Have enough to do some windows
// 	while(input_size >= window_size)
// 	{
// 		if(!freq_real) freq_real = new double[window_size];
// 		if(!freq_imag) freq_imag = new double[window_size];
// 		if(!temp_real) temp_real = new double[window_size];
// 		if(!temp_imag) temp_imag = new double[window_size];
// 	
// 	
// 	
// 		do_fft(window_size,  // must be a power of 2
//     		0,         // 0 = forward FFT, 1 = inverse
//     		input_buffer,     // array of input's real samples
//     		0,     // array of input's imag samples
//     		freq_real,    // array of output's reals
//     		freq_imag);
// 
// 		int result = signal_process();
// 
// 		if(!result)
// 		{
// 			do_fft(window_size,  // must be a power of 2
//     			1,               // 0 = forward FFT, 1 = inverse
//     			freq_real,     // array of input's real samples
//     			freq_imag,     // array of input's imag samples
//     			temp_real,     // array of output's reals
//     			temp_imag);
// 		}
// 
// 
// // Crossfade into the output buffer
// 		long new_allocation = output_size + window_size;
// 		if(new_allocation > output_allocation)
// 		{
// 			double *new_output = new double[new_allocation];
// 
// 			if(output_buffer)
// 			{
// 				memcpy(new_output, output_buffer, sizeof(double) * output_size);
// 				delete [] output_buffer;
// 			}
// 			output_buffer = new_output;
// 			output_allocation = new_allocation;
// 		}
// 
// 		if(output_size >= HALF_WINDOW)
// 		{
// 			for(int i = 0, j = output_size - HALF_WINDOW; 
// 				i < HALF_WINDOW; 
// 				i++, j++)
// 			{
// 				double src_level = (double)i / HALF_WINDOW;
// 				double dst_level = (double)(HALF_WINDOW - i) / HALF_WINDOW;
// 				output_buffer[j] = output_buffer[j] * dst_level + temp_real[i] * src_level;
// 			}
// 
// 			memcpy(output_buffer + output_size, 
// 				temp_real + HALF_WINDOW, 
// 				sizeof(double) * (window_size - HALF_WINDOW));
// 			output_size += window_size - HALF_WINDOW;
// 		}
// 		else
// 		{
// // First buffer has no crossfade
// 			memcpy(output_buffer + output_size, 
// 				temp_real, 
// 				sizeof(double) * window_size);
// 			output_size += window_size;
// 		}
// 
// 
// // Shift input buffer forward
// 		for(int i = window_size - HALF_WINDOW, j = 0; 
// 			i < input_size; 
// 			i++, j++)
// 			input_buffer[j] = input_buffer[i];
// 		input_size -= window_size - HALF_WINDOW;
// 	}
// 
// 
// 
// 
// // Have enough to send to output
// 	int samples_rendered = 0;
// 	if(output_size - HALF_WINDOW >= size)
// 	{
// 		memcpy(output_ptr, output_buffer, sizeof(double) * size);
// 		for(int i = size, j = 0; i < output_size; i++, j++)
// 			output_buffer[j] = output_buffer[i];
// 		output_size -= size;
// 		samples_rendered = size;
// 	}
// 	else
// 	{
// 		bzero(output_ptr, sizeof(double) * size);
// 	}
// 
// 	return samples_rendered;
// }



int CrossfadeFFT::process_buffer(int64_t output_sample, 
	long size, 
	double *output_ptr,
	int direction)
{
	int result = 0;
	int step = (direction == PLAY_FORWARD) ? 1 : -1;

// User seeked so output buffer is invalid
	if(output_sample != this->output_sample)
	{
		output_size = 0;
		input_size = 0;
		first_window = 1;
		this->output_sample = output_sample;
		this->input_sample = output_sample;
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
			result = read_samples(this->input_sample,
				window_size,
				input_buffer);
		else
			result = read_samples(this->input_sample + step * HALF_WINDOW,
				HALF_WINDOW,
				input_buffer + HALF_WINDOW);

		input_size = window_size;

		if(!result)
			do_fft(window_size,   // must be a power of 2
    			0,                // 0 = forward FFT, 1 = inverse
    			input_buffer,     // array of input's real samples
    			0,                // array of input's imag samples
    			freq_real,        // array of output's reals
    			freq_imag);
		if(!result)
			result = signal_process();
		if(!result)
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
		for(int i = window_size - HALF_WINDOW, j = 0;
			i < input_size;
			i++, j++)
		{
			input_buffer[j] = input_buffer[i];
		}
		input_size = HALF_WINDOW;
		this->input_sample += step * HALF_WINDOW;
	}



// Transfer output buffer
	if(output_ptr)
	{
		memcpy(output_ptr, output_buffer, sizeof(double) * size);
	}
	for(int i = 0, j = size; j < output_size + HALF_WINDOW; i++, j++)
		output_buffer[i] = output_buffer[j];
	this->output_sample += step * size;
	this->output_size -= size;

	return 0;
}


int CrossfadeFFT::read_samples(int64_t output_sample, 
		int samples, 
		double *buffer)
{
	return 1;
}

int CrossfadeFFT::signal_process()
{
	return 0;
}
