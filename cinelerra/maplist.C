// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "maplist.h"

#include <stdio.h>

MapItem::MapItem()
 : ListItem<MapItem>()
{
	pts = 0;
	set = 0;
}

void MapItem::dump(int indent)
{
	printf("%*spts %.3f set %d\n", indent, "", pts, set);
}

MapList::MapList()
 : List<MapItem>()
{
	append();
}

void MapList::set_map(ptstime start, ptstime end, int val)
{
	MapItem *current;
	int ov;

	if(PTSEQU(start, end))
		return;

	for(current = first; current; current = NEXT)
	{
		if(current->pts <= start) // v3...v4
		{
			if(current->next)
			{
				if(current->set == val)
				{
					if(current->next->pts < end)
						current->next->pts = end;
					break;
				}
				else
				{
					if(PTSEQU(current->pts, start))
					{
						current = insert_before(current);
						current->pts = start;
						current->next->pts = end;
						current->set = val;
					}
					else
					{
						ov = current->set;
						current = insert_after(current);
						current->pts = start;
						current->set = val;
						if(current->next->pts > end)
						{
							current = insert_after(current);
							current->pts = end;
							current->set = ov;
						}
						else
							current->next->pts = end;
					}
					break;
				}
			}
			else
			{
				if(current->previous)
				{
					if(current->previous->set == val)
						current->pts = end;
					else
					{
						current = insert_before(current);
						current->pts = start;
						current->next->pts = end;
						current->set = val;
					}
					break;
				}
				else
				{
					current = insert_before(current);
					current->pts = start;
					current->next->pts = end;
					current->set = val;
					break;
				}
			}
		}
	}
	if(current == 0)
	{
		current = insert_before(first);
		current->pts = start;
		current->next->pts = end;
		current->set = val;
	}
	if(current = current->next)
	{
		while(current->next && current->pts > current->next->pts)
		{
			current = current->next;
			current->pts = current->previous->pts;
			delete current->previous;
		}
	}

	if(first == last)
		first->pts = 0;
	for(current = first; current; current = current->next)
	{
		if(current->next && PTSEQU(current->next->pts, current->pts))
		{
			current = current->next;
			delete current->previous;
		}
		if(current->previous && current->previous->set == current->set)
		{
			current->pts = current->previous->pts;
			delete current->previous;
		}
	}
}

void MapList::clear_map()
{
	while(last)
		delete last;
}

int MapList::is_set(ptstime pts)
{
	MapItem *current;

	if(pts >= first->pts && pts < last->pts)
	{
		for(current = first; current && current->next; current = current->next)
		{
			if(pts >= current->pts && pts < current->next->pts)
				return current->set;
		}
	}
	return 0;
}

void MapList::dump(int indent)
{
	MapItem *item;

	printf("%*sMaplist %p dump:\n", indent, "", this);

	for(item = first; item; item = item->next)
		item->dump(indent + 2);
}
