// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef TMPFRAMECACHE_H
#define TMPFRAMECACHE_H

#include "linklist.h"
#include "mutex.inc"
#include "tmpframecache.inc"
#include "vframe.inc"

class TmpFrameCacheElem : public ListItem<TmpFrameCacheElem>
{
public:
	TmpFrameCacheElem(int w, int h, int colormodel,  const char *location = 0);
	~TmpFrameCacheElem();

	size_t get_size();
	void dump(int indent = 0);

	const char *allocator;
	int in_use;
	unsigned int age;
	VFrame *frame;
};

class TmpFrameCache : public List<TmpFrameCacheElem>
{
public:
	TmpFrameCache();
	~TmpFrameCache();

	VFrame *get_tmpframe(int w, int h, int colormodel, const char *location = 0);
	VFrame *clone_frame(VFrame *frame, const char *location = 0);
	void release_frame(VFrame *tmp_frame);
	size_t get_size(int *total = 0, int *inuse = 0);
	void delete_unused();
	void dump(int indent = 0);
private:
	void delete_old_frames();
	int delete_oldest();

	unsigned int moment;
	// Maximum allowed allocation
	size_t max_alloc;
	static Mutex listlock;
};

#endif
