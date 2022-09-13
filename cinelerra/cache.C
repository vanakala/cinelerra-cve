// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "cache.h"
#include "condition.h"
#include "datatype.h"
#include "edl.h"
#include "file.h"
#include "mutex.h"
#include "preferences.h"

CICache::CICache()
 : List<CICacheItem>()
{
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

File* CICache::check_out(Asset *asset, int stream, int block)
{
	CICacheItem *current, *new_item = 0;

	while(1)
	{
// Scan directory for item
		int got_it = 0;
		total_lock->lock("CICache::check_out");
		for(current = first; current && !got_it; current = NEXT)
		{
			if(current->asset == asset && current->stream == stream)
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
			new_item = append(new CICacheItem(this, asset, stream));

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

void CICache::check_in(Asset *asset, int stream)
{
	CICacheItem *current;
	int got_it = 0;

	total_lock->lock("CICache::check_in");
	for(current = first; current; current = NEXT)
	{
		if(current->asset == asset && current->stream == stream)
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
		if(current->asset->check_stream(asset))
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
	int prev_memory_usage;
	int memory_usage;
	int result = 0;

// delete old assets if memory usage is exceeded
	do
	{
		memory_usage = get_memory_usage() / MEGABYTE;

		if(memory_usage > preferences_global->cache_size)
			result = delete_oldest();

		prev_memory_usage = memory_usage;
		memory_usage = get_memory_usage() / MEGABYTE;
	}while(prev_memory_usage != memory_usage &&
		memory_usage > preferences_global->cache_size && !result);
}

size_t CICache::get_size(int *count)
{
	size_t size;

	total_lock->lock("CICache::get_size");
	size = get_memory_usage();
	if(count)
		*count = total();
	total_lock->unlock();
	return size;
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


CICacheItem::CICacheItem(CICache *cache, Asset *asset, int stream)
 : ListItem<CICacheItem>()
{
	int result = 0;
	int open_mode = 0;
	age = EDL::next_id();

	this->asset = asset;
	this->stream = stream;
	this->cache = cache;
	checked_out = 0;

	file = new File;
	if(asset->streams[stream].options & STRDSC_AUDIO)
		open_mode = FILE_OPEN_AUDIO;
	else if(asset->streams[stream].options & STRDSC_VIDEO)
	{
		open_mode = FILE_OPEN_VIDEO;
		if(asset->vhwaccel)
			open_mode |= FILE_OPEN_HWACCEL;
	}
	file->set_processors(preferences_global->max_threads);
	if(!open_mode ||
		(result = file->open_file(this->asset, FILE_OPEN_READ | open_mode, stream)))
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
	printf("%*sfile %p checked out %d age %d stream %d\n", indent, "",
		file, checked_out, age, stream);
	asset->dump(indent + 2);
}
