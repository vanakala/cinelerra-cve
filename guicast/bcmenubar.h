#ifndef BCMENUBAR_H
#define BCMENUBAR_H

#include "bcmenubar.inc"
#include "bcpopup.inc"
#include "bcsubwindow.h"

class BC_MenuBar : public BC_SubWindow
{
public:
	BC_MenuBar(int x, int y, int w);
	virtual ~BC_MenuBar();

	friend class BC_Menu;

	int add_menu(BC_Menu* menu);

	int initialize();
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

class BC_Menu
{
public:
	BC_Menu(char *text);
	virtual ~BC_Menu();

	friend class BC_MenuBar;

// Called by user
	int add_item(BC_MenuItem* menuitem);
	int remove_item(BC_MenuItem* item = 0);
	int total_menuitems();
	int set_text(char *text);

// Called by BC_Menubar
	int initialize(BC_WindowBase *top_level, BC_MenuBar *menu_bar, int x, int y, int w, int h);
	int dispatch_button_press();
	int dispatch_button_release();
	int dispatch_keypress();
	int dispatch_motion_event();
	int dispatch_cursor_leave();
	int dispatch_translation_event();
	int deactivate_menu();
	int activate_menu();
	int unhighlight();
	void draw_items();

private:
	int draw_title();
// If this menu is pulled down
	int active;
	char text[1024];
	BC_WindowBase *top_level;
// Owner menubar if there is one
	BC_MenuBar *menu_bar;
// Client popup
	BC_MenuPopup *menu_popup;
	int highlighted;
// Dimensions relative to menubar.
	int x, y, w, h; 
};

class BC_MenuItem
{
public:
	BC_MenuItem(char *text, char *hotkey_text = "", int hotkey = 0);
	virtual ~BC_MenuItem();

	friend class BC_MenuPopup;

	int add_submenu(BC_SubMenu *submenu);
	int set_checked(int value);
	int get_checked();
	int set_text(char *text);
	int set_shift(int value = 1);
	int set_alt(int value = 1);

	int deactivate_submenus(BC_MenuPopup *exclude);
	int activate_submenu();
	virtual int handle_event() { return 0; };
	int dispatch_button_press();
	int dispatch_button_release(int &redraw);
	int dispatch_motion_event(int &redraw);
	int dispatch_translation_event();
	int dispatch_cursor_leave();
	int dispatch_key_press();
	int add_item(BC_MenuItem *item);
	int initialize(BC_WindowBase *top_level, BC_MenuBar *menu_bar, BC_MenuPopup *menu_popup);
	int draw();
	char* get_text();
	BC_WindowBase* get_top_level();
	BC_PopupMenu* get_popup_menu();

private:
	BC_WindowBase *top_level;
	BC_MenuBar *menu_bar;
	BC_MenuPopup *menu_popup;
// Submenu if this item owns one.
	BC_SubMenu *submenu;          
// whether the cursor is over or not
	int highlighted;
// whether the cursor is over and the button is down
	int down;
// check box
	int checked;
// title
	char text[256];
// text of hotkey
	char hotkey_text[32];
	int shift_hotkey;
	int alt_hotkey;
// code of hotkey
	int hotkey;
// y position of this item set during menu activation
	int y;
// height of item is set during menu activation
	int h;
};


class BC_MenuPopup
{
public:
	BC_MenuPopup();
	virtual ~BC_MenuPopup();

	friend class BC_MenuItem;

	int initialize(BC_WindowBase *top_level, 
		BC_MenuBar *menu_bar, 
		BC_Menu *menu, 
		BC_MenuItem *menu_item, 
		BC_PopupMenu *popup_menu);
	int add_item(BC_MenuItem *item);
	int remove_item(BC_MenuItem* item = 0);
	int total_menuitems();

// Deactivates all submenus in a downward progression except for the exclude
	int deactivate_submenus(BC_MenuPopup *exclude = 0);
	int dispatch_button_press();
	int dispatch_button_release();
	int dispatch_key_press();
	int dispatch_motion_event();
	int dispatch_cursor_leave();
	int dispatch_translation_event();
	int deactivate_menu();
	int activate_menu(int x, int y, int w, int h, int top_window_coords, int vertical_justify);
	int get_key_x();
	int get_w();
	int draw_items();
	BC_Popup* get_popup();

private:
	int get_dimensions();

	ArrayList<BC_MenuItem *> menu_items;  
	BC_WindowBase *top_level;
	BC_MenuItem *menu_item;
	BC_MenuBar *menu_bar;
	BC_PopupMenu *popup_menu;
	BC_Menu *menu;
// Dimensions relative to root window
	int x, y, w, h; 
// Horizontal position of hotkey text
	int key_x;
// Popup window that only exists when menu is down.
	BC_Popup *popup; 
	int active;
	int type;
};

class BC_SubMenu : public BC_MenuPopup
{
public:
	BC_SubMenu();
	virtual ~BC_SubMenu();

	int add_submenuitem(BC_MenuItem *item);
};



#endif
