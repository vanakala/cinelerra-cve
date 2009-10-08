
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

#ifndef SRATEWINDOW_H
#define SRATEWINDOW_H

#include "guicast.h"

class NormalizeWindow : public BC_Window
{
public:
	NormalizeWindow(int x, int y);
	~NormalizeWindow();
	
	int create_objects(float *db_over, int *seperate_tracks);
	int close_event();
	
	float *db_over;
	int *separate_tracks;
};

class NormalizeWindowOverload : public BC_TextBox
{
public:
	NormalizeWindowOverload(int x, int y, float *db_over);
	~NormalizeWindowOverload();
	
	int handle_event();
	float *db_over;
};

class NormalizeWindowSeparate : public BC_CheckBox
{
public:
	NormalizeWindowSeparate(int x, int y, int *separate_tracks);
	~NormalizeWindowSeparate();
	
	int handle_event();
	int *separate_tracks;
};

#endif
