
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

#include "aframe.h"
#include "asset.h"
#include "bcsignals.h"
#include "cache.h"
#include "cinelerra.h"
#include "clip.h"
#include "condition.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "framecache.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "resourcethread.h"
#include "resourcepixmap.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "wavecache.h"


ResourceThreadItem::ResourceThreadItem(ResourcePixmap *pixmap, 
	Asset *asset,
	int data_type,
	int operation_count)
{
	this->data_type = data_type;
	this->pixmap = pixmap;
	this->asset = asset;
	this->operation_count = operation_count;
	asset->GarbageObject::add_user();
	last = 0;
}

ResourceThreadItem::~ResourceThreadItem()
{
	asset->GarbageObject::remove_user();
}


VResourceThreadItem::VResourceThreadItem(ResourcePixmap *pixmap, 
	int picon_x, 
	int picon_y, 
	int picon_w,
	int picon_h,
	ptstime postime,
	ptstime duration,
	int layer,
	Asset *asset,
	int operation_count)
 : ResourceThreadItem(pixmap, asset, TRACK_VIDEO, operation_count)
{
	this->picon_x = picon_x;
	this->picon_y = picon_y;
	this->picon_w = picon_w;
	this->picon_h = picon_h;
	this->postime = postime;
	this->duration = duration;
	this->layer = layer;
}


AResourceThreadItem::AResourceThreadItem(ResourcePixmap *pixmap, 
	Asset *asset,
	int x,
	int channel,
	samplenum start,
	samplenum end,
	int operation_count)
 : ResourceThreadItem(pixmap, asset, TRACK_AUDIO, operation_count)
{
	this->x = x;
	this->channel = channel;
	this->start = start;
	this->end = end;
}


ResourceThread::ResourceThread(MWindow *mwindow)
{
	this->mwindow = mwindow;
	interrupted = 1;
	draw_lock = new Condition(0, "ResourceThread::draw_lock", 0);
	item_lock = new Mutex("ResourceThread::item_lock");
	aframe = 0;
	prev_x = -1;
	prev_h = 0;
	prev_l = 0;
	operation_count = 0;
	Thread::start();
}

ResourceThread::~ResourceThread()
{
	delete draw_lock;
	delete item_lock;
	delete aframe;
}

void ResourceThread::add_picon(ResourcePixmap *pixmap, 
	int picon_x, 
	int picon_y, 
	int picon_w,
	int picon_h,
	ptstime postime,
	ptstime duration,
	int layer,
	Asset *asset)
{
	item_lock->lock("ResourceThread::add_picon");

	items.append(new VResourceThreadItem(pixmap, 
		picon_x, 
		picon_y, 
		picon_w,
		picon_h,
		postime,
		duration,
		layer,
		asset,
		operation_count));
	item_lock->unlock();
}

void ResourceThread::add_wave(ResourcePixmap *pixmap,
	Asset *asset,
	int x,
	int channel,
	samplenum source_start,
	samplenum source_end)
{
	item_lock->lock("ResourceThread::add_wave");

	items.append(new AResourceThreadItem(pixmap, 
		asset,
		x,
		channel,
		source_start,
		source_end,
		operation_count));
	item_lock->unlock();
}

void ResourceThread::stop_draw(int reset)
{
	if(!interrupted)
	{
		interrupted = 1;
		item_lock->lock("ResourceThread::stop_draw");
		if(reset) items.remove_all_objects();
		operation_count++;
		item_lock->unlock();
		prev_x = -1;
		prev_h = 0;
		prev_l = 0;
	}
}

void ResourceThread::start_draw()
{
	interrupted = 0;
// Tag last audio item to cause refresh.
	for(int i = items.total - 1; i >= 0; i--)
	{
		ResourceThreadItem *item = items.values[i];
		if(item->data_type == TRACK_AUDIO)
		{
			item->last = 1;
			break;
		}
	}
	draw_lock->unlock();
}

void ResourceThread::run()
{
	int do_update;

	while(1)
	{

		draw_lock->lock("ResourceThread::run");

		do_update = 0;

		while(!interrupted)
		{

// Pull off item
			item_lock->lock("ResourceThread::run");
			int total_items = items.total;
			ResourceThreadItem *item = 0;
			if(items.total) 
			{
				item = items.values[0];
				items.remove_number(0);
			}
			item_lock->unlock();

			if(!total_items) break;

			if(item->data_type == TRACK_VIDEO)
			{
				do_video((VResourceThreadItem*)item);
				do_update = 1;
			}
			else
			if(item->data_type == TRACK_AUDIO)
			{
				do_audio((AResourceThreadItem*)item);
				do_update = 1;
			}

			delete item;
		}
		if(do_update)
			mwindow->gui->update(WUPD_CANVPICIGN);
	}
}

