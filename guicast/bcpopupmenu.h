#ifndef BCPOPUPMENU_H
#define BCPOPUPMENU_H

#include "bcmenubar.inc"
#include "bcmenuitem.inc"
#include "bcsubwindow.h"


// A menu that pops up in the middle of a window or under a button.



class BC_PopupMenu : public BC_SubWindow
{
public:
	BC_PopupMenu(int x, 
		int y, 
		int w, 
		char *text, 
		int use_title = 1);
	virtual ~BC_PopupMenu();

	virtual int handle_event() { return 0; };
	char* get_text();
	int initialize();
	int add_item(BC_MenuItem *item);
	int remove_item(BC_MenuItem *item);
	int total_items();
	BC_MenuItem* get_item(int i);
// Set title of menu
	void set_text(char *text);
// Set icon of menu.  Disables text.
	void set_icon(BC_Pixmap *pixmap);
// Draw title of menu
	int draw_title();
	int deactivate();
	int activate_menu();
	int deactivate_menu();
	int focus_out_event();
	int repeat_event(int64_t duration);
	int button_press_event();
	int button_release_event();
	int cursor_leave_event();
	int cursor_enter_event();
	int cursor_motion_event();
	int translation_event();
	int drag_start_event();
	int drag_stop_event();
	int drag_motion_event();
	int set_images(VFrame **data);

private:
	char text[BCTEXTLEN];
	BC_Pixmap *images[9];
	BC_Pixmap *icon;
	int highlighted;
	int popup_down;
	int use_title;
	int button_releases;
	BC_MenuPopup *menu_popup;
// Remember cursor position when no title
	int button_press_x, button_press_y;
	int status;
};



#endif
