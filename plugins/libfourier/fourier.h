#ifndef FOURIER_H
#define FOURIER_H


#include <stdint.h>


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

// Called by process_buffer to read samples from input.
// Returns 1 on error or 0 on success.
	virtual int read_samples(int64_t output_sample, 
		int samples, 
		double *buffer);

// Process a window in the frequency domain
	virtual int signal_process();        

// Size of a window.  Automatically fixed to a power of 2
	long window_size;   

// Output of FFT
	double *freq_real;
	double *freq_imag;

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
};

#endif
