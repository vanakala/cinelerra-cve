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
 	last_selection1 = last_selection2 = 0;
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
//printf("BC_ListBox::initialize 1\n");
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
//printf("BC_ListBox::initialize 1\n");

	drag_icon = new BC_Pixmap(parent_window, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_UNKNOWN], 
		PIXMAP_ALPHA);
//printf("BC_ListBox::initialize 1\n");
	BC_SubWindow::initialize();
//printf("BC_ListBox::initialize 1\n");

	init_column_width();
//printf("BC_ListBox::initialize 1\n");

	if(top_level->get_resources()->listbox_bg)
		bg_pixmap = new BC_Pixmap(this, 
			get_resources()->listbox_bg, 
			PIXMAP_OPAQUE);
//printf("BC_ListBox::initialize 1\n");

	draw_face();
//printf("BC_ListBox::initialize 1\n");
	draw_items();
//printf("BC_ListBox::initialize 1\n");
	if(!popup) gui->flash();
//printf("BC_ListBox::initialize 2\n");
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
//printf("BC_ListBox::draw_face 1 %d\n", status);
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

	int next_text_y = 0;
	int icon_x = 0;
	int next_icon_x = 0;
	int next_icon_y = 0;
	int next_icon_col_x = 0;
// Change the display_format to get the right item dimensions
	int display_format_temp = display_format;

//printf("BC_ListBox::calculate_item_coords 1\n");
// Scan the first column for lowest y coord of all text
// and lowest right x and y coord for all icons
	for(int i = 0; i < data[0].total; i++)
	{
		int current_text_y = 0;
		int current_icon_x = 0;
		int current_icon_y = 0;

//printf("BC_ListBox::calculate_item_coords 2\n");
		if(!data[0].values[i]->autoplace_text)
		{
// Lowest text coordinate
			display_format = LISTBOX_TEXT;
			current_text_y = data[0].values[i]->text_y + 
				get_text_height(MEDIUMFONT);
			if(current_text_y > next_text_y) next_text_y = current_text_y;
		}



		if(!data[0].values[i]->autoplace_icon)
		{
			display_format = LISTBOX_ICONS;
// Lowest right icon coordinate.
			current_icon_x = data[0].values[i]->icon_x;
			if(current_icon_x > icon_x) icon_x = current_icon_x;
			if(current_icon_x + get_item_w(0, i) > next_icon_x) 
				next_icon_x = current_icon_x + get_item_w(0, i);

			current_icon_y = data[0].values[i]->icon_y + get_item_h(0, i);
			if(current_icon_y > next_icon_y) next_icon_y = current_icon_y;
		}
//printf("BC_ListBox::calculate_item_coords 3\n");
	}
//printf("BC_ListBox::calculate_item_coords 4 %d\n", next_text_y);





// Set up items which need autoplacement.
// Should fill icons down and then across
	for(int i = 0; i < data[0].total; i++)
	{
		int next_text_x = 0;
// Don't increase y unless the row required autoplacing.
		int total_autoplaced_columns = 0;
//printf("BC_ListBox::calculate_item_coords 4.5 %d\n", next_text_y);

		for(int j = 0; j < columns; j++)
		{
			if(data[j].values[i]->autoplace_text)
			{
				display_format = LISTBOX_TEXT;
				data[j].values[i]->set_text_x(next_text_x);
				data[j].values[i]->set_text_y(next_text_y);
//printf("BC_ListBox::calculate_item_coords 5 %d\n", next_text_y);
				if(j < columns - 1) next_text_x += (column_width ? column_width[j] : default_column_width[j]);
				total_autoplaced_columns++;
			}

			if(data[j].values[i]->autoplace_icon)
			{
// 1 column only if icons are used
				if(j == 0)
				{
					display_format = LISTBOX_ICONS;
// Test row height
// Start new row.
//printf("BC_ListBox::calculate_item_coords 1 %d %d %d\n", i, next_icon_x, next_icon_y);
					if(next_icon_y + get_item_h(0, i) >= get_h() && 
						next_icon_y > 0)
					{
						icon_x = next_icon_x;
						next_icon_y = 0;
					}

					if(icon_x + get_item_w(0, i) > next_icon_x) 
						next_icon_x = icon_x + get_item_w(0, i);


//printf("BC_ListBox::calculate_item_coords 2 %d %d %d\n", i, icon_x, next_icon_y);
					data[j].values[i]->set_icon_x(icon_x);
					data[j].values[i]->set_icon_y(next_icon_y);

					next_icon_y += get_item_h(0, i);
				}
			}
		}

		if(total_autoplaced_columns)
		{
			display_format = LISTBOX_TEXT;
			next_text_y += get_text_height(MEDIUMFONT);
		}
	}

	display_format = display_format_temp;

//printf("BC_ListBox::calculate_item_coords 5\n");
	return 0;
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


int BC_ListBox::get_item_x(int column, int item)
{
	if(display_format == LISTBOX_TEXT)
		return data[column].values[item]->text_x - xposition + 2;
	else
		return data[column].values[item]->icon_x - xposition + 2;
}

