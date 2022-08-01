// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcdragwindow.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "clip.h"
#include "cursors.h"
#include "fonts.h"
#include "keys.h"
#include "language.h"
#include "bctimer.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>

// ====================================================== scrollbars
BC_ListBoxYScroll::BC_ListBoxYScroll(BC_ListBox *listbox, 
		int total_height,
		int view_height,
		int position)
 : BC_ScrollBar(listbox->get_yscroll_x(), 
	listbox->get_yscroll_y(),
	SCROLL_VERT, 
	listbox->get_yscroll_height(),
	total_height, 
	position, 
	view_height)
{
	this->listbox = listbox;
}

int BC_ListBoxYScroll::handle_event()
{
	listbox->set_yposition(get_value());
	return 1;
}


BC_ListBoxXScroll::BC_ListBoxXScroll(BC_ListBox *listbox, 
		int total_width,
		int view_width,
		int position)
 : BC_ScrollBar(listbox->get_xscroll_x(), 
	listbox->get_xscroll_y(),
	SCROLL_HORIZ, 
	listbox->get_xscroll_width(),
	total_width, 
	position, 
	view_width)
{
	this->listbox = listbox;
}

int BC_ListBoxXScroll::handle_event()
{
	listbox->set_xposition(get_value());
	return 1;
}


BC_ListBoxToggle::BC_ListBoxToggle(BC_ListBox *listbox, 
	BC_ListBoxItem *item, 
	int x, 
	int y)
{
	this->listbox = listbox;
	this->item = item;
	this->x = x;
	this->y = y;
	this->value = item->get_expand();
	if(this->value) 
		state = BC_Toggle::TOGGLE_CHECKED;
	else
		state = BC_Toggle::TOGGLE_UP;
}

void BC_ListBoxToggle::update(BC_ListBoxItem *item, 
	int x, 
	int y,
	int flash)
{
	this->value = item->get_expand();
	this->item = item;
	this->x = x;
	this->y = y;

// update state
	switch(state)
	{
	case TOGGLE_UP:
		if(value)
			state = TOGGLE_CHECKED;
		break;

	case TOGGLE_UPHI:
		if(value)
			state = TOGGLE_CHECKEDHI;
		break;

	case TOGGLE_CHECKED:
		if(!value)
			state = TOGGLE_UP;
		break;

	case TOGGLE_DOWN:
		break;

	case TOGGLE_CHECKEDHI:
		if(!value)
			state = TOGGLE_UPHI;
		break;

	case TOGGLE_DOWN_EXIT:
		break;
	}
	draw(flash);
}

int BC_ListBoxToggle::cursor_motion_event(int *redraw_toggles)
{
	int w = listbox->toggle_images[0]->get_w();
	int h = listbox->toggle_images[0]->get_h();
	int cursor_inside = listbox->get_cursor_x() >= x && 
		listbox->get_cursor_x() < x + w &&
		listbox->get_cursor_y() >= y && 
		listbox->get_cursor_y() < y + h;
	int result = 0;

	switch(state)
	{
	case BC_ListBoxToggle::TOGGLE_UPHI:
		if(!cursor_inside)
		{
			state = BC_ListBoxToggle::TOGGLE_UP;
			*redraw_toggles = 1;
		}
		break;

	case BC_ListBoxToggle::TOGGLE_CHECKEDHI:
		if(!cursor_inside)
		{
			state = BC_ListBoxToggle::TOGGLE_CHECKED;
			*redraw_toggles = 1;
		}
		break;

	case BC_ListBoxToggle::TOGGLE_DOWN:
		if(!cursor_inside)
		{
			state = BC_ListBoxToggle::TOGGLE_DOWN_EXIT;
			*redraw_toggles = 1;
		}
		result = 1;
		break;

	case BC_ListBoxToggle::TOGGLE_DOWN_EXIT:
		if(cursor_inside)
		{
			state = BC_ListBoxToggle::TOGGLE_DOWN;
			*redraw_toggles = 1;
		}
		result = 1;
		break;

	default:
		if(cursor_inside)
		{
			if(value)
				state = BC_ListBoxToggle::TOGGLE_CHECKEDHI;
			else
				state = BC_ListBoxToggle::TOGGLE_UPHI;
			*redraw_toggles = 1;
		}
		break;
	}
	return result;
}

void BC_ListBoxToggle::cursor_leave_event(int *redraw_toggles)
{
	if(value)
		state = BC_ListBoxToggle::TOGGLE_CHECKED;
	else
		state = BC_ListBoxToggle::TOGGLE_UP;
}

int BC_ListBoxToggle::button_press_event()
{
	int w = listbox->toggle_images[0]->get_w();
	int h = listbox->toggle_images[0]->get_h();

	if(listbox->gui->get_cursor_x() >= x && 
		listbox->gui->get_cursor_x() < x + w &&
		listbox->gui->get_cursor_y() >= y && 
		listbox->gui->get_cursor_y() < y + h)
	{
		state = BC_ListBoxToggle::TOGGLE_DOWN;
		return 1;
	}
	return 0;
}

int BC_ListBoxToggle::button_release_event(int *redraw_toggles)
{
	int result = 0;
	switch(state)
	{
	case BC_ListBoxToggle::TOGGLE_DOWN:
		value = !value;
		if(value)
			state = BC_ListBoxToggle::TOGGLE_CHECKEDHI;
		else
			state = BC_ListBoxToggle::TOGGLE_UPHI;
		listbox->expand_item(item, value);
		result = 1;
		break;

	case BC_ListBoxToggle::TOGGLE_DOWN_EXIT:
		if(value)
			state = BC_ListBoxToggle::TOGGLE_CHECKED;
		else
			state = BC_ListBoxToggle::TOGGLE_UP;
		*redraw_toggles = 1;
		result = 1;
		break;
	}
	return result;
}

void BC_ListBoxToggle::draw(int flash)
{
	if(listbox->gui)
	{
		int image_number = 0;
		int w = listbox->toggle_images[0]->get_w();
		int h = listbox->toggle_images[0]->get_h();

		switch(state)
		{
		case BC_ListBoxToggle::TOGGLE_UP:
			image_number = 0;
			break;
		case BC_ListBoxToggle::TOGGLE_UPHI:
			image_number = 1;
			break;
		case BC_ListBoxToggle::TOGGLE_CHECKED:
			image_number = 2;
			break;
		case BC_ListBoxToggle::TOGGLE_DOWN:
			image_number = 3;
			break;
		case BC_ListBoxToggle::TOGGLE_CHECKEDHI:
			image_number = 4;
			break;
		case BC_ListBoxToggle::TOGGLE_DOWN_EXIT:
			if(value)
				image_number = 2;
			else
				image_number = 0;
			break;
		}

		listbox->gui->draw_pixmap(listbox->toggle_images[image_number],
			x,
			y);

		if(flash)
			listbox->gui->flash(x, y, w, h);
	}
}


// ====================================================== box
BC_ListBox::BC_ListBox(int x, 
	int y, 
	int w, 
	int h,
	ArrayList<BC_ListBoxItem*> *data,
	int options,
	const char **column_titles,
	int *column_width,
	int columns,
	int yposition)
 : BC_SubWindow(x, y, w, h, -1)
{
	justify = LISTBOX_RIGHT;
	xposition = 0;
	highlighted_item = -1;
	highlighted_title = -1;
	highlighted_division = -1;
	highlighted_ptr = 0;
	xscrollbar = 0;
	yscrollbar = 0;
	current_cursor = ARROW_CURSOR;
	gui = 0;
	view_h = 0;
	view_w = 0;
	title_h = 0;
	active = 0;
	new_value = 0;
	need_xscroll = 0;
	need_yscroll = 0;
	bg_tile = 0;
	drag_popup = 0;
	selection_number1 = -1;
	selection_number2 = -1;
	bg_surface = 0;
	bg_pixmap = 0;

	current_operation = NO_OPERATION;
	button_highlighted = 0;

	disabled = 0;

	list_highlighted = 0;

	allow_drag_scroll = 1;
	process_drag = 1;

	sort_column = -1;
	sort_order = 0;

	allow_drag_column = 0;
	master_column = 0;
	search_column = 0;

	popup_w = w;
	popup_h = h;

	for(int i = 0; i < 3; i++)
		column_bg[i] = 0;

	for(int i = 0; i < 4; i++)
		button_images[i] = 0;

	for(int i = 0; i < 5; i++)
		toggle_images[i] = 0;

	column_sort_up = 0;
	column_sort_dn = 0;

	this->data = data;
	this->columns = columns;
	this->yposition = yposition;
	this->is_popup = options & LISTBOX_POPUP;
	this->display_format = options & LISTBOX_ICONS;
	this->selection_mode = options & LISTBOX_MULTIPLE;
	this->icon_position = options & LISTBOX_ICON_TOP;
	this->allow_drag = options & LISTBOX_DRAG;
	single_row = options & LISTBOX_SROW;
	if(options & LISTBOX_SMALLFONT)
		labelfont = SMALLFONT;
	else
		labelfont = MEDIUMFONT;

	shrink = options & LISTBOX_SHRINK;
	popup_max_h = h;

	this->column_titles = 0;
	this->column_width = 0;

	if((!column_titles && column_width) ||
		(column_titles && !column_width))
	{
		printf("BC_ListBox::BC_ListBox either column_titles or column_widths == NULL but not both.\n");
		abort();
	}
	set_columns(column_titles,
		column_width,
		columns);

	drag_icon_vframe = 0;
	drag_column_icon_vframe = 0;

// reset the search engine
	reset_query();
}

BC_ListBox::~BC_ListBox()
{
	expanders.remove_all_objects();
	if(bg_surface) delete bg_surface;
	if(bg_pixmap) delete bg_pixmap;
	if(xscrollbar) delete xscrollbar;
	if(yscrollbar) delete yscrollbar;
	for(int i = 0; i < 3; i++)
		if(column_bg[i]) delete column_bg[i];
	for(int i = 0; i < 4; i++)
		if(button_images[i]) delete button_images[i];
	for(int i = 0; i < 5; i++)
		if(toggle_images[i]) delete toggle_images[i];
	if(column_sort_up) delete column_sort_up;
	if(column_sort_dn) delete column_sort_dn;

	delete_columns();
	if(drag_popup) delete drag_popup;
}

void BC_ListBox::enable()
{
	disabled = 0;
	draw_button();
}

void BC_ListBox::disable()
{
	disabled = 1;
	draw_button();
}

void BC_ListBox::reset_query()
{
	query[0] = 0;  // reset query
}

int BC_ListBox::evaluate_query(int list_item, char *string)
{
	return(strcmp(string, data[search_column].values[list_item]->text) <= 0 && 
		data[search_column].values[list_item]->searchable);
}

int BC_ListBox::query_list()
{
	if(query[0] == 0) return 0;

	int done = 0;
	int result;
	int selection_changed = 0;
	int prev_selection = -1;
	for(int i = 0; !done && i < data[0].total; i++)
	{
		if(evaluate_query(i, query))
		{
			result = i;
			done = 1;
		}
	}

	if(done)
	{
// Deselect all
		for(int i = 0; i < data[0].total; i++)
		{
			for(int j = 0; j < columns; j++)
			{
				if(data[j].values[i]->selected) prev_selection = i;
				data[j].values[i]->selected = 0;
			}
		}

// Select one
		if(prev_selection != result)
			selection_changed = 1;
		for(int j = 0; j < columns; j++)
		{
			data[j].values[result]->selected = 1;
		}
		center_selection(result);
	}

	return selection_changed;
}

void BC_ListBox::shrink_height()
{
	int new_h;

	if(shrink && data)
	{
		new_h = get_items_height(data, columns) + LISTBOX_MARGIN;
		if(popup_max_h > new_h)
			popup_h = new_h;
	}
}

void BC_ListBox::init_column_width()
{
	if(!column_width && data && top_level)
	{
		int widest = 5, w;
		for(int i = 0; i < data[0].total; i++)
		{
			w = get_text_width(labelfont, data[0].values[i]->get_text()) + 2 * LISTBOX_MARGIN;
			if(w > widest) widest = w;
		}
		default_column_width[0] = widest;
	}
}

