#include "bcmenubar.h"
#include "bcpopup.h"
#include "bcpopupmenu.h"
#include "bcresources.h"
#include "colors.h"
#include "fonts.h"
#include <string.h>


// ================================== Menu bar ==================================

BC_MenuBar::BC_MenuBar(int x, int y, int w)
 : BC_SubWindow(x, y, w, 0, -1)
{
// Height is really determined by the font in create_tool_objects.
	button_releases = 0;
	active = 0;
}


BC_MenuBar::~BC_MenuBar()
{
// Delete all titles.
	for(int i = 0; i < menu_titles.total; i++) delete menu_titles.values[i];
	menu_titles.remove_all();
}

int BC_MenuBar::initialize()
{
// Initialize dimensions
	h = get_text_height(MEDIUMFONT) + 8;
	bg_color = top_level->get_resources()->menu_up;
	

// Create the subwindow
	BC_SubWindow::initialize();

	if(top_level->get_resources()->menu_bg) 
		set_background(top_level->get_resources()->menu_bg);
	draw_face();
	return 0;
}

void BC_MenuBar::draw_items()
{
//printf("BC_MenuBar::draw_items 1\n");
	for(int i = 0; i < menu_titles.total; i++)
		menu_titles.values[i]->draw_items();
	flush();
}

int BC_MenuBar::add_menu(BC_Menu* menu)
{
	int x, w;

// Get dimensions
	if(menu_titles.total == 0)
		x = 2;
	else
		x = menu_titles.values[menu_titles.total - 1]->x + 
			menu_titles.values[menu_titles.total - 1]->w;

	w = get_text_width(MEDIUMFONT, menu->text) + 20;
// get pointer
	menu_titles.append(menu);
// initialize and draw
	menu->initialize(top_level, this, x, 2, w, get_h() - 4); 
	return 0;
}

int BC_MenuBar::button_press_event()
{
	int result = 0;

	for(int i = 0; i < menu_titles.total && !result; i++)
	{
		result = menu_titles.values[i]->dispatch_button_press();
	}

	return result;
}

int BC_MenuBar::button_release_event()
{
	int result = 0;

	button_down = 0;
	button_releases++;
	for(int i = 0; i < menu_titles.total; i++)
	{
		result += menu_titles.values[i]->dispatch_button_release();
	}

// Button was released outside any menu.
	if(!result)
	{
		deactivate();
	}
//printf("BC_MenuBar::button_release_event %d\n", result);

	return result;
}

int BC_MenuBar::resize_event(int w, int h)
{
	resize_window(w, get_h());
	draw_face();
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->draw_title();
	}
	return 0;
}

int BC_MenuBar::keypress_event()
{
	int result = 0;
	if(!top_level->active_subwindow || !top_level->active_subwindow->uses_text())
	{
		for(int i = 0; i < menu_titles.total && !result; i++)
		{
			result = menu_titles.values[i]->dispatch_keypress();
		}
	}
	return result;
}

int BC_MenuBar::cursor_motion_event()
{
	int result = 0;
	for(int i = 0; i < menu_titles.total && !result; i++)
	{
		result = menu_titles.values[i]->dispatch_motion_event();
	}
	return result;
}

int BC_MenuBar::cursor_leave_event()
{
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->dispatch_cursor_leave();
	}
	return 0;
}

int BC_MenuBar::cursor_enter_event()
{
	if(active) return 1;
	return 0;
}

int BC_MenuBar::translation_event()
{
	if(active)
	{
		for(int i = 0; i < menu_titles.total; i++)
		{
			menu_titles.values[i]->dispatch_translation_event();
		}
	}
	return 0;
}

int BC_MenuBar::activate()
{
	top_level->deactivate();
	top_level->active_menubar = this;
	active = 1;
	return 0;
}

int BC_MenuBar::deactivate()
{
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->deactivate_menu();
	}
	top_level->active_menubar = 0;
	active = 0;
	return 0;
}

int BC_MenuBar::unhighlight()
{
	for(int i = 0; i < menu_titles.total; i++)
	{
		menu_titles.values[i]->unhighlight();
	}
	return 0;
}

