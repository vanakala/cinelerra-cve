// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "atmpframecache.h"
#include "bcsignals.h"
#include "clip.h"
#include "fourier.h"

#include <math.h>
#include <fftw3.h>
#include <values.h>

Mutex Fourier::plans_lock = Mutex("Fourier::plans_lock");

Fourier::Fourier(int winsize)
{
	window_size = winsize;
	half_size = window_size / 2;
	fftw_window = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * window_size);

	plans_lock.lock("Fourier");
	plan_forward = fftw_plan_dft_1d(window_size, fftw_window, fftw_window,
		FFTW_FORWARD, FFTW_ESTIMATE);

	plan_backward = fftw_plan_dft_1d(window_size, fftw_window, fftw_window,
		FFTW_BACKWARD, FFTW_ESTIMATE);
	plans_lock.unlock();
}

Fourier::~Fourier()
{
	fftw_destroy_plan(plan_forward);
	fftw_destroy_plan(plan_backward);
	fftw_free(fftw_window);
}

AFrame *Fourier::process_frame(AFrame *aframe)
{
	int frame_pos = 0;
	int output_pos = 0;
	AFrame *output = 0;

	aframe->clear_buffer();

	while(frame_pos < aframe->length)
	{
		for(int i = 0; i < window_size; i++)
		{
			fftw_window[i][0] = aframe->buffer[frame_pos + i];
			fftw_window[i][1] = 0;
		}
		frame_pos += window_size;
		fftw_execute_dft(plan_forward, fftw_window, fftw_window);
		if(signal_process())
		{
			fftw_execute_dft(plan_backward, fftw_window, fftw_window);
			if(!output)
			{
				output = audio_frames.clone_frame(aframe);
				output->pts = aframe->pts;
			}
			for(int i = 0; i < window_size; i++)
			{
				output->buffer[output_pos + i] =
					fftw_window[i][0] / window_size;
			}
			output_pos += window_size;
		}
	}

	if(output)
	{
		output->set_filled(aframe->length);
		audio_frames.release_frame(aframe);
		aframe = output;
	}
	return aframe;
}

void Fourier::symmetry(int size, fftw_complex *fftw_window)
{
	int h = size / 2;

	for(int i = h + 1; i < size; i++)
	{
		fftw_window[i][0] = fftw_window[size - i][0];
		fftw_window[i][1] = -fftw_window[size - i][1];
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
