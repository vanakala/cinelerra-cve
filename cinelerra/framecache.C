
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
	asset = 0;
}

FrameCacheItem::~FrameCacheItem()
{
	delete data;
}

size_t FrameCacheItem::get_size()
{
	size_t size = sizeof(*this);
	if(data)
		size += data->get_data_size();
	return size;
}

void FrameCacheItem::dump(int indent)
{
	printf("%*sFrameCacheItem %p: data %p\n", indent, "",
		this, data);
	CacheItemBase::dump(indent);
	if(data)
		data->dump(indent);
}


FrameCache::FrameCache()
 : CacheBase()
{
}

VFrame* FrameCache::get_frame_ptr(ptstime postime,
	int layer,
	int color_model,
	int w,
	int h,
	Asset *asset)
{
	lock->lock("FrameCache::get_frame_ptr");
	FrameCacheItem *item;

	if(item = frame_exists(postime, layer, color_model,
		w, h, asset))
	{
		item->age = get_age();
		return item->data;
	}

	lock->unlock();
	return 0;
}

// Puts frame in cache if the frame doesn't already exist.
VFrame *FrameCache::put_frame(VFrame *frame, Asset *asset)
{
	lock->lock("FrameCache::put_frame");
	FrameCacheItem *item;

	if(item = frame_exists(frame, asset))
	{
		delete frame;
		item->age = get_age();
		return item->data;
	}

	item = new FrameCacheItem;

	item->data = frame;

// Copy metadata
	item->position = frame->get_source_pts();
	item->asset = asset;
	item->age = get_age();

	put_item(item);
	lock->unlock();
	return frame;
}

FrameCacheItem *FrameCache::frame_exists(VFrame *format, Asset *asset)
{
	ptstime postime = format->get_source_pts();

	FrameCacheItem *item = (FrameCacheItem*)get_item(postime);

	while(item && item->data->pts_in_frame_source(postime))
	{
		if(format->get_layer() == item->data->get_layer() &&
				format->equivalent(item->data) &&
				asset == item->asset)
			return item;

		item = (FrameCacheItem*)item->next;
	}
	return 0;
}

FrameCacheItem  *FrameCache::frame_exists(ptstime postime,
	int layer,
	int color_model,
	int w,
	int h,
	Asset *asset)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(postime);

	while(item && item->data->pts_in_frame_source(postime))
	{
		if(layer == item->data->get_layer() &&
				color_model == item->data->get_color_model() &&
				w == item->data->get_w() &&
				h == item->data->get_h() &&
				asset == item->asset)
			return item;

		item = (FrameCacheItem*)item->next;
	}
	return 0;
}