int BC_MenuBar::draw_face()
{
	int lx,ly,ux,uy;
	int h, w;
	h = get_h();
	w = get_w();
	h--; 
	w--;

	lx = 1;  ly = 1;
	ux = w - 1;  uy = h - 1;

	set_color(top_level->get_resources()->menu_light);
	draw_line(0, 0, 0, uy);
	draw_line(0, 0, ux, 0);

	set_color(top_level->get_resources()->menu_shadow);
	draw_line(ux, ly, ux, uy);
	draw_line(lx, uy, ux, uy);
	set_color(BLACK);
	draw_line(w, 0, w, h);
	draw_line(0, h, w, h);

	flash();
	return 0;
}

// ==================================== Menu ===================================

BC_Menu::BC_Menu(char *text)
{
	strcpy(this->text, text);
	menu_bar = 0;
	active = 0;
	highlighted = 0;
}

BC_Menu::~BC_Menu()
{
	delete menu_popup;
}

int BC_Menu::initialize(BC_WindowBase *top_level, 
		BC_MenuBar *menu_bar, 
		int x, 
		int y, 
		int w, 
		int h)
{
	this->x = x; 
	this->y = y; 
	this->w = w; 
	this->h = h;
	this->menu_bar = menu_bar;
	this->top_level = top_level;
	menu_popup = new BC_MenuPopup;
	menu_popup->initialize(top_level, menu_bar, this, 0, 0);
	draw_title();
	return 0;
}

int BC_Menu::add_item(BC_MenuItem* menuitem)
{
	menu_popup->add_item(menuitem);
	return 0;
}

int BC_Menu::remove_item(BC_MenuItem *item)
{
	menu_popup->remove_item(item);
	return 0;
}

int BC_Menu::total_menuitems()
{
	return menu_popup->total_menuitems();
}

int BC_Menu::dispatch_button_press()
{
	int result = 0;

// Menu is down so dispatch to popup
	if(active)
	{
		result = menu_popup->dispatch_button_press();
	}

// Try title.
	if(!result)
	{
		if(top_level->event_win == menu_bar->win &&
			top_level->cursor_x >= x && top_level->cursor_x < x + w &&
			top_level->cursor_y >= y && top_level->cursor_y < y + h)
		{
			if(!active)
			{
				menu_bar->deactivate();
				menu_bar->unhighlight();
				menu_bar->button_releases = 0;
				menu_bar->activate();
				activate_menu();
			}
			result = 1;
		}
	}
	return result;
}

int BC_Menu::dispatch_button_release()
{
// try the title
	int result = 0;
	if(top_level->event_win == menu_bar->win &&
		top_level->cursor_x >= x && top_level->cursor_y < x + w &&
		top_level->cursor_y >= y && top_level->cursor_y < y + h)
	{
		if(menu_bar->button_releases >= 2)
		{
			highlighted = 1;
			menu_bar->deactivate();
		}
		result = 1;
	}
	else
// try the popup
		result = menu_popup->dispatch_button_release();
	return result;
}

int BC_Menu::dispatch_keypress()
{
	return menu_popup->dispatch_key_press();
}

int BC_Menu::dispatch_motion_event()
{
	int result = 0;
	int cursor_x, cursor_y;
	Window tempwin;

// try the popup
	if(active)
	{
		result = menu_popup->dispatch_motion_event();
	}

	if(!result)
	{
		top_level->translate_coordinates(top_level->event_win, 
			menu_bar->win,
			top_level->cursor_x,
			top_level->cursor_y,
			&cursor_x,
			&cursor_y);

// change focus from other menu
		if(menu_bar->active && !active &&
			cursor_x >= x && cursor_x < x + w &&
			cursor_y >= y && cursor_y < y + h)
		{
			menu_bar->activate();
			activate_menu();
			result = 1;
		}
		else
// control highlighting
		if(highlighted)
		{
			if(cursor_x < x || cursor_x >= x + w ||
				cursor_y < y || cursor_y >= y + h)
			{
				highlighted = 0;
				draw_title();
			}
		}
		else
		{
			if(cursor_x >= x && cursor_x < x + w &&
				cursor_y >= y && cursor_y < y + h)
			{
				menu_bar->unhighlight();
				highlighted = 1;
				draw_title();
				result = 1;
			}
		}
	}
	return result;
}

