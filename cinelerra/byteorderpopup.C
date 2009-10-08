
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

#include "byteorderpopup.h"
#include "file.h"


ByteOrderPopup::ByteOrderPopup(BC_WindowBase *parent_window, 
	int x, 
	int y, 
	int *output)
{
	this->parent_window = parent_window;
	this->output = output;
	this->x = x;
	this->y = y;
}

ByteOrderPopup::~ByteOrderPopup()
{
	delete menu;
	delete textbox;
	for(int i = 0; i < byteorder_items.total; i++)
		delete byteorder_items.values[i];
}

int ByteOrderPopup::create_objects()
{
	byteorder_items.append(new BC_ListBoxItem(File::byteorder_to_str(0)));
	byteorder_items.append(new BC_ListBoxItem(File::byteorder_to_str(1)));

	parent_window->add_subwindow(textbox = new ByteOrderText(this, x, y));
	x += textbox->get_w();
	parent_window->add_subwindow(menu = new ByteOrderList(this, x, y));
	return 0;
}

ByteOrderList::ByteOrderList(ByteOrderPopup *popup, int x, int y)
 : BC_ListBox(x,
 	y,
	100,
	100,
	LISTBOX_TEXT,
	&popup->byteorder_items,
	0,
	0,
	1,
	0,
	1)
{
	this->popup = popup;
}

int ByteOrderList::handle_event()
{
	popup->textbox->update(get_selection(0, 0)->get_text());
	popup->textbox->handle_event();
	return 1;
}

ByteOrderText::ByteOrderText(ByteOrderPopup *popup, int x, int y)
 : BC_TextBox(x, y, 100, 1, File::byteorder_to_str(*popup->output))
{
	this->popup = popup;
}

int ByteOrderText::handle_event()
{
	*popup->output = File::str_to_byteorder(get_text());
	return 1;
}