void BC_ListBox::initialize()
{
	if(is_popup)
	{
		for(int i = 0; i < 4; i++)
		{
			button_images[i] = new BC_Pixmap(parent_window, 
				resources.listbox_button[i],
				PIXMAP_ALPHA);
		}
		w = button_images[0]->get_w();
		h = button_images[0]->get_h();
		
		gui = 0;
		current_operation = NO_OPERATION;
	}
	else
	{
		gui = this;
		current_operation = NO_OPERATION;
	}

	for(int i = 0; i < 3; i++)
	{
		column_bg[i] = new BC_Pixmap(parent_window,
			resources.listbox_column[i],
			PIXMAP_ALPHA);
	}
	for(int i = 0; i < 5; i++)
	{
		toggle_images[i] = new BC_Pixmap(parent_window,
			resources.listbox_expand[i],
			PIXMAP_ALPHA);
	}

	column_sort_up = new BC_Pixmap(parent_window,
		resources.listbox_up, PIXMAP_ALPHA);
	column_sort_dn = new BC_Pixmap(parent_window,
		resources.listbox_dn, PIXMAP_ALPHA);

	drag_icon_vframe = resources.type_to_icon[ICON_UNKNOWN];
	drag_column_icon_vframe = resources.type_to_icon[ICON_COLUMN];

	BC_SubWindow::initialize();

	init_column_width();
	shrink_height();

	if(resources.listbox_bg)
		bg_pixmap = new BC_Pixmap(this, 
			resources.listbox_bg,
			PIXMAP_OPAQUE);

	draw_button();
	draw_items(1);
}

void BC_ListBox::deactivate_selection()
{
	current_operation = NO_OPERATION;
}

void BC_ListBox::draw_button()
{
// Draw the button for a popup listbox
	if(is_popup)
	{
		int image_number = 0;
		top_level->lock_window("BC_ListBox::draw_button");
		draw_top_background(parent_window, 0, 0, w, h);

		if(button_highlighted)
			image_number = 1;
		if(current_operation == BUTTON_DN)
			image_number = 2;
		if(disabled)
			image_number = 3;

		pixmap->draw_pixmap(button_images[image_number], 
			0, 
			0,
			w,
			h,
			0,
			0);
		flash();
		top_level->unlock_window();
	}
}

void BC_ListBox::calculate_item_coords()
{
	if(!data) return;

	int icon_x = 0;
	int next_icon_x = 0;
	int next_icon_y = 0;
	int next_text_y = 0;

// Change the display_format to get the right item dimensions for both
// text and icons.
	int display_format_temp = display_format;

// Scan the first column for lowest y coord of all text
// and lowest right x and y coord for all icons which aren't auto placable
	calculate_last_coords_recursive(data,
		&icon_x,
		&next_icon_x, 
		&next_icon_y,
		&next_text_y,
		1);

// Reset last column width.  It's recalculated based on text width.

	calculate_item_coords_recursive(data,
		&icon_x,
		&next_icon_x, 
		&next_icon_y,
		&next_text_y,
		1);

	display_format = display_format_temp;
}

void BC_ListBox::calculate_last_coords_recursive(
	ArrayList<BC_ListBoxItem*> *data,
	int *icon_x,
	int *next_icon_x,
	int *next_icon_y,
	int *next_text_y,
	int top_level)
{
	for(int i = 0; i < data[0].total; i++)
	{
		int current_text_y = 0;
		int current_icon_x = 0;
		int current_icon_y = 0;
		BC_ListBoxItem *item = data[0].values[i];

// Get next_text_y
		if(!item->autoplace_text)
		{
// Lowest text coordinate
			display_format = 0;
			current_text_y = item->text_y + 
				get_text_height(labelfont);
			if(current_text_y > *next_text_y)
				*next_text_y = current_text_y;

// Add sublist depth if it is expanded
			if(item->get_sublist() && 
				item->get_columns() &&
				item->get_expand())
			{
				calculate_last_coords_recursive(item->get_sublist(),
					icon_x,
					next_icon_x, 
					next_icon_y,
					next_text_y,
					0);
			}
		}

// Get next_icon coordinate
		if(top_level)
		{
			BC_ListBoxItem *item = data[master_column].values[i];
			if(!item->autoplace_icon)
			{
				display_format = LISTBOX_ICONS;
// Lowest right icon coordinate.
				current_icon_x = item->icon_x;
				if(current_icon_x > *icon_x) *icon_x = current_icon_x;
				if(current_icon_x + get_item_w(item) > *next_icon_x)
					*next_icon_x = current_icon_x + get_item_w(item);

				current_icon_y = item->icon_y + get_item_h(item);
				if(current_icon_y > *next_icon_y) 
					*next_icon_y = current_icon_y;
			}
		}
	}
}

void BC_ListBox::calculate_item_coords_recursive(
	ArrayList<BC_ListBoxItem*> *data,
	int *icon_x,
	int *next_icon_x,
	int *next_icon_y,
	int *next_text_y,
	int top_level)
{
// Set up items which need autoplacement.
// Should fill icons down and then across
	for(int i = 0; i < data[0].total; i++)
	{
// Don't increase y unless the row requires autoplacing.
		int total_autoplaced_columns = 0;

// Set up icons in first column
		if(top_level)
		{
			BC_ListBoxItem *item = data[master_column].values[i];
			if(item->autoplace_icon)
			{
// 1 column only if icons are used
				display_format = LISTBOX_ICONS;
				if(!single_row)
				{
// Test row height
// Start new row.
					if(*next_icon_y + get_item_h(item) >= get_h() && 
						*next_icon_y > 0)
					{
						*icon_x = *next_icon_x;
						*next_icon_y = 0;
					}
				}

				if(*icon_x + get_item_w(item) > *next_icon_x)
					*next_icon_x = *icon_x + get_item_w(item);

				item->set_icon_x(*icon_x);
				item->set_icon_y(*next_icon_y);

				*next_icon_y += get_item_h(item);
			}
		}

// Set up a text row
		int next_text_x = 0;
		for(int j = 0; j < columns; j++)
		{
			BC_ListBoxItem *item = data[j].values[i];
			if(item->autoplace_text)
			{
				display_format = 0;
				item->set_text_x(next_text_x);
				item->set_text_y(*next_text_y);

// Increment position of next column
				if(j < columns - 1)
					next_text_x += (column_width ? 
						column_width[j] : 
						default_column_width[j]);
				else
// Set last column width based on text width
				{
					int new_w = get_item_w(item);

					int *previous_w = (column_width ? 
						&column_width[j] : 
						&default_column_width[j]);
					if(new_w > *previous_w)
						*previous_w = new_w;
				}
				total_autoplaced_columns++;
			}
		}

// Increase the text vertical position
		if(total_autoplaced_columns)
		{
			display_format = 0;
			*next_text_y += get_text_height(labelfont);
		}

// Set up a sublist
		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist() &&
			item->get_columns() &&
			item->get_expand())
		{
			calculate_item_coords_recursive(
				item->get_sublist(),
				icon_x,
				next_icon_x,
				next_icon_y,
				next_text_y,
				0);
		}
	}
}

void BC_ListBox::set_justify(int value)
{
	this->justify = value;
}

void BC_ListBox::set_allow_drag_column(int value)
{
	this->allow_drag_column = value;
}

void BC_ListBox::set_process_drag(int value)
{
	this->process_drag = value;
}

void BC_ListBox::set_master_column(int value, int redraw)
{
	this->master_column = value;
	if(redraw)
	{
		draw_items(1);
	}
}

void BC_ListBox::set_search_column(int value)
{
	this->search_column = value;
}

int BC_ListBox::get_sort_column()
{
	return sort_column;
}

void BC_ListBox::set_sort_column(int value, int redraw)
{
	sort_column = value;
	if(redraw)
	{
		draw_titles(1);
	}
}

int BC_ListBox::get_sort_order()
{
	return sort_order;
}

void BC_ListBox::set_sort_order(int value, int redraw)
{
	sort_order = value;
	if(redraw)
	{
		draw_titles(1);
	}
}

int BC_ListBox::get_display_mode()
{
	return display_format ? LISTBOX_ICONS : 0;
}

int BC_ListBox::get_yposition()
{
	return yposition;
}

int BC_ListBox::get_xposition()
{
	return xposition;
}

int BC_ListBox::get_highlighted_item()
{
	return highlighted_item;
}

int BC_ListBox::get_item_x(BC_ListBoxItem *item)
{
	if(!display_format)
		return item->text_x - xposition + 2;
	else
		return item->icon_x - xposition + 2;
}

int BC_ListBox::get_item_y(BC_ListBoxItem *item)
{
	int result;
	if(!display_format)
		result = item->text_y - yposition + title_h + 2;
	else
		result = item->icon_y - yposition + title_h + 2;
	return result;
}

int BC_ListBox::get_item_w(BC_ListBoxItem *item)
{
	if(display_format)
	{
		int x, y, w, h;
		get_icon_mask(item, x, y, w, h);
		int icon_w = w;
		get_text_mask(item, x, y, w, h);
		int text_w = w;

		if(!icon_position)
			return icon_w + text_w;
		else
			return (icon_w > text_w) ? icon_w : text_w;
	}
	else
	{
		return get_text_width(labelfont, item->text) + 2 * LISTBOX_MARGIN;
	}
}

int BC_ListBox::get_item_h(BC_ListBoxItem *item)
{
	if(display_format)
	{
		int x, y, w, h;
		get_icon_mask(item, x, y, w, h);
		int icon_h = h;
		get_text_mask(item, x, y, w, h);
		int text_h = h;

		if(!icon_position)
			return (icon_h > text_h) ? icon_h : text_h;
		else
			return icon_h + text_h;
	}
	return get_text_height(labelfont);
}


int BC_ListBox::get_icon_w(BC_ListBoxItem *item)
{
	BC_Pixmap *icon = item->icon;
	if(icon) return icon->get_w();
	return 0;
}

int BC_ListBox::get_icon_h(BC_ListBoxItem *item)
{
	BC_Pixmap *icon = item->icon;
	if(icon) return icon->get_h();
	return 0;
}

int BC_ListBox::get_items_width()
{
	int widest = 0;

	if(display_format)
	{
		for(int i = 0; i < columns; i++)
		{
			for(int j = 0; j < data[i].total; j++)
			{
				int x1, x, y, w, h;
				BC_ListBoxItem *item = data[i].values[j];
				x1 = item->icon_x;

				get_icon_mask(item, x, y, w, h);
				if(x1 + w > widest) widest = x1 + w;

				if(display_format && !icon_position)
					x1 += w;

				get_text_mask(item, x, y, w, h);
				if(x1 + w > widest) widest = x1 + w;
			}
		}
	}
	else
	if(!display_format)
	{
		return get_column_offset(columns);
	}
	return widest;
}

int BC_ListBox::get_items_height(ArrayList<BC_ListBoxItem*> *data, 
	int columns,
	int *result)
{
	int temp = 0;
	int top_level = 0;
	int highest = 0;
	if(!result)
	{
		result = &temp;
		top_level = 1;
	}

	for(int j = 0; j < (data ? data[master_column].total : 0); j++)
	{
		int y1, x, y, w, h;
		BC_ListBoxItem *item = data[master_column].values[j];

		if(display_format)
		{
			get_icon_mask(item, x, y, w, h);
			if(y + h + yposition > highest) highest = y + h + yposition;

			get_text_mask(item, x, y, w, h);
			if(y + h + yposition > highest) highest = y + h + yposition;
		}
		else
		{
			get_text_mask(item, x, y, w, h);
			*result += h;

// Descend into sublist
			if(item->get_sublist() &&
				item->get_expand())
			{
				get_items_height(item->get_sublist(), 
					item->get_columns(), 
					result);
			}
		}
	}
	if(!display_format && top_level) 
	{
		highest = LISTBOX_MARGIN + *result;
	}

	return highest;
}

void BC_ListBox::set_yposition(int position, int draw_items)
{
	this->yposition = position;
	if(draw_items)
	{
		this->draw_items(1);
	}
}

void BC_ListBox::set_xposition(int position)
{
	this->xposition = position;
	draw_items(1);
}

void BC_ListBox::expand_item(BC_ListBoxItem *item, int expand)
{
	if(item)
	{
		item->expand = expand;
// Collapse sublists if this is collapsed to make it easier to calculate
// coordinates
		if(item->get_sublist())
			collapse_recursive(item->get_sublist(), master_column);

// Set everything for autoplacement
		set_autoplacement(data, 0, 1);

		draw_items(1);
	}
}

void BC_ListBox::collapse_recursive(ArrayList<BC_ListBoxItem*> *data,
		int master_column)
{
	for(int i = 0; i < data[master_column].total; i++)
	{
		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist() && item->expand)
		{
			item->expand = 0;
			collapse_recursive(item->get_sublist(), master_column);
		}
	}
}

