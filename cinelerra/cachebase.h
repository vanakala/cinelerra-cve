
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

#ifndef CACHEBASE_H
#define CACHEBASE_H


#include "asset.inc"
#include "datatype.h"
#include "linklist.h"
#include "mutex.inc"
#include <stdint.h>


// Store rendered elements from files but not the files.
// Drawing caches must be separate from file caches to avoid
// delaying other file accesses for the drawing routines.


class CacheItemBase : public ListItem<CacheItemBase>
{
public:
	CacheItemBase();
	virtual ~CacheItemBase() {};

	virtual size_t get_size();
	virtual void dump(int indent = 0);

// Pointer to asset
	Asset *asset;
// Position
	ptstime position;

// Number of last get or put operation involving this object.
	int age;
};


class CacheBase : public List<CacheItemBase>
{
public:
	CacheBase();
	virtual ~CacheBase();

	int get_age();

	void remove_all();

// Remove all items with the asset id.
	void remove_asset(Asset *asset);

// Insert item in list in ascending position order.
	void put_item(CacheItemBase *item);

// Get first item from list with matching position or 0 if none found.
	CacheItemBase* get_item(ptstime postime);

// Called when done with the item returned by get_.
// Ignore if item was 0.
	void unlock();

// Delete oldest item.  Return 0 if successful.  Return 1 if nothing to delete.
	int delete_oldest();

// Calculate current size of cache in bytes
	size_t get_memory_usage(int *total = 0);

	void dump(int indent = 0);

	Mutex *lock;
// Current position of search
	CacheItemBase *current_item;
};

#endif
