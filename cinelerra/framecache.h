// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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

	size_t get_size();
	void dump(int indent = 0);

	VFrame *data;
};

class FrameCache : public CacheBase
{
public:
	FrameCache();

// Returns pointer to cache entry if frame exists or 0.
// If a frame is found, the frame cache is left in the locked state until 
// unlock is called.  If nothing is found, the frame cache is unlocked before
// returning.  This keeps the item from being deleted.
	VFrame* get_frame_ptr(ptstime postime,
		int layer,
		int color_model,
		int w,
		int h,
		Asset *asset,
		int stream);
// Puts the frame in cache.
//  The frame cache remains locked
// The frame is deleted by FrameCache in a future delete_oldest.
	VFrame *put_frame(VFrame *frame, Asset *asset, int stream);
// Change the duration of cached frames
	void change_duration(ptstime new_dur, int layer,
		int color_model, int w, int h, Asset *asset, int stream);

private:
// Returns item if found
	FrameCacheItem *frame_exists(VFrame *format,
		Asset *asset, int stream);
	FrameCacheItem *frame_exists(ptstime postime,
		int layer,
		int color_model,
		int w,
		int h,
		Asset *asset,
		int stream);
};

#endif
