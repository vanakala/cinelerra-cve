// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

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


TmpFrameCacheElem::TmpFrameCacheElem(int w, int h, int colormodel, const char *location)
 : ListItem<TmpFrameCacheElem>()
{
	frame = new VFrame(0, w, h, colormodel);
	in_use = 0;
	age = 0;
	allocator = location;
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
	indent += 2;
	printf("%*s[%dx%d] '%s' in_use %d age %u frame %p\n", indent, "",
		frame->get_w(), frame->get_h(),
		ColorModels::name(frame->get_color_model()), in_use, age, frame);
	if(allocator)
		printf("%*sAllocator: %s\n", indent, "", allocator);
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

VFrame *TmpFrameCache::get_tmpframe(int w, int h, int colormodel, const char *location)
{
	TmpFrameCacheElem *elem = 0;

	listlock.lock("TmpFrameCache::get_tmpframe");

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		if(!cur->in_use && cur->frame->params_match(w, h, colormodel))
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
	elem->allocator = location;

	listlock.unlock();

	return elem->frame;
}

VFrame *TmpFrameCache::clone_frame(VFrame *frame, const char *location)
{
	if(!frame)
		return 0;

	return get_tmpframe(frame->get_w(), frame->get_h(),
		frame->get_color_model(), location);
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
			cur->allocator = 0;
			cur->frame->clear_status();
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

size_t TmpFrameCache::get_size(int *total, int *inuse)
{
	size_t res = 0;
	int used, count;

	used = 0;
	count = 0;

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		count++;
		if(cur->in_use)
			used++;
		res += cur->get_size();
	}
	if(inuse)
		*inuse = used;
	if(total)
		*total = count;
	return res / 1024;
}

void TmpFrameCache::dump(int indent)
{
	printf("%*sTmpFrameCache %p dump:\n", indent, "", this);

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
		cur->dump(indent + 2);
}
