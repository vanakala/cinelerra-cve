#ifndef BCPOPUPMENU_H
#define BCPOPUPMENU_H

#include "bcmenubar.inc"
#include "bcsubwindow.h"

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
	int set_text(char *text);
	int draw_text();
	int deactivate();
	int activate_menu();
	int deactivate_menu();
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
