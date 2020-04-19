// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "atmpframecache.h"
#include "bcsignals.h"
#include "clip.h"
#include "fourier.h"

#include <math.h>
#include <string.h>
#include <fftw3.h>

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
			fftw_window[i][0] = aframe->buffer[frame_pos + i]; // * pre_window[i];
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
					fftw_window[i][0] / window_size; // * post_window[i];
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
