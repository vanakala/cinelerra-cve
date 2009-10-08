
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

#ifndef BYTEORDERPOPUP_H
#define BYTEORDERPOPUP_H

#include "guicast.h"

class ByteOrderList;
class ByteOrderText;

class ByteOrderPopup
{
public:
	ByteOrderPopup(BC_WindowBase *parent_window, int x, int y, int *output);
	~ByteOrderPopup();

	int create_objects();

	ArrayList<BC_ListBoxItem*> byteorder_items;
	BC_WindowBase *parent_window;
	int x;
	int y;
	int *output;
	ByteOrderList *menu;
	ByteOrderText *textbox;
};

class ByteOrderList : public BC_ListBox
{
public:
	ByteOrderList(ByteOrderPopup *popup, int x, int y);
	int handle_event();
	ByteOrderPopup *popup;
};

class ByteOrderText : public BC_TextBox
{
public:
	ByteOrderText(ByteOrderPopup *popup, int x, int y);
	int handle_event();
	ByteOrderPopup *popup;
};


#endif
