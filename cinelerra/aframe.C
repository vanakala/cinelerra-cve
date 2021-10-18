// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2011 Einar Rünkaru <einarrunkaru@gmail dot com>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "aframe.h"
#include "bcsignals.h"

AFrame::AFrame(int buflen, int float_data)
{
	reset_buffer();
	channel = 0;
	samplerate = 0;
	shared = 0;
	buffer = 0;
	float_buffer = 0;
	buffer_length = 0;
	this->float_data = float_data;
	trackno = -1;
	if(buflen > 0)
	{
		if(float_data)
			float_buffer = new float[buflen];
		else
			buffer = new double[buflen];
		buffer_length = buflen;
	}
}

AFrame::~AFrame()
{
	if(!shared)
	{
		if(buffer)
			delete [] buffer;
		if(float_buffer)
			delete [] float_buffer;
	}
}

void AFrame::set_buffer(double *newbuffer, int length)
{
	if(newbuffer && length)
	{
		if(!shared)
		{
			if(buffer)
				delete [] buffer;
			buffer = 0;
			if(float_buffer)
				delete [] float_buffer;
			float_buffer = 0;
		}
		float_data = 0;
		shared = 1;
		buffer = newbuffer;
		buffer_length = length;
	}
}

void AFrame::set_buffer(float *newbuffer, int length)
{
	if(newbuffer && length)
	{
		if(!shared)
		{
			if(buffer)
				delete [] buffer;
			buffer = 0;
			if(float_buffer)
				delete [] float_buffer;
			float_buffer = 0;
		}
		float_data = 1;
		shared = 1;
		float_buffer = newbuffer;
		buffer_length = length;
	}
}

void AFrame::check_buffer(int length)
{
	if(!shared && buffer_length < length)
	{
		if(buffer)
			delete [] buffer;
		buffer = 0;
		if(float_buffer)
			delete [] float_buffer;
		float_buffer = 0;
		if(float_data)
			float_buffer = new float[length];
		else
			buffer = new double[length];
		buffer_length = length;
		this->length = 0;
	}
}

void AFrame::clear_buffer(void)
{
	if(buffer)
		memset(&buffer[length], 0, (buffer_length - length) * sizeof(double));
	if(float_buffer)
		memset(&float_buffer[length], 0, (buffer_length - length) * sizeof(float));
}

void AFrame::clear_frame(ptstime pts, ptstime duration)
{
	set_filled(duration);
	set_pts(pts);

	if(buffer)
		memset(buffer, 0, length * sizeof(double));
	if(float_buffer)
		memset(float_buffer, 0, length * sizeof(float));
}

void AFrame::reset_buffer(void)
{
	pts = 0;
	duration = 0;
	source_pts = 0;
	source_duration = 0;
	source_length = 0;
	position = 0;
	length = 0;
}

void AFrame::set_empty()
{
	duration = 0;
	length = 0;
}

void AFrame::init_aframe(ptstime postime, int len, int rate)
{
	samplerate = rate;
	reset_buffer();
	check_buffer(len);
	clear_buffer();
	pts = round_to_sample(postime);
}

void AFrame::copy_pts(AFrame *that)
{
	if(this == that)
		return;

	pts = that->pts;
	source_pts = that->source_pts;
	source_duration = that->source_duration;
	source_length = that->source_length;
	length = 0;
	duration = 0;
	samplerate = that->samplerate;
	channel = that->channel;
}

void AFrame::copy(AFrame *that)
{
	if(this == that)
		return;

	copy_pts(that);
	float_data = that->float_data;
	check_buffer(that->length);
	if(float_data)
		memcpy(float_buffer, that->float_buffer, sizeof(float) * that->length);
	else
		memcpy(buffer, that->buffer, sizeof(double) * that->length);
	length = that->length;
	duration = that->duration;
}

void AFrame::copy_of(AFrame *that)
{
	copy_pts(that);
	float_data = that->float_data;
	check_buffer(that->length);
	length = that->length;
	duration = that->duration;
}

