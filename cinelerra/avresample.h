// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef AVRESAMPLE_H
#define AVRESAMPLE_H

#include "aframe.inc"
#include "avresample.inc"

extern "C"
{
#include <libswresample/swresample.h>
}

class AVResample
{
public:
	AVResample();
	~AVResample();

	void reset();
	int resample_frame(AFrame *frame, double scale);
	int get_output_size();
	void read_output(double *output, int samples);

private:
	SwrContext *swr_ctx;
	int current_in_samplerate;
	int current_out_samplerate;
	int out_used;
	int out_allocated;
	uint8_t *out_buffer;

};

#endif