void BC_ListBox::set_autoplacement(ArrayList<BC_ListBoxItem*> *data,
	int do_icons, 
	int do_text)
{
	for(int i = 0; i < data[0].total; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			if(do_icons) data[j].values[i]->autoplace_icon = 1;
			if(do_text) data[j].values[i]->autoplace_text = 1;
		}

		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist())
		{
			set_autoplacement(item->get_sublist(), do_icons, do_text);
		}
	}
}

int BC_ListBox::get_w()
{
	if(is_popup)
		return BCPOPUPLISTBOX_W;
	else
		return popup_w;
}

int BC_ListBox::get_h()
{
	if(is_popup)
		return BCPOPUPLISTBOX_H;
	else
		return popup_h;
}

int BC_ListBox::get_yscroll_x()
{
	if(is_popup)
		return popup_w - resources.vscroll_data[SCROLL_HANDLE_UP]->get_w();
	else
		return get_x() + popup_w -
			resources.vscroll_data[SCROLL_HANDLE_UP]->get_w();
}

int BC_ListBox::get_yscroll_y()
{
	if(is_popup)
		return 0;
	else
		return get_y();
}

int BC_ListBox::get_yscroll_height()
{
	return popup_h - (need_xscroll ? 
		resources.hscroll_data[SCROLL_HANDLE_UP]->get_h() : 0);
}

int BC_ListBox::get_xscroll_x()
{
	if(is_popup)
		return 0;
	else
		return get_x();
}

int BC_ListBox::get_xscroll_y()
{
	if(is_popup)
		return popup_h - 
			resources.hscroll_data[SCROLL_HANDLE_UP]->get_h();
	else
		return get_y() + popup_h -
			resources.hscroll_data[SCROLL_HANDLE_UP]->get_h();
}

int BC_ListBox::get_xscroll_width()
{
	return popup_w - (need_yscroll ? 
		resources.vscroll_data[SCROLL_HANDLE_UP]->get_w() : 0);
}

int BC_ListBox::get_column_offset(int column)
{
	int x = 0;
	while(column > 0)
	{
		x += column_width ? 
			column_width[--column] : 
			default_column_width[--column];
	}
	return x;
}

void BC_ListBox::column_width_boundaries()
{
	if(column_width)
	{
		for(int i = 0; i < columns; i++)
		{
			if(column_width[i] < MIN_COLUMN_WIDTH) column_width[i] = MIN_COLUMN_WIDTH;
		}
	}
	else
	{
		for(int i = 0; i < columns; i++)
		{
			if(default_column_width[i] < MIN_COLUMN_WIDTH) default_column_width[i] = MIN_COLUMN_WIDTH;
		}
	}
}

int BC_ListBox::get_column_width(int column, int clamp_right)
{
	if(column < columns - 1 || !clamp_right)
		return column_width ? 
			column_width[column] : 
			default_column_width[column];
	else
		return popup_w + 
			xposition - 
			get_column_offset(column);
}

void BC_ListBox::get_icon_mask(BC_ListBoxItem *item, 
	int &x, 
	int &y, 
	int &w, 
	int &h)
{
	if(display_format)
	{
		x = get_item_x(item);
		y = get_item_y(item);
		w = get_icon_w(item) + ICON_MARGIN * 2;
		h = get_icon_h(item) + ICON_MARGIN * 2;
	}
	else
	if(!display_format)
	{
		x = y = w = h = 0;
	}
}

void BC_ListBox::get_text_mask(BC_ListBoxItem *item, 
	int &x, 
	int &y, 
	int &w, 
	int &h)
{
	x = get_item_x(item);
	y = get_item_y(item);

	if(display_format)
	{
		if(!icon_position)
		{
			x += get_icon_w(item) + ICON_MARGIN * 2;
			y += get_icon_h(item) - get_text_height(labelfont);
		}
		else
		{
			y += get_icon_h(item) + ICON_MARGIN;
		}

		w = get_text_width(labelfont, item->text) + ICON_MARGIN * 2;
		h = get_text_height(labelfont) + ICON_MARGIN * 2;
	}
	else
	if(!display_format)
	{
		w = get_text_width(labelfont, item->text) + LISTBOX_MARGIN * 2;
		h = get_text_height(labelfont);
	}
}

int BC_ListBox::get_item_highlight(ArrayList<BC_ListBoxItem*> *data, 
	int column, 
	int item)
{
	if(data[column].values[item]->selected)
		return resources.listbox_selected;
	else
	if(highlighted_item >= 0 &&
		highlighted_ptr == data[master_column].values[item])
		return resources.listbox_highlighted;
	else
		return resources.listbox_inactive;
}

int BC_ListBox::get_item_color(ArrayList<BC_ListBoxItem*> *data, 
	int column, 
	int item)
{
	int color = data[column].values[item]->color;
	if(color == -1) color = resources.listbox_text;
	if(get_item_highlight(data, column, item) == color)
		return BLACK;
	else
		return color;
}

int BC_ListBox::get_from_column()
{
	return dragged_title;
}

int BC_ListBox::get_to_column()
{
	return highlighted_title;
}


BC_ListBoxItem* BC_ListBox::get_selection(int column, 
	int selection_number)
{
	return get_selection_recursive(data,
		column,
		selection_number);
}

BC_ListBoxItem* BC_ListBox::get_selection_recursive(
	ArrayList<BC_ListBoxItem*> *data,
	int column,
	int selection_number)
{
	if(!data) return 0;

	for(int i = 0; i < data[master_column].total; i++)
	{
		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->selected)
		{
			selection_number--;
			if(selection_number < 0)
			{
				return data[column].values[i];
			}
		}

		if(item->get_sublist())
		{
			BC_ListBoxItem *result = get_selection_recursive(item->get_sublist(),
				column,
				selection_number);
			if(result) return result;
		}
	}
	return 0;
}


int BC_ListBox::get_selection_number(int column, 
	int selection_number)
{
	return get_selection_number_recursive(data,
		column,
		selection_number);
}

int BC_ListBox::get_selection_number_recursive(
	ArrayList<BC_ListBoxItem*> *data,
	int column,
	int selection_number,
	int *counter)
{
	int temp = -1;
	if(!data) return 0;
	if(!counter) counter = &temp;

	for(int i = 0; i < data[master_column].total; i++)
	{
		(*counter)++;
		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->selected)
		{
			selection_number--;
			if(selection_number < 0)
			{
				return (*counter);
			}
		}
		if(item->get_sublist())
		{
			int result = get_selection_number_recursive(
				item->get_sublist(),
				column,
				selection_number,
				counter);
			if(result >= 0) return result;
		}
	}
	return -1;
}

void BC_ListBox::set_selection_mode(int mode)
{
	this->selection_mode = mode;
}

void BC_ListBox::delete_columns()
{
	if(column_titles)
	{
		for(int i = 0; i < columns; i++)
		{
			delete [] column_titles[i];
		}
		delete [] column_titles;
	}

	if(column_width) delete [] column_width;

	column_titles = 0;
	column_width = 0;
}

// Need to copy titles so EDL can change
void BC_ListBox::set_columns(const char **column_titles, 
	int *column_width, 
	int columns)
{
	if((!column_titles && column_width) ||
		(column_titles && !column_width))
	{
		printf("BC_ListBox::set_columns either column_titles or column_width == NULL but not both.\n");
		return;
	}

	delete_columns();

	if(column_titles)
	{
		this->column_titles = new char*[columns];
		for(int i = 0; i < columns; i++)
		{
			this->column_titles[i] = new char[strlen(column_titles[i]) + 1];
			strcpy(this->column_titles[i], column_titles[i]);
		}
	}

	if(column_width)
	{
		this->column_width = new int[columns];
		for(int i = 0; i < columns; i++)
		{
			this->column_width[i] = column_width[i];
		}
	}

	this->columns = columns;
}

void BC_ListBox::update(ArrayList<BC_ListBoxItem*> *data,
	const char **column_titles,
	int *column_widths,
	int columns,
	int xposition,
	int yposition, 
	int highlighted_number,
	int recalc_positions,
	int draw)
{
	set_columns(column_titles, 
		column_widths, 
		columns);

	this->data = data;

	this->yposition = yposition;
	this->xposition = xposition;
	this->highlighted_item = highlighted_number;
	this->highlighted_ptr = index_to_item(data, highlighted_number, 0);

	if(recalc_positions)
		set_autoplacement(data, 1, 1);

	init_column_width();
	shrink_height();

	if(gui && draw)
	{
		draw_background();
		draw_items(1);
		update_scrollbars();
	}
}

void BC_ListBox::center_selection()
{
	int selection = get_selection_number(0, 0);

	calculate_item_coords();
	center_selection(selection);

	if(gui)
	{
		draw_background();
		draw_items(1);
		update_scrollbars();
	}
}

int BC_ListBox::select_previous(int skip, 
	BC_ListBoxItem *selected_item,
	int *counter,
	ArrayList<BC_ListBoxItem*> *data,
	int *got_first,
	int *got_second)
{
	int top_level = 0;
	if(!selected_item)
		selected_item = get_selection(0, 0);
	int temp = -1;
	if(!counter)
		counter = &temp;
	int temp2 = 0;
	if(!got_first)
	{
		got_first = &temp2;
		top_level = 1;
	}
	int temp3 = 0;
	if(!got_second)
		got_second = &temp3;
	if(!data)
		data = this->data;
	int done = 0;

// Scan backwards to item pointer.  Then count visible items to get 
// destination.  Repeat to get wraparound.
	do
	{
		for(int i = data[master_column].total - 1; i >= 0; i--)
		{
			BC_ListBoxItem *current_item = data[master_column].values[i];
			if(current_item->get_sublist() &&
				current_item->get_expand())
			{
				int result = select_previous(skip, 
					selected_item,
					counter,
					current_item->get_sublist(),
					got_first,
					got_second);
				if(*got_second)
				{
					return result;
				}
			}

			if(*got_first)
			{
				(*counter)++;
				if((*counter) >= skip)
				{
					for(int j = 0; j < columns; j++)
						data[j].values[i]->selected = 1;
					(*got_second) = 1;
					return item_to_index(this->data, current_item);
				}
			}
			else
			{
				if(current_item->selected)
				{
					for(int j = 0; j < columns; j++)
						data[j].values[i]->selected = 0;
					(*got_first) = 1;
					(*counter)++;
				}
			}
		}

// Hit bottom of top level without finding a selected item.
		if(top_level && !(*got_first)) (*got_first) = 1;
	}while(top_level && data[master_column].total);
	return -1;
}

int BC_ListBox::select_next(int skip, 
	BC_ListBoxItem *selected_item,
	int *counter,
	ArrayList<BC_ListBoxItem*> *data,
	int *got_first,
	int *got_second)
{
	int top_level = 0;
	if(!selected_item)
		selected_item = get_selection(0, 0);
	int temp = -1;
	if(!counter)
		counter = &temp;
	int temp2 = 0;
	if(!got_first)
	{
		got_first = &temp2;
		top_level = 1;
	}
	int temp3 = 0;
	if(!got_second)
		got_second = &temp3;
	if(!data)
		data = this->data;
	int done = 0;

// Scan backwards to item pointer.  Then count visible items to get 
// destination.  Repeat to get wraparound.
	do
	{
		for(int i = 0; i < data[master_column].total; i++)
		{
			BC_ListBoxItem *current_item = data[master_column].values[i];
			if(*got_first)
			{
				(*counter)++;
				if((*counter) >= skip)
				{
					for(int j = 0; j < columns; j++)
						data[j].values[i]->selected = 1;
					(*got_second) = 1;
					return item_to_index(this->data, current_item);
				}
			}
			else
			{
				if(current_item->selected)
				{
					for(int j = 0; j < columns; j++)
						data[j].values[i]->selected = 0;
					(*got_first) = 1;
					(*counter)++;
				}
			}

			if(current_item->get_sublist() &&
				current_item->get_expand())
			{
				int result = select_next(skip, 
					selected_item,
					counter,
					current_item->get_sublist(),
					got_first,
					got_second);
				if(*got_second)
				{
					return result;
				}
			}
		}

// Hit bottom of top level without finding a selected item.
		if(top_level && !(*got_first)) (*got_first) = 1;
	}while(top_level && data[master_column].total);
	return -1;
}

