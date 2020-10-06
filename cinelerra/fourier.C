// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "atmpframecache.h"
#include "cinelerra.h"
#include "bcsignals.h"
#include "clip.h"
#include "fourier.h"

#include <math.h>
#include <fftw3.h>
#include <values.h>
#include <string.h>

Mutex Fourier::plans_lock = Mutex("Fourier::plans_lock");

Fourier::Fourier(int winsize, int oversample)
{
	window_size = winsize;
	half_size = window_size / 2;
	this->oversample = oversample;
	prev_frame_end = -1;
	fftw_window = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * window_size);

	plans_lock.lock("Fourier");
	plan_forward = fftw_plan_dft_1d(window_size, fftw_window, fftw_window,
		FFTW_FORWARD, FFTW_ESTIMATE);

	plan_backward = fftw_plan_dft_1d(window_size, fftw_window, fftw_window,
		FFTW_BACKWARD, FFTW_ESTIMATE);

	if(oversample)
	{
		pre_window = new double[window_size];
		post_window = new double[window_size];
		accumulator = new double[2 * window_size];
		step_size = window_size / oversample;
		latency = window_size - step_size;
		pre_shift_buffer = new double[2 * AUDIO_BUFFER_SIZE];

		for(int i = 0; i < window_size; i++)
		{
			pre_window[i] = 0.5 - 0.5 * cos(2. * M_PI * i / window_size);
			post_window[i] = 0.667 * pre_window[i] /
				(half_size * oversample);
		}
	}
	else
	{
		pre_window = 0;
		post_window = 0;
		accumulator = 0;
		pre_shift_buffer = 0;
	}
	plans_lock.unlock();
}

Fourier::~Fourier()
{
	fftw_destroy_plan(plan_forward);
	fftw_destroy_plan(plan_backward);
	fftw_free(fftw_window);
	delete [] pre_window;
	delete [] post_window;
	delete [] accumulator;
	delete [] pre_shift_buffer;
}

AFrame *Fourier::process_frame(AFrame *aframe)
{
	int frame_pos = 0;
	int output_pos = 0;
	AFrame *output = 0;
	int frame_length = aframe->get_length();
	int pre_shift_end;

	aframe->clear_buffer();

	if(accumulator)
	{
		ptstime frame_pts = aframe->get_pts();

		if(!PTSEQU(prev_frame_end, frame_pts))
			clear_accumulator();
		prev_frame_end = aframe->get_end_pts();

		for(int i = 0; i < frame_length; i++)
			pre_shift_buffer[i + latency] = aframe->buffer[i];
		pre_shift_end = frame_length + latency;
	}

	while(frame_pos < frame_length)
	{
		if(pre_window)
		{
			for(int i = 0; i < window_size; i++)
			{
				if(frame_pos + i < pre_shift_end)
					fftw_window[i][0] = pre_shift_buffer[frame_pos + i] *
						pre_window[i];
				else
					fftw_window[i][0] = 0;
				fftw_window[i][1] = 0;
			}
			frame_pos += step_size;
		}
		else
		{
			for(int i = 0; i < window_size; i++)
			{
				fftw_window[i][0] = aframe->buffer[frame_pos + i];
				fftw_window[i][1] = 0;
			}
			frame_pos += window_size;
		}
		fftw_execute_dft(plan_forward, fftw_window, fftw_window);
		if(signal_process())
		{
			fftw_execute_dft(plan_backward, fftw_window, fftw_window);

			if(!output)
			{
				output = audio_frames.clone_frame(aframe);
				output->set_pts(aframe->get_pts());
			}

			if(post_window)
			{
				for(int i = 0; i < window_size; i++)
					accumulator[i] += fftw_window[i][0] * post_window[i];

				for(int i = 0; i < step_size; i++)
					output->buffer[output_pos + i] =
						accumulator[i];

				memmove(accumulator, accumulator + step_size,
						window_size * sizeof(double));
				output_pos += step_size;
			}
			else
			{
				for(int i = 0; i < window_size; i++)
				{
					output->buffer[output_pos + i] =
						fftw_window[i][0] / window_size;
				}
				output_pos += window_size;
			}
		}
	}

	if(pre_shift_buffer)
		memmove(pre_shift_buffer, &pre_shift_buffer[frame_length],
			latency * sizeof(double));

	if(output)
	{
		output->set_filled(aframe->get_length());
		audio_frames.release_frame(aframe);
		aframe = output;
	}
	return aframe;
}

void Fourier::clear_accumulator()
{
	if(accumulator)
	{
		memset(accumulator, 0, 2 * window_size * sizeof(double));
		if(latency)
			memset(pre_shift_buffer, 0, latency * sizeof(double));
	}
}

