
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

#ifndef WAVECACHE_H
#define WAVECACHE_H



// Store audio waveform fragments for drawing.
#include "cachebase.h"


class WaveCacheItem : public CacheItemBase
{
public:
	WaveCacheItem();
	~WaveCacheItem();

	int get_size();
	
	int channel;
// End sample in asset samplerate.  Starting point is CacheItemBase::position
	int64_t end;
	double high;
	double low;
};





class WaveCache : public CacheBase
{
public:
	WaveCache();
	~WaveCache();

// Returns the first item on or after the start argument or 0 if none found.
	WaveCacheItem* get_wave(int asset_id,
		int channel,
		int64_t start,
		int64_t end);
// Put waveform segment into cache
	void put_wave(Asset *asset,
		int channel,
		int64_t start,
		int64_t end,
		double high,
		double low);
};



#endif