void BC_ListBox::clamp_positions()
{
	items_w = get_items_width();
	items_h = get_items_height(data, columns);

	if(yposition < 0) yposition = 0;
	else
	if(yposition > items_h - view_h)
		yposition = items_h - view_h;

	if(yposition < 0) yposition = 0;

	if(xposition < 0) xposition = 0;
	else
	if(xposition >= items_w - view_w)
		xposition = items_w - view_w;

	if(xposition < 0) xposition = 0;
}

int BC_ListBox::center_selection(int selection,
	ArrayList<BC_ListBoxItem*> *data,
	int *counter)
{
	int temp = -1;
	if(!data) data = this->data;
	if(!counter) counter = &temp;

	for(int i = 0; i < data[master_column].total; i++)
	{
		(*counter)++;

// Got it
		BC_ListBoxItem *item = data[master_column].values[i];
		if((*counter) == selection)
		{
			BC_ListBoxItem *top_item = this->data[master_column].values[0];

			if(display_format)
			{
// Icon is out of window
				if(item->icon_y - yposition  > 
					view_h - get_text_height(labelfont) ||
					item->icon_y - yposition < 0)
				{
					yposition = item->icon_y - view_h / 2;
				}

				if(data[master_column].values[selection]->icon_x - xposition > view_w ||
					data[master_column].values[selection]->icon_x - xposition < 0)
				{
					xposition = item->icon_x - view_w / 2;
				}
			}
			else
			if(!display_format)
			{
// Text coordinate is out of window
				if(item->text_y - yposition  > 
					view_h - get_text_height(labelfont) ||
					item->text_y - yposition < 0)
				{
					yposition = item->text_y - 
						top_item->text_y -
						view_h / 2;
				}
			}
			return 1;
		}

// Descend
		if(item->get_sublist())
		{
			int result = center_selection(selection,
				item->get_sublist(),
				counter);
			if(result) return result;
		}
	}
	return 0;
}

void BC_ListBox::update_scrollbars()
{
	int h_needed = items_h = get_items_height(data, columns);
	int w_needed = items_w = get_items_width();

	if(xscrollbar)
	{
		if(xposition != xscrollbar->get_value())
			xscrollbar->update_value(xposition);

		if(w_needed != xscrollbar->get_length() || 
			view_w != xscrollbar->get_handlelength())
			xscrollbar->update_length(w_needed, xposition, view_w);
	}

	if(yscrollbar)
	{
		if(yposition != yscrollbar->get_value())
			yscrollbar->update_value(yposition);

		if(h_needed != yscrollbar->get_length() || view_h != yscrollbar->get_handlelength())
			yscrollbar->update_length(h_needed, yposition, view_h);
	}
}

void BC_ListBox::get_scrollbars()
{
	int h_needed = items_h = get_items_height(data, columns);
	int w_needed = items_w = get_items_width();

	title_h = get_title_h();

	view_h = popup_h - title_h - 4;
	view_w = popup_w - 4;

// Create scrollbars as needed
	for(int i = 0; i < 2; i++)
	{
		if(w_needed > view_w)
		{
			need_xscroll = 1;
			view_h = popup_h - title_h -
				resources.hscroll_data[SCROLL_HANDLE_UP]->get_h() - 4;
		}
		else
		{
			need_xscroll = 0;
		}

		if(h_needed > view_h)
		{
			need_yscroll = 1;
			view_w = popup_w - 
				resources.vscroll_data[SCROLL_HANDLE_UP]->get_w() - 4;
		}
		else
		{
			need_yscroll = 0;
		}
	}

// Update subwindow size
	int new_w = popup_w;
	int new_h = popup_h;
	if(need_xscroll) new_h -= resources.hscroll_data[SCROLL_HANDLE_UP]->get_h();
	if(need_yscroll) new_w -= resources.vscroll_data[SCROLL_HANDLE_UP]->get_w();

	if(!is_popup)
		if(new_w != BC_WindowBase::get_w() || new_h != BC_WindowBase::get_h())
			gui->resize_window(new_w, new_h);

	BC_WindowBase *destination = (is_popup ? gui : parent_window);
	if(need_xscroll)
	{
		if(!xscrollbar)
		{
			destination->add_subwindow(xscrollbar = 
				new BC_ListBoxXScroll(this, 
					w_needed, 
					view_w, 
					xposition));
			xscrollbar->bound_to = this;
		}
		else
		{
			xscrollbar->update_length(w_needed, xposition, view_w);
			xscrollbar->reposition_window(get_xscroll_x(),
				get_xscroll_y(),
				get_xscroll_width());
		}
	}
	else
	{
		if(xscrollbar) delete xscrollbar;
		xscrollbar = 0;
		xposition = 0;
	}

	if(need_yscroll)
	{
		if(!yscrollbar)
		{
			destination->add_subwindow(yscrollbar = 
				new BC_ListBoxYScroll(this, 
					h_needed, 
					view_h, 
					yposition));
			yscrollbar->bound_to = this;
		}
		else
		{
			yscrollbar->update_length(h_needed, yposition, view_h);
			yscrollbar->reposition_window(get_yscroll_x(),
				get_yscroll_y(),
				get_yscroll_height());
		}
	}
	else
	{
		if(yscrollbar) delete yscrollbar;
		yscrollbar = 0;
		yposition = 0;
	}

	if(!bg_surface ||
		view_w + 4 != bg_surface->get_w() ||
		view_h + 4 != bg_surface->get_h())
	{
		if(bg_surface) delete bg_surface;
		bg_surface = new BC_Pixmap(gui, view_w + 4, view_h + 4);
		draw_background();
	}
}

void BC_ListBox::set_drag_scroll(int value)
{
	allow_drag_scroll = value;
}


// Test for scrolling by dragging
int BC_ListBox::test_drag_scroll(int cursor_x, int cursor_y)
{
	int result = 0;
	if(allow_drag_scroll || 
		current_operation == SELECT_RECT)
	{

		int top_boundary = get_title_h();

		if(cursor_y < top_boundary ||
			cursor_y >= view_h + title_h + LISTBOX_BORDER * 2 ||
			cursor_x < LISTBOX_BORDER ||
			cursor_x >= view_w + LISTBOX_BORDER)
		{
			result = 1;
		}
	}
	return result;
}

int BC_ListBox::drag_scroll_event()
{
	int top_boundary = get_title_h();
	int result = 0;

	if(get_cursor_y() < top_boundary)
	{
		yposition -= top_boundary - get_cursor_y();
		result = 1;
	}
	else
	if(get_cursor_y() >= view_h + title_h + 4)
	{
		yposition += get_cursor_y() - (view_h + title_h + 4);
		result = 1;
	}

	if(get_cursor_x() < 2)
	{
		xposition -= 2 - get_cursor_x();
		result = 1;
	}
	else
	if(get_cursor_x() >= view_w + 2)
	{
		xposition += get_cursor_x() - (view_w + 2);
		result = 1;
	}
	if(result) clamp_positions();
	return result;
}

int BC_ListBox::rectangle_scroll_event()
{
	int old_xposition = xposition;
	int old_yposition = yposition;
	int result = drag_scroll_event();

	if(result)
	{
		rect_x1 += old_xposition - xposition;
		rect_y1 += old_yposition - yposition;
		rect_x2 = get_cursor_x();
		rect_y2 = get_cursor_y();

		int x1 = MIN(rect_x1, rect_x2);
		int x2 = MAX(rect_x1, rect_x2);
		int y1 = MIN(rect_y1, rect_y2);
		int y2 = MAX(rect_y1, rect_y2);

		if(select_rectangle(data,
			x1, 
			y1,
			x2, 
			y2))
		{
			selection_changed();
		}

		clamp_positions();
		draw_items(1);
		update_scrollbars();
	}
	return result;
}

int BC_ListBox::select_scroll_event()
{
	int result = drag_scroll_event();

	if(result)
	{
		highlighted_item = selection_number = get_cursor_item(data, 
			get_cursor_x(), 
			get_cursor_y(),
			&highlighted_ptr);
		clamp_positions();
		draw_items(1);
		update_scrollbars();
		selection_changed();
	}
	return result;
}

int BC_ListBox::select_rectangle(ArrayList<BC_ListBoxItem*> *data,
		int x1, 
		int y1,
		int x2, 
		int y2)
{
	int result = 0;
	for(int i = 0; i < data[master_column].total; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			BC_ListBoxItem *item = data[j].values[i];
			if(display_format)
			{
				int icon_x, icon_y, icon_w, icon_h;
				int text_x, text_y, text_w, text_h;
				get_icon_mask(item, icon_x, icon_y, icon_w, icon_h);
				get_text_mask(item, text_x, text_y, text_w, text_h);

				if((x2 >= icon_x && x1 < icon_x + icon_w &&
					y2 >= icon_y && y1 < icon_y + icon_h) ||
					(x2 >= text_x && x1 < text_x + text_w &&
					y2 >= text_y && y1 < text_y + text_h))
				{
					if(!item->selected)
					{
						item->selected = 1;
						result = 1;
					}
				}
				else
				{
					if(item->selected)
					{
						item->selected = 0;
						result = 1;
					}
				}
			}
			else
			{
				if(x2 >= 0 && 
					x1 < (yscrollbar ? 
						gui->get_w() - resources.vscroll_data[SCROLL_HANDLE_UP]->get_w() :
						gui->get_w()) &&
					y2 > 0 && 
					y1 < gui->get_h() &&
					y2 >= get_item_y(item) &&
					y1 < get_item_y(item) + get_item_h(item))
				{
					if(!item->selected)
					{
						item->selected = 1;
						result = 1;
					}
				}
				else
				{
					if(item->selected)
					{
						item->selected = 0;
						result = 1;
					}
				}
			}
		}

		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist() &&
			item->get_expand())
			result |= select_rectangle(item->get_sublist(),
				x1, 
				y1,
				x2, 
				y2);
	}
	return result;
}

int BC_ListBox::reposition_item(ArrayList<BC_ListBoxItem*> *data,
		int selection_number,
		int x,
		int y,
		int *counter)
{
	int temp = -1;
	if(!counter) counter = &temp;

	for(int i = 0; i < data[master_column].total; i++)
	{
		BC_ListBoxItem *item = data[master_column].values[i];
		(*counter)++;
		if((*counter) == selection_number)
		{
			item->icon_x = x;
			item->icon_y = y;
			return 1;
		}
// Not recursive because it's only used for icons
	}
	return 0;
}

void BC_ListBox::move_selection(ArrayList<BC_ListBoxItem*> *dst,
	ArrayList<BC_ListBoxItem*> *src)
{
	for(int i = 0; i < src[master_column].total; i++)
	{
		BC_ListBoxItem *item = src[master_column].values[i];

// Move item to dst
		if(item->selected)
		{
			for(int j = 0; j < columns; j++)
			{
				dst[j].append(src[j].values[i]);
				src[j].remove_number(i);
			}
		}
		else
// Descend into sublist
		if(item->get_sublist())
		{
			move_selection(dst, 
				item->get_sublist());
		}
	}
}

int BC_ListBox::put_selection(ArrayList<BC_ListBoxItem*> *data,
	ArrayList<BC_ListBoxItem*> *src,
	int destination,
	int *counter)
{
	int temp = -1;
	if(!counter) counter = &temp;

	if(destination < 0)
	{
		for(int j = 0; j < columns; j++)
		{
			for(int i = 0; i < src[j].total; i++)
			{
				data[j].append(src[j].values[i]);
			}
		}
		return 1;
	}
	else
	for(int i = 0; i < data[master_column].total; i++)
	{
		(*counter)++;
		if((*counter) == destination)
		{
			for(int j = 0; j < columns; j++)
			{
				for(int k = 0; k < src[j].total; k++)
				{
					data[j].insert(src[j].values[k], destination + k);
				}
			}
			return 1;
		}

		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist())
		{
			if(put_selection(item->get_sublist(),
					src,
					destination,
					counter))
				return 1;
		}
	}
	return 0;
}

int BC_ListBox::item_to_index(ArrayList<BC_ListBoxItem*> *data,
		BC_ListBoxItem *item,
		int *counter)
{
	int temp = -1;
	if(!counter) counter = &temp;

	for(int i = 0; i < data[master_column].total; i++)
	{
		(*counter)++;
		for(int j = 0; j < columns; j++)
		{
			BC_ListBoxItem *new_item = data[j].values[i];

			if(new_item == item)
			{
				return (*counter);
			}
		}

		BC_ListBoxItem *new_item = data[master_column].values[i];
		if(new_item->get_sublist())
		{
			if(item_to_index(new_item->get_sublist(),
				item,
				counter) >= 0)
				return (*counter);
		}
	}

	return -1;
}

