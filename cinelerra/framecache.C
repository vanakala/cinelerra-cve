#include "bcsignals.h"
#include "clip.h"
#include "framecache.h"
#include "mutex.h"
#include "vframe.h"


#include <math.h>
#include <string.h>



FrameCacheItem::FrameCacheItem()
 : CacheItemBase()
{
	data = 0;
	position = 0;
	frame_rate = (double)30000.0 / 1001;
}

FrameCacheItem::~FrameCacheItem()
{
	delete data;
}

int FrameCacheItem::get_size()
{
	if(data) return data->get_data_size() + (path ? strlen(path) : 0);
	return 0;
}















FrameCache::FrameCache()
 : CacheBase()
{
}

FrameCache::~FrameCache()
{
}


// Returns 1 if frame exists in cache and copies it to the frame argument.
int FrameCache::get_frame(VFrame *frame, 
	int64_t position,
	double frame_rate)
{
	lock->lock("FrameCache::get_frame");
	FrameCacheItem *result = 0;

	if(frame_exists(frame,
		position, 
		frame_rate,
		&result))
	{
		if(result->data) 
		{
			frame->copy_from(result->data);
			frame->copy_stacks(result->data);
		}
		result->age = get_age();
	}

	lock->unlock();
	if(result) return 1;
	return 0;
}


VFrame* FrameCache::get_frame_ptr(int64_t position,
	double frame_rate,
	int color_model,
	int w,
	int h)
{
	lock->lock("FrameCache::get_frame_ptr");
	FrameCacheItem *result = 0;
	if(frame_exists(position,
		frame_rate,
		color_model,
		w,
		h,
		&result))
	{
		result->age = get_age();
		return result->data;
	}


	lock->unlock();
	return 0;
}

// Puts frame in cache if enough space exists and the frame doesn't already
// exist.
void FrameCache::put_frame(VFrame *frame, 
	int64_t position,
	double frame_rate,
	int use_copy)
{
	lock->lock("FrameCache::put_frame");
	FrameCacheItem *item = 0;
	if(frame_exists(frame,
		position, 
		frame_rate,
		&item))
	{
		item->age = get_age();
		lock->unlock();
		return;
	}


	item = new FrameCacheItem;

	if(use_copy)
	{
		item->data = new VFrame(*frame);
	}
	else
	{
		item->data = frame;
	}

	item->position = position;
	item->frame_rate = frame_rate;
	item->age = get_age();

	put_item(item);
	lock->unlock();
}




int FrameCache::frame_exists(VFrame *format,
	int64_t position, 
	double frame_rate,
	FrameCacheItem **item_return)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(position);
	while(item && item->position == position)
	{
		if(EQUIV(item->frame_rate, frame_rate) &&
			format->equivalent(item->data, 1))
		{
			*item_return = item;
			return 1;
		}
		else
			item = (FrameCacheItem*)item->next;
	}
	return 0;
}

int FrameCache::frame_exists(int64_t position, 
	double frame_rate,
	int color_model,
	int w,
	int h,
	FrameCacheItem **item_return)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(position);
	while(item && item->position == position)
	{
		if(EQUIV(item->frame_rate, frame_rate) &&
			color_model == item->data->get_color_model() &&
			w == item->data->get_w() &&
			h == item->data->get_h())
		{
			*item_return = item;
			return 1;
		}
		else
			item = (FrameCacheItem*)item->next;
	}
	return 0;
}


void FrameCache::dump()
{
// 	lock->lock("FrameCache::dump");
// 	printf("FrameCache::dump 1 %d\n", items.total);
// 	for(int i = 0; i < items.total; i++)
// 	{
// 		FrameCacheItem *item = (FrameCacheItem*)items.values[i];
// 		printf("  position=%lld frame_rate=%f age=%d size=%d\n", 
// 			item->position, 
// 			item->frame_rate, 
// 			item->age,
// 			item->data->get_data_size());
// 	}
// 	lock->unlock();
}




