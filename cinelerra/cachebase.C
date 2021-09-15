// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "cachebase.h"
#include "edl.h"
#include "mutex.h"

#include <string.h>
#include <inttypes.h>

CacheItemBase::CacheItemBase()
 : ListItem<CacheItemBase>()
{
	age = 0;
	asset = 0;
	stream = -1;
	position = -1;
}

size_t CacheItemBase::get_size()
{
	return sizeof(*this);
}

void CacheItemBase::dump(int indent)
{
	printf("%*spts %.3f size %zd age %d asset %p stream %d\n", indent, "",
		position, get_size(), age, asset, stream);
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
	lock->lock("CacheBase::remove_all");
	while(last)
	{
		delete last;
	}
	current_item = 0;
	lock->unlock();
}

void CacheBase::remove_asset(Asset *asset)
{
	int total = 0;
	lock->lock("CacheBase::remove_id");
	for(current_item = first; current_item; )
	{
		if(current_item->asset == asset)
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
		delete oldest_item;
		if(current_item == oldest_item)
			current_item = 0;
		lock->unlock();
		return 0;
	}

	lock->unlock();
	return 1;
}

size_t CacheBase::get_memory_usage(int *total)
{
	size_t result = 0;
	int count = 0;

	lock->lock("CacheBase::get_memory_usage");
	for(CacheItemBase *current = first; current; current = NEXT)
	{
		result += current->get_size();
		count++;
	}
	lock->unlock();
	if(total)
		*total = count;
	return result;
}

void CacheBase::put_item(CacheItemBase *item)
{
// Get first position >= item
	if(!current_item)
		current_item = first;

	while(current_item && current_item->position < item->position)
		current_item = current_item->next;

	if(!current_item)
		current_item = last;

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

// Get first item from list with matching postime or 0 if none found.
CacheItemBase* CacheBase::get_item(ptstime postime)
{
	if(!current_item)
		current_item = first;

	while(current_item && current_item->position < postime)
		current_item = current_item->next;

	if(!current_item)
		current_item = last;

	while(current_item && current_item->position >= postime)
		current_item = current_item->previous;

	if(!current_item)
		current_item = first;

	return current_item;
}

void CacheBase::dump(int indent)
{
	CacheItemBase *item;
	printf("%*sCacheBase::dump: count %d, size %zu\n", indent, "",
		total(), get_memory_usage());
	for(item = first; item; item = item->next)
		item->dump(indent + 2);
}