BC_ListBoxItem* BC_ListBox::index_to_item(ArrayList<BC_ListBoxItem*> *data,
		int number,
		int column,
		int *counter)
{
	int temp = -1;
	if(!counter) counter = &temp;
	for(int i = 0; i < data[master_column].total; i++)
	{
		(*counter)++;
		if((*counter) == number)
		{
			return data[column].values[i];
		}
		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist())
		{
			BC_ListBoxItem *result = index_to_item(item->get_sublist(),
				number,
				column,
				counter);
			if(result) return result;
		}
	}
	return 0;
}

int BC_ListBox::get_cursor_item(ArrayList<BC_ListBoxItem*> *data,
	int cursor_x, 
	int cursor_y, 
	BC_ListBoxItem **item_return,
	int *counter,
	int expanded)
{
	int temp = -1;
	if(!data) return -1;
	if(!counter) counter = &temp;

// Icons are not treed
	if(display_format)
	{
		for(int j = data[master_column].total - 1; j >= 0; j--)
		{
			int icon_x, icon_y, icon_w, icon_h;
			int text_x, text_y, text_w, text_h;
			BC_ListBoxItem *item = data[master_column].values[j];
			get_icon_mask(item, icon_x, icon_y, icon_w, icon_h);
			get_text_mask(item, text_x, text_y, text_w, text_h);

			if((cursor_x >= icon_x && cursor_x < icon_x + icon_w &&
				cursor_y >= icon_y && cursor_y < icon_y + icon_h) ||
				(cursor_x >= text_x && cursor_x < text_x + text_w &&
				cursor_y >= text_y && cursor_y < text_y + text_h))
			{
				if(item_return) (*item_return) = item;
				return j;
			}
		}
	}
	else
// Text is treed
	if(!display_format)
	{
// Cursor is inside items rectangle
		if(cursor_x >= 0 && 
			cursor_x < (yscrollbar ? 
				gui->get_w() - resources.vscroll_data[SCROLL_HANDLE_UP]->get_w() :
				gui->get_w()) &&
// Only clamp y if we're not in a SELECT operation.
			(current_operation == BC_ListBox::SELECT ||
				(cursor_y > get_title_h() + LISTBOX_BORDER && 
				cursor_y < gui->get_h())))
		{
// Search table for cursor obstruction
			for(int i = 0; i < data[master_column].total; i++)
			{
				BC_ListBoxItem *item = data[master_column].values[i];
				(*counter)++;

// Cursor is inside item on current level
				if(expanded &&
					item->selectable &&
					cursor_y >= get_item_y(item) &&
					cursor_y < get_item_y(item) + get_item_h(item))
				{
					if(item_return) (*item_return) = item;
					return (*counter);
				}

// Descend into sublist
				if(item->get_sublist())
				{
					if(get_cursor_item(item->get_sublist(),
						cursor_x, 
						cursor_y, 
						item_return,
						counter,
						item->get_expand()) >= 0)
						return (*counter);
				}
			}
		}
	}
	return -1;
}

void BC_ListBox::repeat_event(int duration)
{
	switch(current_operation)
	{
// Repeat out of bounds selection
	case SELECT_RECT:
		if(duration == resources.scroll_repeat)
			rectangle_scroll_event();
		break;

	case SELECT:
		if(duration == resources.scroll_repeat)
			select_scroll_event();
		break;

	case NO_OPERATION:
// Show tooltip
		if(button_highlighted &&
			duration == resources.tooltip_delay &&
			tooltip_wtext &&
			is_popup &&
			!tooltip_done)
		{
			show_tooltip();
			tooltip_done = 1;
		}
		break;
	}
}


int BC_ListBox::cursor_enter_event()
{
	int result = 0;

	switch(current_operation)
	{
// Cursor moved over button, pressed, and exited.
	case BUTTON_DOWN_SELECT:
		if(top_level->event_win == win)
		{
			current_operation = BUTTON_DN;
			result = 1;
			button_highlighted = 1;
			draw_button();
		}
		break;

	case NO_OPERATION:
// Cursor entered button
		if(is_popup && top_level->event_win == win)
		{
			button_highlighted = 1;
			result = 1;
			draw_button();
		}
		else
// TODO: Need to get the highlighted column title or item
		if(gui && top_level->event_win == gui->win)
		{
			list_highlighted = 1;
			draw_border(1);
			result = 1;
		}
		break;
	}

	return result;
}

void BC_ListBox::cursor_leave_event()
{
	int redraw_button = 0;
	int redraw_border = 0;
	int redraw_titles = 0;
	int redraw_items = 0;

	if(current_operation == COLUMN_DRAG) return;

// Left button area
	if(button_highlighted)
	{
		button_highlighted = 0;
		hide_tooltip();
		draw_button();
	}

	if(list_highlighted)
	{
		list_highlighted = 0;
		highlighted_item = -1;
		highlighted_ptr = 0;
		highlighted_title = -1;
		int redraw_toggles = 0;
		for(int i = 0; i < expanders.total; i++)
			expanders.values[i]->cursor_leave_event(&redraw_toggles);

		draw_items(1);
	}
}

int BC_ListBox::get_first_selection(ArrayList<BC_ListBoxItem*> *data, int *result)
{
	int temp = -1;
	if(!result) result = &temp;

	for(int i = 0; i < data[master_column].total; i++)
	{
		BC_ListBoxItem *item = data[master_column].values[i];
		(*result)++;
		if(item->selected) return (*result);
		if(item->get_sublist())
		{
			if(get_first_selection(item->get_sublist(), result) >= 0)
				return (*result);
		}
	}
	return -1;
}

int BC_ListBox::get_total_items(ArrayList<BC_ListBoxItem*> *data, 
	int *result,
	int master_column)
{
	int temp = 0;
	if(!result) result = &temp;

	for(int i = 0; i < data[master_column].total; i++)
	{
		(*result)++;
		if(data[master_column].values[i]->get_sublist())
			get_total_items(data[master_column].values[i]->get_sublist(), 
				result,
				master_column);
	}

	return (*result);
}


int BC_ListBox::get_last_selection(ArrayList<BC_ListBoxItem*> *data, 
	int *result)
{
	int temp = -1;
	int top_level = 0;
	if(!result)
	{
		result = &temp;
		top_level = 1;
	}

	for(int i = data[master_column].total - 1; i >= 0; i--)
	{
		BC_ListBoxItem *item = data[master_column].values[i];
		(*result)++;
		if(item->selected)
		{
			if(top_level)
				return get_total_items(data, 0, master_column) - (*result) /* - 1 */;
			else
				return (*result);
		}

		if(item->get_sublist())
		{
			if(get_last_selection(item->get_sublist(), result) >= 0)
			{
				if(top_level)
					return get_total_items(data, 0, master_column) - (*result) /* - 1 */;
				else
					return (*result);
			}
		}
	}
	return -1;
}

void BC_ListBox::select_range(ArrayList<BC_ListBoxItem*> *data,
		int start,
		int end,
		int *current)
{
	int temp = -1;
	if(!current) current = &temp;

	for(int i = 0; i < data[master_column].total; i++)
	{
		(*current)++;
		if((*current) >= start && (*current) < end)
		{
			for(int j = 0; j < columns; j++)
				data[j].values[i]->selected = 1;
		}
		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist())
			select_range(item->get_sublist(),
				start,
				end,
				current);
	}
}


// Fill items between current selection and new selection
int BC_ListBox::expand_selection(int button_press, int selection_number)
{
	int old_selection_start = selection_start;
	int old_selection_end = selection_end;

// Calculate the range to select based on selection_center and selection_number
	if(selection_number < selection_center)
	{
		selection_start = selection_number;
	}
	else
	{
		selection_end = selection_number + 1;
	}

// Recurse through all the items and select the desired range
	select_range(data, selection_start, selection_end);

// Trigger redraw
	return (old_selection_start != selection_start ||
		old_selection_end != selection_end);
}

int BC_ListBox::toggle_item_selection(ArrayList<BC_ListBoxItem*> *data,
	int selection_number,
	int *counter)
{
	int temp = -1;
	if(!counter) counter = &temp;

	for(int i = 0; i < data[master_column].total; i++)
	{
		BC_ListBoxItem *item = data[master_column].values[i];
		(*counter)++;
		if((*counter) == selection_number)
		{
// Get new value for selection
			int selected = !item->selected;
// Set row
			for(int j = 0; j < columns; j++)
				data[j].values[i]->selected = selected;
			return 1;
		}

// Descend into sublist
		if(item->get_sublist())
		{
			if(toggle_item_selection(item->get_sublist(),
				selection_number,
				counter))
				return 1;
		}
	}

	return 0;
}


void BC_ListBox::set_all_selected(ArrayList<BC_ListBoxItem*> *data, int value)
{
	for(int i = 0; i < data[master_column].total; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			BC_ListBoxItem *item = data[j].values[i];
			item->selected = value;
		}
		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist())
		{
			set_all_selected(item->get_sublist(), value);
		}
	}
}

void BC_ListBox::set_selected(ArrayList<BC_ListBoxItem*> *data, 
		int item_number, 
		int value,
		int *counter)
{
	int temp = -1;
	if(!counter) counter = &temp;
	for(int i = 0; i < data[master_column].total && (*counter) != item_number; i++)
	{
		(*counter)++;
		if((*counter) == item_number)
		{
			for(int j = 0; j < columns; j++)
			{
				BC_ListBoxItem *item = data[j].values[i];
				item->selected = value;
			}
			return;
		}

		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist())
		{
			set_selected(item->get_sublist(), 
				item_number, 
				value,
				counter);
		}
	}
}

int BC_ListBox::update_selection(ArrayList<BC_ListBoxItem*> *data, 
	int selection_number,
	int *counter)
{
	int temp = -1;
	int result = 0;
	if(!counter) counter = &temp;

	for(int i = 0; i < data[master_column].total; i++)
	{
		BC_ListBoxItem *item = data[master_column].values[i];
		(*counter)++;
		if((*counter) == selection_number && !item->selected)
		{
			result = 1;
			for(int j = 0; j < columns; j++)
				data[j].values[i]->selected = 1;
		}
		else
		if((*counter) != selection_number && item->selected)
		{
			result = 1;
			for(int j = 0; j < columns; j++)
				data[j].values[i]->selected = 0;
		}
		if(item->get_sublist())
			result |= update_selection(item->get_sublist(), 
				selection_number,
				counter);
	}
	return result;
}

void BC_ListBox::promote_selections(ArrayList<BC_ListBoxItem*> *data,
	int old_value,
	int new_value)
{
	for(int i = 0; i < data[master_column].total; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			BC_ListBoxItem *item = data[j].values[i];
			if(item->selected == old_value) item->selected = new_value;
		}
		BC_ListBoxItem *item = data[master_column].values[i];
		if(item->get_sublist())
			promote_selections(item->get_sublist(), old_value, new_value);
	}
}

void BC_ListBox::focus_out_event()
{
	deactivate();
}

