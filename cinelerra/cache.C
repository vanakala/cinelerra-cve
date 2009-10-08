
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
#include "assets.h"
#include "bcsignals.h"
#include "cache.h"
#include "condition.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "mutex.h"
#include "preferences.h"

#include <string.h>

// edl came from a command which won't exist anymore
CICache::CICache(Preferences *preferences,
	ArrayList<PluginServer*> *plugindb)
 : List<CICacheItem>()
{
	this->plugindb = plugindb;
	this->preferences = preferences;
	check_out_lock = new Condition(0, "CICache::check_out_lock", 0);
	total_lock = new Mutex("CICache::total_lock");
}

CICache::~CICache()
{
	while(last)
	{
		CICacheItem *item = last;
//printf("CICache::~CICache: %s\n", item->asset->path);
		remove_pointer(item);
		Garbage::delete_object(item);
	}
	delete check_out_lock;
	delete total_lock;
}






File* CICache::check_out(Asset *asset, EDL *edl, int block)
{
	CICacheItem *current, *new_item = 0;

	while(1)
	{
// Scan directory for item
		int got_it = 0;
		total_lock->lock("CICache::check_out");
		for(current = first; current && !got_it; current = NEXT)
		{
			if(!strcmp(current->asset->path, asset->path))
			{
				got_it = 1;
				break;
			}
		}

// Test availability
		if(got_it)
		{
			if(!current->checked_out)
			{
// Return it
				current->age = EDL::next_id();
				current->checked_out = 1;
				current->GarbageObject::add_user();
				total_lock->unlock();
				return current->file;
			}
		}
		else
		{
// Create new item
			new_item = append(new CICacheItem(this, edl, asset));

			if(new_item->file)
			{
// opened successfully.
				new_item->age = EDL::next_id();
				new_item->checked_out = 1;
				new_item->GarbageObject::add_user();
				total_lock->unlock();
				return new_item->file;
			}
// Failed to open
			else
			{
				remove_pointer(new_item);
				Garbage::delete_object(new_item);
				total_lock->unlock();
				return 0;
			}
		}

// Try again after blocking
		total_lock->unlock();
		if(block)
			check_out_lock->lock("CICache::check_out");
		else
			return 0;
	}

	return 0;
}

int CICache::check_in(Asset *asset)
{
	CICacheItem *current;
	int got_it = 0;

	total_lock->lock("CICache::check_in");
	for(current = first; current; current = NEXT)
	{
// Need to compare paths because
// asset pointers are different
		if(!strcmp(current->asset->path, asset->path))
		{
			current->checked_out = 0;
			current->GarbageObject::remove_user();
// Pointer no longer valid here
			break;
		}
	}
	total_lock->unlock();

// Release for blocking check_out operations
	check_out_lock->unlock();

	age();
	return 0;
}

void CICache::remove_all()
{
	total_lock->lock("CICache::remove_all");
	CICacheItem *current, *temp;
	for(current = first; current; current = temp)
	{
		temp = current->next;
// Must not be checked out because we need the pointer to check back in.
// Really need to give the user the CacheItem.
		if(!current->checked_out)
		{
//printf("CICache::remove_all: %s\n", current->asset->path);
			remove_pointer(current);
			Garbage::delete_object(current);
		}
	}
	total_lock->unlock();
}

int CICache::delete_entry(char *path)
{
	total_lock->lock("CICache::delete_entry");
	for(CICacheItem *current = first; current; current = NEXT)
	{
		if(!strcmp(current->asset->path, path))
		{
			if(!current->checked_out)
			{
//printf("CICache::delete_entry: %s\n", current->asset->path);
				remove_pointer(current);
				Garbage::delete_object(current);
				break;
			}
		}
	}
	total_lock->unlock();
	return 0;
}

int CICache::delete_entry(Asset *asset)
{
	total_lock->lock("CICache::delete_entry");
	int result = 0;
	CICacheItem *current, *temp;

	for(current = first; current; current = NEXT)
	{
		if(!strcmp(current->asset->path, asset->path))
		{
			if(!current->checked_out)
			{
//printf("CICache::delete_entry: %s\n", current->asset->path);
				remove_pointer(current);
				Garbage::delete_object(current);
				break;
			}
		}
	}

	total_lock->unlock();
	return 0;
}