int BC_Menu::dispatch_cursor_leave()
{
	if(active)
	{
		menu_popup->dispatch_cursor_leave();
	}
	unhighlight();
	return 0;
}

int BC_Menu::dispatch_translation_event()
{
	if(active)
	{
		menu_popup->dispatch_translation_event();
	}
	return 0;
}

int BC_Menu::activate_menu()
{
	Window tempwin;
	int new_x, new_y, top_w, top_h;
	if(menu_bar)
	{
		XTranslateCoordinates(top_level->display, 
			menu_bar->win, 
			top_level->rootwin, 
			x, 
			y, 
			&new_x, 
			&new_y, 
			&tempwin);
		menu_popup->activate_menu(new_x, new_y, w, h, 0, 1);
	}
	else
		menu_popup->activate_menu(x, y, w, h, 1, 1);

	active = 1;
	draw_title();
	return 0;
}

void BC_Menu::draw_items()
{
	if(active) menu_popup->draw_items();
}

int BC_Menu::set_text(char *text)
{
	strcpy(this->text, text);
	draw_title();
	return 0;
}

int BC_Menu::draw_title()
{
	if(active && menu_popup)
	{
// Menu is pulled down and title is recessed.
		menu_bar->draw_3d_box(x, y, w, h, 
			top_level->get_resources()->menu_shadow, 
			BLACK, 
			top_level->get_resources()->menu_down,
			top_level->get_resources()->menu_down,
			top_level->get_resources()->menu_light);
	}
	else
// Menu is not pulled down.
	{
		if(highlighted)
		{
			menu_bar->set_color(top_level->get_resources()->menu_highlighted);
			menu_bar->draw_box(x, y, w, h);
		}
		else
		{
			menu_bar->draw_background(x, y, w, h);
		}
	
	}

	menu_bar->set_color(BLACK);
	menu_bar->set_font(MEDIUMFONT);
	menu_bar->draw_text(x + 10, h - menu_bar->get_text_descent(MEDIUMFONT), text);
	menu_bar->flash();
	return 0;
}

int BC_Menu::deactivate_menu()
{
	if(active)
	{
		menu_popup->deactivate_menu();
		active = 0;
		draw_title();
	}
	return 0;
}

int BC_Menu::unhighlight()
{
	if(highlighted)
	{
		highlighted = 0;
		draw_title();
	}
	return 0;
}

// ==================================== Menu Popup =============================

// Types of menu popups
#define MENUPOPUP_MENUBAR 0
#define MENUPOPUP_SUBMENU 1
#define MENUPOPUP_POPUP   2

BC_MenuPopup::BC_MenuPopup()
{
}

BC_MenuPopup::~BC_MenuPopup()
{
	while(menu_items.total)
	{
// Each menuitem recursively removes itself from the arraylist
		delete menu_items.values[0];
	}
}

int BC_MenuPopup::initialize(BC_WindowBase *top_level, 
		BC_MenuBar *menu_bar, 
		BC_Menu *menu, 
		BC_MenuItem *menu_item, 
		BC_PopupMenu *popup_menu)
{
	popup = 0;
	active = 0;
	this->menu = menu;
	this->menu_bar = menu_bar;
	this->menu_item = menu_item;
	this->popup_menu = popup_menu;
	this->top_level = top_level;

	if(menu_item) this->type = MENUPOPUP_SUBMENU;
	else
	if(menu) this->type = MENUPOPUP_MENUBAR;
	else
	if(popup_menu) this->type = MENUPOPUP_POPUP;

	return 0;
}

int BC_MenuPopup::add_item(BC_MenuItem *item)
{
	menu_items.append(item);
	item->initialize(top_level, menu_bar, this);
	return 0;
}