void ResourceThread::do_video(VResourceThreadItem *item)
{
	VFrame *picon_frame = 0;
	int cache_locked = 0;

	if((picon_frame = mwindow->frame_cache->get_frame_ptr(item->postime,
		item->layer,
		BC_RGB888,
		item->picon_w,
		item->picon_h,
		item->asset->id)) != 0)
	{
		cache_locked = 1;
	}
	else
	{
		File *source = mwindow->video_cache->check_out(item->asset,
			mwindow->edl);
		if(!source)
			return;

		picon_frame = new VFrame(0, item->picon_w, item->picon_h, BC_RGB888);
		picon_frame->set_layer(item->layer);
		picon_frame->set_source_pts(item->postime);
		source->get_frame(picon_frame);
		picon_frame->set_source_pts(item->postime);
		picon_frame->set_duration(item->duration);
		mwindow->frame_cache->put_frame(picon_frame, 
			0,
			item->asset);
		mwindow->video_cache->check_in(item->asset);
	}

// Allow escape here
	if(interrupted)
	{
		if(cache_locked)
			mwindow->frame_cache->unlock();
		return;
	}

// Draw the picon
// Test for pixmap existence first
	if(item->operation_count == operation_count)
	{
		int exists = 0;

		mwindow->gui->canvas->pixmaps_lock->lock("ResourceThread::do_video");
		for(int i = 0; i < mwindow->gui->canvas->resource_pixmaps.total; i++)
		{
			if(mwindow->gui->canvas->resource_pixmaps.values[i] == item->pixmap)
				exists = 1;
		}
		if(exists)
		{
			item->pixmap->draw_vframe(picon_frame,
				item->picon_x, 
				item->picon_y, 
				item->picon_w, 
				item->picon_h, 
				0, 
				0);
		}
		mwindow->gui->canvas->pixmaps_lock->unlock();
	}

	if(cache_locked)
		mwindow->frame_cache->unlock();

	if(mwindow->frame_cache->total() > 32)
		mwindow->frame_cache->delete_oldest();
}

#define BUFFERSIZE 65536
void ResourceThread::do_audio(AResourceThreadItem *item)
{
// Search again
	WaveCacheItem *wave_item;
	double high;
	double low;

	if((wave_item = mwindow->wave_cache->get_wave(item->asset->id,
		item->channel,
		item->start,
		item->end)))
	{
		high = wave_item->high;
		low = wave_item->low;
		mwindow->wave_cache->unlock();
	}
	else
	{
		int first_sample = 1;
		samplenum start = item->start;
		samplenum end = item->end;
		if(start == end) end = start + 1;

		for(samplenum sample = start; sample < end; sample++)
		{
			double value;
// Get value from previous buffer
			if(!(aframe &&
				item->channel == aframe->channel &&
				item->asset->id == audio_asset_id &&
				sample >= aframe->position &&
				sample < aframe->position + aframe->length))
			{
// Load new buffer
				File *source = mwindow->audio_cache->check_out(item->asset,
					mwindow->edl);
				if(!source)
					return;

				int fragment = BUFFERSIZE;
				samplenum total_samples = source->get_audio_length(-1);

				if(!aframe)
					aframe = new AFrame(BUFFERSIZE);

				aframe->samplerate = item->asset->sample_rate;
				aframe->channel = item->channel;

				if(fragment + sample > total_samples)
					fragment = total_samples - sample;
				aframe->set_fill_request(sample, fragment);

				source->get_samples(aframe);
				audio_asset_id = item->asset->id;
				mwindow->audio_cache->check_in(item->asset);
			}

			value = aframe->buffer[sample - aframe->position];
			if(first_sample)
			{
				high = low = value;
				first_sample = 0;
			}
			else
			{
				if(value > high) 
					high = value;
				else
				if(value < low)
					low = value;
			}
		}

		mwindow->wave_cache->put_wave(item->asset,
			item->channel,
			item->start,
			item->end,
			high,
			low);
	}

// Allow escape here
	if(interrupted)
		return;

// Draw the column
	if(item->operation_count == operation_count)
	{

// Test for pixmap existence first
		int exists = 0;
		for(int i = 0; i < mwindow->gui->canvas->resource_pixmaps.total; i++)
		{
			if(mwindow->gui->canvas->resource_pixmaps.values[i] == item->pixmap)
				exists = 1;
		}

		if(exists)
		{
			if(prev_x == item->x - 1)
			{
				high = MAX(high, prev_l);
				low = MIN(low, prev_h);
			}
			prev_x = item->x;
			prev_h = high;
			prev_l = low;
			item->pixmap->draw_wave(item->x, high, low);
		}
	}
}
