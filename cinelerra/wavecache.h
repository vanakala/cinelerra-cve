// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef WAVECACHE_H
#define WAVECACHE_H

// Store audio waveform fragments for drawing.
#include "cachebase.h"


class WaveCacheItem : public CacheItemBase
{
public:
	WaveCacheItem();

	size_t get_size();

	int channel;
// End sample in asset samplerate.  Starting point is CacheItemBase::position
	samplenum end;
	double high;
	double low;
};


class WaveCache : public CacheBase
{
public:
	WaveCache();

// Returns the first item on or after the start argument or 0 if none found.
	WaveCacheItem* get_wave(Asset *asset,
		int stream,
		int channel,
		samplenum start,
		samplenum end);
// Put waveform segment into cache
	void put_wave(Asset *asset,
		int stream,
		int channel,
		samplenum start,
		samplenum end,
		double high,
		double low);
};

#endif
