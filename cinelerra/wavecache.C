#include "asset.h"
#include "mutex.h"
#include "wavecache.h"

#include <string.h>

WaveCacheItem::WaveCacheItem()
 : CacheItemBase()
{
}

WaveCacheItem::~WaveCacheItem()
{
}

int WaveCacheItem::get_size()
{
	return sizeof(WaveCacheItem) + (path ? strlen(path) : 0);
}



WaveCache::WaveCache()
 : CacheBase()
{
}

WaveCache::~WaveCache()
{
}


WaveCacheItem* WaveCache::get_wave(int asset_id,
	int channel,
	int64_t start,
	int64_t end)
{
	lock->lock("WaveCache::get_wave");
	int item_number = -1;
	WaveCacheItem *result = 0;
	
	result = (WaveCacheItem*)get_item(start);
	while(result && result->position == start)
	{
		if(result->asset_id == asset_id && 
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
	int channel,
	int64_t start,
	int64_t end,
	double high,
	double low)
{
	lock->lock("WaveCache::put_wave");
	WaveCacheItem *item = new WaveCacheItem;
	item->asset_id = asset->id;
	item->path = strdup(asset->path);
	item->channel = channel;
	item->position = start;
	item->end = end;
	item->high = high;
	item->low = low;
	
	put_item(item);
	lock->unlock();
}






