#include "asset.h"
#include "assets.h"
#include "cache.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "mutex.h"
#include "preferences.h"

#include <string.h>

// edl came from a command which won't exist anymore
CICache::CICache(EDL *edl, 
	Preferences *preferences,
	ArrayList<PluginServer*> *plugindb)
 : List<CICacheItem>()
{
	this->edl = new EDL;
	this->edl->create_objects();
	this->edl->copy_all(edl);
	this->plugindb = plugindb;
	this->preferences = preferences;
	check_in_lock = new Mutex("CICache::check_in_lock");
	check_out_lock = new Mutex("CICache::check_out_lock");
	total_lock = new Mutex("CICache::total_lock");
}

CICache::~CICache()
{
	while(last) delete last;
	delete edl;
	delete check_in_lock;
	delete check_out_lock;
	delete total_lock;
}

void CICache::set_edl(EDL *edl)
{
	this->edl->copy_all(edl);
}

void CICache::update(File* &file)
{
// Check if exists
	for(CICacheItem *current = first; current; current = NEXT)
	{
		if(!current->asset->test_path(file->asset->path))
		{
			if(file != current->file)
			{
				delete file;
				file = current->file;
			}
			return;
		}
	}

	CICacheItem *item;
	append(item = new CICacheItem(this, file));
	item->asset = new Asset(*(file->asset));
	file->set_asset(item->asset);
}

File* CICache::check_out(Asset *asset)
{
	File *result = 0;

	check_out_lock->lock("CICache::check_out");

// search for it in the cache
	CICacheItem *current, *new_item = 0;

	for(current = first; current && !new_item; current = NEXT)
	{
		if(!strcmp(current->asset->path, asset->path))
		{
			current->counter = 0;
			new_item = current;
		}
	}

// didn't find it so create a new one
	if(!new_item)
	{
		new_item = append(new CICacheItem(this, asset));
	}

	if(new_item)
	{
		if(new_item->file)
		{
// opened successfully
			new_item->item_lock->lock("CICache::check_out");
			new_item->checked_out = 1;

			result = new_item->file;
		}
		else
		{
// failed
			delete new_item;
			new_item = 0;
		}
	}


//printf("CICache::check_out %s\n", asset->path);
	check_out_lock->unlock();

	return result;
}

int CICache::check_in(Asset *asset)
{
	check_in_lock->lock("CICache::check_in");

	CICacheItem *current;
	int result = 0;
	total_lock->lock("CICache::check_in");
	for(current = first; current && !result; current = NEXT)
	{
// Pointers are different
		if(!strcmp(current->asset->path, asset->path))
		{
			current->checked_out = 0;
			current->item_lock->unlock();
			result = 1;
		}
	}
	total_lock->unlock();

	check_in_lock->unlock();

	age();
//dump();
	return result;
}

int CICache::delete_entry(char *path)
{
	Asset *asset = edl->assets->get_asset(path);
	if(asset) delete_entry(asset);
	return 0;
}

int CICache::delete_entry(Asset *asset)
{
	lock_all();
	int result = 0;
	CICacheItem *current, *temp;

	for(current = first; current; current = temp)
	{
		temp = NEXT;
		if(current->asset->equivalent(*asset, 0, 0))
		{
			if(!current->checked_out)
			{
				delete current;
			}
			else
			{
				printf("CICache::delete_entry asset checked out\n");
			}
		}
		current = temp;
	}

	unlock_all();
	return 0;
}

int CICache::age()
{
	check_out_lock->lock("CICache::age");
	CICacheItem *current;

	for(current = first; current; current = NEXT)
	{
		current->counter++;
	}

// delete old assets if memory usage is exceeded
	int64_t memory_usage;
	int result = 0;
	do
	{
		memory_usage = get_memory_usage();
		
//printf("CICache::age 3 %p %lld %lld\n", this, memory_usage, preferences->cache_size);
		if(memory_usage > preferences->cache_size)
		{
			result = delete_oldest();
		}
	}while(memory_usage > preferences->cache_size && !result);

	check_out_lock->unlock();
}

int64_t CICache::get_memory_usage()
{
	CICacheItem *current;
	int64_t result = 0;
	
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
	int highest_counter = 1;
	CICacheItem *oldest = 0;

	for(current = last; current; current =  PREVIOUS)
	{
		if(current->counter >= highest_counter)
		{
			oldest = current;
			highest_counter = current->counter;
		}
	}

	if(highest_counter > 1 && oldest)
	{
		total_lock->lock("CICache::delete_oldest");


// Got the oldest file.  Try requesting cache purge.
		if(!oldest->file || oldest->file->purge_cache())
		{
// Delete the file if cache already empty and not checked out.
			if(!oldest->checked_out) delete oldest;
		}
		total_lock->unlock();
		return 0;    // success
	}
	else
	{
		return 1;    // nothing was old enough to delete
	}
}

int CICache::dump()
{
	lock_all();
	CICacheItem *current;

	printf("CICache::dump total size %lld\n", get_memory_usage());
	for(current = first; current; current = NEXT)
	{
		printf("cache item %x asset %x %s counter %lld\n", 
			current, 
			current->asset,
			current->asset->path, 
			current->counter);
	}
	
	unlock_all();
}

int CICache::lock_all()
{
	check_in_lock->lock("CICache::lock_all");
	check_out_lock->lock("CICache::lock_all");
}

int CICache::unlock_all()
{
	check_in_lock->unlock();
	check_out_lock->unlock();
}











// File not already opened.
CICacheItem::CICacheItem(CICache *cache, Asset *asset)
 : ListItem<CICacheItem>()
{
	int result = 0;
	counter = 0;
	this->asset = new Asset;
	item_lock = new Mutex("CICacheItem::item_lock");
	
// Must copy Asset since this belongs to an EDL which won't exist forever.
	*this->asset = *asset;
	this->cache = cache;
	checked_out = 0;

	file = new File;
	file->set_processors(cache->preferences->processors);
	file->set_preload(cache->edl->session->playback_preload);


// Copy decoding parameters from session to asset so file can see them.
	this->asset->divx_use_deblocking = cache->edl->session->mpeg4_deblock;



	if(result = file->open_file(cache->plugindb, this->asset, 1, 0, -1, -1))
	{
		delete file;
		file = 0;
	}
}

// File already opened
CICacheItem::CICacheItem(CICache *cache, File *file)
 : ListItem<CICacheItem>()
{
	counter = 0;
	this->asset = new Asset;
	item_lock = new Mutex("CICacheItem::item_lock");
	*this->asset = *file->asset;
	this->file = file;
	this->cache = cache;
	checked_out = 0;

	file->set_processors(cache->preferences->processors);
	file->set_preload(cache->edl->session->playback_preload);
}

CICacheItem::~CICacheItem()
{
	delete file;
	delete asset;
	delete item_lock;
}
