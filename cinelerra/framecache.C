#include "clip.h"
#include "framecache.h"
#include "mutex.h"
#include "vframe.h"


#include <math.h>

FrameCacheItem::FrameCacheItem()
{
	data = 0;
	position = 0;
	frame_rate = (double)30000.0 / 1001;
	age = 0;
}

FrameCacheItem::~FrameCacheItem()
{
	if(data) delete data;
}
















FrameCache::FrameCache()
{
	lock = new Mutex("FrameCache::lock");
	max_bytes = 0;
	current_age = 0;
}

FrameCache::~FrameCache()
{
	items.remove_all_objects();
	delete lock;
}


// Returns 1 if frame exists in cache and copies it to the frame argument.
int FrameCache::get_frame(VFrame *frame, 
	int64_t position,
	double frame_rate)
{
	lock->lock("FrameCache::get_frame");
	int item_number = -1;

	if(frame_exists(frame,
		position, 
		frame_rate,
		&item_number))
	{
		FrameCacheItem *item = items.values[item_number];
		if(item->data) frame->copy_from(item->data);
		item->age = current_age;
		current_age++;
	}

	lock->unlock();
	if(item_number >= 0) return 1;
	return 0;
}


VFrame* FrameCache::get_frame_ptr(int64_t position,
	double frame_rate,
	int color_model,
	int w,
	int h)
{
	lock->lock("FrameCache::get_frame");
	int item_number = -1;
	FrameCacheItem *item = 0;
	if(frame_exists(position,
		frame_rate,
		color_model,
		w,
		h,
		&item_number))
	{
		item = items.values[item_number];
		item->age = current_age;
		current_age++;
	}


	if(item)
		return item->data;
	else
	{
		lock->unlock();
		return 0;
	}
}

void FrameCache::unlock()
{
    lock->unlock();
}

// Puts frame in cache if enough space exists and the frame doesn't already
// exist.
void FrameCache::put_frame(VFrame *frame, 
	int64_t position,
	double frame_rate,
	int use_copy)
{
	lock->lock("FrameCache::put_frame");
	int item_number = -1;
	if(frame_exists(frame,
		position, 
		frame_rate,
		&item_number))
	{
		FrameCacheItem *item = items.values[item_number];
		item->age = current_age;
		current_age++;
		lock->unlock();
		return;
	}


	FrameCacheItem *item = new FrameCacheItem;

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
	item->age = current_age;

	items.append(item);
	current_age++;
	lock->unlock();
}




int FrameCache::frame_exists(VFrame *format,
	int64_t position, 
	double frame_rate,
	int *item_return)
{
	for(int i = 0; i < items.total; i++)
	{
		FrameCacheItem *item = items.values[i];
		if(item->position == position &&
			EQUIV(item->frame_rate, frame_rate) &&
			format->equivalent(item->data))
		{
			*item_return = i;
			return 1;
		}
	}
	return 0;
}

int FrameCache::frame_exists(int64_t position, 
	double frame_rate,
	int color_model,
	int w,
	int h,
	int *item_return)
{
	for(int i = 0; i < items.total; i++)
	{
		FrameCacheItem *item = items.values[i];
		if(item->position == position &&
			EQUIV(item->frame_rate, frame_rate) &&
			color_model == item->data->get_color_model() &&
			w == item->data->get_w() &&
			h == item->data->get_h())
		{
			*item_return = i;
			return 1;
		}
	}
	return 0;
}

// Calculate current size of cache in bytes
int64_t FrameCache::get_memory_usage()
{
	int64_t result = 0;
	lock->lock("FrameCache::get_memory_usage");
	for(int i = 0; i < items.total; i++)
	{
		FrameCacheItem *item = items.values[i];
		result += item->data->get_data_size();
	}
	lock->unlock();
	return result;
}

int FrameCache::delete_oldest()
{
	int64_t oldest = 0x7fffffff;
	int oldest_item = -1;

	lock->lock("FrameCache::delete_oldest");
	for(int i = 0; i < items.total; i++)
	{
		if(items.values[i]->age < oldest)
		{
			oldest = items.values[i]->age;
			oldest_item = i;
		}
	}

	if(oldest_item >= 0)
	{
		items.remove_object_number(oldest_item);
		lock->unlock();
		return 0;
	}
	lock->unlock();
	return 1;
}

void FrameCache::dump()
{
	lock->lock("FrameCache::dump");
	printf("FrameCache::dump 1 %d\n", items.total);
	for(int i = 0; i < items.total; i++)
	{
		FrameCacheItem *item = items.values[i];
		printf("  position=%lld frame_rate=%f age=%d size=%d\n", 
			item->position, 
			item->frame_rate, 
			item->age,
			item->data->get_data_size());
	}
	lock->unlock();
}




