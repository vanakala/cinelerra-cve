#ifndef BCRECENTLIST_H
#define BCRECENTLIST_H

#include "bclistbox.h"
#include "bctextbox.inc"
#include "defaults.inc"

// Maintain a list of text items (up to 'max') from session to session.
// List automatically saved to Cinelerra_rc as 'RECENT_prefix_0', etc.
// Item value is written to 'text_box' when item is selected (if textbox set)

#define RECENT_MAX_ITEMS 10
#define RECENT_POPUP_HEIGHT 100

class BC_RecentList : public BC_ListBox
{
 public:
	BC_RecentList(const char *type, Defaults *defaults, 
		      BC_TextBox *textbox, int max,
		      int x, int y, int w, int h);
	BC_RecentList(const char *type, Defaults *defaults);
	BC_RecentList(const char *type, Defaults *defaults, 
		      BC_TextBox *textbox);

	~BC_RecentList();

	int handle_event();
	int load_items(const char *prefix = NULL);
	int add_item(const char *prefix, char *text);
	
	ArrayList<BC_ListBoxItem*> items;
 private: 
	const char *type;
	BC_TextBox *textbox;
	Defaults *defaults;
};

#endif /* BCRECENTLIST_H */