void Fourier::dump(int indent)
{
	printf("%*sFourier %p winsize %d dump:\n", indent, "", this, window_size);
	if(window_size > 0)
	{
		double re_max = -DBL_MAX;
		double im_max = -DBL_MAX;
		double re_min = DBL_MAX;
		double im_min = DBL_MAX;
		double re_avg = 0;
		double im_avg = 0;
		int remaxpos;
		int reminpos;
		int immaxpos;
		int imminpos;

		remaxpos = reminpos = immaxpos = imminpos = -1;

		for(int i = 0; i < window_size; i++)
		{
			re_avg += fftw_window[i][0];
			im_avg += fftw_window[i][1];
			if(fftw_window[i][0] > re_max)
			{
				re_max = fftw_window[i][0];
				remaxpos = i;
			}
			if(fftw_window[i][0] < re_min)
			{
				re_min = fftw_window[i][0];
				reminpos = i;
			}
			if(fftw_window[i][1] > im_max)
			{
				im_max = fftw_window[i][1];
				immaxpos = i;
			}
			if(fftw_window[i][1] < im_min)
			{
				im_min = fftw_window[i][1];
				imminpos = i;
			}
		}
		printf("%*sre: avg %.3g min[%d] %.3g max[%d] %.3g\n", indent, "",
			re_avg / window_size, reminpos, re_min, remaxpos, re_max);
		printf("%*sim: avg %.3g min[%d] %.3g max[%d] %.3g\n", indent, "",
			im_avg / window_size, imminpos, im_min, immaxpos, im_max);
	}
}

void Fourier::dump_file(const char *filename, int samplerate, int append)
{
	const char *mode;
	FILE *fp;
	int k;
	double itmrate;
	double itmval;

	if(append)
		mode = "a";
	else
		mode = "w";

	if(samplerate > 0)         // frequency
		itmrate = (double)samplerate / window_size;
	else if(samplerate < 0)    // time
		itmrate = 1.0 / -samplerate;
	else
		itmrate = 1.0;    // elem number

	if(!(fp = fopen(filename, mode)))
	{
		perror(filename);
		return;
	}

	for(int i = 0; i < window_size; i++)
	{
		itmval = itmrate * i;
		k = fwrite(&itmval, 1, sizeof(double), fp);
		k += fwrite(&fftw_window[i][0], 1, sizeof(double), fp);
		k += fwrite(&fftw_window[i][1], 1, sizeof(double), fp);
		if(k != 24)
		{
			fprintf(stderr, "Writing to %s at elem %d failed\n", filename, i);
			break;
		}
	}
	fclose(fp);
}

Pitch::Pitch(int samplerate, int window_size)
 : Fourier(window_size, 4)
{
	scale = 1;
	this->samplerate = samplerate;
	reset_phase = 1;
	last_phase = new double[window_size];
	new_freq = new double[window_size];
	new_magn = new double[window_size];
	sum_phase = new double[window_size];
}

Pitch::~Pitch()
{
	delete [] last_phase;
	delete [] new_freq;
	delete [] new_magn;
	delete [] sum_phase;
}

void Pitch::set_scale(double new_scale)
{
	scale = new_scale;
	reset_phase = 1;
}

int Pitch::signal_process()
{
	int window_size = get_window_size();
	int half_size = window_size / 2;
	int oversample = get_oversample();

	memset(new_freq, 0, window_size * sizeof(double));
	memset(new_magn, 0, window_size * sizeof(double));

	if(reset_phase)
	{
		memset(last_phase, 0, window_size * sizeof(double));
		memset(sum_phase, 0, window_size * sizeof(double));
		clear_accumulator();
		reset_phase = 0;
	}

// frequency per bin
	double freq_per_bin = (double)samplerate / window_size;
	double expect = 2. * M_PI * (double)step_size / window_size;

	for(int i = 0; i < window_size; i++)
	{
// Convert to magnitude and phase
		double re = fftw_window[i][0];
		double im = fftw_window[i][1];

		double magn = 2.0 * sqrt(re * re + im * im);
		double phase = atan2(im, re);
// Remember last phase
		double temp = phase - last_phase[i];
		last_phase[i] = phase;
		temp -= expect * i;
// wrap temp into -/+ PI ...  good trick!
		int qpd = temp / M_PI;

		if(qpd >= 0)
			qpd += qpd & 1;
		else
			qpd -= qpd & 1;
		temp -= M_PI * qpd;

		temp = oversample * temp / (2. * M_PI);
		temp = (temp + i) * freq_per_bin;
// Now temp is the real freq... move it!
		int new_bin = round(i * scale);

		if(new_bin >= 0 && new_bin < window_size)
		{
			new_freq[new_bin] = temp * scale;
			new_magn[new_bin] += magn;
		}
	}

// Synthesize back the fft window
	for(int i = 0; i < window_size; i++)
	{
		double magn = new_magn[i];
		double temp = new_freq[i];
// substract the bin frequency
		temp -= freq_per_bin * i;
// get bin deviation from freq deviation
		temp /= freq_per_bin;
// take oversample into account
		temp = 2.0 * M_PI * temp / oversample;
		temp += expect * i;

// accumulate delta phase, to get bin phase
		sum_phase[i] += temp;
		double phase = sum_phase[i];

		fftw_window[i][0] = magn * cos(phase);
		fftw_window[i][1] = magn * sin(phase);
	}
	return 1;
}
