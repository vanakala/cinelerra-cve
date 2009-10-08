
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

#ifndef BCPOPUP_H
#define BCPOPUP_H

#include "arraylist.h"
#include "bcpixmap.inc"
#include "bcwindowbase.h"

class BC_Popup : public BC_WindowBase
{
public:
	BC_Popup(BC_WindowBase *parent_window, 
				int x, 
				int y, 
				int w, 
				int h, 
				int bg_color, 
				int hide = 0, 
				BC_Pixmap *bg_pixmap = 0);
	virtual ~BC_Popup();

	int initialize() { return 0; };
private:
};

class BC_FullScreen : public BC_WindowBase
{
public:
   BC_FullScreen(BC_WindowBase *parent_window, 
			   int w, 
			   int h, 
			   int bg_color, 
			   int vm_scale,
			   int hide = 0, 
			   BC_Pixmap *bg_pixmap = 0);
   virtual ~BC_FullScreen();
};

#endif
