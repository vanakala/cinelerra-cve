#ifndef RECENTLIST_H
#define RECENTLIST_H

#include "guicast.h"
#include "defaults.inc"

// Maintain a list of text items (up to 'max') from session to session.
// List automatically saved to Cinelerra_rc as 'prefix_0', prefix_1', etc.
// Item value is written to 'text_box' when item is selected.

class RecentList : public BC_ListBox
{
 public:
	RecentList(char *type, Defaults *defaults, 
		   BC_TextBox *textbox, int max,
		   int x, int y, int w, int h);
	~RecentList();

	int handle_event();
	int load_items(int format);
	int add_item(int format, char *text);

 private:
	int max;
	char *type;
	BC_TextBox *textbox;
	Defaults *defaults;
	ArrayList<BC_ListBoxItem*> items;
};

#endif /* RECENTLIST_H */
