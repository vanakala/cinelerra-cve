
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

#ifndef CACHE_H
#define CACHE_H

// CICache for quickly reading data that is hard to decompress yet used
// over and over.

// Actual caching is done in the File object
// the CICache keeps files open while rendering.

// Since the CICache outlives EDLs it must copy every parameter given to it.

// Files given as arguments must outlive the cache.

#include "arraylist.h"
#include "asset.inc"
#include "cache.inc"
#include "condition.inc"
#include "edl.inc"
#include "file.inc"
#include "garbage.h"
#include "linklist.h"
#include "mutex.inc"
#include "pluginserver.inc"
#include "preferences.inc"

#include <stdint.h>

class CICacheItem : public ListItem<CICacheItem>, public GarbageObject
{
public:
	CICacheItem(CICache *cache, EDL *edl, Asset *asset);
	CICacheItem();
	~CICacheItem();

	File *file;
// Number of last get or put operation involving this object.
	int age;
	Asset *asset;     // Copy of asset.  CICache should outlive EDLs.
	Condition *item_lock;
	int checked_out;
private:
	CICache *cache;
};

class CICache : public List<CICacheItem>
{
public:
	CICache(Preferences *preferences,
		ArrayList<PluginServer*> *plugindb);
	~CICache();

	friend class CICacheItem;

// Enter a new file into the cache which is already open.
// If the file doesn't exist return the arguments.
// If the file exists delete the arguments and return the file which exists.
//	void update(File* &file);
//	void set_edl(EDL *edl);

// open it, lock it and add it to the cache if it isn't here already
// If it's already checked out, the value of block causes it to wait
// until it's checked in.
	File* check_out(Asset *asset, EDL *edl, int block = 1);

// unlock a file from the cache
	int check_in(Asset *asset);

// delete an entry from the cache
// before deleting an asset, starting a new project or something
	int delete_entry(Asset *asset);
	int delete_entry(char *path);
// Remove all entries from the cache.
	void remove_all();

// Get ID of oldest member.
// Called by MWindow::age_caches.
	int get_oldest();
	int64_t get_memory_usage(int use_lock);

// Called by age() and MWindow::age_caches
// returns 1 if nothing was available to delete
// 0 if successful
	int delete_oldest();

// Called by check_in() and modules.
// deletes oldest assets until under the memory limit
	int age();


	int dump();

	ArrayList<PluginServer*> *plugindb;

private:

// for deleting items
	int lock_all();
	int unlock_all();

// to prevent one from checking the same asset out before it's checked in
// yet without blocking the asset trying to get checked in
// use a seperate mutex for checkouts and checkins
	Mutex *total_lock;
	Condition *check_out_lock;
// Copy of EDL
	EDL *edl;
	Preferences *preferences;
};








#endif
