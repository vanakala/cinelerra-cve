// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "clip.h"
#include "framecache.h"
#include "mutex.h"
#include "vframe.h"

#include <stdio.h>

FrameCacheItem::FrameCacheItem()
 : CacheItemBase()
{
	data = 0;
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
	CacheItemBase::dump(indent + 2);
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
	Asset *asset,
	int stream)
{
	lock->lock("FrameCache::get_frame_ptr");
	FrameCacheItem *item;

	if(item = frame_exists(postime, layer, color_model,
		w, h, asset, stream))
	{
		item->age = get_age();
		return item->data;
	}

	lock->unlock();
	return 0;
}

// Puts frame in cache if the frame doesn't already exist.
VFrame *FrameCache::put_frame(VFrame *frame, Asset *asset, int stream)
{
	lock->lock("FrameCache::put_frame");
	FrameCacheItem *item;

	if(item = frame_exists(frame, asset, stream))
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
	item->stream = stream;
	item->age = get_age();

	put_item(item);
	lock->unlock();
	return frame;
}

FrameCacheItem *FrameCache::frame_exists(VFrame *format, Asset *asset, int stream)
{
	ptstime postime = format->get_source_pts();

	FrameCacheItem *item = (FrameCacheItem*)get_item(postime);

	while(item && item->data->pts_in_frame_source(postime))
	{
		if(format->get_layer() == item->data->get_layer() &&
				format->equivalent(item->data) &&
				asset == item->asset && stream == item->stream)
			return item;

		item = (FrameCacheItem*)item->next;
	}
	return 0;
}

FrameCacheItem *FrameCache::frame_exists(ptstime postime,
	int layer,
	int color_model,
	int w,
	int h,
	Asset *asset,
	int stream)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(postime);

	while(item && item->data->pts_in_frame_source(postime))
	{
		if(layer == item->data->get_layer() &&
				color_model == item->data->get_color_model() &&
				w == item->data->get_w() &&
				h == item->data->get_h() &&
				asset == item->asset && stream == item->stream)
			return item;

		item = (FrameCacheItem*)item->next;
	}
	return 0;
}

void FrameCache::change_duration(ptstime new_dur, int layer,
	int color_model, int w, int h, Asset *asset, int stream)
{
	lock->lock("FrameCache::change_duration");
	for(FrameCacheItem *current = (FrameCacheItem*)first; current;
		current = (FrameCacheItem*)current->next)
	{
		VFrame *frame = current->data;

		if(current->asset == asset && layer == frame->get_layer() &&
			stream == current->stream &&
			color_model == frame->get_color_model() &&
			h == frame->get_h() && w == frame->get_w())
		{
			frame->set_duration(new_dur);
		}
	}
	lock->unlock();
}