int BC_ListBox::button_press_event()
{
	int result = 0;
	BC_ListBoxItem *current_item = 0;
	int new_cursor;
	int do_selection_change = 0;

	hide_tooltip();

// Pressed in button
	if(is_popup && top_level->event_win == win)
	{
		current_operation = BUTTON_DN;
		draw_button();

// Deploy listbox
		if(!active && !disabled)
		{
			top_level->deactivate();
			activate();
		}

		result = 1;
	}
	else
// Pressed in scrollbar
	if((xscrollbar && top_level->event_win == xscrollbar->win) ||
		(yscrollbar && top_level->event_win == yscrollbar->win))
	{
		result = 0;
	}
	else
// Pressed in items
	if(gui && top_level->event_win == gui->win)
	{
// Activate list items
		if(!active)
		{
			top_level->deactivate();
			activate();
		}

// Wheel mouse pressed
		if(get_buttonpress() == 4)
		{
			if(current_operation == NO_OPERATION)
			{
				current_operation = WHEEL;
				if(yscrollbar)
				{
					set_yposition(yposition - gui->get_h() / 10, 0);
					clamp_positions();
					update_scrollbars();
					highlighted_ptr = 0;
					highlighted_item = get_cursor_item(data,
						top_level->cursor_x, 
						top_level->cursor_y, 
						&highlighted_ptr);
					draw_items(1);
					result = 1;
				}
			}
		}
		else
		if(get_buttonpress() == 5)
		{
			if(current_operation == NO_OPERATION)
			{
				current_operation = WHEEL;
				if(yscrollbar)
				{
					set_yposition(yposition + gui->get_h() / 10, 0);
					clamp_positions();
					update_scrollbars();
					highlighted_ptr = 0;
					highlighted_item = get_cursor_item(data,
						top_level->cursor_x, 
						top_level->cursor_y,
						&highlighted_ptr);
					draw_items(1);
					result = 1;
				}
			}
		}
		else
// Pressed over column title division
		if(test_column_divisions(gui->get_cursor_x(), 
			gui->get_cursor_y(), 
			new_cursor))
		{
			current_operation = DRAG_DIVISION;
			reset_query();
		}
		else
// Pressed in column title
		if(test_column_titles(gui->get_cursor_x(), gui->get_cursor_y()))
		{
			current_operation = COLUMN_DN;
			button_highlighted = 0;
			list_highlighted = 1;
			draw_items(1);
			result = 1;
		}
		else
// Pressed in expander
		if(test_expanders())
		{
			current_operation = EXPAND_DN;
// Need to redraw items because of alpha
			draw_items(1);
			result = 1;
		}
		else
// Pressed over item
		if((selection_number = get_cursor_item(data, 
					gui->get_cursor_x(), 
					gui->get_cursor_y(),
					&current_item)) >= 0)
		{
// Get item button was pressed over
			selection_number2 = selection_number1;
			selection_number1 = selection_number;

			selection_start = -1;
			selection_end = -1;

// Multiple item selection is possible
			if(selection_mode &&
				(ctrl_down() || shift_down()))
			{
// Expand text selection.
// Fill items between selected region and current item.
				if(shift_down() && !display_format)
				{
// Get first item selected
					selection_start = get_first_selection(data);
// Get last item selected
					selection_end = get_last_selection(data);
// Get center of selected region
					if(selection_end > selection_start)
					{
						selection_center = (selection_end + selection_start) >> 1;
					}
					else
					{
						selection_center = selection_number;
					}

// Deselect everything.
					set_all_selected(data, 0);
// Select just the items
					expand_selection(1, selection_number);
					new_value = 1;
				}
				else
// Toggle a single item on or off
				{
					toggle_item_selection(data, selection_number);
					new_value = current_item->selected;
				}
			}
			else
// Select single item
			{
				if(!current_item->selected)
				{
					set_all_selected(data, 0);
					set_selected(data,
						selection_number,
						1);
				}
				new_value = 1;
			}

			current_operation = SELECT;
			highlighted_item = selection_number;
			highlighted_ptr = current_item;
			button_highlighted = 0;
			list_highlighted = 1;
			reset_query();
			draw_items(1);
			do_selection_change = 1;
			result = 1;
		}
		else
		if(data)
// Pressed over nothing.  Start rectangle selection.
		{
			if(get_buttonpress() == 1 && selection_mode)
			{
				if(!shift_down())
				{
// Deselect all and redraw if anything was selected
					if(get_selection_number(0, 0) >= 0)
					{
						set_all_selected(data, 0);
						draw_items(1);
						do_selection_change = 1;
						result = 1;
					}
				}
				else
				{
// Promote selections to protect from a rectangle selection
					promote_selections(data, 1, 2);
				}

// Start rectangle selection
				current_operation = SELECT_RECT;
				rect_x1 = rect_x2 = get_cursor_x();
				rect_y1 = rect_y2 = get_cursor_y();
			}
		}

		reset_query();
	}
	else
	if(is_popup && active)
	{
		deactivate();
		result = 1;
	}

	if(do_selection_change) selection_changed();

	return result;
}

int BC_ListBox::button_release_event()
{
	int result = 0;
	int cursor_x, cursor_y;
	int do_event = 0;
	new_value = 0;

	switch(current_operation)
	{
	case DRAG_DIVISION:
		current_operation = NO_OPERATION;
		result = 1;
		break;

	case WHEEL:
		current_operation = NO_OPERATION;
		result = 1;
		break;

// Release item selection
	case BUTTON_DOWN_SELECT:
	case SELECT:
		unset_repeat(resources.scroll_repeat);
		current_operation = NO_OPERATION;
		translate_coordinates(top_level->event_win,
			gui->win,
			gui->get_cursor_x(),
			gui->get_cursor_y(),
			&cursor_x,
			&cursor_y);

		selection_number1 = 
			selection_number = 
			get_cursor_item(data, cursor_x, cursor_y);

		if(is_popup)
		{
			button_releases++;
			if(selection_number >= 0)
			{
				deactivate();
				do_event = 1;
			}
			else
// Second button release outside button
			if(button_releases > 1)
			{
				deactivate();
			}
		}
		else
		{
			if(top_level->get_double_click() &&
				selection_number2 == selection_number1 &&
				selection_number2 >= 0 &&
				selection_number1 >= 0)
			{
				do_event = 1;
			}
			result = 1;
		}
		break;

	case SELECT_RECT:
		unset_repeat(resources.scroll_repeat);
		if(data)
		{
// Demote selections from rectangle selection
			promote_selections(data, 2, 1);
		}

// Hide rectangle overlay
		draw_rectangle(1);
		current_operation = NO_OPERATION;
		result = 1;
		break;

// Release popup button
	case BUTTON_DN:
		hide_tooltip();
		current_operation = NO_OPERATION;
		button_releases++;
		draw_button();

// Second button release inside button
		if(button_releases > 1)
		{
			deactivate();
		}
		result = 1;
		break;

	case COLUMN_DN:
		current_operation = NO_OPERATION;
// Update the sort column and the sort order for the user only if the existing
// sort column is valid.
		if(sort_column >= 0)
		{
// Invert order only if column is the same
			if(highlighted_title == sort_column)
				sort_order =
					(sort_order == SORT_ASCENDING) ?
						SORT_DESCENDING :
						SORT_ASCENDING;
// Set the new sort column
			sort_column = highlighted_title;
			if(!sort_order_event())
			{
				draw_titles(1);
			}
		}
		else
// Sorting not enabled.  Redraw the title state.
		{
			draw_titles(1);
		}
		result = 1;
		break;

	case EXPAND_DN:
		{
			int redraw_toggles = 0;
			for(int i = 0; i < expanders.total && !result; i++)
			{
				if(expanders.values[i]->button_release_event(&redraw_toggles))
				{
					result = 1;
				}
			}
// Need to redraw items because of alpha
			if(redraw_toggles) draw_items(1);
			current_operation = NO_OPERATION;
			break;
		}

	default:
// Can't default to NO_OPERATION because it may be used in a drag event.
		break;
	}

	if(do_event) handle_event();

	return result;
}

int BC_ListBox::get_title_h()
{
	if(!display_format)
		return column_titles ? column_bg[0]->get_h() : 0;
	else
		return 0;
}

void BC_ListBox::reset_cursor(int new_cursor)
{
	if(is_popup)
	{
		if(gui->get_cursor() != new_cursor)
		{
			gui->set_cursor(new_cursor);
		}
	}
	else
	if(get_cursor() != new_cursor)
	{
		set_cursor(new_cursor);
	}
}

int BC_ListBox::test_column_divisions(int cursor_x, int cursor_y, int &new_cursor)
{
	if(gui &&
		column_titles && 
		cursor_y >= 0 && 
		cursor_y < get_title_h() &&
		cursor_x >= 0 &&
		cursor_x < gui->get_w())
	{
		for(int i = 1; i < columns; i++)
		{
			if(cursor_x >= -xposition + get_column_offset(i) - 5 &&
				cursor_x <  -xposition + get_column_offset(i) + 
					resources.listbox_title_hotspot)
			{
				highlighted_item = -1;
				highlighted_ptr = 0;
				highlighted_division = i;
				highlighted_title = -1;
				list_highlighted = 1;
				new_cursor = HSEPARATE_CURSOR;
				return 1;
			}
		}
	}
	highlighted_division = -1;
	return 0;
}

int BC_ListBox::test_column_titles(int cursor_x, int cursor_y)
{
	if(gui &&
		column_titles && 
		cursor_y >= 0 && 
		cursor_y < get_title_h() &&
		cursor_x >= 0 && 
		cursor_x < gui->get_w())
	{
		for(int i = 0; i < columns; i++)
		{
			if(cursor_x >= -xposition + get_column_offset(i) &&
				(cursor_x < -xposition + get_column_offset(i + 1) ||
					i == columns - 1))
			{
				highlighted_item = -1;
				highlighted_ptr = 0;
				highlighted_division = -1;
				highlighted_title = i;
				list_highlighted = 1;
				return 1;
			}
		}
	}
	highlighted_title = -1;
	return 0;
}

int BC_ListBox::test_expanders()
{
	for(int i = 0; i < expanders.total; i++)
	{
		if(expanders.values[i]->button_press_event())
		{
			current_operation = EXPAND_DN;
			draw_toggles(1);
			return 1;
		}
	}
	return 0;
}

