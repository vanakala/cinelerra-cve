// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "aframe.h"
#include "atmpframecache.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "mutex.h"

// Cache macimal size is 1% of memory
// It is divisor here
#define MAX_ALLOC 100

Mutex ATmpFrameCache::listlock("ATmpFrameCache::listlock");


ATmpFrameCacheElem::ATmpFrameCacheElem(int buflen)
 : ListItem<ATmpFrameCacheElem>()
{
	length = buflen;
	frame = new AFrame(buflen);
	in_use = 0;
	age = 0;
}

ATmpFrameCacheElem::~ATmpFrameCacheElem()
{
	delete frame;
}

size_t ATmpFrameCacheElem::get_size()
{
	return sizeof(AFrame) + frame->get_data_size();
}

void ATmpFrameCacheElem::dump(int indent)
{
	printf("%*sATmpFrameCacheElem %p dump:\n", indent, "", this);
	printf("%*s length %d in_use %d age %u frame %p\n", indent + 2, "",
		length, in_use, age, frame);
}

ATmpFrameCache::ATmpFrameCache()
 : List<ATmpFrameCacheElem>()
{
	moment = 0;
}

ATmpFrameCache::~ATmpFrameCache()
{
	while(last)
		delete last;
}

AFrame *ATmpFrameCache::get_tmpframe(int buflen)
{
	ATmpFrameCacheElem *elem = 0;

	listlock.lock("ATmpFrameCache::get_tmpframe");

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		if(!cur->in_use && buflen == cur->length)
		{
			elem = cur;
			break;
		}
	}
	if(!elem)
	{
		for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
		{
			if(!cur->in_use)
			{
				elem = cur;
				elem->frame->check_buffer(buflen);
				break;
			}
		}
	}
	if(!elem)
	{
		if(!max_alloc)
			max_alloc = BC_Resources::memory_size / MAX_ALLOC;
		if(get_size() > max_alloc)
			delete_old_frames();
		elem = append(new ATmpFrameCacheElem(buflen));
	}
	elem->in_use = 1;

	listlock.unlock();

	return elem->frame;
}

AFrame *ATmpFrameCache::clone_frame(AFrame *frame)
{
	AFrame *aframe;

	if(!frame)
		return 0;

	aframe = get_tmpframe(frame->get_buffer_length());
// Debugging - should never happen
	if(aframe == frame)
		printf("ERR: Cloned the same audio frame %p\n", frame);
	aframe->set_samplerate(frame->get_samplerate());
	aframe->channel = frame->channel;
	aframe->set_track(frame->get_track());
	aframe->set_empty();

	return aframe;
}

void ATmpFrameCache::release_frame(AFrame *tmp_frame)
{
	AFrame *released = 0;

	if(!tmp_frame)
		return;

	listlock.lock("TmpFrameCache::release_frame");

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
	{
		if(tmp_frame == cur->frame)
		{
// Debugging - should never happen
			if(!cur->in_use)
				printf("ERR: Double release of audio frame %p\n", tmp_frame);
			cur->in_use = 0;
			cur->length = cur->frame->get_buffer_length();
			cur->age = ++moment;
			released = tmp_frame;
			break;
		}
	}

	listlock.unlock();
	if(!released)
		printf("Attempt to release non-tmpframe %p\n", tmp_frame);
}

void ATmpFrameCache::delete_old_frames()
{
	while(get_size() > max_alloc)
	{
		if(delete_oldest())
			break;
	}
}

int ATmpFrameCache::delete_oldest()
{
	unsigned int min_age = UINT_MAX;
	ATmpFrameCacheElem *min_elem = 0;

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
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

void ATmpFrameCache::delete_unused()
{
	ATmpFrameCacheElem *nxt;

	listlock.lock("TmpFrameCache::delete_unused");

	for(ATmpFrameCacheElem *cur = first; cur;)
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
	listlock.unlock();
}

size_t ATmpFrameCache::get_size(int *total, int *inuse)
{
	size_t res = 0;
	int used = 0;
	int count = 0;

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
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

void ATmpFrameCache::dump(int indent)
{
	printf("%*sATmpFrameCache %p dump:\n", indent, "", this);

	for(ATmpFrameCacheElem *cur = first; cur; cur = cur->next)
		cur->dump(indent + 2);
}
