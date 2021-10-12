// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "asset.h"
#include "bcsignals.h"
#include "cache.h"
#include "cinelerra.h"
#include "clip.h"
#include "condition.h"
#include "datatype.h"
#include "edl.h"
#include "file.h"
#include "framecache.h"
#include "mutex.h"
#include "resourcethread.h"
#include "resourcepixmap.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "wavecache.h"

#define MAX_FRAME_CACHE_ITEMS 64


ResourceThreadItem::ResourceThreadItem(ResourcePixmap *pixmap, 
	Asset *asset,
	int stream,
	int data_type)
{
	this->data_type = data_type;
	this->pixmap = pixmap;
	this->asset = asset;
	this->stream = stream;
	last = 0;
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
	int stream)
 : ResourceThreadItem(pixmap, asset, stream, TRACK_VIDEO)
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
	int stream,
	int x,
	int channel,
	samplenum start,
	samplenum end)
 : ResourceThreadItem(pixmap, asset, stream, TRACK_AUDIO)
{
	this->x = x;
	this->channel = channel;
	this->start = start;
	this->end = end;
}


ResourceThread::ResourceThread(TrackCanvas *canvas)
{
	trackcanvas = canvas;
	draw_lock = new Condition(0, "ResourceThread::draw_lock", 0);
	item_lock = new Mutex("ResourceThread::item_lock");
	aframe = 0;
	prev_x = -1;
	prev_h = 0;
	prev_l = 0;
	audio_cache = new CICache();
	video_cache = new CICache();
	frame_cache = new FrameCache;
	wave_cache = new WaveCache;
	Thread::start();
}

ResourceThread::~ResourceThread()
{
	delete draw_lock;
	delete item_lock;
	delete aframe;
	delete audio_cache;
	delete video_cache;
	delete frame_cache;
	delete wave_cache;
}

void ResourceThread::add_picon(ResourcePixmap *pixmap, 
	int picon_x, 
	int picon_y, 
	int picon_w,
	int picon_h,
	ptstime postime,
	ptstime duration,
	int layer,
	Asset *asset,
	int stream)
{
	item_lock->lock("ResourceThread::add_picon");

	items.append(new VResourceThreadItem(pixmap,
		picon_x, picon_y, picon_w, picon_h,
		postime, duration, layer, asset, stream));
	item_lock->unlock();
}

void ResourceThread::add_wave(ResourcePixmap *pixmap,
	Asset *asset,
	int stream,
	int x,
	int channel,
	samplenum source_start,
	samplenum source_end)
{
	item_lock->lock("ResourceThread::add_wave");

	items.append(new AResourceThreadItem(pixmap, 
		asset, stream,
		x,
		channel,
		source_start,
		source_end));
	item_lock->unlock();
}

void ResourceThread::start_draw()
{
	draw_lock->unlock();
}

