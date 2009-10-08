
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

#include "bcpopup.h"


BC_FullScreen::BC_FullScreen(BC_WindowBase *parent_window, int w, int h, 
			   int bg_color,
			   int vm_scale,
			   int hide,
			   BC_Pixmap *bg_pixmap)
 : BC_WindowBase()
{
#ifdef HAVE_LIBXXF86VM
   if (vm_scale) 
	   create_window(parent_window,
			   "Fullscreen", 
			   0,
			   0,
			   w, 
			   h, 
			   w, 
			   h, 
			   0,
			   parent_window->top_level->private_color,
			   hide,
			   bg_color,
			   NULL,
			   VIDMODE_SCALED_WINDOW,
			   bg_pixmap,
			   0);
   else
#endif
   create_window(parent_window,
			   "Fullscreen", 
			   0,
			   0,
			   w, 
			   h, 
			   w, 
			   h, 
			   0,
			   parent_window->top_level->private_color, 
			   hide,
			   bg_color,
			   NULL,
			   POPUP_WINDOW,
			   bg_pixmap,
			   0);
}


BC_FullScreen::~BC_FullScreen()
{
}


BC_Popup::BC_Popup(BC_WindowBase *parent_window, 
				int x,
				int y,
				int w, 
				int h, 
				int bg_color,
				int hide,
				BC_Pixmap *bg_pixmap)
 : BC_WindowBase()
{
	create_window(parent_window,
				"Popup", 
				x,
				y,
				w, 
				h, 
				w, 
				h, 
				0,
				parent_window->top_level->private_color, 
				hide,
				bg_color,
				NULL,
				POPUP_WINDOW,
				bg_pixmap,
				0);
}


BC_Popup::~BC_Popup()
{
}

