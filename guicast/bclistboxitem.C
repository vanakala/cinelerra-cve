
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

#include "bclistboxitem.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcwindowbase.h"

#include <string.h>


// ====================================================== item

BC_ListBoxItem::BC_ListBoxItem()
{
	initialize();

	color = BC_WindowBase::get_resources()->listbox_text;
	this->text = new char[1];
	text[0] = 0;
	selectable = 1;
}

BC_ListBoxItem::BC_ListBoxItem(char *text, 
	BC_Pixmap *icon, 
	int color)
{
	initialize();

	if(color == -1) color = BC_WindowBase::get_resources()->listbox_text;
	this->text = new char[strlen(text) + 1];
	this->icon = icon;

	strcpy(this->text, text);
	this->color = color;
	selectable = 1;
}

BC_ListBoxItem::BC_ListBoxItem(char *text, int color)
{
	initialize();

	if(color == -1) color = BC_WindowBase::get_resources()->listbox_text;
	this->text = new char[strlen(text) + 1];
	strcpy(this->text, text);
	this->color = color;
	selectable = 1;
}

BC_ListBoxItem::~BC_ListBoxItem()
{
	if(text) delete [] text;
	if(sublist)
	{
		for(int i = 0; i < columns; i++)
			sublist[i].remove_all_objects();
		delete sublist;
	}
}

int BC_ListBoxItem::initialize()
{
	autoplace_icon = 1;
	autoplace_text = 1;
	text = 0;
	color = BLACK;
	selected = 0;
	icon = 0;
	icon_vframe = 0;
	text_x = -1;
	text_y = -1;
	icon_x = -1;
	icon_y = -1;
	searchable = 1;
	sublist = 0;
	columns = 0;
	expand = 0;
	return 0;
}

int BC_ListBoxItem::get_icon_x()
{
	return icon_x;
}

int BC_ListBoxItem::get_icon_y()
{
	return icon_y;
}

int BC_ListBoxItem::get_text_x()
{
	return text_x;
}

int BC_ListBoxItem::get_text_y()
{
	return text_y;
}

int BC_ListBoxItem::set_autoplace_icon(int value)
{
	autoplace_icon = value;
	return 0;
}

int BC_ListBoxItem::set_autoplace_text(int value)
{
	autoplace_text = value;
	return 0;
}

void BC_ListBoxItem::set_icon_x(int x)
{
	icon_x = x;
	autoplace_icon = 0;
}

void BC_ListBoxItem::set_icon_y(int y)
{
	icon_y = y;
	autoplace_icon = 0;
}

void BC_ListBoxItem::set_selected(int value)
{
	this->selected = value;
}

void BC_ListBoxItem::set_searchable(int value)
{
	this->searchable = value;
}

void BC_ListBoxItem::set_selectable(int value)
{
	this->selectable = value;
}

int BC_ListBoxItem::get_selectable()
{
	return selectable;
}



void BC_ListBoxItem::set_text_x(int x)
{
	text_x = x;
	autoplace_text = 0;
}
void BC_ListBoxItem::set_text_y(int y)
{
	text_y = y;
	autoplace_text = 0;
}

int BC_ListBoxItem::get_icon_w()
{
	return icon->get_w();
}

int BC_ListBoxItem::get_icon_h()
{
	return icon->get_h();
}

void BC_ListBoxItem::set_text(char *new_text)
{
	if(this->text) delete [] this->text;
	this->text = 0;

	if(new_text)
	{
		this->text = new char[strlen(new_text) + 1];
		strcpy(this->text, new_text);
	}
}

char* BC_ListBoxItem::get_text()
{
	return text;
}

void BC_ListBoxItem::set_icon(BC_Pixmap *icon)
{
	this->icon = icon;
}

void BC_ListBoxItem::set_icon_vframe(VFrame *icon_vframe)
{
	this->icon_vframe = icon_vframe;
}

void BC_ListBoxItem::set_color(int color)
{
	this->color = color;
}

int BC_ListBoxItem::get_color()
{
	return color;
}


BC_ListBoxItem& BC_ListBoxItem::operator=(BC_ListBoxItem& item)
{
	copy_from(&item);
	return *this;
}

void BC_ListBoxItem::copy_from(BC_ListBoxItem *item)
{
	if(item->text) set_text(item->text);
	color = item->color;
	text_x = item->text_x;
	text_y = item->text_y;
	icon_x = item->icon_x;
	icon_y = item->icon_y;
	selectable = item->selectable;
	columns = item->columns;
	if(item->sublist)
	{
		sublist = new ArrayList<BC_ListBoxItem*>[columns];
		for(int i = 0; i < columns; i++)
		{
			ArrayList<BC_ListBoxItem*> *list = &item->get_sublist()[i];

			for(int j = 0; j < list->total; j++)
			{
				BC_ListBoxItem *new_item = new BC_ListBoxItem;
				BC_ListBoxItem *old_item = list->values[j];
				sublist[i].append(new_item);
				new_item->copy_from(old_item);
			}
		}
	}
}

ArrayList<BC_ListBoxItem*>* BC_ListBoxItem::new_sublist(int columns)
{
	sublist = new ArrayList<BC_ListBoxItem*>[columns];
	this->columns = columns;
	return sublist;
}

ArrayList<BC_ListBoxItem*>* BC_ListBoxItem::get_sublist()
{
	return sublist;
}

int BC_ListBoxItem::get_columns()
{
	return columns;
}

int BC_ListBoxItem::get_expand()
{
	return expand;
}

void BC_ListBoxItem::set_expand(int value)
{
	expand = value;
}




