#ifndef FOURIER_H
#define FOURIER_H



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
// Process all windows in the input
	int process_fifo(long size, double *input_ptr, double *output_ptr);

	virtual int signal_process() {};        // Process in the frequency domain

// data for fourier
	long window_size;   // Size of a window.  Automatically fixed to a power of 2

	double *input_buffer;   // input for complete windows
	double *output_buffer;  // output for crossfaded windows

	long input_size;      // samples in input_buffer and output_buffer
	long output_size;
	long input_allocation;
	long output_allocation;

// Output of FFT
	double *freq_real;
	double *freq_imag;

	double *temp_real;
	double *temp_imag;

private:
	int first_window;   // Don't crossfade the first window
};

#endif
