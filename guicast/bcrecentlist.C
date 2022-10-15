// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2004 Nathan Kurz

#include "bchash.h"
#include "bcsignals.h"
#include "bctextbox.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "bcrecentlist.h"
#include "language.h"

#include <string.h>

BC_RecentList::BC_RecentList(const char *type, BC_Hash *defaults, 
	BC_TextBox *textbox, int max, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, 0, LISTBOX_POPUP)
{
	this->type = type;
	this->defaults = defaults;
	this->textbox = textbox;
	set_tooltip(_("Choose from recently used"));
	options = 0;
}

BC_RecentList::BC_RecentList(const char *type, BC_Hash *defaults, BC_TextBox *textbox)
 : BC_ListBox(textbox->get_x() + textbox->get_w(), textbox->get_y(),
	textbox->get_w(), RECENT_POPUP_HEIGHT, 0, LISTBOX_POPUP)
{
	this->type = type;
	this->defaults = defaults;
	this->textbox = textbox;
	set_tooltip(_("Choose from recently used"));
	options = 0;
}

BC_RecentList::BC_RecentList(const char *type, BC_Hash *defaults) 
 : BC_ListBox(-1, -1, -1, -1, 0, LISTBOX_POPUP)
{
	this->type = type;
	this->defaults = defaults;
	this->textbox = NULL;
	options = 0;
}

BC_RecentList::~BC_RecentList()
{
	items.remove_all_objects();
}

int BC_RecentList::handle_event()
{
	BC_ListBoxItem *item = get_selection(0, 0);

	if(item < 0)
		return 0;

	char *text = item->get_text();
	if(text && textbox)
	{
		// change the text in the textbox
		textbox->update(text);
		// tell the textbox to deal with it
		textbox->handle_event();
	}
	return 0;
}

int BC_RecentList::set_options(int opts)
{
	int o = options;

	options = opts;
	return o;
}

int BC_RecentList::load_items(const char *prefix)
{
	int count;

	if(!prefix)
		prefix = "ANY";

	if(items.total > 0)
		items.remove_all_objects();

	for(count = 0; count < RECENT_MAX_ITEMS; count++)
	{
		sprintf(str1, "RECENT_%s_%s_%d", prefix, type, count);
		str2[0] = 0;
		defaults->get(str1, str2);
		if(str2[0] == 0)
			break;
		items.append(new BC_ListBoxItem(str2));
	}

	// only update if we are part of a window
	if(textbox)
		update(&items, 0, 0, 1);

	return count;
}

int BC_RecentList::add_item(const char *prefix, const char *text)
{
	int count;
	char *bn1, *bn2;

	if(!prefix)
		prefix = "ANY";

	if(options & RECENT_OPT_BASEUNQ)
	{
		strcpy(str1, text);
		bn1 = basename(str1);
	}

	// remove an old item if it matches the new text
	for(int i = 0; i < items.total; i++)
	{
		BC_ListBoxItem *item = items.values[i];

		if(options & RECENT_OPT_BASEUNQ)
		{
			strcpy(str2, item->get_text());
			bn2 = basename(str2);

			if(strcmp(bn1, bn2) == 0)
			{
				items.remove_object(item);
				i--;
			}
		}
		else
		{
			if(strcmp(text, item->get_text()) == 0)
			{
				items.remove_object(item);
				i--;
			}
		}
	}

	// make a new item and put it at the top of the list
	items.insert(new BC_ListBoxItem(text), 0);

	// save up to maximum items of the list for future use
	for(count = 0; count < RECENT_MAX_ITEMS && count < items.total; count++)
	{
		BC_ListBoxItem *item = items.values[count];

		sprintf(str1, "RECENT_%s_%s_%d", prefix, type, count);
		defaults->update(str1, item->get_text());
	}
	// Remove rest from defaults
	for(int i = count; i < RECENT_MAX_ITEMS; i++)
	{
		sprintf(str1, "RECENT_%s_%s_%d", prefix, type, count);
		defaults->delete_key(str1);
	}
	return count;
}
