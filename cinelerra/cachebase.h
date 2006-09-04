#ifndef CACHEBASE_H
#define CACHEBASE_H


#include "asset.inc"
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
	virtual ~CacheItemBase();



	virtual int get_size();

// asset_id - supplied by user if the cache is not part of a file.
// Used for fast accesses.
	int asset_id;
// path is needed since the item may need to be deleted based on file.
// Used for deletion.
	char *path;
// Number of last get or put operation involving this object.
	int age;
// Starting point of item in asset's native rate.
	int64_t position;
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
	CacheItemBase* get_item(int64_t position);

// Called when done with the item returned by get_.
// Ignore if item was 0.
	void unlock();

// Get ID of oldest member.
// Called by MWindow::age_caches.
	int get_oldest();

// Delete oldest item.  Return 0 if successful.  Return 1 if nothing to delete.
	int delete_oldest();

// Calculate current size of cache in bytes
	int64_t get_memory_usage();

	Mutex *lock;
// Current position of search
	CacheItemBase *current_item;
};



#endif
