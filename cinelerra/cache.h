#ifndef CACHE_H
#define CACHE_H

// CICache for quickly reading data that is hard to decompress yet used
// over and over.

// Actual caching is done in the File object
// the CICache keeps files open while rendering.

// Since the CICache outlives EDLs it must copy every parameter given to it.

// Files given as arguments must outlive the cache.

#include "arraylist.h"
#include "assets.inc"
#include "cache.inc"
#include "edl.inc"
#include "file.inc"
#include "linklist.h"
#include "mutex.h"
#include "pluginserver.inc"
#include "preferences.inc"

class CICacheItem : public ListItem<CICacheItem>
{
public:
	CICacheItem(CICache *cache, File *file);
	CICacheItem(CICache *cache, Asset *asset);
	CICacheItem() {};
	~CICacheItem();

	File *file;
	long counter;     // number of age calls ago this asset was last needed
	                  // assets used in the last render have counter == 1
	Asset *asset;     // Copy of asset.  CICache should outlive EDLs.
	Mutex item_lock;
	int checked_out;
private:
	CICache *cache;
};

class CICache : public List<CICacheItem>
{
public:
	CICache(EDL *edl, 
		Preferences *preferences,
		ArrayList<PluginServer*> *plugindb);
	~CICache();

	friend class CICacheItem;

// Enter a new file into the cache which is already open.
// If the file doesn't exist return the arguments.
// If the file exists delete the arguments and return the file which exists.
	void update(File* &file);
	void set_edl(EDL *edl);

// open it, lock it and add it to the cache if it isn't here already
	File* check_out(Asset *asset);

// unlock a file from the cache
	int check_in(Asset *asset);

// delete an entry from the cache
// before deleting an asset, starting a new project or something
	int delete_entry(Asset *asset);
	int delete_entry(char *path);

// increment counters after rendering a buffer length
// since you can't know how big the cache is until after rendering the buffer
// deletes oldest assets until under the memory limit
	int age();

	int dump();

	ArrayList<PluginServer*> *plugindb;

private:
	int delete_oldest();        // returns 0 if successful
	                        // 1 if nothing was old
	long get_memory_usage();

// for deleting items
	int lock_all();
	int unlock_all();

// to prevent one from checking the same asset out before it's checked in
// yet without blocking the asset trying to get checked in
// use a seperate mutex for checkouts and checkins
	Mutex check_in_lock, check_out_lock, total_lock;
// Copy of EDL
	EDL *edl;
	Preferences *preferences;
};








#endif
