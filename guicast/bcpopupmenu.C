
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcmenubar.h"
#include "bcmenuitem.h"
#include "bcmenupopup.h"
#include "bcpixmap.h"
#include "bcpopupmenu.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "colors.h"
#include "fonts.h"
#include <string.h>
#include "vframe.h"

#define BUTTON_UP 0
#define BUTTON_HI 1
#define BUTTON_DN 2
#define TOTAL_IMAGES 3


#define TRIANGLE_W 10
#define TRIANGLE_H 10


BC_PopupMenu::BC_PopupMenu(int x, 
		int y, 
		int w, 
		char *text, 
		int use_title,
		VFrame **data,
		int margin)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	highlighted = popup_down = 0;
	menu_popup = 0;
	icon = 0;
	if(margin >= 0)
		this->margin = margin;
	else
		this->margin = BC_WindowBase::get_resources()->popupmenu_margin;

	this->use_title = use_title;
	strcpy(this->text, text);
	for(int i = 0; i < TOTAL_IMAGES; i++)
	{
		images[i] = 0;
	}
	this->data = data;
	this->w_argument = w;
	status = BUTTON_UP;
}

BC_PopupMenu::BC_PopupMenu(int x, 
		int y, 
		char *text, 
		int use_title,
		VFrame **data)
 : BC_SubWindow(x, y, w, -1, -1)
{
	highlighted = popup_down = 0;
	menu_popup = 0;
	icon = 0;
	this->use_title = use_title;
	strcpy(this->text, text);
	for(int i = 0; i < TOTAL_IMAGES; i++)
	{
		images[i] = 0;
	}
	this->data = data;
	this->w_argument = 0;
	status = BUTTON_UP;
}

BC_PopupMenu::~BC_PopupMenu()
{
	if(menu_popup) delete menu_popup;
	for(int i = 0; i < TOTAL_IMAGES; i++)
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
		if(data)
			set_images(data);
		else
		if(BC_WindowBase::get_resources()->popupmenu_images)
			set_images(BC_WindowBase::get_resources()->popupmenu_images);
		else
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
	BC_Resources *resources = get_resources();
	for(int i = 0; i < 3; i++)
	{
		if(images[i]) delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}

	if(w_argument > 0)
		w = w_argument + 
			margin +
			resources->popupmenu_triangle_margin;
	else
		w = get_text_width(MEDIUMFONT, text) + 
			margin +
			resources->popupmenu_triangle_margin;

	h = images[BUTTON_UP]->get_h();
	return 0;
}

int BC_PopupMenu::calculate_h(VFrame **data)
{
	if(data)
		;
	else
	if(BC_WindowBase::get_resources()->popupmenu_images)
		data = BC_WindowBase::get_resources()->popupmenu_images;
	else
		data = BC_WindowBase::get_resources()->generic_button_images;

	
	return data[BUTTON_UP]->get_h();
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
	BC_Resources *resources = get_resources();

// Background
	draw_top_background(parent_window, 0, 0, w, h);
	draw_3segmenth(0, 0, w, images[status]);

// Overlay text
	set_color(get_resources()->popup_title_text);
	int offset = 0;
	if(status == BUTTON_DN)
		offset = 1;
	if(!icon)
	{
		set_font(MEDIUMFONT);
		BC_WindowBase::draw_center_text(
			(get_w() - margin * 2 - resources->popupmenu_triangle_margin) / 2 + margin + offset, 
			(int)((float)get_h() / 2 + get_text_ascent(MEDIUMFONT) / 2 - 2) + offset, 
			text);
	}

	if(icon)
	{
		draw_pixmap(icon,
			(get_w() - margin * 2 - resources->popupmenu_triangle_margin) / 2 + margin + offset - icon->get_w() / 2 ,
			get_h() / 2 - icon->get_h() / 2 + offset);
	}

	draw_triangle_down_flat(get_w() - margin - resources->popupmenu_triangle_margin, 
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
			x = top_level->get_abs_cursor_x(0) - get_w();
			y = top_level->get_abs_cursor_y(0) - get_h();
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


int BC_PopupMenu::reposition_window(int x, int y)
{
	BC_WindowBase::reposition_window(x, y);
	draw_title();
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

	// Scrolling section
	if (is_event_win() 
		&& (get_buttonpress() == 4 || get_buttonpress() == 5) 
		&& menu_popup->total_menuitems() > 1
	) 
	{ 
		int theval = -1;
		for (int i = 0; i < menu_popup->total_menuitems(); i++) {
			if (!strcmp(menu_popup->menu_items.values[i]->get_text(),get_text())) {
				theval=i; 
				break;
			}
		}

		if (theval == -1)                  theval=0;
		else if (get_buttonpress() == 4)   theval--;
		else if (get_buttonpress() == 5)   theval++;

		if (theval < 0)
			theval=0;
		if (theval >= menu_popup->total_menuitems()) 
			theval = menu_popup->total_menuitems() - 1;

		BC_MenuItem *tmp = menu_popup->menu_items.values[theval];
		set_text(tmp->get_text());
 		if (!tmp->handle_event())
			this->handle_event();
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

	if(is_event_win() && use_title)
	{
		hide_tooltip();
		if(status == BUTTON_DN)
		{
			status = BUTTON_HI;
			draw_title();
		}
	}

	if(popup_down)
	{
// Menu is down so dispatch to popup.
		result = menu_popup->dispatch_button_release();
	}

	if(popup_down && button_releases >= 2)
	{
		deactivate();
	}

	if(!result && use_title && cursor_inside() && is_event_win())
	{
		hide_tooltip();
		result = 1;
	}
	else
	if(!result && !use_title && popup_down && button_releases < 2)
	{
		result = 1;
	}


	if(!result && popup_down)
	{
// Button was released outside any menu.
		deactivate();
		result = 1;
	}

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







