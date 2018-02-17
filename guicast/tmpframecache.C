
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

#include "bcsignals.h"
#include "bcresources.h"
#include "colormodels.h"
#include "tmpframecache.h"
#include "mutex.h"
#include "vframe.h"

// Cache macimal size is 1% of memory
// It is divisor here
#define MAX_ALLOC 100

Mutex TmpFrameCache::listlock("TmpFrameCache::listlock");


TmpFrameCacheElem::TmpFrameCacheElem(int w, int h, int colormodel)
 : ListItem<TmpFrameCacheElem>()
{
	width = w;
	height = h;
	cmodel = colormodel;
	frame = new VFrame(0, w, h, colormodel);
	in_use = 0;
	age = 0;
}

TmpFrameCacheElem::~TmpFrameCacheElem()
{
	delete frame;
}

size_t TmpFrameCacheElem::get_size()
{
	return sizeof(VFrame) + frame->get_data_size();
}

void TmpFrameCacheElem::dump(int indent)
{
	printf("%*sTmpFrameCacheElem %p dump:\n", indent, "", this);
	printf("%*s [%dx%d] '%s' in_use %d age %u frame %p\n", indent + 2, "",
		width, height, ColorModels::name(cmodel), in_use, age, frame);
}

TmpFrameCache::TmpFrameCache()
 : List<TmpFrameCacheElem>()
{
	moment = 0;
}

TmpFrameCache::~TmpFrameCache()
{
	while(last)
		delete last;
}

VFrame *TmpFrameCache::get_tmpframe(int w, int h, int colormodel)
{
	TmpFrameCacheElem *elem = 0;

	listlock.lock("TmpFrameCache::get_tmpframe");

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		if(!cur->in_use && w == cur->width &&
			h == cur->height && colormodel == cur->cmodel)
		{
			elem = cur;
			break;
		}
	}
	if(!elem)
	{
		if(!max_alloc)
			max_alloc = BC_Resources::memory_size / MAX_ALLOC;
		if(get_size() > max_alloc)
			delete_old_frames();
		elem = append(new TmpFrameCacheElem(w, h, colormodel));
	}
	elem->in_use = 1;

	listlock.unlock();

	return elem->frame;
}

void TmpFrameCache::release_frame(VFrame *tmp_frame)
{
	if(!tmp_frame)
		return;

	listlock.lock("TmpFrameCache::release_frame");

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		if(tmp_frame == cur->frame)
		{
			cur->in_use = 0;
			cur->age = ++moment;
			break;
		}
	}

	listlock.unlock();
}

void TmpFrameCache::delete_old_frames()
{
	while(get_size() > max_alloc)
	{
		if(delete_oldest())
			break;
	}
}

int TmpFrameCache::delete_oldest()
{
	unsigned int min_age = UINT_MAX;
	TmpFrameCacheElem *min_elem = 0;

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		if(!cur->in_use && cur->age < min_age)
		{
			min_age = cur->age;
			min_elem = cur;
		}
	}

	if(!min_elem)
		return 1;

	delete min_elem;
	return 0;
}

void TmpFrameCache::delete_unused()
{
	TmpFrameCacheElem *nxt;

	for(TmpFrameCacheElem *cur = first; cur;)
	{
		if(!cur->in_use)
		{
			nxt = cur->next;
			delete cur;
			cur = nxt;
		}
		else
			cur = cur->next;
	}
}

size_t TmpFrameCache::get_size()
{
	size_t res = 0;

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
		res += cur->get_size();

	return res / 1024;
}

void TmpFrameCache::dump(int indent)
{
	printf("%*sTmpFrameCache %p dump:\n", indent, "", this);

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
		cur->dump(indent + 2);
}
