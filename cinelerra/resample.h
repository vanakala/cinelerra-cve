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

#endif
