#include "bcmenubar.h"
#include "bcmenupopup.h"
#include "bcpixmap.h"
#include "bcpopupmenu.h"
#include "bcresources.h"
#include "colors.h"
#include "fonts.h"
#include <string.h>

#define BUTTON_UP 0
#define BUTTON_HI 1
#define BUTTON_DN 2

BC_PopupMenu::BC_PopupMenu(int x, 
		int y, 
		int w, 
		char *text, 
		int use_title)
 : BC_SubWindow(x, y, w, -1, -1)
{
	highlighted = popup_down = 0;
	menu_popup = 0;
	icon = 0;
	this->use_title = use_title;
	strcpy(this->text, text);
	for(int i = 0; i < 9; i++)
	{
		images[i] = 0;
	}
	status = BUTTON_UP;
}

BC_PopupMenu::~BC_PopupMenu()
{
	if(menu_popup) delete menu_popup;
	for(int i = 0; i < 9; i++)
	{
		if(images[i]) delete images[i];
	}
}

char* BC_PopupMenu::get_text()
{
	return text;
}

void BC_PopupMenu::set_text(char *text)
{
	if(use_title)
	{
		strcpy(this->text, text);
		draw_title();
	}
}

void BC_PopupMenu::set_icon(BC_Pixmap *icon)
{
	if(use_title)
	{
		this->icon = icon;
		if(menu_popup) draw_title();
	}
}

int BC_PopupMenu::initialize()
{
	if(use_title)
	{
		set_images(BC_WindowBase::get_resources()->generic_button_images);
	}
	else
// Move outside window if no title
	{
		x = -10;
		y = -10;
		w = 10;
		h = 10;
	}

	BC_SubWindow::initialize();

	menu_popup = new BC_MenuPopup;
	menu_popup->initialize(top_level, 
		0, 
		0, 
		0, 
		this);

	if(use_title) draw_title();

	return 0;
}

int BC_PopupMenu::set_images(VFrame **data)
{
	for(int i = 0; i < 3; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}

	h = images[BUTTON_UP]->get_h();
	return 0;
}

int BC_PopupMenu::add_item(BC_MenuItem *item)
{
	menu_popup->add_item(item);
	return 0;
}

int BC_PopupMenu::remove_item(BC_MenuItem *item)
{
	menu_popup->remove_item(item);
	return 0;
}

int BC_PopupMenu::total_items()
{
	return menu_popup->total_menuitems();
	return 0;
}

BC_MenuItem* BC_PopupMenu::get_item(int i)
{
	return menu_popup->menu_items.values[i];
}

int BC_PopupMenu::draw_title()
{
	if(!use_title) return 0;

// Background
	draw_top_background(parent_window, 0, 0, w, h);
	draw_3segmenth(0, 0, w, images[status]);

// Overlay text
	set_color(BLACK);
	if(!icon)
	{
		set_font(MEDIUMFONT);
		BC_WindowBase::draw_center_text(get_w() / 2, 
			(int)((float)get_h() / 2 + get_text_ascent(MEDIUMFONT) / 2 - 2), 
			text);
	}

#define MARGIN 10
#define TRIANGLE_W 10
#define TRIANGLE_H 10

	if(icon)
	{
		draw_pixmap(icon,
			MARGIN,
			get_h() / 2 - icon->get_h() / 2);
	}

	draw_triangle_down_flat(get_w() - MARGIN - TRIANGLE_W, 
		get_h() / 2 - TRIANGLE_H / 2, 
		TRIANGLE_W, TRIANGLE_H);

	flash();
	return 0;
}

int BC_PopupMenu::deactivate()
{
	if(popup_down)
	{
		top_level->active_popup_menu = 0;
		popup_down = 0;
		menu_popup->deactivate_menu();

		if(use_title) draw_title();    // draw the title
	}
	return 0;
}

int BC_PopupMenu::activate_menu()
{
	if(!popup_down)
	{
		int x = this->x;
		int y = this->y;

		top_level->deactivate();
		top_level->active_popup_menu = this;
		if(!use_title)
		{
			x = top_level->get_abs_cursor_x() - get_w();
			y = top_level->get_abs_cursor_y() - get_h();
			button_press_x = top_level->cursor_x;
			button_press_y = top_level->cursor_y;
		}

		button_releases = 0;
		if(use_title)
		{
			Window tempwin;
			int new_x, new_y, top_w, top_h;
			XTranslateCoordinates(top_level->display, 
				win, 
				top_level->rootwin, 
				0, 
				0, 
				&new_x, 
				&new_y, 
				&tempwin);
			menu_popup->activate_menu(new_x, 
				new_y, 
				w, 
				h, 
				0, 
				1);
		}
		else
			menu_popup->activate_menu(x, y, w, h, 0, 1);
		popup_down = 1;
		if(use_title) draw_title();
	}
	return 0;
}

int BC_PopupMenu::deactivate_menu()
{
	deactivate();
	return 0;
}

