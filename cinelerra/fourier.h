// SPDX-License-Identifier: GPL-2.0-or-later

// This file is part of Cinelerra-CVE
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef FOURIER_H
#define FOURIER_H

#include "aframe.h"
#include "fourier.inc"
#include "mutex.h"
#include <fftw3.h>

class Fourier
{
public:
	Fourier(int win_size = 4096, int oversample = 0);
	~Fourier();

	AFrame *process_frame(AFrame *aframe);
	int get_window_size() { return window_size; };
	int get_oversample() { return oversample; };
	// called when fftw_window contains fourier transformation
	// return nz when inverse transformation is needed
	virtual int signal_process() { return 0; };
	void clear_accumulator();
	void dump(int indent = 0);
	void dump_file(const char *filename, int samplerate = 0, int append = 0);

	fftw_complex *fftw_window;
protected:
	int latency;
	int step_size;
private:
	fftw_plan plan_forward;
	fftw_plan plan_backward;
	int window_size;
	int half_size;
	int oversample;
	static Mutex plans_lock;
	double *pre_window;
	double *post_window;
	double *accumulator;
	double *pre_shift_buffer;
	ptstime prev_frame_end;
};

class Pitch : public Fourier
{
public:
	Pitch(int samplerate, int window_size);
	~Pitch();

	int signal_process();
	void set_scale(double new_scale);

	int samplerate;
	int reset_phase;
	double scale;
	double *last_phase;
	double *new_freq;
	double *new_magn;
	double *sum_phase;
};

#endif
