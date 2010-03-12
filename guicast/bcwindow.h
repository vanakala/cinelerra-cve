
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

#ifndef BCWINDOW_H
#define BCWINDOW_H

#include "bcwindowbase.h"

class BC_Window : public BC_WindowBase
{
public:
	BC_Window(const char *title, 
				int x,
				int y,
				int w, 
				int h, 
				int minw = -1, 
				int minh = -1, 
				int allow_resize = 1,
				int private_color = 0, 
				int hide = 0,
				int bg_color = -1,
				const char *display_name = "",
				int group_it = 1);
	virtual ~BC_Window();

private:
};


#endif