int BC_ListBox::cursor_motion_event()
{
	int redraw = 0, result = 0;
	int new_cursor = ARROW_CURSOR;

	selection_number = -1;

	switch(current_operation)
	{
	case BUTTON_DN:
// Button pressed and slid off button
		if(!cursor_inside())
		{
			current_operation = BUTTON_DOWN_SELECT;
			draw_button();
			result = 1;
		}
		break;

	case DRAG_DIVISION:
		{
			int new_w = get_cursor_x() + 
				xposition - 
				get_column_offset(highlighted_division - 1);
			new_cursor = HSEPARATE_CURSOR;

			if(column_width)
			{
				column_width[highlighted_division - 1] = new_w;
			}
			else
			{
				default_column_width[highlighted_division - 1] = new_w;
			}

			column_width_boundaries();

// Force update of coords
			set_autoplacement(data, 0, 1);
			column_resize_event();

			clamp_positions();
			draw_items(1);
			update_scrollbars();
			result = 1;
			break;
		}

	case SELECT_RECT:
		{
			if(test_drag_scroll(get_cursor_x(), get_cursor_y()))
			{
				set_repeat(resources.scroll_repeat);
			}

			int old_x1 = MIN(rect_x1, rect_x2);
			int old_x2 = MAX(rect_x1, rect_x2);
			int old_y1 = MIN(rect_y1, rect_y2);
			int old_y2 = MAX(rect_y1, rect_y2);

			int new_rect_x2 = get_cursor_x();
			int new_rect_y2 = get_cursor_y();

			int x1 = MIN(rect_x1, new_rect_x2);
			int x2 = MAX(rect_x1, new_rect_x2);
			int y1 = MIN(rect_y1, new_rect_y2);
			int y2 = MAX(rect_y1, new_rect_y2);

// Adjust rectangle coverage
			if(old_x1 != x1 ||
				old_x2 != x2 ||
				old_y1 != y1 ||
				old_y2 != y2)
			{
				if(data)
				{
					redraw = select_rectangle(data,
						x1,
						y1,
						x2,
						y2);
				}

// hide rectangle
				if(!redraw)
					draw_rectangle(0);
			}

			rect_x2 = get_cursor_x();
			rect_y2 = get_cursor_y();
			if(redraw)
			{
				clamp_positions();
				draw_items(1);
				update_scrollbars();
				selection_changed();
			}
			else
			{
				draw_rectangle(1);
			}
			result = 1;
			break;
		}

	case SELECT:
		{
			int old_highlighted_item = highlighted_item;
			int old_highlighted_title = highlighted_title;
			BC_ListBoxItem *old_highlighted_ptr = highlighted_ptr;

			if(test_drag_scroll(get_cursor_x(), 
					get_cursor_y()))
			{
				set_repeat(resources.scroll_repeat);
			}

			highlighted_item = selection_number = get_cursor_item(data, 
				get_cursor_x(),
				get_cursor_y(),
				&highlighted_ptr);
			result = 1;

// Deselect all items and select just the one we're over
			if(selection_number >= 0 && 
				!allow_drag &&
				((!shift_down() &&
					!ctrl_down()) ||
					!selection_mode))
			{
				redraw = update_selection(data, selection_number);
			}
			else
			if(selection_mode && (shift_down() || ctrl_down()))
// Expand multiple selection
			{
// Expand selected region in text mode centered around initial range
				if(!display_format && shift_down())
				{
// Deselect everything.
					set_all_selected(data, 0);

// Select just the items
					redraw = expand_selection(0, selection_number);
				}
				else
// Set the one item we're over to the selection value determined in
// button_press_event.
				{
					set_selected(data, 
						selection_number, 
						new_value);
				}
			}

			if(highlighted_item != old_highlighted_item)
			{
				clamp_positions();
				draw_items(1);
				update_scrollbars();
				selection_changed();
			}
			break;
		}

	case BUTTON_DOWN_SELECT:
// Went back into button area
		if(cursor_inside())
		{
			current_operation = BUTTON_DN;
			draw_button();
			result = 1;
		}
		else
// Went into item area
		if(gui)
		{
			int cursor_x = 0, cursor_y = 0;
			translate_coordinates(top_level->event_win, 
				gui->win,
				top_level->cursor_x,
				top_level->cursor_y,
				&cursor_x,
				&cursor_y);
			int old_highlighted_item = highlighted_item;
			highlighted_item = selection_number = get_cursor_item(data, 
					cursor_x, 
					cursor_y,
					&highlighted_ptr);

			if(highlighted_item != old_highlighted_item)
			{
				update_selection(data, selection_number);
				draw_items(1);
				selection_changed();
			}
		}
		break;

	case EXPAND_DN:
		{
			int redraw_toggles = 0;
			for(int i = 0; i < expanders.total && !result; i++)
			{
				result = expanders.values[i]->cursor_motion_event(
					&redraw_toggles);
			}
			if(redraw_toggles)
			{
// Need to redraw items because of the alpha
				draw_items(1);
			}
			break;
		}

	case NO_OPERATION:
		{
			int cursor_x = get_cursor_x(), cursor_y = get_cursor_y();

			if(gui && top_level->event_win == gui->win)
			{
				int old_highlighted_title = highlighted_title;
				int old_list_highlighted = list_highlighted;
				int old_highlighted_division = highlighted_division;
				int old_highlighted_item = highlighted_item;
				int redraw_titles = 0;
				int redraw_border = 0;
				int redraw_items = 0;
				int redraw_toggles = 0;
				result = 1;

// Test if cursor moved over a title division
				test_column_divisions(cursor_x, cursor_y, new_cursor);

// Test if cursor moved over a title
				if(highlighted_division < 0)
				{
					test_column_titles(cursor_x, cursor_y);
				}

// Test if cursor moved over expander
				if(highlighted_division < 0 && 
					highlighted_title < 0 &&
					!display_format)
				{
					for(int i = 0; i < expanders.total; i++)
					{
						expanders.values[i]->cursor_motion_event(
							&redraw_toggles);
					}
				}

// Test if cursor moved over an item
				if(highlighted_division < 0 && 
					highlighted_title < 0)
				{
					highlighted_item = get_cursor_item(data,
						cursor_x,
						cursor_y,
						&highlighted_ptr);
				}

// Clear title highlighting if moved over division
				if(old_highlighted_title != highlighted_title)
				{
					redraw_titles = 1;
				}

// Highlight list border
				if(old_list_highlighted != list_highlighted)
				{
					redraw_border = 1;
				}

// Moved out of item area
				if(old_highlighted_item != highlighted_item)
				{
					redraw_items = 1;
				}

// Change cursor to title division adjustment
				reset_cursor(new_cursor);

				if(redraw_items)
				{
					draw_items(0);
				}
				else
				{
					if(redraw_titles)
						draw_titles(0);
					if(redraw_border)
						draw_border(0);
					if(redraw_toggles)
						draw_toggles(0);
				}

				if(redraw_items || redraw_titles ||
						redraw_border || redraw_toggles)
					gui->flash();
			}

			if(!result && list_highlighted)
			{
				list_highlighted = 0;
				highlighted_item = -1;
				highlighted_ptr = 0;
				highlighted_title = -1;
				highlighted_division = -1;
				draw_items(1);
				result = 0;
			}
			break;
		}
	}
	return result;
}

int BC_ListBox::drag_start_event()
{
	int cx, cy;

	BC_Resources::get_abs_cursor(&cx, &cy);

	switch(current_operation)
	{
	case SELECT:
		if(gui && 
			gui->is_event_win() && 
			allow_drag)
		{
			BC_ListBoxItem *item_return = 0;
			selection_number = get_cursor_item(data, 
				top_level->cursor_x, 
				top_level->cursor_y,
				&item_return);

			if(selection_number >= 0)
			{
				if (item_return->icon_vframe)
				{
					drag_popup = new BC_DragWindow(this, 
						item_return->icon_vframe, 
						cx - item_return->icon_vframe->get_w() / 2,
						cy - item_return->icon_vframe->get_h() / 2);
				}
				else
// this probably works not!
				if (item_return->icon)
					drag_popup = new BC_DragWindow(this, 
						item_return->icon, 
						cx - item_return->icon->get_w() / 2,
						cy - item_return->icon->get_h() / 2);
				else
					drag_popup = new BC_DragWindow(this, 
						drag_icon_vframe, 
						cx - drag_icon_vframe->get_w() / 2,
						cy - drag_icon_vframe->get_h() / 2);
				current_operation = DRAG_ITEM;
				return 1;
			}
		}
		break;

	case COLUMN_DN:
		if(gui && gui->is_event_win() && allow_drag_column)
		{
			drag_popup = new BC_DragWindow(this, 
				drag_column_icon_vframe, 
				cx - drag_column_icon_vframe->get_w() / 2,
				cy - drag_column_icon_vframe->get_h() / 2);
			dragged_title = highlighted_title;
			current_operation = COLUMN_DRAG;
			draw_titles(1);
			return 1;
		}
		break;
	}

	return 0;
}

void BC_ListBox::drag_motion_event()
{
	switch(current_operation)
	{
	case DRAG_ITEM:
		{
			int redraw = 0;
			int new_highlighted_item = -1;
			BC_ListBoxItem *new_highlighted_ptr = 0;
			int new_highlight = new_highlighted_item = get_cursor_item(data,
				top_level->cursor_x, 
				top_level->cursor_y,
				&new_highlighted_ptr);

			if(new_highlighted_item != highlighted_item)
			{
				redraw = 1;
			}

// Always update highlighted value for drag_stop
			highlighted_item = new_highlighted_item;
			highlighted_ptr = new_highlighted_ptr;
			if(redraw)
			{
				clamp_positions();
				draw_items(1);
				update_scrollbars();
			}

			drag_popup->cursor_motion_event();
			break;
		}

	case COLUMN_DRAG:
		{
			int old_highlighted_title = highlighted_title;
			test_column_titles(get_cursor_x(), get_cursor_y());
			if(old_highlighted_title != highlighted_title)
			{
				draw_titles(1);
			}
			drag_popup->cursor_motion_event();
			break;
		}
	}
}

void BC_ListBox::drag_stop_event()
{
	switch(current_operation)
	{
	case DRAG_ITEM:
// Inside window boundary
		if(top_level->cursor_x > 0 && 
			top_level->cursor_x < gui->get_w() - drag_popup->get_w() / 2 && 
			top_level->cursor_y > 0 &&
			top_level->cursor_y < gui->get_h() - drag_popup->get_h() / 2)
		{
// Move icon
			if(display_format)
			{
				reposition_item(data, 
					selection_number, 
					top_level->cursor_x + 
						drag_popup->get_offset_x() - 
						LISTBOX_MARGIN - 
						2 + 
						xposition,
					top_level->cursor_y + 
						drag_popup->get_offset_y() - 
						LISTBOX_MARGIN - 
						2 + 
						yposition);
			}
			else
// Move rows
			if(process_drag)
			{
// Get destination
				int destination = highlighted_item = item_to_index(data,
					highlighted_ptr);

// Move selected items from data to temporary
				ArrayList<BC_ListBoxItem*> *src_items = 
					new ArrayList<BC_ListBoxItem*>[columns];

				move_selection(src_items, data);

// Insert items from temporary to data
				put_selection(data,
					src_items,
					destination);

				delete [] src_items;
				set_autoplacement(data, 0, 1);
			}

			draw_items(1);
		}
		else
			drag_popup->drag_failure_event();

			delete drag_popup;
			drag_popup = 0;
			current_operation = NO_OPERATION;
			new_value = 0;
			return;

	case COLUMN_DRAG:
		if(dragged_title != highlighted_title)
		{
			if(highlighted_title >= 0)
			{
				if(!move_column_event()) draw_titles(1);
			}
			else
				drag_popup->drag_failure_event();
		}
		current_operation = NO_OPERATION;
		delete drag_popup;
		drag_popup = 0;
		return;
	}
}

BC_DragWindow* BC_ListBox::get_drag_popup()
{
	return drag_popup;
}

void BC_ListBox::translation_event()
{
	if(is_popup && gui)
	{
		int new_x = gui->get_x() + 
			(top_level->last_translate_x - 
				top_level->prev_x - 
				resources.get_left_border());
		int new_y = gui->get_y() + 
			(top_level->last_translate_y - 
				top_level->prev_y -
				resources.get_top_border());

		gui->reposition_window(new_x, new_y);
	}
}

void BC_ListBox::reposition_window(int x, int y, int w, int h)
{
	if(w != -1)
	{
		if(w != -1) popup_w = w;
		if(h != -1) popup_h = h;

		if(!is_popup)
		{
			if(w != -1) popup_w = w;
			if(h != -1) popup_h = h;
			if(xscrollbar)
				xscrollbar->reposition_window(get_xscroll_x(), 
					get_xscroll_y(), 
					get_xscroll_width());
			if(yscrollbar)
				yscrollbar->reposition_window(get_yscroll_x(), 
					get_yscroll_y(), 
					get_yscroll_height());
		}
	}

	BC_WindowBase::reposition_window(x, y, w, h);
	draw_button();
	draw_items(1);
}

void BC_ListBox::deactivate()
{
	if(active)
	{
		active = 0;
		if(is_popup)
		{
			if(gui) delete gui;
			xscrollbar = 0;
			yscrollbar = 0;
			gui = 0;
			highlighted_item = -1;
			highlighted_ptr = 0;
		}
		top_level->active_subwindow = 0;
	}
}

void BC_ListBox::activate()
{
	if(!active)
	{
		top_level->active_subwindow = this;
		active = 1;
		button_releases = 0;

		if(is_popup)
		{
			Window tempwin;
			int x, y;
			int new_x, new_y;
			y = get_y() + get_h();
			if(justify == LISTBOX_RIGHT)
			{
				x = get_x() - popup_w + get_w();
			}
			else
			{
				x = get_x();
			}
			lock_window("BC_ListBox::activate");
			XTranslateCoordinates(top_level->display, 
				parent_window->win, 
				top_level->rootwin, 
				x, 
				y, 
				&new_x, 
				&new_y, 
				&tempwin);

			if(new_x < 0) new_x = 0;
			if(new_y + popup_h > top_level->get_root_h())
				new_y -= get_h() + popup_h;
// Avoid top going out of screen
			if(new_y < 0)
				new_y = 2;

			add_subwindow(gui = new BC_Popup(this, 
				new_x, 
				new_y, 
				popup_w, 
				popup_h, 
				-1,
				0,
				0));
			draw_items(1);
			unlock_window();
		}
	}
}

int BC_ListBox::keypress_event()
{
	if(!active) return 0;

	int result = 0, redraw = 0, done, view_items = view_h / get_text_height(labelfont);
	int new_item = -1, new_selection = 0;

	switch(top_level->get_keypress())
	{
		case ESC:
		case RETURN:
			top_level->deactivate();
			result = 0;
			break;

		case UP:
			new_selection = new_item = select_previous(0);

			if(new_item >= 0)
			{
				center_selection(new_item);
				redraw = 1;
			}
			result = 1;
			break;

		case DOWN:
			new_selection = new_item = select_next(0);

			if(new_item >= 0)
			{
				center_selection(new_item);
				redraw = 1;
			}
			result = 1;
			break;

		case PGUP:
			new_selection = new_item = select_previous(view_items - 1);

			if(new_item >= 0)
			{
				center_selection(new_item);
				redraw = 1;
			}
			result = 1;
			break;

		case PGDN:
			new_selection = new_item = select_next(view_items - 1);

			if(new_item >= 0)
			{
				center_selection(new_item);
				redraw = 1;
			}
			result = 1;
			break;

		case LEFT:
			xposition -= 10;
			redraw = 1;
			result = 1;
			break;

		case RIGHT:
			xposition += 10;
			redraw = 1;
			result = 1;
			break;

		default:
			if(!ctrl_down())
			{
				if(top_level->get_keypress() > 30 && 
					top_level->get_keypress() < 127)
				{
					int query_len = strlen(query);
					query[query_len++] = top_level->get_keypress();
					query[query_len] = 0;
					new_selection = query_list();
				}
				else
				if(top_level->get_keypress() == BACKSPACE)
				{
					int query_len = strlen(query);
					if(query_len > 0) query[--query_len] = 0;
					new_selection = query_list();
				}

				redraw = 1;
				result = 1;
			}
			break;
	}

	if(redraw)
	{
		clamp_positions();
		draw_items(1);
		update_scrollbars();
	}
	
	if(new_selection >= 0)
	{
		selection_changed();
	}

	return result;
}

