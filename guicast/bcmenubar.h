#ifndef BCMENUBAR_H
#define BCMENUBAR_H

#include "bcmenu.inc"
#include "bcmenubar.inc"
#include "bcsubwindow.h"

class BC_MenuBar : public BC_SubWindow
{
public:
	BC_MenuBar(int x, int y, int w);
	virtual ~BC_MenuBar();

	friend class BC_Menu;

	int add_menu(BC_Menu* menu);
	static int calculate_height(BC_WindowBase *window);

	int initialize();
	int focus_out_event();
	int keypress_event();
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int cursor_enter_event();
	int cursor_leave_event();
	int resize_event(int w, int h);
	int translation_event();
	int deactivate();
	int unhighlight();
// Redraws items in active menu
	void draw_items();

private:
	int draw_face();
	int activate();
// Array of menu titles
	ArrayList<BC_Menu*> menu_titles;  
// number of button releases since activation
	int button_releases;        
// When a menu is pulled down
	int active;
};





#endif
