
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include <stdio.h>
#include <string.h>
#include "aframe.h"
#include "bcsignals.h"

AFrame::AFrame(int buflen)
{
	reset_buffer();
	channel = 0;
	samplerate = 0;
	shared = 0;
	buffer = 0;
	buffer_length = 0;
	if(buflen > 0)
	{
		buffer = new double[buflen];
		buffer_length = buflen;
	}
}

AFrame::~AFrame()
{
	if(!shared && buffer)
		delete [] buffer;
}

void AFrame::set_buffer(double *newbuffer, int length)
{
	if(newbuffer && length)
	{
		if(!shared && buffer)
			delete [] buffer;
		shared = 1;
		buffer = newbuffer;
		buffer_length = length;
	}
}

void AFrame::check_buffer(int length)
{
	if(!shared && buffer_length < length)
	{
		if(buffer)
			delete [] buffer;
		buffer = new double[length];
		buffer_length = length;
		this->length = 0;
	}
}

void AFrame::clear_buffer(void)
{
	if(buffer)
		memset(&buffer[length], 0, (buffer_length - length) * sizeof(double));
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

void AFrame::init_aframe(ptstime postime, int len)
{
	reset_buffer();
	check_buffer(len);
	clear_buffer();
	pts = postime;
}

void AFrame::copy_pts(AFrame *that)
{
	pts = that->pts;
	length = 0;
	duration = 0;
	samplerate = that->samplerate;
	channel = that->channel;
}

void AFrame::copy(AFrame *that)
{
	copy_pts(that);
	check_buffer(that->length);
	memcpy(buffer, that->buffer, sizeof(double) * that->length);
	length = that->length;
	duration = that->duration;
}

void AFrame::copy_of(AFrame *that)
{
	copy_pts(that);
	check_buffer(that->length);
	length = that->length;
	duration = that->duration;
}

void AFrame::set_filled(int length)
{
	check_buffer(length);
	this->length = length;
	duration = (ptstime)length / samplerate;
}

void AFrame::extend_buffer(int length)
{
	if(!shared && buffer_length < length)
	{
		if(buffer_length)
		{
			double *oldbuf = buffer;
			buffer = new double[length];
			if(this->length)
				memcpy(buffer, oldbuf, this->length * sizeof(double));
			delete [] oldbuf;
		}
		buffer_length = length;
	}
}

samplenum AFrame::to_samples(ptstime duration)
{
	return round(duration * samplerate);
}

ptstime AFrame::to_duration(samplenum samples)
{
	return (ptstime)samples / samplerate;
}

ptstime AFrame::get_source_duration()
{
	if(source_duration)
		return source_duration;
	return (ptstime)source_length / samplerate;
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

void AFrame::set_fill_request(ptstime pts, ptstime duration)
{
	reset_buffer();
	this->pts = pts;
	source_duration = duration;
}

void AFrame::set_fill_request(ptstime pts, int length)
{
	reset_buffer();
	this->pts = pts;
	source_length = length;
}

void AFrame::set_fill_request(samplenum position, int length)
{
	reset_buffer();
	this->pts = to_duration(position);
	source_length = length;
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

void AFrame::dump(int dumpdata)
{
	double avg, min, max;

	printf("AFrame::dump: %p\n", this);
	printf("    pts %.3f[%.3f=%d] src:pts %.3f[%.3f=%d] chnl %d rate %d sample %lld\n",
		pts, duration, length, source_pts, source_duration, source_length, channel, samplerate, position);
	printf("    buffer %p buffer_length %d shared %d\n", buffer, buffer_length, shared);
	if(dumpdata && length > 0)
	{
		avg = 0;
		min = +100;
		max = -100;
		for(int i = 0; i < length; i++)
		{
			avg += fabs(buffer[i]);
			if(buffer[i] < min)
				min = buffer[i];
			if(buffer[i] > max)
				max = buffer[i];
		}
		printf("    buffer vals avg %.4f min %.3f max %.3f\n", 
			avg/length, min, max);
	}
}
