
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

#ifndef KEYFRAMEHANDLES_H
#define KEYFRAMEHANDLES_H

#include "canvastools.h"
#include "guicast.h"
#include "mwindow.inc"
#include "track.inc"
#include "trackcanvas.inc"

class KeyframeHandle : public CanvasTool
{
public:
	KeyframeHandle(MWindow *mwindow, 
		TrackCanvas *trackcanvas,
		Track *track,
		int64_t position,
		int x,
		int y);
	~KeyframeHandle();
	
	int handle_event();
	
	Track *track;
	int64_t position;
};


class KeyframeHandles : public CanvasTools
{
public:
	KeyframeHandles(MWindow *mwindow, 
		TrackCanvas *trackcanvas);
	~KeyframeHandles();
};


#endif
