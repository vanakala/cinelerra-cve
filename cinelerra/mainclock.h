// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINCLOCK_H
#define MAINCLOCK_H

#include "bctitle.h"
#include "datatype.h"

class MainClock : public BC_Title
{
public:
	MainClock(int x, int y, int w);

	void set_frame_offset(ptstime value);
	void update(ptstime position);

	ptstime position_offset;
};

#endif
