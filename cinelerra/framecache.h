#ifndef FRAMECACHE_H
#define FRAMECACHE_H


#include "arraylist.h"
#include "mutex.inc"
#include "vframe.inc"


#include <stdint.h>

// Simply a table of images described by frame position and dimensions.
// The frame position is relative to the frame rate of the source file.
// This object is held by File.  CICache scans all the files for
// frame caches and deletes what's needed to maintain the cache size.

class FrameCacheItem
{
public:
	FrameCacheItem();
	~FrameCacheItem();

	VFrame *data;
	int64_t position;
	double frame_rate;
// Number of last get or put operation involving this object.
	int age;
};

class FrameCache
{
public:
	FrameCache();
	~FrameCache();

// Returns 1 if frame exists in cache and copies it to the frame argument.
	int get_frame(VFrame *frame, 
		int64_t position,
		double frame_rate);
// Returns pointer to cache entry if frame exists or 0.
	VFrame* get_frame_ptr(int64_t position,
		double frame_rate,
		int color_model,
		int w,
		int h);
// Puts the frame in cache.
// use_copy - if 1 a copy of the frame is made.  if 0 the argument is stored.
// The copy of the frame is deleted by FrameCache in a future delete_oldest.
	void put_frame(VFrame *frame, 
		int64_t position,
		double frame_rate,
		int use_copy);

// Delete oldest item.  Return 0 if successful.  Return 1 if nothing to delete.
	int delete_oldest();

// Calculate current size of cache in bytes
	int64_t get_memory_usage();
	void dump();





private:
// Return 1 if matching frame exists.
// Return 0 if not.
	int frame_exists(VFrame *format,
		int64_t position,
		double frame_rate,
		int *item_return);
	int FrameCache::frame_exists(int64_t position, 
		double frame_rate,
		int color_model,
		int w,
		int h,
		int *item_return);

	Mutex *lock;
// Current get or put operation since creation of FrameCache object
	int current_age;
	ArrayList<FrameCacheItem*> items;
// Maximum size of cache in bytes.
	int64_t max_bytes;
};



#endif
