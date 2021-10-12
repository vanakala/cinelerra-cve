// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "mutex.h"
#include "wavecache.h"

#include <string.h>

WaveCacheItem::WaveCacheItem()
 : CacheItemBase()
{
}

size_t WaveCacheItem::get_size()
{
	return sizeof(WaveCacheItem);
}


WaveCache::WaveCache()
 : CacheBase()
{
}

WaveCacheItem* WaveCache::get_wave(Asset *asset,
	int stream,
	int channel,
	samplenum start,
	samplenum end)
{
	lock->lock("WaveCache::get_wave");
	int item_number = -1;
	WaveCacheItem *result = 0;

	result = (WaveCacheItem*)get_item(start);
	while(result && result->position == start)
	{
		if(result->asset == asset && result->stream == stream &&
			result->channel == channel &&
			result->end == end)
		{
			result->age = get_age();
			return result;
		}
		else
			result = (WaveCacheItem*)result->next;
	}

	lock->unlock();
	return 0;
}

void WaveCache::put_wave(Asset *asset,
	int stream,
	int channel,
	samplenum start,
	samplenum end,
	double high,
	double low)
{
	lock->lock("WaveCache::put_wave");
	WaveCacheItem *item = new WaveCacheItem;
	item->asset = asset;
	item->stream = stream;
	item->channel = channel;
	item->position = start;
	item->end = end;
	item->high = high;
	item->low = low;

	put_item(item);
	lock->unlock();
}
