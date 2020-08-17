// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "aframe.h"
#include "avresample.h"
#include "bcsignals.h"
#include "language.h"
#include "mainerror.h"

AVResample::AVResample()
{
	swr_ctx = 0;
	current_in_samplerate = 0;
	current_out_samplerate = 0;
	out_used = 0;
	out_buffer = 0;
	out_allocated = 0;
}

AVResample::~AVResample()
{
	swr_free(&swr_ctx);
	delete [] out_buffer;
}

void AVResample::reset()
{
	out_used = 0;
	if(swr_ctx)
		swr_init(swr_ctx);
}

int AVResample::resample_frame(AFrame *frame, double scale)
{
	int out_samplerate = round(frame->get_samplerate() * scale);
	int in_samplerate = frame->get_samplerate();
	int in_samples = frame->get_length();
	int out_samples;
	int out_bytes;
	uint8_t *obufp;
	const uint8_t *ibufp;

	if(in_samplerate != current_in_samplerate ||
		out_samplerate != current_out_samplerate)
	{
		swr_ctx = swr_alloc_set_opts(NULL,
			AV_CH_LAYOUT_MONO,
			AV_SAMPLE_FMT_DBLP,
			out_samplerate,
			AV_CH_LAYOUT_MONO,
			AV_SAMPLE_FMT_DBLP,
			in_samplerate,
			0, 0);
		if(!swr_ctx)
		{
			errormsg(_("Failed to allocate resampler context"));
			return 1;
		}
		if(swr_init(swr_ctx))
		{
			errormsg(_("Failed to initialize resampler"));
			return 1;
		}
		current_in_samplerate = in_samplerate;
		current_out_samplerate = out_samplerate;
	}

	out_samples = av_rescale_rnd(swr_get_delay(swr_ctx, in_samplerate) +
		in_samples, out_samplerate, in_samplerate, AV_ROUND_UP);
	out_bytes = out_samples * sizeof(double);

	if(!out_buffer)
	{
		out_buffer = new uint8_t[out_bytes];
		out_allocated = out_bytes;
	}
	else if(out_allocated < out_bytes + out_used)
	{
		uint8_t *new_buffer = new uint8_t[out_bytes + out_used];

		if(out_used)
			memcpy(new_buffer, out_buffer, out_used);
		delete [] out_buffer;
		out_buffer = new_buffer;
		out_allocated = out_bytes + out_used;
	}
	obufp = &out_buffer[out_used];
	ibufp = (uint8_t*)frame->buffer;
	int buffer_samples = swr_convert(swr_ctx, &obufp, out_samples,
		&ibufp, in_samples);
	out_used += buffer_samples * sizeof(double);
	return 0;
}

int AVResample::get_output_size()
{
	return out_used / sizeof(double);
}

void AVResample::read_output(double *output, int samples)
{
	int size = samples * sizeof(double);

	memcpy(output, out_buffer, size);
	if(size < out_used)
	{
		memmove(out_buffer, &out_buffer[size], out_used - size);
		out_used -= size;
	}
	else
		out_used = 0;
}
