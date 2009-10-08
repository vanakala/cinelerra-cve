
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

#ifndef BCSUBWINDOW_H
#define BCSUBWINDOW_H

#include "arraylist.h"
#include "bcwindowbase.h"

class BC_SubWindow : public BC_WindowBase
{
public:
	BC_SubWindow(int x, int y, int w, int h, int bg_color = -1);
	virtual ~BC_SubWindow();

	virtual int initialize();
private:
};

class BC_SubWindowList : public ArrayList<BC_WindowBase*>
{
public:
	BC_SubWindowList();
	~BC_SubWindowList();


private:
	
};



#endif
