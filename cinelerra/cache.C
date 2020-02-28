
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
#include "cache.h"
#include "condition.h"
#include "datatype.h"
#include "edl.h"
#include "file.h"
#include "mutex.h"
#include "preferences.h"

CICache::CICache(int open_mode)
 : List<CICacheItem>()
{
	this->open_mode = open_mode;
	check_out_lock = new Condition(0, "CICache::check_out_lock", 0);
	total_lock = new Mutex("CICache::total_lock");
}

CICache::~CICache()
{
	while(last)
		remove(last);

	delete check_out_lock;
	delete total_lock;
}

File* CICache::check_out(Asset *asset, int block)
{
	CICacheItem *current, *new_item = 0;

	while(1)
	{
// Scan directory for item
		int got_it = 0;
		total_lock->lock("CICache::check_out");
		for(current = first; current && !got_it; current = NEXT)
		{
			if(current->asset == asset)
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
				total_lock->unlock();
				return current->file;
			}
		}
		else
		{
// Create new item
			new_item = append(new CICacheItem(this, asset));

			if(new_item->file)
			{
// opened successfully.
				new_item->age = EDL::next_id();
				new_item->checked_out = 1;
				total_lock->unlock();
				return new_item->file;
			}
// Failed to open
			else
			{
				remove(new_item);
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

void CICache::check_in(Asset *asset)
{
	CICacheItem *current;
	int got_it = 0;

	total_lock->lock("CICache::check_in");
	for(current = first; current; current = NEXT)
	{
		if(current->asset == asset)
		{
			current->checked_out = 0;
// Pointer no longer valid here
			break;
		}
	}
	total_lock->unlock();

// Release for blocking check_out operations
	check_out_lock->unlock();

	age();
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
			remove(current);
	}
	total_lock->unlock();
}

void CICache::delete_entry(Asset *asset)
{
	total_lock->lock("CICache::delete_entry");
	CICacheItem *current, *temp;

	for(current = first; current; current = NEXT)
	{
		if(current->asset->test_path(asset))
		{
			if(!current->checked_out)
			{
				remove(current);
				break;
			}
		}
	}

	total_lock->unlock();
}

void CICache::age()
{
	CICacheItem *current;
	size_t prev_memory_usage;
	size_t memory_usage;
	int result = 0;

	total_lock->lock("CICache::age");
// delete old assets if memory usage is exceeded
	do
	{
		memory_usage = get_memory_usage();

		if(memory_usage > preferences_global->cache_size)
		{
			result = delete_oldest();
		}
		prev_memory_usage = memory_usage;
		memory_usage = get_memory_usage();
	}while(prev_memory_usage != memory_usage &&
		memory_usage > preferences_global->cache_size && !result);
	total_lock->unlock();
}

size_t CICache::get_memory_usage()
{
	CICacheItem *current;
	size_t result = 0;

	for(current = first; current; current = NEXT)
	{
		File *file = current->file;
		if(file) result += file->get_memory_usage();
	}
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

		if(!oldest->file)
		{

// Delete the file if cache already empty and not checked out.
			if(!oldest->checked_out)
			{
				remove(oldest);
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

void CICache::dump(int indent)
{
	CICacheItem *current;
	total_lock->lock("CICache::dump");
	printf("%*sCICache %p dump total size %zd\n", indent, "",
		this, get_memory_usage());
	for(current = first; current; current = NEXT)
		current->dump(indent + 2);
	total_lock->unlock();
}


CICacheItem::CICacheItem()
: ListItem<CICacheItem>()
{
}


CICacheItem::CICacheItem(CICache *cache, Asset *asset)
 : ListItem<CICacheItem>()
{
	int result = 0;
	age = EDL::next_id();

	this->asset = asset;
	this->cache = cache;
	checked_out = 0;

	file = new File;
	file->set_processors(preferences_global->processors);
	if(result = file->open_file(this->asset, FILE_OPEN_READ | cache->open_mode))
	{
		delete file;
		file = 0;
	}
}

CICacheItem::~CICacheItem()
{
	delete file;
}

void CICacheItem::dump(int indent)
{
	printf("%*sCICacheItem %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*sfile %p checked out %d age %d\n", indent, "",
		file, checked_out, age);
	asset->dump(indent + 2);
}
