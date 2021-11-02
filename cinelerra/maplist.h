// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAPLIST_H
#define MAPLIST_H

#include "datatype.h"
#include "linklist.h"
#include "maplist.inc"

class MapItem : public ListItem<MapItem>
{
public:
	MapItem();

	void dump(int indent);

	ptstime pts;
	int set;
};


class MapList : public List<MapItem>
{
public:
	MapList();

	void set_map(ptstime start, ptstime end, int val);
	int is_set(ptstime);
	void clear_map();
	void dump(int indent);
};

#endif
