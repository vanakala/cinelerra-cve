// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "edlsession.h"
#include "fonts.h"
#include "mainclock.h"

MainClock::MainClock(int x, int y, int w)
 : BC_Title(x, y, (const char *)0, MEDIUM_7SEGMENT, BLACK, 0, w)
{
	position_offset = 0;
}

void MainClock::set_frame_offset(ptstime value)
{
	position_offset = value;
}

void MainClock::update(ptstime position)
{
	char string[BCTEXTLEN];

	position += position_offset;
	edlsession->ptstotext(string, position);
	BC_Title::update(string);
}