BC_Pixmap* BC_ListBox::get_bg_surface()
{
	return bg_surface;
}

void BC_ListBox::draw_background()
{
	top_level->lock_window("BC_ListBox::draw_background");
// White background pixmap
	set_color(resources.listbox_inactive);
	draw_box(0, 0, bg_surface->get_w(), bg_surface->get_h(), bg_surface);

// Optional heroine pixmap
	if(bg_pixmap)
		bg_surface->draw_pixmap(bg_pixmap,
			bg_surface->get_w() - resources.listbox_bg->get_w(),
			0);
	top_level->unlock_window();
}

void BC_ListBox::clear_listbox(int x, int y, int w, int h)
{
	gui->draw_pixmap(bg_surface, 
		x, 
		y, 
		w, 
		h,
		x,
		y - title_h);
}

void BC_ListBox::update_format(int options, int redraw)
{
	int oldfont = labelfont;

	display_format = options & LISTBOX_ICONS;
	if(options & LISTBOX_SMALLFONT)
		labelfont = SMALLFONT;
	else
		labelfont = MEDIUMFONT;
	if(labelfont != oldfont)
		set_autoplacement(data, 1, 1);
	if(redraw)
	{
		if(gui) draw_items(1);
	}
}

int BC_ListBox::get_format()
{
	return display_format | (labelfont == SMALLFONT ? LISTBOX_SMALLFONT : 0);
}

void BC_ListBox::draw_items(int flash)
{
	if(gui)
	{
// Calculate items width 
		calculate_item_coords();

// Create and destroy scrollbars as needed
		get_scrollbars();
		top_level->lock_window("BC_ListBox::draw_items");
// Icon display
		if(display_format)
		{
			clear_listbox(2, 2 + title_h, view_w, view_h);

			set_font(labelfont);
			for(int i = 0; i < data[master_column].total; i++)
			{
				BC_ListBoxItem *item = data[master_column].values[i];
				if(get_item_x(item) >= -get_item_w(item) && 
					get_item_x(item) < view_w &&
					get_item_y(item) >= -get_item_h(item) + title_h &&
					get_item_y(item) < view_h + title_h)
				{
					int item_color = get_item_highlight(data, 0, i);
					int icon_x, icon_y, icon_w, icon_h;
					int text_x, text_y, text_w, text_h;

// Draw highlights
					get_icon_mask(item, icon_x, icon_y, icon_w, icon_h);
					get_text_mask(item, text_x, text_y, text_w, text_h);

					if(item_color != resources.listbox_inactive)
					{
						gui->set_color(BLACK);
						gui->draw_rectangle(icon_x, icon_y, icon_w, icon_h);
						gui->set_color(item_color);
						gui->draw_box(icon_x + 1, icon_y + 1, icon_w - 2, icon_h - 2);
						gui->set_color(BLACK);
						gui->draw_rectangle(text_x, text_y, text_w, text_h);
						gui->set_color(item_color);
						gui->draw_box(text_x + 1, text_y + 1, text_w - 2, text_h - 2);

						if(!icon_position)
							gui->draw_box(text_x - 1, text_y + 1, 2, text_h - 2);
						else
							gui->draw_line(text_x + 1, text_y, text_x + icon_w - 2, text_y);
						if(text_x + text_w < icon_x + icon_w)
						{
							gui->set_color(BLACK);
							gui->draw_line(text_x + text_w, 
								icon_y + icon_h,
								icon_x + icon_w,
								icon_y + icon_h);
						}
					}

// Draw icons
					gui->set_color(get_item_color(data, 0, i));
					if(item->icon)
						gui->pixmap->draw_pixmap(item->icon, 
							icon_x + ICON_MARGIN, 
							icon_y + ICON_MARGIN);
					gui->draw_text(text_x + ICON_MARGIN, 
						text_y + ICON_MARGIN + get_text_ascent(labelfont),
						item->text);
				}
			}
		}
		else
// Text display
		if(!display_format)
		{
// Draw one column at a time so text overruns don't go into the next column
// clear column backgrounds
			int current_toggle = 0;
			for(int j = 0; j < columns; j++)
			{
				clear_listbox(LISTBOX_BORDER + get_column_offset(j) - xposition, 
					LISTBOX_BORDER + title_h, 
					get_column_width(j, 1), 
					view_h);

// Draw rows in the column recursively
				draw_text_recursive(data, j, 0, &current_toggle);
			}

// Delete excess expanders
			while(expanders.total > current_toggle)
			{
				expanders.remove_object();
			}
		}

// Draw titles on top of rows for superposition effect
		draw_titles(0);

// Clear garbage from bottom right corner
		if(xscrollbar && yscrollbar && is_popup)
		{
			gui->draw_top_background(parent_window, 
				popup_w - resources.vscroll_data[SCROLL_HANDLE_UP]->get_w(),
				popup_h - resources.hscroll_data[SCROLL_HANDLE_UP]->get_h(),
				resources.vscroll_data[SCROLL_HANDLE_UP]->get_w(),
				resources.hscroll_data[SCROLL_HANDLE_UP]->get_h());
		}

// Draw borders
		draw_border(0);

		if(current_operation == SELECT_RECT)
			draw_rectangle(0);

		if(flash)
			gui->flash();

		top_level->unlock_window();
	}
}

void BC_ListBox::draw_text_recursive(ArrayList<BC_ListBoxItem*> *data, 
	int column,
	int indent,
	int *current_toggle)
{
	if(!data) return;

	top_level->lock_window("BC_ListBox::draw_text_recursive");
	set_font(labelfont);
	int subindent = 0;

// Search for a branch and make room for toggle if there is one
	if(column == 0)
	{
		for(int i = 0; i < data[column].total; i++)
		{
			if(data[column].values[i]->get_sublist())
			{
				subindent = resources.listbox_expand[0]->get_w();
				break;
			}
		}
	}

	for(int i = 0; i < data[column].total; i++)
	{
// Draw a row
		BC_ListBoxItem *item = data[column].values[i];
		BC_ListBoxItem *first_item = data[master_column].values[i];

		if(get_item_y(item) >= -get_item_h(item) + title_h &&
			get_item_y(item) < view_h + title_h)
		{
			int row_color = get_item_highlight(data, 0, i);
			int x, y, w, h, column_width;

			get_text_mask(item, x, y, w, h);
			column_width = get_column_width(column, 1);
			if(x + column_width > view_w + LISTBOX_BORDER * 2)
				column_width = view_w + LISTBOX_BORDER * 2 - x;

			if(row_color != resources.listbox_inactive)
			{
				gui->set_color(row_color);
				gui->draw_box(x, 
					y, 
					column_width, 
					h);
				gui->set_color(BLACK);
				gui->draw_line(x, 
					y, 
					x + column_width - 1, 
					y);
				gui->draw_line(x, 
					y + get_text_height(labelfont),
					x + column_width - 1, 
					y + get_text_height(labelfont));
			}

			gui->set_color(get_item_color(data, column, i));

// Indent only applies to first column
			gui->draw_text(
				x + 
				LISTBOX_BORDER +
				LISTBOX_MARGIN +
				(column == 0 ? indent + subindent : 0),
				y + get_text_ascent(labelfont),
				item->text);

// Update expander
			if(column == 0 &&
				item->get_sublist() && 
				item->get_columns())
			{
// Create new expander
				if(*current_toggle >= expanders.total)
				{
					BC_ListBoxToggle *toggle = 
						new BC_ListBoxToggle(this, 
							item, 
							x + LISTBOX_BORDER + LISTBOX_MARGIN + indent,
							y);
					toggle->draw(0);
					expanders.append(toggle);
				}
				else
// Reposition existing expander
				{
					BC_ListBoxToggle *toggle = expanders.values[*current_toggle];

					toggle->update(item, 
						x + LISTBOX_BORDER + LISTBOX_MARGIN + indent,
						y,
						0);
				}
				(*current_toggle)++;
			}
		}

// Descend into sublist
		if(first_item->get_expand())
		{
			draw_text_recursive(first_item->get_sublist(), 
				column, 
				indent + LISTBOX_INDENT, 
				current_toggle);
		}
	}
	top_level->unlock_window();
}

void BC_ListBox::draw_border(int flash)
{
	gui->draw_3d_border(0, 
		0, 
		view_w + LISTBOX_BORDER * 2, 
		view_h + title_h + LISTBOX_BORDER * 2, 
		resources.listbox_border1,
		list_highlighted ? 
			resources.listbox_border2_hi :
			resources.listbox_border2,
		list_highlighted ? 
			resources.listbox_border3_hi :
			resources.listbox_border3,
		resources.listbox_border4);

	if(flash)
		gui->flash();
}

void BC_ListBox::draw_titles(int flash)
{
	if(column_titles && !display_format)
	{
		top_level->lock_window("BC_ListBox::draw_titles");
		for(int i = 0; i < columns; i++)
		{
// Column title background
			int image_number = 0;
			if(i == highlighted_title)
			{
				image_number = 1;
				if(current_operation == COLUMN_DN)
					image_number = 2;
			}

			int column_offset = get_column_offset(i) - xposition + LISTBOX_BORDER;
			int column_width = get_column_width(i, 1);
			gui->draw_3segmenth(get_column_offset(i) - xposition + LISTBOX_BORDER,
				LISTBOX_BORDER,
				get_column_width(i, 1),
				column_bg[image_number]);

// Column title sort order
			if(i == sort_column)
			{
				BC_Pixmap *src;
				if(sort_order == SORT_ASCENDING) 
					src = column_sort_dn;
				else
					src = column_sort_up;

				int x = column_offset + 
					column_width - 
					LISTBOX_BORDER;
				if(x > items_w) x = items_w;
				x -= 5 + src->get_w();
				gui->draw_pixmap(src,
					x,
					title_h / 2 - src->get_h() / 2 + LISTBOX_BORDER);
			}

			int x = -xposition + 
				get_column_offset(i) + 
				LISTBOX_MARGIN + 
				LISTBOX_BORDER;
			x += resources.listbox_title_margin;

			gui->set_color(resources.listbox_title_color);
			gui->draw_text(x, 
				LISTBOX_MARGIN + LISTBOX_BORDER + get_text_ascent(labelfont),
				_(column_titles[i]));
		}
		draw_border(0);

		if(flash)
			gui->flash();

		top_level->unlock_window();
	}

}

void BC_ListBox::draw_toggles(int flash)
{
	for(int i = 0; i < expanders.total; i++)
		expanders.values[i]->draw(0);

	if(flash && expanders.total)
		gui->flash();
}

void BC_ListBox::draw_rectangle(int flash)
{
	int x1 = MIN(rect_x1, rect_x2);
	int x2 = MAX(rect_x1, rect_x2);
	int y1 = MIN(rect_y1, rect_y2);
	int y2 = MAX(rect_y1, rect_y2);

	if(x1 == x2 || y1 == y2) return;

	top_level->lock_window("BC_ListBox::draw_rectangle");
	gui->set_inverse();
	gui->set_color(WHITE);
	gui->draw_rectangle(x1, y1, x2 - x1, y2 - y1);
	gui->set_opaque();

	if(flash)
		gui->flash();

	top_level->unlock_window();
}

void BC_ListBox::dump(int indent)
{
	const char *sm, *df, *ip;

	sm = selection_mode ? "multiple" : "single";
	df = display_format ? "icons" : "text";
	ip = icon_position ? "top" : "left";

	printf("%*sBC_ListBox %p dump:\n", indent, "", this);
	printf("%*s size %dx%d columns:%d master:%d popup %d drag %d disabled %d\n", indent, "",
		get_w(), get_h(), columns, master_column, is_popup, allow_drag, disabled);
	printf("%*s selection: %s, format: %s, position: %s\n", indent, "",
		sm, df, ip);

	for(int i = 0; i < data[master_column].total; i++)
	{
		for(int j = 0; j < columns; j++)
			data[j].values[i]->dump(indent + 2);
	}
}
