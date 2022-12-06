// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCLISTBOXITEM_H
#define BCLISTBOXITEM_H

#include "arraylist.h"
#include "bcpixmap.inc"
#include "colors.h"
#include "vframe.inc"

// Every item in a list inherits this
class BC_ListBoxItem
{
public:
	BC_ListBoxItem();
// New items
	BC_ListBoxItem(const char *text, int color = -1);
	BC_ListBoxItem(const char *text, BC_Pixmap *icon, int color = -1);

// autoplace is always 1 in initialization.
// Positions set with the set_ commands cancel autoplacement.
// Final positions are calculated in the next draw_items.

	virtual ~BC_ListBoxItem();

	friend class BC_ListBox;

	BC_ListBoxItem& operator=(BC_ListBoxItem& item);
	void copy_from(BC_ListBoxItem *item);
	void set_text(char *new_text);
	inline char* get_text() { return text; };
	void set_icon(BC_Pixmap *icon);
	void set_icon_vframe(VFrame *icon_vframe);
	inline int get_icon_x() { return icon_x; };
	inline int get_icon_y() { return icon_y; };
	inline int get_text_x() { return text_x; };
	inline int get_text_y() { return text_y; };
	void set_icon_x(int x);
	void set_icon_y(int y);
	void set_text_x(int x);
	void set_text_y(int y);
	void set_color(int color);
	void set_searchable(int value);
	inline int get_color() { return color; };
	void set_selected(int value);
	void set_autoplace_icon(int value);
	void set_autoplace_text(int value);
	void set_selectable(int value);
	inline int get_selectable() { return selectable; }
	virtual void dump(int indent);

// The item with the sublist must be in column 0.  Only this is searched by
// BC_ListBox.
// Mind you, sublists are ignored in icon mode.
	ArrayList<BC_ListBoxItem*>* new_sublist(int columns);
	inline ArrayList<BC_ListBoxItem*>* get_sublist() { return sublist; };
// Return the number of columns in the sublist
	inline int get_columns() { return columns; };
// Return if it's expanded
	inline int get_expand() { return expand; };
	void set_expand(int value);

private:
	void initialize();
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
