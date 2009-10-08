
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

#ifndef FOURIER_H
#define FOURIER_H



#include <stdint.h>
#include <fftw3.h>

#include "mutex.h"

typedef struct fftw_plan_desc {
	int samples;
	fftw_plan plan_forward;
	fftw_plan plan_backward;
	fftw_plan_desc *next;
};

class FFT
{
public:
	FFT();
	~FFT();

	int do_fft(unsigned int samples,  // must be a power of 2
	    	int inverse,         // 0 = forward FFT, 1 = inverse
	    	double *real_in,     // array of input's real samples
	    	double *imag_in,     // array of input's imag samples
    		double *real_out,    // array of output's reals
    		double *imag_out);   // array of output's imaginaries
	int symmetry(int size, double *freq_real, double *freq_imag);
	unsigned int samples_to_bits(unsigned int samples);
	unsigned int reverse_bits(unsigned int index, unsigned int bits);
	virtual int update_progress(int current_position);

	fftw_plan_desc *my_fftw_plan;
	int ready_fftw(unsigned int samples);
	int do_fftw_inplace(unsigned int samples,
		int inverse,
		fftw_complex *data);

// We have to get around the thread unsafety of fftw
	static fftw_plan_desc *fftw_plans;
	static Mutex plans_lock;


};


class CrossfadeFFT : public FFT
{
public:
	CrossfadeFFT();
	virtual ~CrossfadeFFT();

	int reset();
	int initialize(int window_size);
	long get_delay();     // Number of samples fifo is delayed
	int reconfigure();
	int fix_window_size();
	int delete_fft();
	// functioy to be called to initialize oversampling
	void set_oversample(int oversample); // 2, 4,8 are good values
	


// Read enough samples from input to generate the requested number of samples.
// output_sample - tells it if we've seeked and the output overflow is invalid.
// return - 0 on success 1 on failure
// output_sample - start of samples if forward.  End of samples if reverse.
//               It's always contiguous.
// output_ptr - if nonzero, output is put here
// direction - PLAY_FORWARD or PLAY_REVERSE
	int process_buffer(int64_t output_sample,
		long size, 
		double *output_ptr,
		int direction);

	int process_buffer_oversample(int64_t output_sample,
		long size, 
		double *output_ptr,
		int direction);

// Called by process_buffer to read samples from input.
// Returns 1 on error or 0 on success.
	virtual int read_samples(int64_t output_sample, 
		int samples, 
		double *buffer);

// Process a window in the frequency domain, called by process_buffer()
	virtual int signal_process();        

// Process a window in the frequency domain, called by process_buffer_oversample()
// Reset parameter should cause to reset all accumulated data
	virtual int signal_process_oversample(int reset);
   

// Size of a window.  Automatically fixed to a power of 2
	long window_size;   

// Output of FFT
	double *freq_real;
	double *freq_imag;
// data for FFT that is going to be done by FFTW
	fftw_complex *fftw_data;

private:

// input for complete windows
	double *input_buffer;
// output for crossfaded windows with overflow
	double *output_buffer;

	double *temp_real;
	double *temp_imag;

// samples in input_buffer
	long input_size;
// window_size
	long input_allocation;
// Samples in output buffer less window border
	long output_size;
// Space in output buffer including window border
	long output_allocation;
// Starting sample of output buffer in project
	int64_t output_sample;
// Starting sample of input buffer in project
	int64_t input_sample;
// Don't crossfade the first window
	int first_window;


// Number of samples that are already processed and waiting in output_buffer
	int samples_ready; 
// Hanning window precalculated
	double *pre_window;
// Triangle window precalculated
	double *post_window;
protected:
// Oversample factor
	int oversample;

};

#endif
