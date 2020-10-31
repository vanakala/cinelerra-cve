// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2013 Einar Rünkaru <einarrunkaru at gmail dot com>

#include <string.h>

#include "aframe.h"
#include "bcsignals.h"
#include "levelhist.h"
#include "tracking.inc"

LevelHistory::LevelHistory()
{
	meter_render_fragment = 0;
	total_peaks = 0;
	channels = 0;
	rate = 0;
	level_pts = 0;

	for(int i = 0; i < MAXCHANNELS; i++)
		level_history[i] = 0;
}

LevelHistory::~LevelHistory()
{
	delete [] level_pts;

	for(int i = 0; i < MAXCHANNELS;i++)
	{
		if(level_history[i])
			delete [] level_history[i];
	}
}

void LevelHistory::reset(int fragment_len, int samplerate, int channels)
{
	int old_total;

	if(meter_render_fragment == fragment_len
			&& rate == samplerate
			&& channels == this->channels)
	{
		current_peak = 0;
		for(int i = 0; i < MAXCHANNELS; i++)
		{
			if(level_history[i])
				memset(level_history[i], 0, total_peaks * sizeof(double));
		}
		return;
	}

	meter_render_fragment = fragment_len;
	this->channels = channels;
	rate = samplerate;

	while(meter_render_fragment > samplerate / TRACKING_RATE_DEFAULT)
			meter_render_fragment /= 2;

	old_total = total_peaks;
	total_peaks = 16 * fragment_len / meter_render_fragment;

	if(level_pts && old_total != total_peaks)
	{
		delete [] level_pts;
		level_pts = 0;
	}

	if(!level_pts)
	{
		level_pts = new ptstime[total_peaks];
		memset(level_pts, 0, total_peaks * sizeof(ptstime));
	}

	current_peak = 0;

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		if(i < channels)
		{
			if(level_history[i] && old_total != total_peaks)
			{
				delete [] level_history[i];
				level_history[i] = 0;
			}
			if(!level_history[i])
				level_history[i] = new double[total_peaks];

			memset(level_history[i], 0, total_peaks * sizeof(double));
		}
		else
		{
			if(level_history[i])
				delete [] level_history[i];

			level_history[i] = 0;
		}
	}
}

void LevelHistory::fill(AFrame **frames)
{
	int len = frames[0]->get_length();
	double peak, sample;
	int start, end;

	if(total_peaks == 0)
		return;

	for(start = 0; start < len;)
	{
		end = start + meter_render_fragment;
		if(end > len)
			end = len;

		level_pts[current_peak] = frames[0]->get_pts() + frames[0]->to_duration(start);

		for(int c = 0; c < channels; c++)
		{
			peak = 0;
			if(frames[c])
			{
				double *buffer = frames[c]->buffer;

				for(int j = start; j < end; j++)
				{
					sample = fabs(buffer[j]);
					if(sample > peak)
						peak = sample;
				}
			}
			level_history[c][current_peak] = peak;
		}
		start = end;
		if(++current_peak >= total_peaks)
			current_peak = 0;
	}
}

void LevelHistory::clear()
{
	if(!total_peaks)
		return;

	current_peak = 0;

	for(int i = 0; i < channels; i++)
		memset(level_history[i], 0, total_peaks * sizeof(double));

	memset(level_pts, 0, total_peaks * sizeof(ptstime));
}

int LevelHistory::get_levels(double *levels, ptstime pts)
{
	ptstime min_diff = 86400;
	int c, pkx;

	if(total_peaks == 0)
		return 0;

	pkx = 0;
	for(int i = 0; i < total_peaks; i++)
	{
		if(fabs(level_pts[i] - pts) < min_diff)
		{
			min_diff = fabs(level_pts[i] - pts);
			pkx = i;
		}
	}

	for(c = 0; c < channels; c++)
		levels[c] = level_history[c][pkx];

	return c;
}
