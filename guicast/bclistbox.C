#include "bcdragwindow.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "clip.h"
#include "cursors.h"
#include "fonts.h"
#include "keys.h"
#include "timer.h"
#include "vframe.h"

#include <string.h>


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

BC_ListBoxYScroll::~BC_ListBoxYScroll()
{
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

BC_ListBoxXScroll::~BC_ListBoxXScroll()
{
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
 : BC_Toggle(x, 
 	y, 
	BC_WindowBase::get_resources()->listbox_expand, 
	item->get_expand())
{
	this->listbox = listbox;
	this->item = item;
}

void BC_ListBoxToggle::update(BC_ListBoxItem *item, int x, int y)
{
	this->item = item;
	if(item && item->get_expand() != get_value())
	{
		BC_Toggle::update(item->get_expand(), 0);
	}
	reposition_window(x, y);
}

int BC_ListBoxToggle::handle_event()
{
	listbox->expand_item(item, get_value());
	return 1;
}















// ====================================================== box

BC_ListBox::BC_ListBox(int x, 
	int y, 
	int w, 
	int h,
	int display_format,
	ArrayList<BC_ListBoxItem*> *data,
	char **column_titles,
	int *column_width,
	int columns,
	int yposition,
	int popup,
	int selection_mode,
	int icon_position,
	int allow_drag)
 : BC_SubWindow(x, y, w, h, -1)
{

	xposition = 0;
	highlighted_item = -1;
	highlighted_ptr = 0;
	highlighted = 0;
	xscrollbar = 0;
	yscrollbar = 0;
	highlighted_division = 0;
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
// GCC 3.0.2 optimization fails if bg_surface is zeroed.
// printf("malloc 5\n");
// unsigned char *test3 = (unsigned char*)malloc(4628);
// printf("malloc 6\n");
 	bg_pixmap = 0;
 	current_operation = BCLISTBOX_NO_OPERATION;
 	allow_drag_scroll = 1;

	popup_w = w;
	popup_h = h;

//printf("BC_ListBox::BC_ListBox 1\n");
	this->data = data;
	this->columns = columns;
	this->yposition = yposition;
	this->popup = popup;
	this->display_format = display_format;
	this->selection_mode = selection_mode;
	this->icon_position = icon_position;
	this->allow_drag = allow_drag;
	this->column_titles = 0;
	this->column_width = 0;
//printf("BC_ListBox::BC_ListBox 1\n");

	if((!column_titles && column_width) ||
		(column_titles && !column_width))
	{
		printf("BC_ListBox::BC_ListBox either column_titles or column_widths == NULL but not both.\n");
	}
//printf("BC_ListBox::BC_ListBox 2 %p %p\n", column_titles, column_width);
	set_columns(column_titles, 
		column_width, 
		columns);

//printf("BC_ListBox::BC_ListBox 3\n");





// reset the search engine
//printf("BC_ListBox::BC_ListBox 4\n");
	reset_query();
//printf("BC_ListBox::BC_ListBox 5\n");
}

BC_ListBox::~BC_ListBox()
{
	expanders.remove_all_objects();
	if(bg_surface) delete bg_surface;
	if(bg_pixmap) delete bg_pixmap;
	if(xscrollbar) delete xscrollbar;
	if(yscrollbar) delete yscrollbar;
	if(popup)
	{
		delete images[0];
		delete images[1];
		delete images[2];
	}
	
	delete_columns();
	delete drag_icon;
}

void BC_ListBox::reset_query()
{
	query[0] = 0;  // reset query
}

int BC_ListBox::evaluate_query(int list_item, char *string)
{
	return(strcmp(string, data[0].values[list_item]->text) <= 0 && 
		data[0].values[list_item]->searchable);
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

void BC_ListBox::init_column_width()
{
	if(!column_width && data)
	{
		int widest = 5, w;
		for(int i = 0; i < data[0].total; i++)
		{
			w = get_text_width(MEDIUMFONT, data[0].values[i]->get_text()) + 2 * LISTBOX_MARGIN;
			if(w > widest) widest = w;
		}
//		if(widest < popup_w - 4) widest = popup_w - 4;
		default_column_width[0] = widest;
	}
}

int BC_ListBox::initialize()
{
	if(popup)
	{
		images[0] = new BC_Pixmap(parent_window, BC_WindowBase::get_resources()->listbox_button[0], PIXMAP_ALPHA);
		images[1] = new BC_Pixmap(parent_window, BC_WindowBase::get_resources()->listbox_button[1], PIXMAP_ALPHA);
		images[2] = new BC_Pixmap(parent_window, BC_WindowBase::get_resources()->listbox_button[2], PIXMAP_ALPHA);
		w = images[0]->get_w();
		h = images[0]->get_h();
		gui = 0;
		status = LISTBOX_UP;
	}
	else
	{
		gui = this;
	}

	drag_icon = new BC_Pixmap(parent_window, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_UNKNOWN], 
		PIXMAP_ALPHA);
	BC_SubWindow::initialize();

	init_column_width();

	if(top_level->get_resources()->listbox_bg)
		bg_pixmap = new BC_Pixmap(this, 
			get_resources()->listbox_bg, 
			PIXMAP_OPAQUE);

	draw_face();
	draw_items();
	if(!popup) gui->flash();
	return 0;
}

void BC_ListBox::deactivate_selection()
{
	current_operation = BCLISTBOX_NO_OPERATION;
}

int BC_ListBox::draw_face()
{
// Draw the button for a popup listbox
	if(popup)
	{
		draw_top_background(parent_window, 0, 0, w, h);
		images[status]->write_drawable(pixmap, 
			0, 
			0,
			w,
			h,
			0,
			0);
		flash();
	}
	return 0;
}

int BC_ListBox::calculate_item_coords()
{
	if(!data) return 0;

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
	int *last_column_w = (column_width ? 
						&column_width[columns - 1] : 
						&default_column_width[columns - 1]);
	if(this->column_titles)
	{
		*last_column_w = 
			get_text_width(MEDIUMFONT, this->column_titles[this->columns - 1]) + 
			2 * 
			LISTBOX_MARGIN;
	}
	else
	{
		*last_column_w = MIN_COLUMN_WIDTH;
	}

	calculate_item_coords_recursive(data,
		&icon_x,
		&next_icon_x, 
		&next_icon_y,
		&next_text_y,
		1);



	display_format = display_format_temp;

	return 0;
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

		if(!item->autoplace_text)
		{
// Lowest text coordinate
			display_format = LISTBOX_TEXT;
			current_text_y = item->text_y + 
				get_text_height(MEDIUMFONT);
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


		if(top_level)
		{
			BC_ListBoxItem *item = data[0].values[i];
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
			BC_ListBoxItem *item = data[0].values[i];
			if(item->autoplace_icon)
			{
// 1 column only if icons are used
				display_format = LISTBOX_ICONS;
// Test row height
// Start new row.
				if(*next_icon_y + get_item_h(item) >= get_h() && 
					*next_icon_y > 0)
				{
					*icon_x = *next_icon_x;
					*next_icon_y = 0;
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
				display_format = LISTBOX_TEXT;
				item->set_text_x(next_text_x);
				item->set_text_y(*next_text_y);

// printf("BC_ListBox::calculate_item_coords_recursive %p %d %d %d %d %s \n", 
// item->get_sublist(), 
// item->get_columns(), 
// item->get_expand(), 
// next_text_x, 
// *next_text_y,
// item->get_text());
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
			display_format = LISTBOX_TEXT;
			*next_text_y += get_text_height(MEDIUMFONT);
		}

// Set up a sublist
		BC_ListBoxItem *item = data[0].values[i];
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

int BC_ListBox::get_display_mode()
{
	return display_format;
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
	if(display_format == LISTBOX_TEXT)
		return item->text_x - xposition + 2;
	else
		return item->icon_x - xposition + 2;
}

int BC_ListBox::get_item_y(BC_ListBoxItem *item)
{
	int result;
	if(display_format == LISTBOX_TEXT)
		result = item->text_y - yposition + title_h + 2;
	else
		result = item->icon_y - yposition + title_h + 2;
	return result;
}

int BC_ListBox::get_item_w(BC_ListBoxItem *item)
{
	if(display_format == LISTBOX_ICONS)
	{
		int x, y, w, h;
		get_icon_mask(item, x, y, w, h);
		int icon_w = w;
		get_text_mask(item, x, y, w, h);
		int text_w = w;

		if(icon_position == ICON_LEFT)
			return icon_w + text_w;
		else
			return (icon_w > text_w) ? icon_w : text_w;
	}
	else
	{
		return get_text_width(MEDIUMFONT, item->text) + 2 * LISTBOX_MARGIN;
	}
}

int BC_ListBox::get_item_h(BC_ListBoxItem *item)
{
	if(display_format == LISTBOX_ICONS)
	{
		int x, y, w, h;
		get_icon_mask(item, x, y, w, h);
		int icon_h = h;
		get_text_mask(item, x, y, w, h);
		int text_h = h;

		if(icon_position == ICON_LEFT)
			return (icon_h > text_h) ? icon_h : text_h;
		else
			return icon_h + text_h;
	}
	else
	{
		return get_text_height(MEDIUMFONT);
	}
	return 0;
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

	if(display_format == LISTBOX_ICONS)
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

				if(display_format == LISTBOX_ICONS && icon_position == ICON_LEFT)
					x1 += w;

				get_text_mask(item, x, y, w, h);
				if(x1 + w > widest) widest = x1 + w;
			}
		}
	}
	else
	if(display_format == LISTBOX_TEXT)
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





	for(int j = 0; j < (data ? data[0].total : 0); j++)
	{
		int y1, x, y, w, h;
		BC_ListBoxItem *item = data[0].values[j];

		if(display_format == LISTBOX_ICONS)
		{
			get_icon_mask(item, x, y, w, h);
			if(y + h > highest) highest = y + h;

			get_text_mask(item, x, y, w, h);
			if(y + h > highest) highest = y + h;
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
	if(display_format == LISTBOX_TEXT && top_level) 
	{
		highest = LISTBOX_MARGIN + *result;
	}


	return highest;
}

int BC_ListBox::set_yposition(int position, int draw_items)
{
	this->yposition = position;
	if(draw_items)
	{
		this->draw_items();
		gui->flash();
	}
	return 0;
}

int BC_ListBox::set_xposition(int position)
{
	this->xposition = position;
	draw_items();
	gui->flash();
	return 0;
}

void BC_ListBox::expand_item(BC_ListBoxItem *item, int expand)
{
	if(item)
	{
		item->expand = expand;
// Collapse sublists if this is collapsed to make it easier to calculate
// coordinates
		if(item->get_sublist())
			collapse_recursive(item->get_sublist());


// Set everything for autoplacement
		
		set_autoplacement(data, 0, 1);

		draw_items();
		gui->flash();
	}
}

void BC_ListBox::collapse_recursive(ArrayList<BC_ListBoxItem*> *data)
{
	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *item = data[0].values[i];
		if(item->get_sublist() && item->expand)
		{
			item->expand = 0;
			collapse_recursive(item->get_sublist());
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

		BC_ListBoxItem *item = data[0].values[i];
		if(item->get_sublist())
		{
			set_autoplacement(item->get_sublist(), do_icons, do_text);
		}
	}
}



int BC_ListBox::get_w()
{
	if(popup)
		return BCPOPUPLISTBOX_W;
	else
		return popup_w;
}

int BC_ListBox::get_h()
{
	if(popup)
		return BCPOPUPLISTBOX_H;
	else
		return popup_h;
}

int BC_ListBox::get_yscroll_x()
{
	if(popup)
		return popup_w - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w();
	else
		return get_x() + 
			popup_w - 
			get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w();
}

int BC_ListBox::get_yscroll_y()
{
	if(popup)
		return 0;
	else
		return get_y();
}

int BC_ListBox::get_yscroll_height()
{
	return popup_h - (need_xscroll ? 
		get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h() : 
		0);
}

int BC_ListBox::get_xscroll_x()
{
	if(popup)
		return 0;
	else
		return get_x();
}

int BC_ListBox::get_xscroll_y()
{
	if(popup)
		return popup_h - 
			get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h();
	else
		return get_y() + 
			popup_h - 
			get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h();
}

int BC_ListBox::get_xscroll_width()
{
	return popup_w - (need_yscroll ? 
		get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w() : 
		0);
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

int BC_ListBox::get_icon_mask(BC_ListBoxItem *item, 
	int &x, 
	int &y, 
	int &w, 
	int &h)
{
	if(display_format == LISTBOX_ICONS)
	{
		x = get_item_x(item);
		y = get_item_y(item);
		w = get_icon_w(item) + ICON_MARGIN * 2;
		h = get_icon_h(item) + ICON_MARGIN * 2;
	}
	else
	if(display_format == LISTBOX_TEXT)
	{
		x = y = w = h = 0;
	}
	return 0;
}

int BC_ListBox::get_text_mask(BC_ListBoxItem *item, 
	int &x, 
	int &y, 
	int &w, 
	int &h)
{
	x = get_item_x(item);
	y = get_item_y(item);

	if(display_format == LISTBOX_ICONS)
	{
		if(icon_position == ICON_LEFT)
		{
			x += get_icon_w(item) + ICON_MARGIN * 2;
			y += get_icon_h(item) - get_text_height(MEDIUMFONT);
		}
		else
		{
			y += get_icon_h(item) + ICON_MARGIN;
		}

		w = get_text_width(MEDIUMFONT, item->text) + ICON_MARGIN * 2;
		h = get_text_height(MEDIUMFONT) + ICON_MARGIN * 2;
	}
	else
	if(display_format == LISTBOX_TEXT)
	{
		w = get_text_width(MEDIUMFONT, item->text) + LISTBOX_MARGIN * 2;
		h = get_text_height(MEDIUMFONT);
	}
	return 0;
}

int BC_ListBox::get_item_highlight(ArrayList<BC_ListBoxItem*> *data, 
	int column, 
	int item)
{
	if(data[column].values[item]->selected)
		return BLUE;
	else
	if(highlighted_ptr == data[0].values[item])
		return LTGREY;
	else
		return WHITE;
}

int BC_ListBox::get_item_color(ArrayList<BC_ListBoxItem*> *data, 
	int column, 
	int item)
{
	int color = data[column].values[item]->color;
	if(get_item_highlight(data, column, item) == color)
		return BLACK;
	else
		return color;
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

	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *item = data[0].values[i];
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

	for(int i = 0; i < data[0].total; i++)
	{
		(*counter)++;
		BC_ListBoxItem *item = data[0].values[i];
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


int BC_ListBox::set_selection_mode(int mode)
{
	this->selection_mode = mode;
	return 0;
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
void BC_ListBox::set_columns(char **column_titles, 
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


int BC_ListBox::update(ArrayList<BC_ListBoxItem*> *data,
	char **column_titles,
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

	if(gui && draw)
	{
		draw_background();
		draw_items();
		update_scrollbars();
		gui->flash();
	}

	return 0;
}

void BC_ListBox::center_selection()
{
	int selection = get_selection_number(0, 0);

	calculate_item_coords();
	center_selection(selection);


	if(gui)
	{
		draw_background();
		draw_items();
		update_scrollbars();
		gui->flash();
	}
}

void BC_ListBox::move_vertical(int pixels)
{
}

void BC_ListBox::move_horizontal(int pixels)
{
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
		for(int i = data[0].total - 1; i >= 0; i--)
		{
			BC_ListBoxItem *current_item = data[0].values[i];
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
	}while(top_level && data[0].total);
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
		for(int i = 0; i < data[0].total; i++)
		{
			BC_ListBoxItem *current_item = data[0].values[i];
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
	}while(top_level && data[0].total);
	return -1;
}


void BC_ListBox::fix_positions()
{
	if(yposition < 0) yposition = 0;
	else
	if(yposition > get_items_height(data, columns) - view_h)
		yposition = get_items_height(data, columns) - view_h;

	if(yposition < 0) yposition = 0;

	if(xposition < 0) xposition = 0;
	else
	if(xposition >= get_items_width() - view_w)
		xposition = get_items_width() - view_w;

	if(xposition < 0) xposition = 0;
}

int BC_ListBox::center_selection(int selection,
	ArrayList<BC_ListBoxItem*> *data,
	int *counter)
{
	int temp = -1;
	if(!data) data = this->data;
	if(!counter) counter = &temp;

	for(int i = 0; i < data[0].total; i++)
	{
		(*counter)++;

// Got it
		BC_ListBoxItem *item = data[0].values[i];
		if((*counter) == selection)
		{
			BC_ListBoxItem *top_item = this->data[0].values[0];


			if(display_format == LISTBOX_ICONS)
			{
// Icon is out of window
				if(item->icon_y - yposition  > 
					view_h - get_text_height(MEDIUMFONT) ||
					item->icon_y - yposition < 0)
				{
					yposition = item->icon_y - view_h / 2;
				}

				if(data[0].values[selection]->icon_x - xposition > view_w ||
					data[0].values[selection]->icon_x - xposition < 0)
				{
					xposition = item->icon_x - view_w / 2;
				}
			}
			else
			if(display_format == LISTBOX_TEXT)
			{
// Text coordinate is out of window
				if(item->text_y - yposition  > 
					view_h - get_text_height(MEDIUMFONT) ||
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
	int h_needed = get_items_height(data, columns);
	int w_needed = get_items_width();

	if(xscrollbar)
	{
		if(xposition != xscrollbar->get_value())
			xscrollbar->update_value(xposition);

		if(w_needed != xscrollbar->get_length() || view_w != xscrollbar->get_handlelength())
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



void BC_ListBox::set_drag_scroll(int value)
{
	allow_drag_scroll = value;
}


// Test for scrolling by dragging

void BC_ListBox::test_drag_scroll(int &redraw, int cursor_x, int cursor_y)
{
	int top_boundary = 2;
	if(display_format == LISTBOX_TEXT && column_titles)
		top_boundary += title_h;

	if(!allow_drag_scroll) return;

	if(current_operation == BCLISTBOX_DRAG_DIVISION) return;

	if(cursor_y < top_boundary)
	{
		yposition -= top_boundary - cursor_y;
		redraw = 1;
	}
	else
	if(cursor_y >= view_h + title_h + 4)
	{
		yposition += cursor_y - (view_h + title_h + 4);
		redraw = 1;
	}

	if(cursor_x < 2)
	{
		xposition -= 2 - cursor_x;
		redraw = 1;
	}
	else
	if(cursor_x >= view_w + 2)
	{
		xposition += cursor_x - (view_w + 2);
		redraw = 1;
	}
}



int BC_ListBox::select_rectangle(ArrayList<BC_ListBoxItem*> *data,
		int x1, 
		int y1,
		int x2, 
		int y2)
{
	int result = 0;
	for(int i = 0; i < data[0].total; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			BC_ListBoxItem *item = data[j].values[i];
			if(display_format == LISTBOX_ICONS)
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
						gui->get_w() - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w() : 
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

		BC_ListBoxItem *item = data[0].values[i];
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


	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *item = data[0].values[i];
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
	for(int i = 0; i < src[0].total; i++)
	{
		BC_ListBoxItem *item = src[0].values[i];

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
			for(int i = 0; i < src[0].total; i++)
			{
				data[j].append(src[j].values[i]);
			}
		}
		return 1;
	}
	else
	for(int i = 0; i < data[0].total; i++)
	{
		(*counter)++;
		if((*counter) == destination)
		{
			for(int j = 0; j < columns; j++)
			{
				for(int k = 0; k < src[0].total; k++)
				{
					data[j].insert(src[j].values[k], destination + k);
				}
			}
			return 1;
		}

		BC_ListBoxItem *item = data[0].values[i];
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

	for(int i = 0; i < data[0].total; i++)
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

		BC_ListBoxItem *new_item = data[0].values[i];
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
	for(int i = 0; i < data[0].total; i++)
	{
		(*counter)++;
		if((*counter) == number)
		{
			return data[column].values[i];
		}
		BC_ListBoxItem *item = data[0].values[i];
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

int BC_ListBox::cursor_item(ArrayList<BC_ListBoxItem*> *data,
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
	if(display_format == LISTBOX_ICONS)
	{
		for(int j = data[0].total - 1; j >= 0; j--)
		{
			int icon_x, icon_y, icon_w, icon_h;
			int text_x, text_y, text_w, text_h;
			BC_ListBoxItem *item = data[0].values[j];
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
	if(display_format == LISTBOX_TEXT)
	{
// Cursor is inside item rectangle
		if(cursor_x >= 0 && 
			cursor_x < (yscrollbar ? 
				gui->get_w() - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w() : 
				gui->get_w()) &&
			cursor_y > 0 && 
			cursor_y < gui->get_h())
		{
// Search table for cursor obstruction
			for(int i = 0; i < data[0].total; i++)
			{
				BC_ListBoxItem *item = data[0].values[i];
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
					if(cursor_item(item->get_sublist(),
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

int BC_ListBox::repeat_event(int64_t duration)
{
	if(duration == top_level->get_resources()->tooltip_delay &&
		tooltip_text[0] != 0 &&
		popup &&
		status == LISTBOX_HIGH &&
		!tooltip_done)
	{
		show_tooltip();
		tooltip_done = 1;
		return 1;
	}
	return 0;
}


int BC_ListBox::cursor_enter_event()
{
	int result = 0;
	
//	if(active) result = 1;

	if(popup)
	{
		if(top_level->event_win == win)
		{
			tooltip_done = 0;
			if(top_level->button_down)
			{
				status = LISTBOX_DN;
			}
			else
			if(status == LISTBOX_UP)
			{
				status = LISTBOX_HIGH;
			}
//printf("BC_ListBox::cursor_enter_event 1\n");
			draw_face();
			result = 1;
		}
	}

	if(gui && top_level->event_win == gui->win)
	{
		if(!highlighted)
		{
//printf("BC_ListBox::cursor_enter_event 2\n");
			highlighted = 1;
			draw_border();
//			flash();
		}
		result = 1;
	}
	return result;
}

int BC_ListBox::cursor_leave_event()
{
	if(popup)
	{
		hide_tooltip();
		if(status == LISTBOX_HIGH)
		{
			status = LISTBOX_UP;
			draw_face();
		}
	}

	if(gui && highlighted)
	{
		highlighted = 0;
		if(highlighted_ptr) 
		{
			highlighted_ptr = 0;
			highlighted_item = -1;
			draw_items();
		}
		else
			draw_border();
		gui->flash();
	}
	return 0;
}

int BC_ListBox::get_first_selection(ArrayList<BC_ListBoxItem*> *data, int *result)
{
	int temp = -1;
	if(!result) result = &temp;

	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *item = data[0].values[i];
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

int BC_ListBox::get_total_items(ArrayList<BC_ListBoxItem*> *data, int *result)
{
	int temp = 0;
	if(!result) result = &temp;

	for(int i = 0; i < data[0].total; i++)
	{
		(*result)++;
		if(data[0].values[i]->get_sublist())
			get_total_items(data[0].values[i]->get_sublist(), result);
	}

	return (*result);
}


int BC_ListBox::get_last_selection(ArrayList<BC_ListBoxItem*> *data, int *result)
{
	int temp = -1;
	int top_level = 0;
	if(!result)
	{
		result = &temp;
		top_level = 1;
	}

	for(int i = data[0].total - 1; i >= 0; i--)
	{
		BC_ListBoxItem *item = data[0].values[i];
		(*result)++;
		if(item->selected)
		{
			if(top_level)
				return get_total_items(data) - (*result) /* - 1 */;
			else
				return (*result);
		}

		if(item->get_sublist())
		{
			if(get_last_selection(item->get_sublist(), result) >= 0)
			{
				if(top_level)
					return get_total_items(data) - (*result) /* - 1 */;
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

	for(int i = 0; i < data[0].total; i++)
	{
		(*current)++;
		if((*current) >= start && (*current) < end)
		{
			for(int j = 0; j < columns; j++)
				data[j].values[i]->selected = 1;
		}
		BC_ListBoxItem *item = data[0].values[i];
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

//printf("BC_ListBox::expand_selection %d %d\n", selection_center, selection_number);

// Calculate the range to select based on selection_center and selection_number
	if(selection_number < selection_center)
	{
		selection_start = selection_number;
	}
	else
	{
		selection_end = selection_number + 1;
	}

//printf("BC_ListBox::expand_selection %d %d %d %d\n", old_selection_start, old_selection_end, selection_start, selection_end);
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

	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *item = data[0].values[i];
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
	for(int i = 0; i < data[0].total; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			BC_ListBoxItem *item = data[j].values[i];
			item->selected = value;
		}
		BC_ListBoxItem *item = data[0].values[i];
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
	for(int i = 0; i < data[0].total && (*counter) != item_number; i++)
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

		BC_ListBoxItem *item = data[0].values[i];
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

	for(int i = 0; i < data[0].total; i++)
	{
		BC_ListBoxItem *item = data[0].values[i];
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
	for(int i = 0; i < data[0].total; i++)
	{
		for(int j = 0; j < columns; j++)
		{
			BC_ListBoxItem *item = data[j].values[i];
			if(item->selected == old_value) item->selected = new_value;
		}
		BC_ListBoxItem *item = data[0].values[i];
		if(item->get_sublist())
			promote_selections(item->get_sublist(), old_value, new_value);
	}
}

int BC_ListBox::button_press_event()
{
	int redraw = 0;
	int result = 0;

	selection_number = -1;
	hide_tooltip();
	if(popup)
	{
		if(top_level->event_win == win)
		{
			status = LISTBOX_DN;
			draw_face();
// Deploy listbox
			if(!active)
			{
				top_level->deactivate();
				activate();
			}

			result = 1;
		}
		else
		if((xscrollbar && top_level->event_win == xscrollbar->win) ||
			(yscrollbar && top_level->event_win == yscrollbar->win) ||
			(gui && top_level->event_win == gui->win))
		{
			result = 0;
		}
		else
		if(active)
		{
			deactivate();
			result = 1;
		}
	}

	if(gui && top_level->event_win == gui->win)
	{
		if(!active)
		{
			top_level->deactivate();
			activate();
		}

// Wheel mouse behavior
		if(get_buttonpress() == 4)
		{
			current_operation = BCLISTBOX_WHEEL;
			if(yscrollbar)
			{
				set_yposition(yposition - gui->get_h() / 10, 0);
				fix_positions();
				update_scrollbars();
				highlighted_ptr = 0;
				highlighted_item = cursor_item(data,
					top_level->cursor_x, 
					top_level->cursor_y, 
					&highlighted_ptr);
				draw_items();
				gui->flash();
				result = 1;
			}
		}
		else
		if(get_buttonpress() == 5)
		{
			current_operation = BCLISTBOX_WHEEL;
			if(yscrollbar)
			{
				set_yposition(yposition + gui->get_h() / 10, 0);
				fix_positions();
				update_scrollbars();
				highlighted_ptr = 0;
				highlighted_item = cursor_item(data,
					top_level->cursor_x, 
					top_level->cursor_y,
					&highlighted_ptr);
				draw_items();
				gui->flash();
				result = 1;
			}
		}
		else
		{
// Get item button was pressed over
			int new_cursor;
			BC_ListBoxItem *current_item = 0;
			selection_number2 = selection_number1;
			selection_number1 = selection_number = cursor_item(data, 
					top_level->cursor_x, 
					top_level->cursor_y,
					&current_item);

// Pressed over a title division
			if(test_divisions(gui->get_cursor_x(), 
				gui->get_cursor_y(), 
				new_cursor))
			{
				current_operation = BCLISTBOX_DRAG_DIVISION;
				reset_query();
			}
			else
// Pressed over an item
			if(current_item)
			{
				selection_start = -1;
				selection_end = -1;


// Multiple item selection is possible
				if(selection_mode == LISTBOX_MULTIPLE && 
					(ctrl_down() || shift_down()))
				{
// Expand text selection.
// Fill items between selected region and current item.
					if(shift_down() && display_format == LISTBOX_TEXT)
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
					current_operation = BCLISTBOX_SELECT;
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
						new_value = 1;
						current_operation = BCLISTBOX_SELECT;
					}
				}


				highlighted_item = -1;
				highlighted_ptr = 0;
				reset_query();
				redraw = 1;
				result = 1;
			}
			else
			if(data)
// Pressed over nothing
			{
				if(get_buttonpress() == 1 && 
					selection_mode == LISTBOX_MULTIPLE)
				{
					if(!shift_down())
					{
// Deselect all and redraw if anything was selected
						if(get_selection_number(0, 0) >= 0)
						{
							set_all_selected(data, 0);
							redraw = 1;
							result = 1;
						}
					}
					else
					{
// Promote selections to protect from a rectangle selection
						promote_selections(data, 1, 2);
					}

// Start rectangle selection
					current_operation = BCLISTBOX_SELECT_RECT;
					rect_x1 = rect_x2 = get_cursor_x();
					rect_y1 = rect_y2 = get_cursor_y();
				}
			}
		}
		reset_query();
	}
	else
	if(!popup && active)
	{
		deactivate();
	}


	if(redraw)
	{
		draw_items();
		gui->flash();
		selection_changed();
	}

	return result;
}

int BC_ListBox::button_release_event()
{
	int result = 0;
	int cursor_x, cursor_y;
	new_value = 0;

	if(current_operation == BCLISTBOX_WHEEL)
	{
		current_operation = BCLISTBOX_NO_OPERATION;
		return 1;
	}

	if(current_operation == BCLISTBOX_SELECT_RECT)
	{
		if(data)
		{
// Demote selections from rectangle selection
			promote_selections(data, 2, 1);
		}

// Hide rectangle overlay
		draw_rectangle();
		gui->flash();
		gui->flush();
	}

	current_operation = BCLISTBOX_NO_OPERATION;

// Popup window
	if(popup)
	{
		hide_tooltip();
		button_releases++;
		if(status == LISTBOX_DN)
		{
			status = LISTBOX_HIGH;
			draw_face();
			result = 1;
		}

// Second button release inside button
		if(top_level->event_win == win && 
			cursor_inside() && 
			button_releases > 1)
		{
			deactivate();
			result = 1;
		}
// First button release inside button
		else
		if(top_level->event_win == win && cursor_inside())
		{
			result = 1;
		}
// Button released in popup window
		else
		if(gui && 
			(top_level->event_win == win || top_level->event_win == gui->win))
		{
			Window tempwin;
			XTranslateCoordinates(top_level->display, 
				top_level->event_win, 
				gui->win, 
				top_level->cursor_x, 
				top_level->cursor_y, 
				&cursor_x, 
				&cursor_y, 
				&tempwin);

			selection_number = cursor_item(data,
				cursor_x, 
				cursor_y);

			if(selection_number >= 0)
			{
				handle_event();
			}
			deactivate();
			result = 1;
		}
// Button release outside all ranges
		else
		if(active)
		{
			deactivate();
			result = 1;
		}
	}
	else
// No popup window
	if(gui &&
		top_level->event_win == gui->win &&
		top_level->get_double_click() &&
		selection_number2 == selection_number1 &&
		selection_number2 >= 0 &&
		selection_number1 >= 0)
	{
		handle_event();
		result = 1;
	}

	return result;
}

int BC_ListBox::get_title_h()
{
	if(display_format != LISTBOX_ICONS)
		return column_titles ? (get_text_height(MEDIUMFONT) + 4) : 0;
	else
		return 0;
}

void BC_ListBox::reset_cursor(int new_cursor)
{
	if(popup)
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

int BC_ListBox::test_divisions(int cursor_x, int cursor_y, int &new_cursor)
{
	if(column_titles && cursor_y < get_title_h())
	{
		for(int i = 1; i < columns; i++)
		{
			if(cursor_x > -xposition + get_column_offset(i) - 5 &&
				cursor_x <  -xposition + get_column_offset(i) + 5)
			{
				highlighted_division = i;
				new_cursor = HSEPARATE_CURSOR;
				return 1;
			}
		}
	}
	return 0;
}

int BC_ListBox::cursor_motion_event()
{
	int redraw = 0, result = 0;
	int cursor_x = 0, cursor_y = 0;
	int new_cursor = ARROW_CURSOR;

	selection_number = -1;

	if(popup && 
		top_level->event_win == win && 
		status == LISTBOX_DN && 
		!cursor_inside())
	{
		status = LISTBOX_UP;
		draw_face();
	}

	if(gui && 
		(top_level->event_win == win || 
		(popup && top_level->event_win == gui->win)))
	{

// For some reason XTranslateCoordinates can take a long time to return.
// We work around this by only calling it when the event windows are different.
		translate_coordinates(top_level->event_win, 
			gui->win,
			top_level->cursor_x,
			top_level->cursor_y,
			&cursor_x,
			&cursor_y);

		result = 1;
		int old_highlighted_item = highlighted_item;
		BC_ListBoxItem *old_highlighted_ptr = highlighted_ptr;
		highlighted_item = -1;
		highlighted_ptr = 0;
		highlighted_item = selection_number = cursor_item(data, 
			cursor_x, 
			cursor_y,
			&highlighted_ptr);

//printf("BC_ListBox::cursor_motion_event %p %p\n", old_highlighted_item, highlighted_item);

// Cursor just moved in after pressing popup button
		if(popup && 
			top_level->get_button_down() && 
			selection_number >= 0 && current_operation != BCLISTBOX_SELECT) 
			current_operation = BCLISTBOX_SELECT;

// Moving column division
		if(current_operation == BCLISTBOX_DRAG_DIVISION && 
			top_level->get_button_down())
		{
			new_cursor = HSEPARATE_CURSOR;
			int new_w = cursor_x + xposition - get_column_offset(highlighted_division - 1);
//printf("BC_ListBox::cursor_motion_event 1 %d\n", new_w);

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

			redraw = 1;
		}
		else
// Cursor is inside and selecting an item
		if(selection_number >= 0 && 
			current_operation == BCLISTBOX_SELECT &&
			!allow_drag)
		{
// Deselect all items and select just the one we're over
			if(selection_mode == LISTBOX_SINGLE ||
				(!shift_down() && 
					!ctrl_down()))
			{
				redraw = update_selection(data, selection_number);
			}
			else
// Expand multiple selection
			{
// Expand selected region in text mode centered around initial range
				if(display_format == LISTBOX_TEXT && shift_down())
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
		}
		else
		if(current_operation == BCLISTBOX_SELECT_RECT)
		{
			draw_rectangle();
			rect_x2 = get_cursor_x();
			rect_y2 = get_cursor_y();
// Adjust rectangle coverage
			if(data)
			{
				int x1 = MIN(rect_x1, rect_x2);
				int x2 = MAX(rect_x1, rect_x2);
				int y1 = MIN(rect_y1, rect_y2);
				int y2 = MAX(rect_y1, rect_y2);

				redraw = select_rectangle(data,
					x1, 
					y1,
					x2, 
					y2);

			}

// Refresh just rectangle
			if(!redraw)
			{
				draw_rectangle();
				flash();
				flush();
			}
		}
		else
// Test if cursor moved over a title division
		{
			test_divisions(cursor_x, cursor_y, new_cursor);
		}

		if(top_level->get_button_down() && 
			current_operation == BCLISTBOX_SELECT)
		{
			test_drag_scroll(redraw, cursor_x, cursor_y);
		}
		else
		if(highlighted_item != old_highlighted_item)
		{
			redraw = 1;
		}


// Change cursor to title division adjustment
		reset_cursor(new_cursor);

	}


//printf("BC_ListBox::cursor_motion_event 50 %d %d %p\n", redraw, result, highlighted_item);


	if(redraw)
	{
		fix_positions();
		draw_items();
		update_scrollbars();
		gui->flash();
		gui->flush();
		if(current_operation == BCLISTBOX_SELECT) selection_changed();
		result = 1;
	}
	else
	if(!result && highlighted_item >= 0)
	{
		highlighted_item = -1;
		highlighted_ptr = 0;
		draw_items();
		gui->flash();
		gui->flush();
		result = 0;
	}

//printf("BC_ListBox::cursor_motion_event 100\n");
	return result;
}

int BC_ListBox::drag_start_event()
{
	if(gui && 
		top_level->event_win == gui->win && 
		allow_drag && 
		current_operation != BCLISTBOX_DRAG_DIVISION)
	{
		BC_ListBoxItem *item_return = 0;
		selection_number = cursor_item(data, 
			top_level->cursor_x, 
			top_level->cursor_y,
			&item_return);

		if(selection_number >= 0)
		{
			BC_Pixmap *pixmap = item_return->icon ? 
				item_return->icon : 
				drag_icon;
			drag_popup = new BC_DragWindow(this, 
				pixmap, 
				get_abs_cursor_x() - pixmap->get_w() / 2,
				get_abs_cursor_y() - pixmap->get_h() / 2);
			return 1;
		}
	}
	return 0;
}

int BC_ListBox::drag_motion_event()
{
	if(drag_popup)
	{
		int redraw = 0;
		test_drag_scroll(redraw, 
			top_level->cursor_x, 
			top_level->cursor_y);

		int new_highlighted_item = -1;
		BC_ListBoxItem *new_highlighted_ptr = 0;
		int new_highlight = new_highlighted_item = cursor_item(data,
			top_level->cursor_x, 
			top_level->cursor_y,
			&new_highlighted_ptr);

//printf("BC_ListBox::drag_motion_event %d %d %d\n", 
//	top_level->cursor_x, top_level->cursor_y, new_highlight);
		if(new_highlighted_item != highlighted_item)
		{
			redraw = 1;
			highlighted_item = new_highlighted_item;
			highlighted_ptr = new_highlighted_ptr;
		}

		if(redraw)
		{
			fix_positions();
			draw_items();
			update_scrollbars();
			gui->flash();
		}

		return drag_popup->cursor_motion_event();
		return 1;
	}
	return 0;
}

int BC_ListBox::drag_stop_event()
{
	if(drag_popup)
	{
// Inside window boundary
		if(top_level->cursor_x > 0 && 
			top_level->cursor_x < gui->get_w() - drag_popup->get_w() / 2 && 
			top_level->cursor_y > 0 &&
			top_level->cursor_y < gui->get_h() - drag_popup->get_h() / 2)
		{
// Move icon


			if(display_format == LISTBOX_ICONS)
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
			{
// Move selected items from data to temporary
				ArrayList<BC_ListBoxItem*> *src_items = 
					new ArrayList<BC_ListBoxItem*>[columns];
				move_selection(src_items, data);

// Insert items from temporary to data
				int destination = highlighted_item = item_to_index(data,
					highlighted_ptr);
				put_selection(data,
					src_items,
					destination);


				delete [] src_items;				
				set_autoplacement(data, 0, 1);
			}
			
			
			draw_items();
			gui->flash();
		}
		else
			drag_popup->drag_failure_event();

		delete drag_popup;
		drag_popup = 0;
		current_operation = BCLISTBOX_NO_OPERATION;
		new_value = 0;
		return 1;
	}
	return 0;
}

BC_DragWindow* BC_ListBox::get_drag_popup()
{
	return drag_popup;
}

int BC_ListBox::translation_event()
{
	if(popup && gui)
	{
		int new_x = gui->get_x() + 
			(top_level->last_translate_x - 
				top_level->prev_x - 
				top_level->get_resources()->get_left_border());
		int new_y = gui->get_y() + 
			(top_level->last_translate_y - 
				top_level->prev_y -
				top_level->get_resources()->get_top_border());

		gui->reposition_window(new_x, new_y);
		
	}
	return 0;
}

int BC_ListBox::reposition_window(int x, int y, int w, int h)
{
	if(w != -1)
	{
		if(w != -1) popup_w = w;
		if(h != -1) popup_h = h;
//printf("BC_ListBox::reposition_window %d %d\n", popup_w, popup_h);

		if(!popup)
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
	draw_face();
	draw_items();
	flash();
	return 0;
}

int BC_ListBox::deactivate()
{
	if(active)
	{
		active = 0;
		if(popup)
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
	return 0;
}

int BC_ListBox::activate()
{
	if(!active)
	{
		top_level->active_subwindow = this;
		active = 1;
		button_releases = 0;

		if(popup)
		{
			Window tempwin;
			int new_x, new_y;
			XTranslateCoordinates(top_level->display, 
				parent_window->win, 
				top_level->rootwin, 
				get_x() - popup_w + get_w(), 
				get_y() + get_h(), 
				&new_x, 
				&new_y, 
				&tempwin);

			if(new_x < 0) new_x = 0;
			if(new_y + popup_h > top_level->get_root_h()) new_y -= get_h() + popup_h;

//printf("BC_ListBox::activate %d %d\n", popup_w, popup_h);
			add_subwindow(gui = new BC_Popup(this, 
				new_x, 
				new_y, 
				popup_w, 
				popup_h, 
				-1,
				0,
				0));
			draw_items();
//printf("BC_ListBox::activate 1\n");
			gui->flash();
		}
	}
	return 0;
}

int BC_ListBox::keypress_event()
{
	if(!active) return 0;
	
	int result = 0, redraw = 0, done, view_items = view_h / get_text_height(MEDIUMFONT);
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

//printf("BC_ListBox::keypress_event 1 %d\n", new_item);
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
		fix_positions();
		draw_items();
		update_scrollbars();
		gui->flash();
	}
	
	if(new_selection >= 0)
	{
		selection_changed();
	}

	return result;
}

int BC_ListBox::get_scrollbars()
{
	int h_needed = get_items_height(data, columns);
	int w_needed = get_items_width();



	title_h = get_title_h();

	view_h = popup_h - title_h - 4;
	view_w = popup_w - 4;

// Create scrollbars as needed
	for(int i = 0; i < 2; i++)
	{
		if(w_needed > view_w)
		{
			need_xscroll = 1;
			view_h = popup_h - 
				title_h - 
				get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h() - 
				4;
		}
		else
		{
			need_xscroll = 0;
		}

		if(h_needed > view_h)
		{
			need_yscroll = 1;
			view_w = popup_w - 
				get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w() - 
				4;
		}
		else
		{
			need_yscroll = 0;
		}
	}

// Update subwindow size
	int new_w = popup_w;
	int new_h = popup_h;
	if(need_xscroll) new_h -= get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h();
	if(need_yscroll) new_w -= get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w();

	if(!popup)
		if(new_w != BC_WindowBase::get_w() || new_h != BC_WindowBase::get_h())
			gui->resize_window(new_w, new_h);

	BC_WindowBase *destination = (popup ? gui : parent_window);
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
			xscrollbar->reposition_window(get_xscroll_x(),
				get_xscroll_y(),
				get_xscroll_width());
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
			yscrollbar->reposition_window(get_yscroll_x(),
				get_yscroll_y(),
				get_yscroll_height());
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

	return 0;
}

BC_Pixmap* BC_ListBox::get_bg_surface()
{
	return bg_surface;
}


void BC_ListBox::draw_background()
{
// White background pixmap
	set_color(WHITE);
	draw_box(0, 0, bg_surface->get_w(), bg_surface->get_h(), bg_surface);

// Optional heroine pixmap
	if(bg_pixmap)
		bg_surface->draw_pixmap(bg_pixmap,
			bg_surface->get_w() - top_level->get_resources()->listbox_bg->get_w(),
			0);
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

void BC_ListBox::update_format(int display_format, int redraw)
{
	this->display_format = display_format;
	if(redraw)
	{
		draw_items();
		if(gui) gui->flash();
	}
}

int BC_ListBox::get_format()
{
	return display_format;
}



int BC_ListBox::draw_items()
{
	if(gui)
	{
//dump(data, columns);

// Calculate items width 
		calculate_item_coords();


// Create and destroy scrollbars as needed
		get_scrollbars();



//		draw_background();

// Icon display
		if(display_format == LISTBOX_ICONS)
		{
			clear_listbox(2, 2 + title_h, view_w, view_h);

			set_font(MEDIUMFONT);
			for(int i = 0; i < data[0].total; i++)
			{
				BC_ListBoxItem *item = data[0].values[i];
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

					if(item_color != WHITE)
					{
						gui->set_color(BLACK);
						gui->draw_rectangle(icon_x, icon_y, icon_w, icon_h);
						gui->set_color(item_color);
						gui->draw_box(icon_x + 1, icon_y + 1, icon_w - 2, icon_h - 2);
						gui->set_color(BLACK);
						gui->draw_rectangle(text_x, text_y, text_w, text_h);
						gui->set_color(item_color);
						gui->draw_box(text_x + 1, text_y + 1, text_w - 2, text_h - 2);

						if(icon_position == ICON_LEFT)
							gui->draw_box(text_x - 1, text_y + 1, 2, text_h - 2);
						else
						if(icon_position == ICON_TOP)
							gui->draw_line(text_x + 1, text_y, text_x + icon_w - 2, text_y);
					}

// Draw icons
					gui->set_color(get_item_color(data, 0, i));
					if(item->icon)
						item->icon->write_drawable(gui->pixmap, 
							icon_x + ICON_MARGIN, 
							icon_y + ICON_MARGIN);
					gui->draw_text(text_x + ICON_MARGIN, 
						text_y + ICON_MARGIN + get_text_ascent(MEDIUMFONT), 
						item->text);
				}
			}
		}
		else
// Text display
		if(display_format == LISTBOX_TEXT)
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
		if(column_titles && display_format != LISTBOX_ICONS)
		{
			for(int i = 0; i < columns; i++)
			{
				gui->draw_3d_box(get_column_offset(i) - xposition + 2, 
					2, 
					get_column_width(i, 1), 
					title_h, 
					top_level->get_resources()->button_light, 
					top_level->get_resources()->button_up, 
					top_level->get_resources()->button_up, 
					top_level->get_resources()->button_shadow,
					BLACK);

				gui->set_color(BLACK);
				gui->draw_text(-xposition + get_column_offset(i) + LISTBOX_MARGIN + 2, 
					2 + get_text_ascent(MEDIUMFONT), column_titles[i]);
			}
		}

// Clear garbage from bottom right corner
		if(xscrollbar && yscrollbar && popup)
		{
			gui->draw_top_background(parent_window, 
				popup_w - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(), 
				popup_h - get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h(), 
				get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(),
				get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h());
		}

// Draw borders
		draw_border();


		if(current_operation == BCLISTBOX_SELECT_RECT)
			draw_rectangle();
	}

	return 0;
}


void BC_ListBox::draw_text_recursive(ArrayList<BC_ListBoxItem*> *data, 
	int column,
	int indent,
	int *current_toggle)
{
	if(!data) return;

	set_font(MEDIUMFONT);
	int subindent = 0;

// Search for a branch and make room for toggle if there is one
	if(column == 0)
	{
		for(int i = 0; i < data[column].total; i++)
		{
			if(data[column].values[i]->get_sublist())
			{
				subindent = BC_WindowBase::get_resources()->listbox_expand[0]->get_w();
				break;
			}
		}
	}

	for(int i = 0; i < data[column].total; i++)
	{
// Draw a row
		BC_ListBoxItem *item = data[column].values[i];
		BC_ListBoxItem *first_item = data[0].values[i];

		if(get_item_y(item) >= -get_item_h(item) + title_h &&
			get_item_y(item) < view_h + title_h)
		{
			int row_color = get_item_highlight(data, 0, i);
			int x, y, w, h, column_width;

			get_text_mask(item, x, y, w, h);
			column_width = get_column_width(column, 1);
			if(x + column_width > view_w + LISTBOX_BORDER * 2)
				column_width = view_w + LISTBOX_BORDER * 2 - x;

			if(row_color != WHITE)
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
					y + get_text_height(MEDIUMFONT), 
					x + column_width - 1, 
					y + get_text_height(MEDIUMFONT));
			}

			gui->set_color(get_item_color(data, column, i));


// Indent only applies to first column
			gui->draw_text(
				x + 
					LISTBOX_BORDER + 
					LISTBOX_MARGIN + 
					(column == 0 ? indent + subindent : 0), 
				y + get_text_ascent(MEDIUMFONT), 
				item->text);


// Update expander
			if(column == 0 &&
				item->get_sublist() && 
				item->get_columns())
			{
// Must be below column titles
				if(!column_titles || y > get_text_height(MEDIUMFONT))
				{
				


// Create new expander
					if(*current_toggle >= expanders.total)
					{
						BC_ListBoxToggle *toggle = 
							new BC_ListBoxToggle(this, 
								item, 
								x + LISTBOX_BORDER + LISTBOX_MARGIN + indent,
								y);
						gui->add_subwindow(toggle);
						expanders.append(toggle);
					}
					else
// Reposition existing expander
					{
						BC_ListBoxToggle *toggle = expanders.values[*current_toggle];
						toggle->update(item, 
							x + LISTBOX_BORDER + LISTBOX_MARGIN + indent,
							y);
					}
					(*current_toggle)++;
				}
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
}





int BC_ListBox::draw_border()
{
	gui->draw_3d_border(0, 
		0, 
		view_w + LISTBOX_BORDER * 2, 
		view_h + title_h + LISTBOX_BORDER * 2, 
		top_level->get_resources()->button_shadow, 
		highlighted ? RED : BLACK, 
		highlighted ? RED : top_level->get_resources()->button_up, 
		top_level->get_resources()->button_light);
	return 0;
}

int BC_ListBox::draw_rectangle()
{
	int x1 = MIN(rect_x1, rect_x2);
	int x2 = MAX(rect_x1, rect_x2);
	int y1 = MIN(rect_y1, rect_y2);
	int y2 = MAX(rect_y1, rect_y2);

	if(x1 == x2 || y1 == y2) return 0;

	gui->set_inverse();
	gui->set_color(WHITE);
	gui->draw_rectangle(x1, y1, x2 - x1, y2 - y1);
	gui->set_opaque();
	return 0;
}

void BC_ListBox::dump(ArrayList<BC_ListBoxItem*> *data, int columns, int indent)
{
	if(!indent)
	{
		printf("BC_ListBox::dump 1\n");
	}

	for(int i = 0; i < data[0].total; i++)
	{
		for(int k = 0; k < indent; k++)
			printf(" ");
		for(int j = 0; j < columns; j++)
		{
			BC_ListBoxItem *item = data[j].values[i];
			printf("%d,%d,%d=%s ", 
				item->get_text_x(), 
				item->get_text_y(),
				item->autoplace_text, 
				item->get_text());
		}
		printf("\n");

		if(data[0].values[i]->get_sublist())
		{
			dump(data[0].values[i]->get_sublist(),
				data[0].values[i]->get_columns(),
				indent + 4);
		}
	}

	
}




