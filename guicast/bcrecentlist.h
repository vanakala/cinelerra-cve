// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2004 Nathan Kurz

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

#define RECENT_OPT_BASEUNQ   1

class BC_RecentList : public BC_ListBox
{
public:
	BC_RecentList(const char *type, BC_Hash *defaults, 
		BC_TextBox *textbox, int max,
		int x, int y, int w, int h);
	~BC_RecentList();

	BC_RecentList(const char *type, BC_Hash *defaults);
	BC_RecentList(const char *type, BC_Hash *defaults,
		BC_TextBox *textbox);
	int handle_event();
	int set_options(int opts);
	int load_items(const char *prefix = NULL);
	int add_item(const char *prefix, const char *text);

	ArrayList<BC_ListBoxItem*> items;
private:
	// option bits;
	// 1 - unique basename
	int options;
	const char *type;
	BC_TextBox *textbox;
	BC_Hash *defaults;
	char str1[BCTEXTLEN];
	char str2[BCTEXTLEN];
};

#endif /* BCRECENTLIST_H */
