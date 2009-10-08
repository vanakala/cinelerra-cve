
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

#ifndef BITSPOPUP_H
#define BITSPOPUP_H

#include "guicast.h"

class BitsPopupMenu;
class BitsPopupText;

class BitsPopup
{
public:
	BitsPopup(BC_WindowBase *parent_window, 
		int x, 
		int y, 
		int *output, 
		int use_ima4, 
		int use_ulaw, 
		int use_adpcm,
		int use_float,
		int use_32linear);
	~BitsPopup();
	int create_objects();
	int get_w();
	int get_h();
	
	BitsPopupMenu *menu;
	ArrayList<BC_ListBoxItem*> bits_items;
	BitsPopupText *textbox;
	int x, y, use_ima4, use_ulaw, use_float, use_adpcm, *output;
	int use_32linear;
	BC_WindowBase *parent_window;
};

class BitsPopupMenu : public BC_ListBox
{
public:
	BitsPopupMenu(BitsPopup *popup, int x, int y);
	int handle_event();
	BitsPopup *popup;
};

class BitsPopupText : public BC_TextBox
{
public:
	BitsPopupText(BitsPopup *popup, int x, int y);
	int handle_event();
	BitsPopup *popup;
};





#endif
