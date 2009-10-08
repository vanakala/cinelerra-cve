
/*
 * CINELERRA
 * Copyright (C) 2004 Nathan Kurz
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

#ifndef BCRECENTLIST_H
#define BCRECENTLIST_H

#include "bchash.inc"
#include "bclistbox.h"
#include "bctextbox.inc"

// Maintain a list of text items (up to 'max') from session to session.
// List automatically saved to Cinelerra_rc as 'RECENT_prefix_0', etc.
// Item value is written to 'text_box' when item is selected (if textbox set)

#define RECENT_MAX_ITEMS 10
#define RECENT_POPUP_HEIGHT 100

class BC_RecentList : public BC_ListBox
{
 public:
	BC_RecentList(const char *type, BC_Hash *defaults, 
		      BC_TextBox *textbox, int max,
		      int x, int y, int w, int h);
	BC_RecentList(const char *type, BC_Hash *defaults);
	BC_RecentList(const char *type, BC_Hash *defaults, 
		      BC_TextBox *textbox);

	~BC_RecentList();

	int handle_event();
	int load_items(const char *prefix = NULL);
	int add_item(const char *prefix, char *text);
	
	ArrayList<BC_ListBoxItem*> items;
 private: 
	const char *type;
	BC_TextBox *textbox;
	BC_Hash *defaults;
};

#endif /* BCRECENTLIST_H */
