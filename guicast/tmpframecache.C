
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
#include "colormodels.h"
#include "tmpframecache.h"
#include "mutex.h"
#include "vframe.h"

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
	printf("%*s [%dx%d] '%s' in_use %d age %u\n", indent + 2, "",
		width, height, ColorModels::name(cmodel), in_use, age);
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
		elem = append(new TmpFrameCacheElem(w, h, colormodel));

	elem->in_use = 1;

	listlock.unlock();

	return elem->frame;
}

void TmpFrameCache::release_frame(VFrame *tmp_frame)
{
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

size_t TmpFrameCache::get_size()
{
	size_t res = 0;

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
		res += cur->get_size();

	return res;
}

void TmpFrameCache::dump(int indent)
{
	printf("%*sTmpFrameCache %p dump:\n", indent, "", this);

	for(TmpFrameCacheElem *cur = first; cur; cur = cur->next)
		cur->dump(indent + 2);
}
