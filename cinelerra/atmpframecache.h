// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef ATMPFRAMECACHE_H
#define ATMPFRAMECACHE_H

#include "atmpframecache.inc"
#include "linklist.h"
#include "mutex.inc"
#include "atmpframecache.inc"
#include "aframe.inc"

#include <stddef.h>

class ATmpFrameCacheElem : public ListItem<ATmpFrameCacheElem>
{
public:
	ATmpFrameCacheElem(int buflen);
	~ATmpFrameCacheElem();

	size_t get_size();
	void dump(int indent = 0);

	int in_use;
	int length;
	unsigned int age;
	AFrame *frame;
};

class ATmpFrameCache : public List<ATmpFrameCacheElem>
{
public:
	ATmpFrameCache();
	~ATmpFrameCache();

	AFrame *get_tmpframe(int buflen);
	AFrame *clone_frame(AFrame *frame);
	void release_frame(AFrame *tmp_frame);
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
