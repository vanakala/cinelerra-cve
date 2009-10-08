
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

#include "asset.h"
#include "bcsignals.h"
#include "clip.h"
#include "framecache.h"
#include "mutex.h"
#include "vframe.h"


#include <math.h>
#include <string.h>
#include <unistd.h>



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
	int layer,
	double frame_rate,
	int asset_id)
{
	lock->lock("FrameCache::get_frame");
	FrameCacheItem *result = 0;

	if(frame_exists(frame,
		position, 
		layer,
		frame_rate,
		&result,
		asset_id))
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
	int layer,
	double frame_rate,
	int color_model,
	int w,
	int h,
	int asset_id)
{
	lock->lock("FrameCache::get_frame_ptr");
	FrameCacheItem *result = 0;
	if(frame_exists(position,
		layer,
		frame_rate,
		color_model,
		w,
		h,
		&result,
		asset_id))
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
	int layer,
	double frame_rate,
	int use_copy,
	Asset *asset)
{
	lock->lock("FrameCache::put_frame");
	FrameCacheItem *item = 0;
	if(frame_exists(frame,
		position, 
		layer,
		frame_rate,
		&item,
		asset ? asset->id : -1))
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

// Copy metadata
	item->position = position;
	item->layer = layer;
	item->frame_rate = frame_rate;
	if(asset)
	{
		item->asset_id = asset->id;
		item->path = strdup(asset->path);
	}
	else
	{
		item->asset_id = -1;
	}
	item->age = get_age();

	put_item(item);
	lock->unlock();
}




int FrameCache::frame_exists(VFrame *format,
	int64_t position, 
	int layer,
	double frame_rate,
	FrameCacheItem **item_return,
	int asset_id)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(position);
	while(item && item->position == position)
	{
		if(EQUIV(item->frame_rate, frame_rate) &&
			layer == item->layer &&
			format->equivalent(item->data, 1) &&
			(asset_id == -1 || item->asset_id == -1 || asset_id == item->asset_id))
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
	int layer,
	double frame_rate,
	int color_model,
	int w,
	int h,
	FrameCacheItem **item_return,
	int asset_id)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(position);
	while(item && item->position == position)
	{
		if(EQUIV(item->frame_rate, frame_rate) &&
			layer == item->layer &&
			color_model == item->data->get_color_model() &&
			w == item->data->get_w() &&
			h == item->data->get_h() &&
			(asset_id == -1 || item->asset_id == -1 || asset_id == item->asset_id))
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




