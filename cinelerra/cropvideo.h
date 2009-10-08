
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

#ifndef CROPVIDEO_H
#define CROPVIDEO_H

#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"
#include "cropvideo.inc"

class CropVideo : public BC_MenuItem, public Thread
{
public:
	CropVideo(MWindow *mwindow);
	~CropVideo();

	int handle_event();
	void run();
	int load_defaults();
	int save_defaults();
	int fix_aspect_ratio();

	MWindow *mwindow;
};

class CropVideoWindow : public BC_Window
{
public:
	CropVideoWindow(MWindow *mwindow, CropVideo *thread);
	~CropVideoWindow();

	int create_objects();

	CropVideo *thread;
	MWindow *mwindow;
};


#endif