int BC_PopupMenu::focus_out_event()
{
	deactivate();
	return 0;
}


int BC_PopupMenu::repeat_event(int64_t duration)
{
	if(duration == top_level->get_resources()->tooltip_delay &&
		tooltip_text[0] != 0 &&
		status == BUTTON_HI &&
		!tooltip_done)
	{
		show_tooltip();
		tooltip_done = 1;
		return 1;
	}
	return 0;
}

int BC_PopupMenu::button_press_event()
{
	int result = 0;

	if(get_buttonpress() == 1 &&
		is_event_win() && 
		use_title)
	{
		top_level->hide_tooltip();
		if(status == BUTTON_HI || status == BUTTON_UP) status = BUTTON_DN;
		activate_menu();
		draw_title();
		return 1;
	}

	if(popup_down)
	{
// Menu is down so dispatch to popup.
		menu_popup->dispatch_button_press();
		return 1;
	}

	return 0;
}

int BC_PopupMenu::button_release_event()
{
// try the title
	int result = 0;

	button_releases++;
//printf("BC_PopupMenu::button_release_event 1 %d\n", result);

	if(is_event_win() && use_title)
	{
		hide_tooltip();
		if(status == BUTTON_DN)
		{
			status = BUTTON_HI;
			draw_title();
		}
	}
//printf("BC_PopupMenu::button_release_event 2 %d\n", result);

	if(popup_down)
	{
// Menu is down so dispatch to popup.
		result = menu_popup->dispatch_button_release();
	}

	if(popup_down && button_releases >= 2)
	{
		deactivate();
	}
//printf("BC_PopupMenu::button_release_event 3 %d\n", result);

	if(!result && use_title && cursor_inside() && is_event_win())
	{
//printf("BC_PopupMenu::button_release_event 4 %d\n", result);
		hide_tooltip();
		result = 1;
	}
	else
	if(!result && !use_title && popup_down && button_releases < 2)
	{
//printf("BC_PopupMenu::button_release_event 5 %d\n", result);
		result = 1;
	}
//printf("BC_PopupMenu::button_release_event 6 %d\n", result);


	if(!result && popup_down)
	{
// Button was released outside any menu.
		deactivate();
		result = 1;
	}
//printf("BC_PopupMenu::button_release_event 5 %d\n", result);

	return result;










	if(popup_down)
	{
// Menu is down so dispatch to popup.
		result = menu_popup->dispatch_button_release();
	}

	if(!result && use_title && cursor_inside() && top_level->event_win == win)
	{
// Inside title
		if(button_releases >= 2)
		{
			highlighted = 1;
			deactivate();
		}
		result = 1;
	}
	else
	if(!result && !use_title && button_releases < 2)
	{
// First release outside a floating menu
// Released outside a fictitious title area
// 		if(top_level->cursor_x < button_press_x - 5 ||
// 			top_level->cursor_y < button_press_y - 5 ||
// 			top_level->cursor_x > button_press_x + 5 ||
// 			top_level->cursor_y > button_press_y + 5)	
			deactivate();
		result = 1;
	}

	return result;
}

int BC_PopupMenu::translation_event()
{
//printf("BC_PopupMenu::translation_event 1\n");
	if(popup_down) menu_popup->dispatch_translation_event();
	return 0;
}

int BC_PopupMenu::cursor_leave_event()
{

	if(status == BUTTON_HI && use_title)
	{
		status = BUTTON_UP;
		draw_title();
		hide_tooltip();
	}

// dispatch to popup
	if(popup_down)
	{
		menu_popup->dispatch_cursor_leave();
	}

	return 0;
}


int BC_PopupMenu::cursor_enter_event()
{
	if(is_event_win() && use_title)
	{
		tooltip_done = 0;
		if(top_level->button_down)
		{
			status = BUTTON_DN;
		}
		else
		if(status == BUTTON_UP) 
			status = BUTTON_HI;
		draw_title();
	}

	return 0;
}

int BC_PopupMenu::cursor_motion_event()
{
	int result = 0;

// This menu is down.
	if(popup_down)
	{
		result = menu_popup->dispatch_motion_event();
	}

	if(!result && use_title && top_level->event_win == win)
	{
		if(highlighted)
		{
			if(cursor_inside())
			{
				highlighted = 0;
				draw_title();
			}
		}
		else
		{
			if(cursor_inside())
			{
				highlighted = 1;
				draw_title();
				result = 1;
			}
		}
	}

	return result;
}

int BC_PopupMenu::drag_start_event()
{
//printf("BC_PopupMenu::drag_start_event %d\n", popup_down);
	if(popup_down) return 1;
	return 0;
}

int BC_PopupMenu::drag_stop_event()
{
	if(popup_down) return 1;
	return 0;
}

int BC_PopupMenu::drag_motion_event()
{
	if(popup_down) return 1;
	return 0;
}







