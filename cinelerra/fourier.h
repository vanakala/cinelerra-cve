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
	Fourier(int win_size = 4096);
	~Fourier();

	AFrame *process_frame(AFrame *aframe);
	void symmetry();
	int get_window_size() { return window_size; };
	// called when fftw_window contains fourier transformation
	// return nz when inverse transformation is needed
	virtual int signal_process() { return 0; };
	void dump(int indent = 0);
	void dump_file(const char *filename, int samplerate = 0, int append = 0);

	fftw_complex *fftw_window;
private:
	fftw_plan plan_forward;
	fftw_plan plan_backward;
	int window_size;
	int half_size;
	static Mutex plans_lock;
};

#endif
