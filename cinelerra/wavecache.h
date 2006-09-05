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