int BC_MenuPopup::remove_item(BC_MenuItem *item)
{
	if(!item)
	{
		item = menu_items.values[menu_items.total - 1];
		delete item;
	}
	if(item) menu_items.remove(item);
	return 0;
}

int BC_MenuPopup::total_menuitems()
{
	return menu_items.total;
}

int BC_MenuPopup::dispatch_button_press()
{
	int result = 0;
	if(popup)
	{
		for(int i = 0; i < menu_items.total && !result; i++)
		{
			result = menu_items.values[i]->dispatch_button_press();
		}
		if(result) draw_items();
	}
	return 0;
}

int BC_MenuPopup::dispatch_button_release()
{
	int result = 0, redraw = 0;
	if(popup)
	{
		for(int i = 0; i < menu_items.total && !result; i++)
		{
			result = menu_items.values[i]->dispatch_button_release(redraw);
		}
		if(redraw) draw_items();
	}
	return result;
}

int BC_MenuPopup::dispatch_key_press()
{
	int result = 0;
	for(int i = 0; i < menu_items.total && !result; i++)
	{
		result = menu_items.values[i]->dispatch_key_press();
	}
	return result;
}

int BC_MenuPopup::dispatch_motion_event()
{
	int i, result = 0, redraw = 0;
	Window tempwin;

	if(popup)
	{
// Try submenus and items
		for(i = 0; i < menu_items.total; i++)
		{
			result |= menu_items.values[i]->dispatch_motion_event(redraw);
		}

		if(redraw) draw_items();
	}

	return result;
}

int BC_MenuPopup::dispatch_translation_event()
{
	if(popup)
	{
		int new_x = x + 
			(top_level->last_translate_x - 
			top_level->prev_x - 
			top_level->get_resources()->get_left_border());
		int new_y = y + 
			(top_level->last_translate_y - 
			top_level->prev_y -
			top_level->get_resources()->get_top_border());

// printf("BC_MenuPopup::dispatch_translation_event %d %d %d %d\n", 
// top_level->prev_x, 
// top_level->last_translate_x, 
// top_level->prev_y, 
// top_level->last_translate_y);
		popup->reposition_window(new_x, new_y, popup->get_w(), popup->get_h());
		top_level->flush();
		this->x = new_x;
		this->y = new_y;

		for(int i = 0; i < menu_items.total; i++)
		{
			menu_items.values[i]->dispatch_translation_event();
		}
	}
	return 0;
}


int BC_MenuPopup::dispatch_cursor_leave()
{
	int result = 0;
	
	if(popup)
	{
		for(int i = 0; i < menu_items.total; i++)
		{
			result |= menu_items.values[i]->dispatch_cursor_leave();
		}
		if(result) draw_items();
	}
	return 0;
}

int BC_MenuPopup::activate_menu(int x, 
	int y, 
	int w, 
	int h, 
	int top_window_coords, 
	int vertical_justify)
{
	Window tempwin;
	int new_x, new_y, top_w, top_h;
	top_w = top_level->get_root_w(1);
	top_h = top_level->get_root_h();

	get_dimensions();

// Coords are relative to the main window
	if(top_window_coords)
		XTranslateCoordinates(top_level->display, 
			top_level->win, 
			top_level->rootwin, 
			x, 
			y, 
			&new_x, 
			&new_y, 
			&tempwin);
	else
// Coords are absolute
	{
		new_x = x; 
		new_y = y; 
	}

// All coords are now relative to root window.
	if(vertical_justify)
	{
		this->x = new_x;
		this->y = new_y + h;
		if(this->x + this->w > top_w) this->x -= this->x + this->w - top_w; // Right justify
		if(this->y + this->h > top_h) this->y -= this->h + h; // Bottom justify
	}
	else
	{
		this->x = new_x + w;
		this->y = new_y;
		if(this->x + this->w > top_w) this->x = new_x - this->w;
		if(this->y + this->h > top_h) this->y = new_y + h - this->h;
	}

	active = 1;
	if(menu_bar)
	{
		popup = new BC_Popup(menu_bar, 
					this->x, 
					this->y, 
					this->w, 
					this->h, 
					top_level->get_resources()->menu_up,
					1,
					menu_bar->bg_pixmap);
	}
	else
	{
		popup = new BC_Popup(top_level, 
					this->x, 
					this->y, 
					this->w, 
					this->h, 
					top_level->get_resources()->menu_up,
					1,
					0);
//		popup->set_background(top_level->get_resources()->menu_bg);
	}
	draw_items();
	popup->show_window();
	return 0;
}

