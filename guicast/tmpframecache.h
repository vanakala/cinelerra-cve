
/*
 * CINELERRA
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef TMPFRAMECACHE_H
#define TMPFRAMECACHE_H

#include "linklist.h"
#include "mutex.inc"
#include "tmpframecache.inc"
#include "vframe.inc"

class TmpFrameCacheElem : public ListItem<TmpFrameCacheElem>
{
public:
	TmpFrameCacheElem(int w, int h, int colormodel);
	~TmpFrameCacheElem();

	size_t get_size();
	void dump(int indent = 0);

	int in_use;
	int width;
	int height;
	int cmodel;
	unsigned int age;
	VFrame *frame;
};

class TmpFrameCache : public List<TmpFrameCacheElem>
{
public:
	TmpFrameCache();
	~TmpFrameCache();

	VFrame *get_tmpframe(int w, int h, int colormodel);
	void release_frame(VFrame *tmp_frame);
	size_t get_size();
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