int CICache::age()
{
	CICacheItem *current;

// delete old assets if memory usage is exceeded
	int64_t prev_memory_usage;
	int64_t memory_usage;
	int result = 0;
	do
	{
		memory_usage = get_memory_usage(1);
		
		if(memory_usage > preferences->cache_size)
		{
//printf("CICache::age 3 %p %lld %lld\n", this, memory_usage, preferences->cache_size);
			result = delete_oldest();
		}
		prev_memory_usage = memory_usage;
		memory_usage = get_memory_usage(0);
	}while(prev_memory_usage != memory_usage &&
		memory_usage > preferences->cache_size && 
		!result);

}

int64_t CICache::get_memory_usage(int use_lock)
{
	CICacheItem *current;
	int64_t result = 0;
	if(use_lock) total_lock->lock("CICache::get_memory_usage");
	for(current = first; current; current = NEXT)
	{
		File *file = current->file;
		if(file) result += file->get_memory_usage();
	}
	if(use_lock) total_lock->unlock();
	return result;
}

int CICache::get_oldest()
{
	CICacheItem *current;
	int oldest = 0x7fffffff;
	total_lock->lock("CICache::get_oldest");
	for(current = last; current; current = PREVIOUS)
	{
		if(current->age < oldest)
		{
			oldest = current->age;
		}
	}
	total_lock->unlock();

	return oldest;
}

int CICache::delete_oldest()
{
	CICacheItem *current;
	int lowest_age = 0x7fffffff;
	CICacheItem *oldest = 0;

	total_lock->lock("CICache::delete_oldest");

	for(current = last; current; current = PREVIOUS)
	{
		if(current->age < lowest_age)
		{
			oldest = current;
			lowest_age = current->age;
		}
	}


	if(oldest)
	{
// Got the oldest file.  Try requesting cache purge.

		if(!oldest->file || oldest->file->purge_cache())
		{

// Delete the file if cache already empty and not checked out.
			if(!oldest->checked_out)
			{

				remove_pointer(oldest);

				Garbage::delete_object(oldest);

			}

		}

		total_lock->unlock();
// success
		return 0;    
	}
	else
	{
		total_lock->unlock();
// nothing was old enough to delete
		return 1;   
	}
}

int CICache::dump()
{
	CICacheItem *current;
	total_lock->lock("CICache::dump");
	printf("CICache::dump total size %lld\n", get_memory_usage(0));
	for(current = first; current; current = NEXT)
	{
		printf("cache item %x asset %x %s age=%d\n", 
			current, 
			current->asset,
			current->asset->path, 
			current->age);
	}
	total_lock->unlock();
}









CICacheItem::CICacheItem()
: ListItem<CICacheItem>(), GarbageObject("CICacheItem")
{
}


CICacheItem::CICacheItem(CICache *cache, EDL *edl, Asset *asset)
 : ListItem<CICacheItem>(), GarbageObject("CICacheItem")
{
	int result = 0;
	age = EDL::next_id();

	this->asset = new Asset;

	item_lock = new Condition(1, "CICacheItem::item_lock", 0);
	

// Must copy Asset since this belongs to an EDL which won't exist forever.
	*this->asset = *asset;
	this->cache = cache;
	checked_out = 0;


	file = new File;
	file->set_processors(cache->preferences->processors);
	file->set_preload(edl->session->playback_preload);
	file->set_subtitle(edl->session->decode_subtitles ? 
		edl->session->subtitle_number : -1);
	file->set_interpolate_raw(edl->session->interpolate_raw);
	file->set_white_balance_raw(edl->session->white_balance_raw);


// Copy decoding parameters from session to asset so file can see them.
	this->asset->divx_use_deblocking = edl->session->mpeg4_deblock;



	if(result = file->open_file(cache->preferences, this->asset, 1, 0, -1, -1))
	{
SET_TRACE
		delete file;
SET_TRACE
		file = 0;
	}

}

CICacheItem::~CICacheItem()
{
	if(file) delete file;
	if(asset) Garbage::delete_object(asset);
	if(item_lock) delete item_lock;
}