int BC_MenuPopup::deactivate_submenus(BC_MenuPopup *exclude)
{
	for(int i = 0; i < menu_items.total; i++)
	{
		menu_items.values[i]->deactivate_submenus(exclude);
	}
	return 0;
}

int BC_MenuPopup::deactivate_menu()
{
	deactivate_submenus(0);

	if(popup) delete popup;
	popup = 0;
	active = 0;

	return 0;
}

int BC_MenuPopup::draw_items()
{
	if(menu_bar)
		popup->draw_top_tiles(menu_bar, 0, 0, w, h);
	else
		popup->draw_top_tiles(popup, 0, 0, w, h);

	popup->draw_3d_border(0, 0, w, h, 
		top_level->get_resources()->menu_light,
		top_level->get_resources()->menu_up,
		top_level->get_resources()->menu_shadow,
		BLACK);

	for(int i = 0; i < menu_items.total; i++)
	{
		menu_items.values[i]->draw();
	}
	popup->flash();
//printf("BC_MenuPopup::draw_items 1\n");
	return 0;
}

int BC_MenuPopup::get_dimensions()
{
	int widest_text = 10, widest_key = 10;
	int text_w, key_w;
	int i = 0;

// pad for border
	h = 2;
// Set up parameters in each item and get total h. 
	for(i = 0; i < menu_items.total; i++)
	{
		text_w = 10 + top_level->get_text_width(MEDIUMFONT, menu_items.values[i]->text);
		if(menu_items.values[i]->checked) text_w += 20;

		key_w = 10 + top_level->get_text_width(MEDIUMFONT, menu_items.values[i]->hotkey_text);
		if(text_w > widest_text) widest_text = text_w;
		if(key_w > widest_key) widest_key = key_w;

		if(!strcmp(menu_items.values[i]->text, "-")) 
			menu_items.values[i]->h = 5;
		else
			menu_items.values[i]->h = top_level->get_text_height(MEDIUMFONT) + 4;

		menu_items.values[i]->y = h;
		menu_items.values[i]->highlighted = 0;
		menu_items.values[i]->down = 0;
		h += menu_items.values[i]->h;
	}
	w = widest_text + widest_key + 10;
// pad for division
	key_x = widest_text + 5;
// pad for border
	h += 2;
	return 0;
}

int BC_MenuPopup::get_key_x()
{
	return key_x;
}

BC_Popup* BC_MenuPopup::get_popup()
{
	return popup;
}

int BC_MenuPopup::get_w()
{
	return w;
}





// ================================ Menu Item ==================================

BC_MenuItem::BC_MenuItem(char *text, char *hotkey_text, int hotkey)
{
	strcpy(this->text, text);
	if(hotkey_text) 
		strcpy(this->hotkey_text, hotkey_text);
	else
		this->hotkey_text[0] = 0;
	this->hotkey = hotkey;
	checked = 0;
	highlighted = 0;
	down = 0;
	submenu = 0;
	shift_hotkey = 0;
	alt_hotkey = 0;
	menu_popup = 0;
}

BC_MenuItem::~BC_MenuItem()
{
	if(submenu) delete submenu;
	if(menu_popup) menu_popup->remove_item(this);
}

int BC_MenuItem::initialize(BC_WindowBase *top_level, BC_MenuBar *menu_bar, BC_MenuPopup *menu_popup)
{
	this->top_level = top_level;
	this->menu_popup = menu_popup;
	this->menu_bar = menu_bar;
	return 0;
}

int BC_MenuItem::set_checked(int value)
{
	this->checked = value;
	return 0;
}

