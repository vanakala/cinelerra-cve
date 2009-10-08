
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

#include <string.h>

#include "bchash.h"
#include "bctextbox.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "bcrecentlist.h"

// NOTE: textbox can be NULL if no textbox is associated
BC_RecentList::BC_RecentList(const char *type, BC_Hash *defaults, 
			     BC_TextBox *textbox, int max, 
			     int x, int y, int w, int h) 
	: BC_ListBox(x, y, w, h, LISTBOX_TEXT, 0, 0, 0, 1, 0, 1) 
{
	this->type = type;
	this->defaults = defaults;
	this->textbox = textbox;
	set_tooltip("Choose from recently used");
}

BC_RecentList::BC_RecentList(const char *type, BC_Hash *defaults, 
			     BC_TextBox *textbox) 
	: BC_ListBox(textbox->get_x() + textbox->get_w(), textbox->get_y(),
		     textbox->get_w(), RECENT_POPUP_HEIGHT,
		     LISTBOX_TEXT, 0, 0, 0, 1, 0, 1)
{
	this->type = type;
	this->defaults = defaults;
	this->textbox = textbox;
	set_tooltip("Choose from recently used");
}

BC_RecentList::BC_RecentList(const char *type, BC_Hash *defaults) 
	: BC_ListBox(-1, -1, -1, -1, LISTBOX_TEXT, 0, 0, 0, 1, 0, 1)
{
	this->type = type;
	this->defaults = defaults;
	this->textbox = NULL;
}

BC_RecentList::~BC_RecentList() {
	items.remove_all_objects();
}

int BC_RecentList::handle_event() {
	BC_ListBoxItem *item = get_selection(0, 0);
	if (item < 0) return 0;
	char *text = item->get_text();
	if (text && textbox) {
		// change the text in the textbox
		textbox->update(text);
		// tell the textbox to deal with it
		textbox->handle_event();
	}
	return 0;
}


int BC_RecentList::load_items(const char *prefix) {

	if (! prefix) prefix = "ANY";

	if (items.total > 0) {
		items.remove_all_objects();
	}

	int count;
	for (count = 0; count < RECENT_MAX_ITEMS; count++) {
		char save[BCTEXTLEN];
		char text[BCTEXTLEN];
		sprintf(save, "RECENT_%s_%s_%d", prefix, type, count);
		sprintf(text, "");
		defaults->get(save, text);
		if (strlen(text) == 0) break;
		items.append(new BC_ListBoxItem(text));
	}

	// only update if we are part of a window
	if (textbox) update(&items, 0, 0, 1);

	return count;
}	


int BC_RecentList::add_item(const char *prefix, char *text) {

	if (! prefix) prefix = "ANY";
	
	// remove an old item if it matches the new text
	for (int i = 0; i < items.total; i++) {
		BC_ListBoxItem *item = items.values[i];
		if (strcmp(text, item->get_text()) == 0) {
			items.remove_object(item);
		}
	}

	// make a new item and put it at the top of the list
	items.insert(new BC_ListBoxItem(text), 0);

	// save up to maximum items of the list for future use
	int count;
	for (count = 0; 
	     count < RECENT_MAX_ITEMS && count < items.total; 
	     count++) {
		BC_ListBoxItem *item = items.values[count];
		char save[BCTEXTLEN];
		sprintf(save, "RECENT_%s_%s_%d", prefix, type, count);
		defaults->update(save, item->get_text());
	}

	return count;
}
