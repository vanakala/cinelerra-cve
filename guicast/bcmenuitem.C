#include "bcmenubar.h"
#include "bcmenuitem.h"
#include "bcmenupopup.h"
#include "bcpopup.h"
#include "bcpopupmenu.h"
#include "bcresources.h"
#include "bcwindowbase.h"
#include "colors.h"

#include <string.h>


// ================================ Menu Item ==================================

BC_MenuItem::BC_MenuItem(char *text, char *hotkey_text, int hotkey)
{
	reset();

	if(text) set_text(text);
	if(hotkey_text) set_hotkey_text(hotkey_text);

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
	if(text) delete [] text;
	if(hotkey_text) delete [] hotkey_text;
	if(submenu) delete submenu;
	if(menu_popup) menu_popup->remove_item(this);
}

void BC_MenuItem::reset()
{
	text = new char[1];
	hotkey_text = new char[1];
	text[0] = 0;
	hotkey_text[0] = 0;
	icon = 0;
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

void BC_MenuItem::set_icon(BC_Pixmap *icon)
{
	this->icon = icon;
}

BC_Pixmap* BC_MenuItem::get_icon()
{
	return icon;
}

void BC_MenuItem::set_text(char *text)
{
	if(this->text) delete [] this->text;
	this->text = new char[strlen(text) + 1];
	strcpy(this->text, text);
}

void BC_MenuItem::set_hotkey_text(char *text)
{
	if(this->hotkey_text) delete [] this->hotkey_text;
	this->hotkey_text = new char[strlen(text) + 1];
	strcpy(this->hotkey_text, text);
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
				menu_popup->get_popup()->set_color(BC_WindowBase::get_resources()->menu_highlighted);
				menu_popup->get_popup()->draw_box(2, y, menu_popup->get_w() - 4, h);
			}
		}

		menu_popup->get_popup()->set_color(BC_WindowBase::get_resources()->menu_item_text);
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
