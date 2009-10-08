
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

#ifndef EDITHANDLES_H
#define EDITHANDLES_H

#include "canvastools.h"
#include "edit.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "trackcanvas.inc"

class EditHandle : public CanvasTool
{
public:
	EditHandle(MWindow *mwindow, 
		TrackCanvas *trackcanvas, 
		Edit *edit, 
		int side, 
		int x, 
		int y);
	virtual ~EditHandle();

	virtual int handle_event();
	int side;
};

class EditHandleIn : public EditHandle
{
public:
	EditHandleIn(MWindow *mwindow, 
		TrackCanvas *trackcanvas, 
		Edit *edit, 
		int x, 
		int y);
	virtual ~EditHandleIn();

	virtual int handle_event();
	int side;
};

class EditHandleOut : public EditHandle
{
public:
	EditHandleOut(MWindow *mwindow, 
		TrackCanvas *trackcanvas, 
		Edit *edit, 
		int x, 
		int y);
	virtual ~EditHandleOut();

	virtual int handle_event();
	int side;
};

class EditHandles : public CanvasTools
{
public:
	EditHandles(MWindow *mwindow, 
		TrackCanvas *trackcanvas);
	~EditHandles();
	
	void update();
};

#endif
