#include <string.h>

#include "defaults.h"
#include "bctextbox.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "bcrecentlist.h"

// NOTE: textbox can be NULL if no textbox is associated
BC_RecentList::BC_RecentList(const char *type, Defaults *defaults, 
		       BC_TextBox *textbox, int max, 
		       int x, int y, int w, int h) 
	: BC_ListBox(x, y, w, h, LISTBOX_TEXT, 0, 0, 0, 1, 0, 1) 
{
	this->type = type;
	this->defaults = defaults;
	this->textbox = textbox;
	this->max = max;
	set_tooltip("Choose from recently used");
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

	if (items.total > 0) {
		items.remove_all_objects();
	}

	int count;
	for (count = 0; count < max; count++) {
		char save[BCTEXTLEN];
		char text[BCTEXTLEN];
		sprintf(save, "RECENT_%s_%s_%d", prefix, type, count);
		sprintf(text, "");
		defaults->get(save, text);
		if (strlen(text) == 0) break;
		items.append(new BC_ListBoxItem(text));
	}

	update(&items, 0, 0, 1);

	return count;
}	


int BC_RecentList::add_item(const char *prefix, char *text) {
	
	// remove an old item if it matches the new text
	for (int i = 0; i < items.total; i++) {
		BC_ListBoxItem *item = items.values[i];
		if (strcmp(text, item->get_text()) == 0) {
			items.remove_object(item);
		}
	}

	// make a new item and put it at the top of the list
	items.insert(new BC_ListBoxItem(text), 0);

	// save up to 'max' items of the list for future use
	int count;
	for (count = 0; count < max && count < items.total; count++) {
		BC_ListBoxItem *item = items.values[count];
		char save[BCTEXTLEN];
		sprintf(save, "RECENT_%s_%s_%d", prefix, type, count);
		defaults->update(save, item->get_text());
	}

	return count;
}
