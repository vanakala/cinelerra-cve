
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "edlsession.h"
#include "fonts.h"
#include "mainclock.h"

MainClock::MainClock(int x, int y, int w)
 : BC_Title(x,
	y,
	(const char *)0,
	MEDIUM_7SEGMENT,
	BLACK,
	0,
	w)
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