void ResourceThread::run()
{
	int do_update;

	while(1)
	{
		draw_lock->lock("ResourceThread::run");

		trackcanvas->pixmaps_lock->lock("ResourceThread::run");
		do_update = 0;

		while(items.total)
		{
// Pull off item
			item_lock->lock("ResourceThread::run");

			ResourceThreadItem *item = items.values[0];
			items.remove_number(0);

			item_lock->unlock();

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

		trackcanvas->pixmaps_lock->unlock();

		if(do_update)
		{
			trackcanvas->draw(0);
			trackcanvas->flash();
		}
	}
}

void ResourceThread::do_video(VResourceThreadItem *item)
{
	VFrame *picon_frame;

	if(!(picon_frame = frame_cache->get_frame_ptr(item->postime,
		item->layer,
		BC_RGB888,
		item->picon_w,
		item->picon_h,
		item->asset,
		item->stream)))
	{
		File *source = video_cache->check_out(item->asset, item->stream);

		if(!source)
			return;

		picon_frame = new VFrame(0, item->picon_w, item->picon_h, BC_RGB888);
		picon_frame->set_layer(item->layer);
		picon_frame->set_source_pts(item->postime);
		source->get_frame(picon_frame);
		picon_frame->set_source_pts(item->postime);
		picon_frame->set_duration(item->duration);
		frame_cache->put_frame(picon_frame, item->asset, item->stream);
		video_cache->check_in(item->asset, item->stream);
	}

// Draw the picon
// Test for pixmap existence first
	for(int i = 0; i < trackcanvas->resource_pixmaps.total; i++)
	{
		if(trackcanvas->resource_pixmaps.values[i] ==
			item->pixmap)
		{
			item->pixmap->draw_vframe(picon_frame,
				item->picon_x, item->picon_y,
				item->picon_w, item->picon_h,
				0, 0);
			break;
		}
	}

	frame_cache->unlock();

	if(frame_cache->total() > MAX_FRAME_CACHE_ITEMS)
		frame_cache->delete_oldest();
}

void ResourceThread::do_audio(AResourceThreadItem *item)
{
// Search again
	WaveCacheItem *wave_item;
	double high;
	double low;

	if((wave_item = wave_cache->get_wave(item->asset,
		item->channel,
		item->start,
		item->end)))
	{
		high = wave_item->high;
		low = wave_item->low;
		wave_cache->unlock();
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
				item->stream == aframe->stream &&
				item->asset->id == audio_asset_id &&
				sample >= aframe->position &&
				sample < aframe->position + aframe->get_length()))
			{
// Load new buffer
				File *source = audio_cache->check_out(item->asset, item->stream);

				if(!source)
					return;

				samplenum total_samples = source->get_audio_length();

				if(!aframe)
					aframe = new AFrame(AUDIO_BUFFER_SIZE);

				int fragment = aframe->get_buffer_length();

				aframe->set_samplerate(item->asset->streams[item->stream].sample_rate);
				aframe->channel = item->channel;
				aframe->stream = item->stream;

				if(fragment + sample > total_samples)
					fragment = total_samples - sample;
				aframe->set_fill_request(sample, fragment);

				source->get_samples(aframe);
				audio_asset_id = item->asset->id;
				audio_cache->check_in(item->asset, item->stream);
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

		wave_cache->put_wave(item->asset,
			item->channel,
			item->start,
			item->end,
			high,
			low);
	}

// Test for pixmap existence first
	int exists = 0;

	for(int i = 0; i < trackcanvas->resource_pixmaps.total; i++)
	{
		if(trackcanvas->resource_pixmaps.values[i] == item->pixmap)
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

size_t ResourceThread::get_cache_size()
{
	return audio_cache->get_size() +
		video_cache->get_size() +
		frame_cache->get_memory_usage() +
		wave_cache->get_memory_usage();
}

void ResourceThread::cache_delete_oldest()
{
	audio_cache->delete_oldest();
	video_cache->delete_oldest();
	frame_cache->delete_oldest();
	wave_cache->delete_oldest();
}

void ResourceThread::reset_caches()
{
	frame_cache->remove_all();
	wave_cache->remove_all();
	audio_cache->remove_all();
	video_cache->remove_all();
}

void ResourceThread::remove_asset_from_caches(Asset *asset)
{
	frame_cache->remove_asset(asset);
	wave_cache->remove_asset(asset);
	audio_cache->delete_entry(asset);
	video_cache->delete_entry(asset);
}

void ResourceThread::show_cache_status(int indent)
{
	int count;
	size_t size;

	size = audio_cache->get_size(&count);
	printf("%*sAudio cache %d %zu\n", indent, "",
		count, size);
	size = video_cache ->get_size(&count);
	printf("%*sVideo cache %d %zu\n", indent, "",
		count, size);
	size = frame_cache->get_memory_usage(&count);
	printf("%*sFrame cache %d %zu\n", indent, "",
		count, size);
	size = wave_cache->get_memory_usage(&count);
	printf("%*sWave cache %d %zu\n", indent, "",
		count, size);
}
