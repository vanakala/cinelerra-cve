
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
#include "bctimer.h"
#include "cache.h"
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
	double frame_rate,
	int64_t position,
	int layer,
	Asset *asset,
	int operation_count)
 : ResourceThreadItem(pixmap, asset, TRACK_VIDEO, operation_count)
{
	this->picon_x = picon_x;
	this->picon_y = picon_y;
	this->picon_w = picon_w;
	this->picon_h = picon_h;
	this->frame_rate = frame_rate;
	this->position = position;
	this->layer = layer;
}

VResourceThreadItem::~VResourceThreadItem()
{
}








AResourceThreadItem::AResourceThreadItem(ResourcePixmap *pixmap, 
	Asset *asset,
	int x,
	int channel,
	int64_t start,
	int64_t end,
	int operation_count)
 : ResourceThreadItem(pixmap, asset, TRACK_AUDIO, operation_count)
{
	this->x = x;
	this->channel = channel;
	this->start = start;
	this->end = end;
}

AResourceThreadItem::~AResourceThreadItem()
{
}

















ResourceThread::ResourceThread(MWindow *mwindow)
{
	this->mwindow = mwindow;
	interrupted = 1;
	temp_picon = 0;
	temp_picon2 = 0;
	draw_lock = new Condition(0, "ResourceThread::draw_lock", 0);
//	interrupted_lock = new Condition(0, "ResourceThread::interrupted_lock", 0);
	item_lock = new Mutex("ResourceThread::item_lock");
	audio_buffer = 0;
	timer = new Timer;
	prev_x = -1;
	prev_h = 0;
	prev_l = 0;
	operation_count = 0;
}

ResourceThread::~ResourceThread()
{
	delete draw_lock;
//	delete interrupted_lock;
	delete item_lock;
	delete temp_picon;
	delete temp_picon2;
	delete [] audio_buffer;
	delete timer;
}

void ResourceThread::create_objects()
{
	Thread::start();
}

void ResourceThread::add_picon(ResourcePixmap *pixmap, 
	int picon_x, 
	int picon_y, 
	int picon_w,
	int picon_h,
	double frame_rate,
	int64_t position,
	int layer,
	Asset *asset)
{
	item_lock->lock("ResourceThread::item_lock");

	items.append(new VResourceThreadItem(pixmap, 
		picon_x, 
		picon_y, 
		picon_w,
		picon_h,
		frame_rate,
		position,
		layer,
		asset,
		operation_count));
	item_lock->unlock();
}

void ResourceThread::add_wave(ResourcePixmap *pixmap,
	Asset *asset,
	int x,
	int channel,
	int64_t source_start,
	int64_t source_end)
{
	item_lock->lock("ResourceThread::item_lock");

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
	timer->update();
	draw_lock->unlock();
}

void ResourceThread::run()
{
	while(1)
	{

		draw_lock->lock("ResourceThread::run");


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
			}
			else
			if(item->data_type == TRACK_AUDIO)
			{
				do_audio((AResourceThreadItem*)item);
			}

			delete item;
		}
	}
}