int BC_MenuItem::get_checked()
{
	return checked;
}

int BC_MenuItem::set_text(char *text)
{
	strcpy(this->text, text);
	return 0;
}

int BC_MenuItem::deactivate_submenus(BC_MenuPopup *exclude)
{
	if(submenu && submenu != exclude)
	{
		submenu->deactivate_submenus(exclude);
		submenu->deactivate_menu();
		highlighted = 0;
	}
	return 0;
}

int BC_MenuItem::activate_submenu()
{
	int new_x, new_y;
	if(menu_popup->popup && submenu && !submenu->popup)
	{
		Window tempwin;
		XTranslateCoordinates(top_level->display, 
			menu_popup->get_popup()->win, 
			top_level->rootwin, 
			0, 
			y, 
			&new_x, 
			&new_y, 
			&tempwin);
		submenu->activate_menu(new_x + 5, new_y, menu_popup->w - 10, h, 0, 0);
		highlighted = 1;
	}
	return 0;
}


int BC_MenuItem::dispatch_button_press()
{
	int result = 0;

	if(submenu)
	{
		result = submenu->dispatch_button_press();
	}

	if(!result && top_level->event_win == menu_popup->get_popup()->win)
	{
		if(top_level->cursor_x >= 0 && top_level->cursor_x < menu_popup->get_w() &&
			top_level->cursor_y >= y && top_level->cursor_y < y + h)
		{
			if(!highlighted)
			{
				highlighted = 1;
			}
			result = 1;
		}
		else
		if(highlighted)
		{
			highlighted = 0;
			result = 1;
		}
	}

	return result;
}

int BC_MenuItem::dispatch_button_release(int &redraw)
{
	int result = 0;
	int cursor_x, cursor_y;
	Window tempwin;

	if(!strcmp(text, "-")) return 0;

	if(submenu)
	{
		result = submenu->dispatch_button_release();
	}

	if(!result)
	{
		XTranslateCoordinates(top_level->display, 
			top_level->event_win, 
			menu_popup->get_popup()->win, 
			top_level->cursor_x, 
			top_level->cursor_y, 
			&cursor_x, 
			&cursor_y, 
			&tempwin);

		if(cursor_x >= 0 && cursor_x < menu_popup->get_w() &&
			cursor_y >= y && cursor_y < y + h)
		{
			if(menu_bar)
				menu_bar->deactivate();
			else
				menu_popup->popup_menu->deactivate();

			if(!handle_event() && menu_popup && menu_popup->popup_menu)
			{
				menu_popup->popup_menu->set_text(text);
				menu_popup->popup_menu->handle_event();
			}
			return 1;
		}
	}
	return 0;
}

int BC_MenuItem::dispatch_motion_event(int &redraw)
{
	int result = 0;
	int cursor_x, cursor_y;
	Window tempwin;

	if(submenu)
	{
		result = submenu->dispatch_motion_event();
	}

	top_level->translate_coordinates(top_level->event_win, 
		menu_popup->get_popup()->win,
		top_level->cursor_x,
		top_level->cursor_y,
		&cursor_x,
		&cursor_y);

	if(cursor_x >= 0 && cursor_x < menu_popup->get_w() &&
		cursor_y >= y && cursor_y < y + h)
	{
// Highlight the item
		if(!highlighted)
		{
// Deactivate submenus in the parent menu excluding this one.
			menu_popup->deactivate_submenus(submenu);
			highlighted = 1;
			if(submenu) activate_submenu();
			redraw = 1;
		}
		result = 1;
	}
	else
	if(highlighted)
	{
		highlighted = 0;
		result = 1;
		redraw = 1;
	}
	return result;
}

int BC_MenuItem::dispatch_translation_event()
{
	if(submenu)
		submenu->dispatch_translation_event();
	return 0;
}

int BC_MenuItem::dispatch_cursor_leave()
{
	int result = 0;
	if(submenu)
	{
		result = submenu->dispatch_cursor_leave();
	}

	if(!result && highlighted && top_level->event_win == menu_popup->get_popup()->win)
	{
		highlighted = 0;
		return 1;
	}
	return 0;
}

