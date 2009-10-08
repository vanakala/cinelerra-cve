
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

#ifndef BCLISTBOXITEM_H
#define BCLISTBOXITEM_H

#include "arraylist.h"
#include "bcpixmap.inc"
#include "colors.h"
#include "vframe.h"



// Every item in a list inherits this
class BC_ListBoxItem
{
public:
	BC_ListBoxItem();
// New items
	BC_ListBoxItem(char *text, 
		int color = -1);
	BC_ListBoxItem(char *text, 
		BC_Pixmap *icon, 
		int color = -1);
	


// autoplace is always 1 in initialization.
// Positions set with the set_ commands cancel autoplacement.
// Final positions are calculated in the next draw_items.

	virtual ~BC_ListBoxItem();

	friend class BC_ListBox;

	BC_ListBoxItem& operator=(BC_ListBoxItem& item);
	void copy_from(BC_ListBoxItem *item);
	void set_text(char *new_text);
	char* get_text();
	void set_icon(BC_Pixmap *icon);
	void set_icon_vframe(VFrame *icon_vframe);
	int get_icon_x();
	int get_icon_y();
	int get_text_x();
	int get_text_y();
	void set_icon_x(int x);
	void set_icon_y(int y);
	void set_text_x(int x);
	void set_text_y(int y);
	void set_color(int color);
	void set_searchable(int value);
	int get_color();
	void set_selected(int value);
	int set_autoplace_icon(int value);
	int set_autoplace_text(int value);
	void set_selectable(int value);
	int get_selectable();

// The item with the sublist must be in column 0.  Only this is searched by
// BC_ListBox.
// Mind you, sublists are ignored in icon mode.
	ArrayList<BC_ListBoxItem*>* new_sublist(int columns);
	ArrayList<BC_ListBoxItem*>* get_sublist();
// Return the number of columns in the sublist
	int get_columns();
// Return if it's expanded
	int get_expand();
	void set_expand(int value);

private:
	int initialize();
	int get_icon_w();
	int get_icon_h();


	BC_Pixmap *icon;
	VFrame *icon_vframe;
// x and y position in listbox relative to top left
// Different positions for icon mode and text mode are maintained
	int icon_x, icon_y;
	int text_x, text_y;
// If autoplacement should be performed in the next draw
	int autoplace_icon, autoplace_text;
	char *text;
	int color;
// 1 - currently selected
// 2 - previously selected and now adding selections with shift
	int selected;
// Allow this item to be included in queries.  Directories are not
// included in queries.
	int searchable;

// Array of one list of pointers for each column for a sublist.
// It's an array so we can pass the sublist directly to another listbox.
	ArrayList<BC_ListBoxItem*> *sublist;
// Columns in sublist
	int columns;
// Sublist is visible
	int expand;
// Item is selectable
	int selectable;
};



#endif
