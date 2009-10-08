
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

#ifndef CANVASTOOLS_H
#define CANVASTOOLS_H

#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "trackcanvas.inc"
#include "vframe.inc"


// This was originally supposed to give a button feel to timeline elements.
// It is no longer used.



class CanvasTool : public BC_Button
{
public:
	CanvasTool(MWindow *mwindow, 
		TrackCanvas *trackcanvas,
		Edit *edit,
		int x,
		int y,
		VFrame **data);
	virtual ~CanvasTool();
	
	int visible;
	MWindow *mwindow;
	TrackCanvas *trackcanvas;
	Edit *edit;
	int x;
	int y;
};


class CanvasTools : public ArrayList<CanvasTool*>
{
public:
	CanvasTools(MWindow *mwindow,
		TrackCanvas *trackcanvas);
	virtual ~CanvasTools();
	
	void decrease_visible();
	void delete_invisible();
	int visible(int x, int y, int w, int h);

	MWindow *mwindow;
	TrackCanvas *trackcanvas;
};


#endif
