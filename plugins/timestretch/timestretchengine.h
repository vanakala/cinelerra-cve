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