void AFrame::set_filled(int length)
{
	check_buffer(length);
	this->length = length;
	duration = round_to_sample((ptstime)length / samplerate);
}

void AFrame::set_filled(ptstime duration)
{
	int length = to_samples(duration);

	check_buffer(length);
	this->length = length;
	this->duration = to_duration(length);
}

samplenum AFrame::to_samples(ptstime duration)
{
	return round(duration * samplerate);
}

ptstime AFrame::to_duration(samplenum samples)
{
	if(!samplerate)
	{
		printf("Missing samplerate in audio frame.\n");
		return 0;
	}
	return (ptstime)samples / samplerate;
}

samplenum AFrame::fill_position(int srcpos)
{
	if(srcpos || source_pts)
		return round(source_pts * samplerate);
	return (samplenum)(round(pts * samplerate)) + length;
}

int AFrame::fill_length()
{
	int len = 0;

	if(source_duration <= 0)
	{
		if(source_length > 0)
			len = source_length;
	} else
		len = round(source_duration * samplerate);
	if(len + length > buffer_length)
		len = buffer_length - length;
	return len;
}

void AFrame::set_filled_length()
{
	length += source_length;
	duration = round_to_sample((ptstime)length / samplerate);
}

void AFrame::set_fill_request(ptstime pts, ptstime duration)
{
	reset_buffer();
	source_pts = this->pts = round_to_sample(pts);
	source_duration = round_to_sample(duration);
	source_length = to_samples(source_duration);
}

void AFrame::set_fill_request(ptstime pts, int length)
{
	reset_buffer();
	this->source_pts = this->pts = round_to_sample(pts);
	source_length = length;
	source_duration = to_duration(length);
}

void AFrame::set_fill_request(samplenum position, int length)
{
	reset_buffer();
	this->source_pts = this->pts = to_duration(position);
	source_length = length;
	source_duration = to_duration(length);
}

int AFrame::ptsequ(ptstime t1, ptstime t2)
{
	if(fabs(t1 - t2) <= ((double) 1 / samplerate))
		return 1;
	return 0;
}

ptstime AFrame::round_to_sample(ptstime t)
{
	return to_duration(to_samples(t));
}

ptstime AFrame::set_pts(ptstime t)
{
	pts = round_to_sample(t);
	return pts;
}

ptstime AFrame::set_duration(ptstime t)
{
	length = to_samples(t);
	duration = to_duration(length);
	return duration;
}

void AFrame::set_length(int l)
{
	length = l;
	duration = to_duration(l);
}

ptstime AFrame::set_source_pts(ptstime t)
{
	source_pts = round_to_sample(t);
	return source_pts;
}

ptstime AFrame::set_source_duration(ptstime t)
{
	source_length = to_samples(t);
	source_duration = to_duration(source_length);
	return source_duration;
}

void AFrame::set_source_length(int length)
{
	source_length = length;
	source_duration = to_duration(length);
}

void AFrame::set_track(int number)
{
	trackno = number;
}

int AFrame::get_track()
{
	return trackno;
}

size_t AFrame::get_data_size()
{
	if(shared)
		return 0;
	if(float_data)
		return buffer_length * sizeof(float);
	return buffer_length * sizeof(double);
}

void AFrame::dump(int indent, int dumpdata)
{
	double avg, min, max;

	printf("%*sAFrame %p dump: trackno %d\n", indent, "", this, trackno);
	indent += 2;
	printf("%*spts %.3f[%.3f=%d] src:pts %.3f[%.3f=%d] channel %d rate %d sample %" PRId64 "\n", indent, "",
		pts, duration, length, source_pts, source_duration, source_length,
		channel, samplerate, position);
	printf("%*sbuffer %p float_buffer %p buffer_length %d shared %d float_data %d\n", indent, "",
		buffer, float_buffer, buffer_length, shared, float_data);
	if(dumpdata && length > 0)
	{
		if(buffer)
			BC_Signals::show_array(buffer, length, indent, 1);

		if(float_buffer)
			BC_Signals::show_array(float_buffer, length, indent, 1);
	}
}
