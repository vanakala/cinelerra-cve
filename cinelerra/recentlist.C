#include <string.h>

#include "defaults.h"
#include "recentlist.h"

RecentList::RecentList(char *prefix, Defaults *defaults, BC_TextBox *textbox,
		       int max, int x, int y, int w, int h) 
	: BC_ListBox(x, y, w, h, LISTBOX_TEXT, 0, 0, 0, 1, 0, 1) 
{
	this->textbox = textbox;
	this->defaults = defaults;
	this->prefix = prefix;
	this->max = max;
}

RecentList::~RecentList() {
	items.remove_all_objects();
}

int RecentList::handle_event() {
	BC_ListBoxItem *item = get_selection(0, 0);
	if (item < 0) return 0;
	char *text = item->get_text();
	if (text) textbox->update(text);
	return 0;
}

int RecentList::create_objects() {
	set_tooltip("Choose from recently used");
	update(&items, 0, 0, 1);
	return 0;
}

int RecentList::load_items() {

	if (items.total > 0) {
		items.remove_all_objects();
	}

	int count;
	for (count = 0; count < max; count++) {
		char save[BCTEXTLEN];
		char text[BCTEXTLEN];
		sprintf(save, "%s_%d", prefix, count);
		sprintf(text, "");
		// FIXME: Defaults::get(char*,char*) at guicast/defaults.C:138 
		//        looks really odd.  Perhaps a typo needs to be fixed?
		defaults->get(save, text);
		if (strlen(text) == 0) break;
		items.append(new BC_ListBoxItem(text));
	}

	return count;
}	


int RecentList::add_item(char *text) {
	
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
		sprintf(save, "%s_%d", prefix, count);
		defaults->update(save, item->get_text());
	}

	return count;
}
