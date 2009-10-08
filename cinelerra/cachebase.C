
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
#include "cachebase.h"
#include "edl.h"
#include "mutex.h"

#include <string.h>




CacheItemBase::CacheItemBase()
 : ListItem<CacheItemBase>()
{
	age = 0;
	asset_id = -1;
	path = 0;
}

CacheItemBase::~CacheItemBase()
{
	free(path); // path was allocated with strdup in FramceCache::put_frame()
}




int CacheItemBase::get_size()
{
	return 0;
}





CacheBase::CacheBase()
 : List<CacheItemBase>()
{
	lock = new Mutex("CacheBase::lock");
	current_item = 0;
}

CacheBase::~CacheBase()
{
	delete lock;
}



int CacheBase::get_age()
{
	return EDL::next_id();
}


// Called when done with the item returned by get_.
// Ignore if item was 0.
void CacheBase::unlock()
{
    lock->unlock();
}

void CacheBase::remove_all()
{
	int total = 0;
	lock->lock("CacheBase::remove_all");
	while(last)
	{
		delete last;
		total++;
	}
	current_item = 0;
	lock->unlock();
//printf("CacheBase::remove_all: removed %d entries\n", total);
}


void CacheBase::remove_asset(Asset *asset)
{
	int total = 0;
	lock->lock("CacheBase::remove_id");
	for(current_item = first; current_item; )
	{
		if(current_item->path && !strcmp(current_item->path, asset->path) ||
			current_item->asset_id == asset->id)
		{
			CacheItemBase *next = current_item->next;
			delete current_item;
			total++;
			current_item = next;
		}
		else
			current_item = current_item->next;
	}
	lock->unlock();
//printf("CacheBase::remove_asset: removed %d entries for %s\n", total, asset->path);
}

int CacheBase::get_oldest()
{
	int oldest = 0x7fffffff;
	lock->lock("CacheBase::get_oldest");
	for(CacheItemBase *current = first; current; current = NEXT)
	{
		if(current->age < oldest)
			oldest = current->age;
	}
	lock->unlock();
	return oldest;
}



int CacheBase::delete_oldest()
{
	int oldest = 0x7fffffff;
	CacheItemBase *oldest_item = 0;

	lock->lock("CacheBase::delete_oldest");
	for(CacheItemBase *current = first; current; current = NEXT)
	{
		if(current->age < oldest)
		{
			oldest = current->age;
			oldest_item = current;
		}
	}

	if(oldest_item)
	{
// Too much data to debug if audio.
// printf("CacheBase::delete_oldest: deleted position=%lld %d bytes\n", 
// oldest_item->position, oldest_item->get_size());
		delete oldest_item;
		if(current_item == oldest_item) current_item = 0;
		lock->unlock();
		return 0;
	}

	lock->unlock();
	return 1;
}


int64_t CacheBase::get_memory_usage()
{
	int64_t result = 0;
	lock->lock("CacheBase::get_memory_usage");
	for(CacheItemBase *current = first; current; current = NEXT)
	{
		result += current->get_size();
	}
	lock->unlock();
	return result;
}

void CacheBase::put_item(CacheItemBase *item)
{
// Get first position >= item
	if(!current_item) current_item = first;
	while(current_item && current_item->position < item->position)
		current_item = current_item->next;
	if(!current_item) current_item = last;
	while(current_item && current_item->position >= item->position)
		current_item = current_item->previous;
	if(!current_item) 
		current_item = first;
	else
		current_item = current_item->next;

	if(!current_item)
	{
		append(item);
		current_item = item;
	}
	else
		insert_before(current_item, item);
}

// Get first item from list with matching position or 0 if none found.
CacheItemBase* CacheBase::get_item(int64_t position)
{
	if(!current_item) current_item = first;
	while(current_item && current_item->position < position)
		current_item = current_item->next;
	if(!current_item) current_item = last;
	while(current_item && current_item->position >= position)
		current_item = current_item->previous;
	if(!current_item)
		current_item = first;
	else
	if(current_item->next)
		current_item = current_item->next;
	if(!current_item || current_item->position != position) return 0;
	return current_item;
}





