#ifndef FRAMECACHE_H
#define FRAMECACHE_H


#include "cachebase.h"
#include "mutex.inc"
#include "vframe.inc"


#include <stdint.h>

// Simply a table of images described by frame position and dimensions.
// The frame position is relative to the frame rate of the source file.
// This object is used by File for playback.
// and MWindow for timeline drawing.
// CICache scans all the files for
// frame caches and deletes what's needed to maintain the cache size.

class FrameCacheItem : public CacheItemBase
{
public:
	FrameCacheItem();
	~FrameCacheItem();

	int get_size();

	VFrame *data;
	double frame_rate;
};

class FrameCache : public CacheBase
{
public:
	FrameCache();
	~FrameCache();

// Returns 1 if frame exists in cache and copies it to the frame argument.
	int get_frame(VFrame *frame, 
		int64_t position,
		double frame_rate);
// Returns pointer to cache entry if frame exists or 0.
// If a frame is found, the frame cache is left in the locked state until 
// unlock is called.  If nothing is found, the frame cache is unlocked before
// returning.  This keeps the item from being deleted.
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

	void dump();




private:
// Return 1 if matching frame exists.
// Return 0 if not.
	int frame_exists(VFrame *format,
		int64_t position,
		double frame_rate,
		FrameCacheItem **item_return);
	int frame_exists(int64_t position, 
		double frame_rate,
		int color_model,
		int w,
		int h,
		FrameCacheItem **item_return);
};



#endif