void ResourceThread::do_video(VResourceThreadItem *item)
{
	if(temp_picon &&
		(temp_picon->get_w() != item->asset->width ||
		temp_picon->get_h() != item->asset->height))
	{
		delete temp_picon;
		temp_picon = 0;
	}

	if(!temp_picon)
	{
		temp_picon = new VFrame(0, 
			item->asset->width, 
			item->asset->height, 
			BC_RGB888);
	}

// Get temporary to copy cached frame to
	if(temp_picon2 &&
		(temp_picon2->get_w() != item->picon_w ||
		temp_picon2->get_h() != item->picon_h))
	{
		delete temp_picon2;
		temp_picon2 = 0;
	}

	if(!temp_picon2)
	{
		temp_picon2 = new VFrame(0, 
			item->picon_w, 
			item->picon_h, 
			BC_RGB888);
	}



// Search frame cache again.

	VFrame *picon_frame = 0;

	if((picon_frame = mwindow->frame_cache->get_frame_ptr(item->position,
		item->layer,
		item->frame_rate,
		BC_RGB888,
		item->picon_w,
		item->picon_h,
		item->asset->id)) != 0)
	{
		temp_picon2->copy_from(picon_frame);
// Unlock the get_frame_ptr command
		mwindow->frame_cache->unlock();
	}
	else
	{

		File *source = mwindow->video_cache->check_out(item->asset,
			mwindow->edl);
		if(!source) 
		{
			return;
		}
		source->set_layer(item->layer);
		source->set_video_position(item->position, 
			item->frame_rate);

		source->read_frame(temp_picon);
		picon_frame = new VFrame(0, item->picon_w, item->picon_h, BC_RGB888);
		cmodel_transfer(picon_frame->get_rows(),
			temp_picon->get_rows(),
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0, 
			temp_picon->get_w(),
			temp_picon->get_h(),
			0,
			0,
			picon_frame->get_w(), 
			picon_frame->get_h(),
			BC_RGB888,
			BC_RGB888,
			0,
			temp_picon->get_bytes_per_line(),
			picon_frame->get_bytes_per_line());
		temp_picon2->copy_from(picon_frame);
		mwindow->frame_cache->put_frame(picon_frame, 
			item->position,
			item->layer,
			mwindow->edl->session->frame_rate,
			0,
			item->asset);
		mwindow->video_cache->check_in(item->asset);
	}


// Allow escape here
	if(interrupted) 
	{
		return;
	}


// Draw the picon
	mwindow->gui->lock_window("ResourceThread::do_video");

	if(interrupted)
	{
		mwindow->gui->unlock_window();
		return;
	}



// Test for pixmap existence first
	if(item->operation_count == operation_count)
	{
		int exists = 0;
		for(int i = 0; i < mwindow->gui->canvas->resource_pixmaps.total; i++)
		{
			if(mwindow->gui->canvas->resource_pixmaps.values[i] == item->pixmap)
				exists = 1;
		}
		if(exists)
		{
			item->pixmap->draw_vframe(temp_picon2, 
				item->picon_x, 
				item->picon_y, 
				item->picon_w, 
				item->picon_h, 
				0, 
				0);
			mwindow->gui->update(0, 3, 0, 0, 0, 0, 0);
		}
	}

	mwindow->gui->unlock_window();
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
		int64_t start = item->start;
		int64_t end = item->end;
		if(start == end) end = start + 1;
		
		for(int64_t sample = start; sample < end; sample++)
		{
			double value;
// Get value from previous buffer
			if(audio_buffer && 
				item->channel == audio_channel &&
				item->asset->id == audio_asset_id &&
				sample >= audio_start &&
				sample < audio_start + audio_samples)
			{
				;
			}
			else
// Load new buffer
			{
				File *source = mwindow->audio_cache->check_out(item->asset,
					mwindow->edl);
				if(!source)
					return;
					
				source->set_channel(item->channel);
				source->set_audio_position(sample, item->asset->sample_rate);
				int64_t total_samples = source->get_audio_length(-1);
				if(!audio_buffer) audio_buffer = new double[BUFFERSIZE];
				int fragment = BUFFERSIZE;
				if(fragment + sample > total_samples)
					fragment = total_samples - sample;
				source->read_samples(audio_buffer, fragment, item->asset->sample_rate);
				audio_channel = item->channel;
				audio_start = sample;
				audio_samples = fragment;
				audio_asset_id = item->asset->id;
				mwindow->audio_cache->check_in(item->asset);
			}


			value = audio_buffer[sample - audio_start];
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
	mwindow->gui->lock_window("ResourceThread::do_audio");
	if(interrupted)
	{
		mwindow->gui->unlock_window();
		return;
	}

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
			if(timer->get_difference() > 250 || item->last)
			{
				mwindow->gui->update(0, 3, 0, 0, 0, 0, 0);
				timer->update();
			}
		}
	}

	mwindow->gui->unlock_window();

}