int BC_ListBox::get_item_y(int column, int item)
{
	int result;
	if(display_format == LISTBOX_TEXT)
		result = data[column].values[item]->text_y - yposition + title_h + 2;
	else
		result = data[column].values[item]->icon_y - yposition + title_h + 2;
	return result;
}

int BC_ListBox::get_item_w(int column, int item)
{
	if(display_format == LISTBOX_ICONS)
	{
		int x, y, w, h;
		get_icon_mask(column, item, x, y, w, h);
		int icon_w = w;
		get_text_mask(column, item, x, y, w, h);
		int text_w = w;

		if(icon_position == ICON_LEFT)
			return icon_w + text_w;
		else
			return (icon_w > text_w) ? icon_w : text_w;
	}
	else
	{
		return get_text_width(MEDIUMFONT, data[column].values[item]->text) + 2 * LISTBOX_MARGIN;
	}
}

int BC_ListBox::get_item_h(int column, int item)
{
	if(display_format == LISTBOX_ICONS)
	{
		int x, y, w, h;
		get_icon_mask(column, item, x, y, w, h);
		int icon_h = h;
		get_text_mask(column, item, x, y, w, h);
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


int BC_ListBox::get_icon_w(int column, int item)
{
	BC_Pixmap *icon = data[column].values[item]->icon;
	if(icon) return icon->get_w();
	return 0;
}

int BC_ListBox::get_icon_h(int column, int item)
{
	BC_Pixmap *icon = data[column].values[item]->icon;
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
				x1 = data[i].values[j]->icon_x;

				get_icon_mask(i, j, x, y, w, h);
				if(x1 + w > widest) widest = x1 + w;

				if(display_format == LISTBOX_ICONS && icon_position == ICON_LEFT)
					x1 += w;

				get_text_mask(i, j, x, y, w, h);
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

int BC_ListBox::get_items_height()
{
	int highest = 0;

	for(int i = 0; i < columns; i++)
	{
		for(int j = 0; j < (data ? data[i].total : 0); j++)
		{
			int y1, x, y, w, h;
			
			if(display_format == LISTBOX_ICONS)
			{
				y1 = data[i].values[j]->icon_y;
				if(icon_position == ICON_LEFT)
				{
					x += get_icon_w(i, j) + ICON_MARGIN * 2;
					y1 += get_icon_h(i, j) - get_text_height(MEDIUMFONT); - ICON_MARGIN * 2;
				}
				else
				{
					y1 += get_icon_h(i, j) + ICON_MARGIN;
				}
				get_text_mask(i, j, x, y, w, h);
				if(y1 + h > highest) highest = y1 + h;
			}
			else
			{
				y1 = data[i].values[j]->text_y;
				get_text_mask(i, j, x, y, w, h);
				if(y1 + h > highest) highest = y1 + h;
			}

		}
	}
	if(display_format == LISTBOX_TEXT) highest += LISTBOX_MARGIN;

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
		x += column_width ? column_width[--column] : default_column_width[--column];
	}
	return x;
}

void BC_ListBox::column_width_boundaries()
{
	if(column_width)
	{
		for(int i = 0; i < columns; i++)
		{
			if(column_width[i] < 10) column_width[i] = 10;
		}
	}
	else
	{
		for(int i = 0; i < columns; i++)
		{
			if(default_column_width[i] < 10) default_column_width[i] = 10;
		}
	}
}

int BC_ListBox::get_column_width(int column)
{
	if(column < columns - 1)
		return column_width ? column_width[column] : default_column_width[column];
	else
		return popup_w + 
			xposition - 
			get_column_offset(column);
}

BC_Pixmap* BC_ListBox::get_item_pixmap(int item)
{
	return data[0].values[item]->icon;
}

int BC_ListBox::get_icon_mask(int column, int item, int &x, int &y, int &w, int &h)
{
	if(display_format == LISTBOX_ICONS)
	{
		x = get_item_x(0, item);
		y = get_item_y(0, item);
		w = get_icon_w(0, item) + ICON_MARGIN * 2;
		h = get_icon_h(0, item) + ICON_MARGIN * 2;
	}
	else
	if(display_format == LISTBOX_TEXT)
	{
		x = y = w = h = 0;
	}
	return 0;
}

int BC_ListBox::get_text_mask(int column, int item, int &x, int &y, int &w, int &h)
{
	x = get_item_x(column, item);
	y = get_item_y(column, item);

	if(display_format == LISTBOX_ICONS)
	{
		if(icon_position == ICON_LEFT)
		{
			x += get_icon_w(column, item) + ICON_MARGIN * 2;
			y += get_icon_h(column, item) - get_text_height(MEDIUMFONT); - ICON_MARGIN * 2;
		}
		else
		{
			y += get_icon_h(column, item) + ICON_MARGIN;
		}

		w = get_text_width(MEDIUMFONT, data[column].values[item]->text) + ICON_MARGIN * 2;
		h = get_text_height(MEDIUMFONT) + ICON_MARGIN * 2;
	}
	else
	if(display_format == LISTBOX_TEXT)
	{
		w = get_text_width(MEDIUMFONT, data[column].values[item]->text) + LISTBOX_MARGIN * 2;
		h = get_text_height(MEDIUMFONT);
	}
	return 0;
}

int BC_ListBox::get_item_highlight(int column, int item)
{
	if(data[column].values[item]->selected)
		return BLUE;
	else
	if(highlighted_item == item)
		return LTGREY;
	else
		return WHITE;
}

int BC_ListBox::get_item_color(int column, int item)
{
	int color = data[column].values[item]->color;
	if(get_item_highlight(column, item) == color)
		return BLACK;
	else
		return color;
}


BC_ListBoxItem* BC_ListBox::get_selection(int column, int selection_number)
{
	for(int i = 0; i < data[column].total; i++)
	{
		BC_ListBoxItem *item = data[column].values[i];

//printf("BC_ListBox::get_selection %d %d\n", i, item->selected);
		if(item->selected)
		{
			if(!selection_number)
			{
				return item;
			}
			selection_number--;
		}
	}

	return 0;
}

int BC_ListBox::get_selection_number(int column, int selection_number)
{
	for(int i = 0; i < data[0].total; i++)
	{
		if(data[0].values[i]->selected)
		{
			if(!selection_number)
			{
				return i;
			}
			selection_number--;
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
//printf("BC_ListBox::delete_columns 1 %p %p\n", column_titles, column_titles[i]);
			delete [] column_titles[i];
//printf("BC_ListBox::delete_columns 2\n");
		}
//printf("BC_ListBox::delete_columns 3 %p\n", column_titles);
		delete [] column_titles;
	}
//printf("BC_ListBox::delete_columns 4 %p %p\n", column_titles, column_width);

	if(column_width) delete [] column_width;
//printf("BC_ListBox::delete_columns 2\n");
	
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

//printf("BC_ListBox::set_columns 1 %p %p %d %p %p %d\n", this->column_titles, this->column_width, this->columns, column_titles, column_width, columns);

	delete_columns();
//printf("BC_ListBox::set_columns 1\n");

	if(column_titles)
	{
		this->column_titles = new char*[columns];
		for(int i = 0; i < columns; i++)
		{
			this->column_titles[i] = new char[strlen(column_titles[i]) + 1];
			strcpy(this->column_titles[i], column_titles[i]);
		}
	}
//printf("BC_ListBox::set_columns 1\n");
	
	if(column_width)
	{
		this->column_width = new int[columns];
		for(int i = 0; i < columns; i++)
		{
			this->column_width[i] = column_width[i];
		}
	}
//printf("BC_ListBox::set_columns 2\n");
	
	this->columns = columns;
}


int BC_ListBox::update(ArrayList<BC_ListBoxItem*> *data,
						char **column_titles,
						int *column_widths,
						int columns,
						int xposition,
						int yposition, 
						int currentitem,
						int recalc_positions)
{

	set_columns(column_titles, 
		column_widths, 
		columns);

	this->data = data;

	this->yposition = yposition;
	this->xposition = xposition;
	highlighted_item = currentitem;

	if(recalc_positions)
		for(int i = 0; i < data[0].total; i++)
			for(int j = 0; j < columns; j++)
			{
				data[j].values[i]->autoplace_icon = 1;
				data[j].values[i]->autoplace_text = 1;
			}

	init_column_width();

	if(gui)
	{
		draw_background();
		draw_items();
		update_scrollbars();
		gui->flash();
	}
	return 0;
}

void BC_ListBox::move_vertical(int pixels)
{
}

void BC_ListBox::move_horizontal(int pixels)
{
}

void BC_ListBox::fix_positions()
{
	if(yposition < 0) yposition = 0;
	else
	if(yposition > get_items_height() - view_h)
		yposition = get_items_height() - view_h;

	if(yposition < 0) yposition = 0;

	if(xposition < 0) xposition = 0;
	else
	if(xposition >= get_items_width() - view_w)
		xposition = get_items_width() - view_w;

	if(xposition < 0) xposition = 0;
}

void BC_ListBox::center_selection(int selection)
{
	if(data[0].values[selection]->text_y - yposition  > 
		view_h - get_text_height(MEDIUMFONT) ||
		data[0].values[selection]->text_y - yposition < 0)
	{
		yposition = selection * get_text_height(MEDIUMFONT) - view_h / 2;
	}
	
	if(display_format == LISTBOX_ICONS)
	{
		if(data[0].values[selection]->icon_y - yposition  > 
			view_h - get_text_height(MEDIUMFONT) ||
			data[0].values[selection]->icon_y - yposition < 0)
		{
			yposition = selection * get_text_height(MEDIUMFONT) - view_h / 2;
		}

		if(data[0].values[selection]->icon_x - xposition > view_w ||
			data[0].values[selection]->icon_x - xposition < 0)
		{
			xposition = data[0].values[selection]->icon_x - view_w / 2;
		}
	}
	else
	if(display_format == LISTBOX_TEXT)
	{
		if(data[0].values[selection]->text_y - yposition  > 
			view_h - get_text_height(MEDIUMFONT) ||
			data[0].values[selection]->text_y - yposition < 0)
		{
			yposition = selection * get_text_height(MEDIUMFONT) - view_h / 2;
		}
	}
}

void BC_ListBox::update_scrollbars()
{
	int h_needed = get_items_height();
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

int BC_ListBox::cursor_inside_item(int row, 
	int column, 
	int x1, 
	int y1,
	int x2, 
	int y2)
{
	if(display_format == LISTBOX_ICONS && data)
	{
		int icon_x, icon_y, icon_w, icon_h;
		int text_x, text_y, text_w, text_h;
		get_icon_mask(column, row, icon_x, icon_y, icon_w, icon_h);
		get_text_mask(column, row, text_x, text_y, text_w, text_h);

		if((x2 >= icon_x && x1 < icon_x + icon_w &&
			y2 >= icon_y && y1 < icon_y + icon_h) ||
			(x2 >= text_x && x1 < text_x + text_w &&
			y2 >= text_y && y1 < text_y + text_h))
		{
			return 1;
		}
	}
	else
	if(display_format == LISTBOX_TEXT && data)
	{
		if(x2 >= 0 && 
			x1 < (yscrollbar ? 
				gui->get_w() - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w() : 
				gui->get_w()) &&
			y2 > 0 && 
			y1 < gui->get_h() &&
			y2 >= get_item_y(0, row) &&
			y1 < get_item_y(0, row) + get_item_h(0, row))
		{
			return 1;
		}
	}

	return 0;
}

int BC_ListBox::cursor_item(int cursor_x, int cursor_y)
{
	if(display_format == LISTBOX_ICONS && data)
	{
		for(int i = 0; i < columns; i++)
		{
			for(int j = data[i].total - 1; j >= 0; j--)
			{
				int icon_x, icon_y, icon_w, icon_h;
				int text_x, text_y, text_w, text_h;
				get_icon_mask(i, j, icon_x, icon_y, icon_w, icon_h);
				get_text_mask(i, j, text_x, text_y, text_w, text_h);

				if((cursor_x >= icon_x && cursor_x < icon_x + icon_w &&
					cursor_y >= icon_y && cursor_y < icon_y + icon_h) ||
					(cursor_x >= text_x && cursor_x < text_x + text_w &&
					cursor_y >= text_y && cursor_y < text_y + text_h))
				{
					return j;
				}
			}
		}
	}
	else
	if(display_format == LISTBOX_TEXT && data)
		if(cursor_x >= 0 && 
			cursor_x < (yscrollbar ? 
				gui->get_w() - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w() : 
				gui->get_w()) &&
			cursor_y > 0 && 
			cursor_y < gui->get_h())
		{
			for(int i = 0; i < data[0].total; i++)
			{
				if(cursor_y >= get_item_y(0, i) &&
					cursor_y < get_item_y(0, i) + get_item_h(0, i))
				{
					return i;
				}
			}
		}
	return -1;
}

int BC_ListBox::repeat_event(long duration)
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
//printf("BC_ListBox::cursor_leave_event 1\n");
	if(popup)
	{
		hide_tooltip();
		if(status == LISTBOX_HIGH)
		{
			status = LISTBOX_UP;
			draw_face();
		}
	}

//printf("BC_ListBox::cursor_leave_event 1\n");
	if(gui && highlighted)
	{
		highlighted = 0;
		if(highlighted_item >= 0) 
		{
			highlighted_item = -1;
			draw_items();
		}
		else
			draw_border();
		gui->flash();
	}
//printf("BC_ListBox::cursor_leave_event 2\n");
	return 0;
}

int BC_ListBox::button_press_event()
{
// Selected item
	int selection = -1;
	int redraw = 0;
	int result = 0;

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

		if(get_buttonpress() == 4)
		{
			current_operation = BCLISTBOX_WHEEL;
			if(yscrollbar)
			{
				set_yposition(yposition - gui->get_h() / 10, 0);
				fix_positions();
				update_scrollbars();
				highlighted_item = cursor_item(top_level->cursor_x, top_level->cursor_y);
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
				highlighted_item = cursor_item(top_level->cursor_x, top_level->cursor_y);
				draw_items();
				gui->flash();
				result = 1;
			}
		}
		else
		{
			last_selection2 = last_selection1;
			selection = cursor_item(top_level->cursor_x, top_level->cursor_y);
			last_selection1 = selection;
			int new_cursor;

// Pressed over a title division
			if(test_divisions(gui->get_cursor_x(), gui->get_cursor_y(), new_cursor))
			{
				current_operation = BCLISTBOX_DRAG_DIVISION;
				reset_query();
			}
			else
// Pressed over an item
			if(selection >= 0)
			{
				selection_start = -1;
				selection_end = -1;

				for(int j = 0; j < columns; j++)
				{
// Select one extra item
					if(selection_mode == LISTBOX_MULTIPLE && 
						(ctrl_down() || shift_down()))
					{
						if(shift_down() && display_format == LISTBOX_TEXT)
						{
							if(j == 0)
							{
								int i;
								int total_selected = 0;

								for(i = 0; i < data[j].total; i++)
								{
									if(data[j].values[i]->selected)
									{
										if(!total_selected) 
											selection_start = i;

										total_selected++;
									}
									else
									if(total_selected && selection_end < 0)
									{
										selection_end = i;
									}
								}

								if(total_selected && selection_end < 0)
									selection_end = data[j].total;

								if(total_selected)
								{
									selection_center = (selection_end + selection_start) / 2;
									if(selection >= selection_center)
									{
										selection_center = selection_start;
										selection_end = selection + 1;
									}
									else
									if(selection < selection_center)
									{
										selection_center = selection_end - 1;
										selection_start = selection;
									}
								}
							}

							for(int i = 0; i < data[j].total; i++)
							{
								if(i >= selection_start && i < selection_end)
									data[j].values[i]->selected = 1;
								else
									data[j].values[i]->selected = 0;
							}

							new_value = 1;
						}
						else
						{
							data[j].values[selection]->selected = !data[j].values[selection]->selected;
							new_value = data[j].values[selection]->selected;
						}
						current_operation = BCLISTBOX_SELECT;
					}
					else
// Select new item
					{
						if(!data[j].values[selection]->selected)
						{
							for(int k = 0; k < data[0].total; k++)
								data[j].values[k]->selected = 0;

							data[j].values[selection]->selected = 1;
							new_value = 1;
							current_operation = BCLISTBOX_SELECT;
						}
					}
				}

				highlighted_item = -1;
				reset_query();
				redraw = 1;
				result = 1;
			}
			else
			if(data)
// Pressed over nothing
			{
				if(get_buttonpress() == 1)
				{
					if(!shift_down())
					{
						for(int j = 0; j < columns; j++)
						{
							for(int k = 0; k < data[0].total; k++)
							{
								if(data[j].values[k]->selected)
								{
									redraw = 1;
									result = 1;
								}
								data[j].values[k]->selected = 0;
							}
						}
					}
					else
					{
						for(int j = 0; j < columns; j++)
						{
							for(int k = 0; k < data[0].total; k++)
							{
								if(data[j].values[k]->selected)
								{
									data[j].values[k]->selected = 2;
								}
							}
						}
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
//printf("BC_ListBox::button_press_event 1\n");
		selection_changed();
	}

	return result;
}

int BC_ListBox::button_release_event()
{
	int result = 0;
	int cursor_x, cursor_y;
	Window tempwin;
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
			for(int i = 0; i < columns; i++)
			{
				for(int j = 0; j < data[i].total; j++)
					if(data[i].values[j]->selected == 2)
						data[i].values[j]->selected = 1;
			}
		}

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
// Button release in popup window
		else
		if(gui && 
			(top_level->event_win == win || top_level->event_win == gui->win))
		{
			XTranslateCoordinates(top_level->display, 
				top_level->event_win, 
				gui->win, 
				top_level->cursor_x, 
				top_level->cursor_y, 
				&cursor_x, 
				&cursor_y, 
				&tempwin);

			selection = cursor_item(cursor_x, cursor_y);

			if(selection >= 0)
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
		last_selection2 == last_selection1)
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
	int selection = -1, redraw = 0, result = 0;
	int cursor_x = 0, cursor_y = 0;
	Window tempwin = 0;
	int new_cursor = ARROW_CURSOR;

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
		selection = cursor_item(cursor_x, cursor_y);

// Cursor just moved in after pressing popup button
		if(popup && 
			top_level->get_button_down() && 
			selection >= 0 && current_operation != BCLISTBOX_SELECT) 
			current_operation = BCLISTBOX_SELECT;

// Moving column division
		if(current_operation == BCLISTBOX_DRAG_DIVISION && 
			top_level->get_button_down())
		{
			new_cursor = HSEPARATE_CURSOR;
			if(column_width)
				column_width[highlighted_division - 1] = 
					cursor_x - xposition - get_column_offset(highlighted_division - 1);
			else
				default_column_width[highlighted_division - 1] = 
					cursor_x - xposition - get_column_offset(highlighted_division - 1);
			column_width_boundaries();

// Force update of coords
			for(int i = 0; i < columns; i++)
			{
				for(int j = 0; j < data[i].total; j++)
				{
					data[i].values[j]->autoplace_text = 1;
				}
			}
			column_resize_event();

			redraw = 1;
		}
		else
// Cursor is inside and selecting an item
		if(selection >= 0 && 
			current_operation == BCLISTBOX_SELECT &&
			!allow_drag)
		{
			for(int j = 0; j < columns; j++)
			{
				if(selection_mode == LISTBOX_SINGLE ||
					(!shift_down() && 
						!ctrl_down()))
				{
					for(int k = 0; k < data[0].total; k++)
					{
						if(k != selection && data[j].values[k]->selected)
						{
							redraw = 1;
							data[j].values[k]->selected = 0;
						}
						else
						if(k == selection && !data[j].values[k]->selected)
						{
							redraw = 1;
							data[j].values[selection]->selected = 1;
						}
					}
				}
				else
// Multiple selections
				{
// Drag selected region in text mode
					if(display_format == LISTBOX_TEXT && shift_down())
					{
						if(j == 0)
						{
							int new_selection_start;
							int new_selection_end;
							if(selection >= selection_center)
							{
								new_selection_start = selection_center;
								new_selection_end = selection + 1;
							}
							else
							{
								new_selection_start = selection;
								new_selection_end = selection_center + 1;
							}
							
							if(new_selection_start != selection_start ||
								new_selection_end != selection_end)
							{
								selection_start = new_selection_start;
								selection_end = new_selection_end;
								redraw = 1;
							}
						}
						
						for(int i = 0; i < data[j].total; i++)
						{
							data[j].values[i]->selected = (i >= selection_start && i < selection_end);
						}
					}
					else
					{
						if(data[j].values[selection]->selected != new_value)
						{
							data[j].values[selection]->selected = new_value;
							redraw = 1;
						}
					}
				}
			}
		}
		else
		if(current_operation == BCLISTBOX_SELECT_RECT)
		{
			draw_rectangle();
			rect_x2 = get_cursor_x();
			rect_y2 = get_cursor_y();
// Adjust selections
			if(data)
			{
				int x1 = MIN(rect_x1, rect_x2);
				int x2 = MAX(rect_x1, rect_x2);
				int y1 = MIN(rect_y1, rect_y2);
				int y2 = MAX(rect_y1, rect_y2);

				if(display_format == LISTBOX_ICONS)
				{
					for(int i = 0; i < columns; i++)
					{
						for(int j = 0; j < data[i].total; j++)
						{
							int new_value = cursor_inside_item(j, 
									i, 
									x1, 
									y1, 
									x2, 
									y2);
							if(new_value && !data[i].values[j]->selected)
							{
								redraw = 1;
								data[i].values[j]->selected = 1;
							}
							else
							if(!new_value && data[i].values[j]->selected)
							{
								if(data[i].values[j]->selected == 1)
								{
									data[i].values[j]->selected = 0;
									redraw = 1;
								}
							}
						}
					}
				}
				else
				if(display_format == LISTBOX_TEXT)
				{
					for(int j = 0; j < data[0].total; j++)
					{
						int new_value = cursor_inside_item(j, 
							0, 
							x1, 
							y1, 
							x2, 
							y2);

						if(new_value && !data[0].values[j]->selected)
						{
							redraw = 1;
							for(int i = 0; i < columns; i++)
								data[i].values[j]->selected = 1;
						}
						else
						if(!new_value && data[0].values[j]->selected)
						{
							if(data[0].values[j]->selected == 1)
							{
								for(int i = 0; i < columns; i++)
									data[i].values[j]->selected = 0;
								redraw = 1;
							}
						}
					}
				}
			}

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
			if(test_divisions(cursor_x, cursor_y, new_cursor))
			{
				;
			}
		}

		if(top_level->get_button_down() && 
			current_operation == BCLISTBOX_SELECT)
		{
			test_drag_scroll(redraw, cursor_x, cursor_y);
		}
		else
		if(highlighted_item != selection)
		{
			highlighted_item = selection;
			redraw = 1;
		}

		reset_cursor(new_cursor);

	}

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
		draw_items();
		gui->flash();
		gui->flush();
		result = 0;
	}

	return result;
}

int BC_ListBox::drag_start_event()
{
	if(gui && 
		top_level->event_win == gui->win && 
		allow_drag && 
		current_operation != BCLISTBOX_DRAG_DIVISION)
	{
		selection = cursor_item(top_level->cursor_x, top_level->cursor_y);

		if(selection >= 0)
		{
			BC_Pixmap *pixmap = get_item_pixmap(selection) ? get_item_pixmap(selection) : drag_icon;
//printf("BC_ListBox::drag_start_event 1 %d %d\n", get_abs_cursor_x(), get_abs_cursor_y());
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
		test_drag_scroll(redraw, top_level->cursor_x, top_level->cursor_y);

		int new_highlight = cursor_item(top_level->cursor_x, top_level->cursor_y);

//printf("BC_ListBox::drag_motion_event %d %d %d\n", 
//	top_level->cursor_x, top_level->cursor_y, new_highlight);
		if(new_highlight != highlighted_item)
		{
			redraw = 1;
			highlighted_item = new_highlight;
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
				data[0].values[selection]->icon_x = top_level->cursor_x + drag_popup->get_offset_x() - LISTBOX_MARGIN - 2 + xposition;
				data[0].values[selection]->icon_y = top_level->cursor_y + drag_popup->get_offset_y() - LISTBOX_MARGIN - 2 + yposition;
			}
			else
// Swap rows
			{


//printf("BC_ListBox::drag_stop_event 1\n");
				for(int i = 0; i < columns; i++)
				{
					ArrayList<BC_ListBoxItem*> src_items;
					ArrayList<BC_ListBoxItem*> *dst_items = &data[i];
					int destination = highlighted_item;
					if(destination < 0) destination = dst_items->total;
					int dst_total = dst_items->total;

//printf("BC_ListBox::drag_stop_event 1\n");


					for(int j = 0; j < dst_total; j++)
					{
//printf("BC_ListBox::drag_stop_event 1.1\n");
						if(dst_items->values[j]->selected)
						{
							src_items.append(dst_items->values[j]);
							for(int k = j; k < dst_items->total - 1; k++)
							{
//printf("BC_ListBox::drag_stop_event 1.2\n");
								dst_items->values[k] = dst_items->values[k + 1];
							}

							if(destination > j) destination--;
							j--;
							dst_total--;
						}
					}


//printf("BC_ListBox::drag_stop_event 1 %d %d\n", src_items.total, destination);
					for(int j = 0; j < src_items.total; j++)
					{
//printf("BC_ListBox::drag_stop_event 2 %s\n", src_items.values[j]->get_text());
						for(int k = dst_items->total - 1; k > destination; k--)
						{
							dst_items->values[k] = dst_items->values[k - 1];
						}
						dst_items->values[destination] = src_items.values[j];
						destination++;
					}
//printf("BC_ListBox::drag_stop_event 2\n");


					for(int i = 0; i < dst_items->total; i++)
						dst_items->values[i]->autoplace_text = 1;
				}
				
				
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
			done = 0;
			for(int i = data[0].total - 1; !done && i >= 0; i--)
			{
				if(data[0].values[i]->selected)
				{
					new_item = i - 1;
					if(new_item < 0) new_item = data[0].total - 1;

					for(int j = 0; j < columns; j++)
					{
						data[j].values[i]->selected = 0;
						data[j].values[new_item]->selected = 1;
					}
					center_selection(new_item);
					new_selection = 1;
					redraw = 1;
					result = 1;
					done = 1;
				}
			}

			if(!result && data[0].total)
			{
				new_item = /* 0 */ data[0].total - 1;
				for(int j = 0; j < columns; j++)
				{
					data[j].values[new_item]->selected = 1;
				}
				redraw = 1;
				result = 1;
				new_selection = 1;
				center_selection(new_item);
			}
			break;

		case DOWN:
			done = 0;
			for(int i = 0; !done && i < data[0].total; i++)
			{
				if(data[0].values[i]->selected)
				{
					new_item = i + 1;
					if(new_item >= data[0].total) new_item = 0;
					for(int j = 0; j < columns; j++)
					{
						data[j].values[i]->selected = 0;
						data[j].values[new_item]->selected = 1;
					}
					center_selection(new_item);
					redraw = 1;
					result = 1;
					new_selection = 1;
					done = 1;
				}
			}

			if(!result && data[0].total)
			{
				new_item = 0;
				for(int j = 0; j < columns; j++)
				{
					data[j].values[new_item]->selected = 1;
				}
				redraw = 1;
				result = 1;
				new_selection = 1;
				center_selection(new_item);
			}
			
			break;

		case PGUP:
			done = 0;
			for(int i = data[0].total - 1; !done && i > 0; i--)
			{
				if(data[0].values[i]->selected)
				{
					new_item = i - view_items;
					if(new_item < 0) new_item = 0;

					for(int j = 0; j < columns; j++)
					{
						data[j].values[i]->selected = 0;
						data[j].values[new_item]->selected = 1;
						redraw = 1;
						result = 1;
						new_selection = 1;
						center_selection(new_item);
					}
					done = 1;
				}
			}
			break;

		case PGDN:
			done = 0;
			for(int i = 0; !done && i < data[0].total - 1; i++)
			{
				if(data[0].values[i]->selected)
				{
					new_item = i + view_items;
					if(new_item >= data[0].total) new_item = data[0].total - 1;

					for(int j = 0; j < columns; j++)
					{
						data[j].values[i]->selected = 0;
						data[j].values[new_item]->selected = 1;
						redraw = 1;
						result = 1;
						new_selection = 1;
						center_selection(new_item);
					}
					done = 1;
				}
			}
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
	
	if(new_selection)
	{
		selection_changed();
	}

	return result;
}

int BC_ListBox::get_scrollbars()
{
//printf("BC_ListBox::get_scrollbars 1\n");
	int h_needed = get_items_height();
	int w_needed = get_items_width();
//printf("BC_ListBox::get_scrollbars 2\n");

	title_h = get_title_h();
//printf("BC_ListBox::get_scrollbars 3\n");

	view_h = popup_h - title_h - 4;
	view_w = popup_w - 4;
//printf("BC_ListBox::get_scrollbars 4 %d %d\n", h_needed, view_h);

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
//printf("BC_ListBox::get_scrollbars 5\n");

// Update subwindow size
	int new_w = popup_w, new_h = popup_h;
	if(need_xscroll) new_h -= get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h();
	if(need_yscroll) new_w -= get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w();

	if(!popup)
		if(new_w != BC_WindowBase::get_w() || new_h != BC_WindowBase::get_h())
			gui->resize_window(new_w, new_h);
//printf("BC_ListBox::get_scrollbars 6\n");

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
//printf("BC_ListBox::get_scrollbars 7\n");

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
//printf("BC_ListBox::get_scrollbars 8\n");
	
	if(!bg_surface ||
		view_w + 4 != bg_surface->get_w() ||
		view_h + 4 != bg_surface->get_h())
	{
		if(bg_surface) delete bg_surface;
//printf("BC_ListBox::get_scrollbars 8.1\n");
//new BC_Pixmap(gui, view_w + 4, view_h + 4);
//printf("BC_ListBox::get_scrollbars 8.2 %p %d %d\n", gui, view_w, view_h);
		bg_surface = new BC_Pixmap(gui, view_w + 4, view_h + 4);
		draw_background();
//printf("BC_ListBox::get_scrollbars 8.3\n");
	}

//printf("BC_ListBox::get_scrollbars 9\n");
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


// 		XFreePixmap(top_level->display, pixmap);
// 		pixmap = XCreatePixmap(top_level->display, 
// 		win, 
// 		this->w, 
// 		this->h, 
// 		top_level->default_depth);




// Calculate items width 
		calculate_item_coords();




		get_scrollbars();
//		draw_background();

// Icon display
		if(display_format == LISTBOX_ICONS)
		{
			clear_listbox(2, 2 + title_h, view_w, view_h);

			set_font(MEDIUMFONT);
			for(int i = 0; i < data[0].total; i++)
			{

				if(get_item_x(0, i) >= -get_item_w(0, i) && 
					get_item_x(0, i) < view_w &&
					get_item_y(0, i) >= -get_item_h(0, i) + title_h &&
					get_item_y(0, i) < view_h + title_h)
				{
					int item_color = get_item_highlight(0, i);
					int icon_x, icon_y, icon_w, icon_h;
					int text_x, text_y, text_w, text_h;

// Draw highlights
					get_icon_mask(0, i, icon_x, icon_y, icon_w, icon_h);
					get_text_mask(0, i, text_x, text_y, text_w, text_h);

//printf("BC_ListBox::draw_items 4\n");
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
//printf("BC_ListBox::draw_items 5\n");

// Draw icons
					gui->set_color(get_item_color(0, i));
//printf("BC_ListBox::draw_items 6\n");
					if(get_item_pixmap(i))
						get_item_pixmap(i)->write_drawable(gui->pixmap, icon_x + ICON_MARGIN, icon_y + ICON_MARGIN);
//printf("BC_ListBox::draw_items 7\n");
					gui->draw_text(text_x + ICON_MARGIN, 
						text_y + ICON_MARGIN + get_text_ascent(MEDIUMFONT), 
						data[0].values[i]->text);
//printf("BC_ListBox::draw_items 8\n");
				}
			}
		}
		else
// Text display
		if(display_format == LISTBOX_TEXT)
		{
			for(int j = 0; j < columns; j++)
			{
//printf("BC_ListBox::draw_items %d %d %d\n", j, popup_w, get_column_offset(j));
				clear_listbox(2 + get_column_offset(j) - xposition, 
					2 + title_h, 
					get_column_width(j), 
					view_h);

				set_font(MEDIUMFONT);
				for(int i = 0; i < (data ? data[j].total : 0); i++)
				{
					if(get_item_y(0, i) >= -get_item_h(0, i) + title_h &&
						get_item_y(0, i) < view_h + title_h)
					{
						int row_color = get_item_highlight(0, i);
						int x, y, w, h, column_width;

						get_text_mask(j, i, x, y, w, h);
						column_width = get_column_width(j);
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

						gui->set_color(get_item_color(j, i));

						gui->draw_text(x + 2 + LISTBOX_MARGIN, 
							y + get_text_ascent(MEDIUMFONT), 
							data[j].values[i]->text);
					}
				}
			}
		}

// Draw titles
		if(column_titles && display_format != LISTBOX_ICONS)
		{
			for(int i = 0; i < columns; i++)
			{
				gui->draw_3d_box(get_column_offset(i) - xposition + 2, 
					2, 
					get_column_width(i), 
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

// Clear garbage
		if(xscrollbar && yscrollbar && popup)
		{
			gui->draw_top_background(parent_window, 
				popup_w - get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(), 
				popup_h - get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h(), 
				get_resources()->vscroll_data[SCROLL_HANDLE_UP]->get_w(),
				get_resources()->hscroll_data[SCROLL_HANDLE_UP]->get_h());
		}


		draw_border();


		if(current_operation == BCLISTBOX_SELECT_RECT)
			draw_rectangle();
//printf("BC_ListBox::draw_items 3 %lld\n", timer.get_difference());
	}

	return 0;
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