int BC_MenuItem::dispatch_key_press()
{
	int result = 0;
	if(submenu)
	{
		result = submenu->dispatch_key_press();
	}
	
	if(!result)
	{
// printf("BC_MenuItem::dispatch_key_press 1 %d == %d %d == %d %d == %d\n",
// 	top_level->get_keypress(), hotkey, 
// 	shift_hotkey, top_level->shift_down(),
// 	alt_hotkey, top_level->alt_down());

		if(top_level->get_keypress() == hotkey && 
			shift_hotkey == top_level->shift_down() &&
			alt_hotkey == top_level->alt_down())
		{

// printf("BC_MenuItem::dispatch_key_press 2 %d == %d %d == %d %d == %d\n",
// 	top_level->get_keypress(), hotkey, 
// 	shift_hotkey, top_level->shift_down(),
// 	alt_hotkey, top_level->alt_down());
			result = 1;
			handle_event();
		}
	}
	return result;
}


int BC_MenuItem::draw()
{
	int text_line = top_level->get_text_descent(MEDIUMFONT);
	if(!strcmp(text, "-"))
	{
		menu_popup->get_popup()->set_color(DKGREY);
		menu_popup->get_popup()->draw_line(5, y + h / 2, menu_popup->get_w() - 5, y + h / 2);
		menu_popup->get_popup()->set_color(LTGREY);
		menu_popup->get_popup()->draw_line(5, y + h / 2 + 1, menu_popup->get_w() - 5, y + h / 2 + 1);
	}
	else
	{
		if(highlighted)
		{
			int y = this->y;
			int w = menu_popup->get_w() - 4;
			int h = this->h;
			
			if(top_level->get_button_down() && !submenu)
			{
				menu_popup->get_popup()->draw_3d_box(2, y, menu_popup->get_w() - 4, h, 
					top_level->get_resources()->menu_shadow,
					BLACK,
					top_level->get_resources()->menu_down,
					top_level->get_resources()->menu_down,
					top_level->get_resources()->menu_light);
			}
			else
			{
				menu_popup->get_popup()->set_color(top_level->get_resources()->menu_highlighted);
				menu_popup->get_popup()->draw_box(2, y, menu_popup->get_w() - 4, h);
			}
		}

		menu_popup->get_popup()->set_color(BLACK);
		if(checked)
		{
			menu_popup->get_popup()->draw_check(10, y + 2);
			menu_popup->get_popup()->set_font(MEDIUMFONT);
			menu_popup->get_popup()->draw_text(30, y + h - text_line - 2, text);
			menu_popup->get_popup()->draw_text(menu_popup->get_key_x(), y + h - text_line - 2, hotkey_text);
		}
		else
		{
			menu_popup->get_popup()->set_font(MEDIUMFONT);
			menu_popup->get_popup()->draw_text(10, y + h - text_line - 2, text);
			menu_popup->get_popup()->draw_text(menu_popup->get_key_x(), y + h - text_line - 2, hotkey_text);
		}
	}
	return 0;
}


int BC_MenuItem::add_submenu(BC_SubMenu *submenu)
{
	this->submenu = submenu;
	submenu->initialize(top_level, menu_bar, 0, this, 0);
	return 0;
}

char* BC_MenuItem::get_text()
{
	return text;
}

BC_WindowBase* BC_MenuItem::get_top_level()
{
	return top_level;
}

BC_PopupMenu* BC_MenuItem::get_popup_menu()
{
	return menu_popup->popup_menu;
}

int BC_MenuItem::set_shift(int value)
{
	shift_hotkey = value;
	return 0;
}

int BC_MenuItem::set_alt(int value)
{
	alt_hotkey = value;
	return 0;
}

// ================================= Sub Menu ==================================

BC_SubMenu::BC_SubMenu() : BC_MenuPopup()
{
}

BC_SubMenu::~BC_SubMenu()
{
}

int BC_SubMenu::add_submenuitem(BC_MenuItem *item)
{
	add_item(item);
	return 0;
}




