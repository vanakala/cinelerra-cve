#ifndef BCLISTBOX_H
#define BCLISTBOX_H

#include "bcdragwindow.inc"
#include "bclistboxitem.inc"
#include "bcpixmap.inc"
#include "bcpopuplistbox.inc"
#include "bcscrollbar.h"
#include "bcsubwindow.h"
#include "colors.h"

#define BCPOPUPLISTBOX_W 25
#define BCPOPUPLISTBOX_H 25

#define LISTBOX_UP 0
#define LISTBOX_HIGH 1
#define LISTBOX_DN 2

enum
{
	BCLISTBOX_NO_OPERATION,
	BCLISTBOX_DRAG_DIVISION,   // Dragging column division
	BCLISTBOX_SELECT,          // Select item
	BCLISTBOX_SELECT_RECT,     // Selection rectangle
	BCLISTBOX_WHEEL            // Wheel mouse
};


class BC_ListBoxYScroll : public BC_ScrollBar
{
public:
	BC_ListBoxYScroll(BC_ListBox *listbox, 
	                  int total_height, 
					  int view_height, 
	                  int position);
	int handle_event();
private:
	BC_ListBox *listbox;
};

class BC_ListBoxXScroll : public BC_ScrollBar
{
public:
	BC_ListBoxXScroll(BC_ListBox *listbox, 
	                  int total_width, 
					  int view_width,
	                  int position);
	int handle_event();
private:
	BC_ListBox *listbox;
};

class BC_ListBox : public BC_SubWindow
{
public:
	BC_ListBox(int x, 
		int y, 
		int w, 
		int h,
		int display_format,                   // Display text list or icons
		ArrayList<BC_ListBoxItem*> *data = 0, // Each column has an ArrayList of BC_ListBoxItems.
		char **column_titles = 0,             // Titles for columns.  Set to 0 for no titles
		int *column_width = 0,                // width of each column
		int columns = 1,                      // Total columns.  Only 1 in icon mode
		int yposition = 0,                    // Pixel of top of window.
		int popup = 0,                        // If this listbox is a popup window
		int selection_mode = LISTBOX_SINGLE,  // Select one item or multiple items
		int icon_position = ICON_LEFT,        // Position of icon relative to text of each item
		int allow_drag = 0);                  // Allow user to drag icons around
	virtual ~BC_ListBox();

	friend class BC_PopupListBox;

	int initialize();

// User event handler for new selections
	virtual int selection_changed() { return 0; };
// User event handler for triggering a selection
	virtual int handle_event() { return 0; };
// User event handler for a column resize
	virtual int column_resize_event() { return 0; };
// Draw background on bg_surface
	virtual void draw_background();
	BC_ListBoxItem* get_selection(int column, int selection_number);
	int get_selection_number(int column, int selection_number);
	virtual int evaluate_query(int list_item, char *string);


	virtual int button_press_event();
	int button_release_event();
	int cursor_enter_event();
	int cursor_leave_event();
	int cursor_motion_event();
	virtual int drag_start_event();
	virtual int drag_motion_event();
	virtual int drag_stop_event();
	int deactivate();
// After popping up a menu call this to interrupt the selection process
	void deactivate_selection();
	int activate();
	virtual int keypress_event();
	int translation_event();
	int repeat_event(long repeat_id);
	int get_yposition();
	int get_xposition();
	BC_DragWindow* get_drag_popup();

// change the contents
	int update(ArrayList<BC_ListBoxItem*> *data,
						char **column_titles,
						int *column_widths,
						int columns,
						int xposition = 0,
						int yposition = 0, 
						int currentitem = -1,   // Item cursor is over
						int recalc_positions = 0);   // set all autoplace flags to 1
	void update_format(int display_format, int redraw);
	int get_format();

// Allow scrolling when dragging items
	void set_drag_scroll(int value);
	int set_selection_mode(int mode);
	int set_yposition(int position, int draw_items = 1);
	int set_xposition(int position);
	int get_highlighted_item();
	int get_yscroll_x();
	int get_yscroll_y();
	int get_yscroll_height();
	int get_xscroll_x();
	int get_xscroll_y();
	int get_xscroll_width();
	int get_column_offset(int column);
	int get_column_width(int column);
	int get_w();
	int get_h();
	int get_display_mode();
	void reset_query();
	int reposition_window(int x, int y, int w = -1, int h = -1);
	BC_Pixmap* get_bg_surface();

private:
	void delete_columns();
	void set_columns(char **column_titles, 
		int *column_widths, 
		int columns);
	int draw_face();
	int draw_items();
	int draw_border();
	int draw_rectangle();
// Returns 1 if selection changed
	int query_list();
	void init_column_width();
	void reset_cursor(int new_cursor);
// Fix boundary conditions after resize
	void column_width_boundaries();
	int test_divisions(int cursor_x, int cursor_y, int &new_cursor);

	int get_title_h();
	int calculate_item_coords();
	int get_items_width();
	int get_items_height();
	int get_icon_w(int column, int item);
	int get_icon_h(int column, int item);
	int get_item_x(int column, int item);
	int get_item_y(int column, int item);
	int get_item_w(int column, int item);
	int get_item_h(int column, int item);
	int get_item_highlight(int column, int item);
	int get_item_color(int column, int item);
	int get_icon_mask(int column, int item, int &x, int &y, int &w, int &h);
	int get_text_mask(int column, int item, int &x, int &y, int &w, int &h);
	BC_Pixmap* get_item_pixmap(int item);
// Copy sections of the bg_surface to the gui
	void clear_listbox(int x, int y, int w, int h);
	void test_drag_scroll(int &redraw, int cursor_x, int cursor_y);
	void move_vertical(int pixels);
	void move_horizontal(int pixels);
	void fix_positions();

	int get_scrollbars();
	void update_scrollbars();

// Item the cursor is over
	int cursor_item(int cursor_x, int cursor_y);
	int cursor_inside_item(int row, 
		int column, 
		int x1, 
		int y1,
		int x2, 
		int y2);

	void center_selection(int selection);

// Array of one list of pointers for each column
	ArrayList<BC_ListBoxItem*> *data;
// Mode
	int popup;
// Dimensions for a popup if there is one
	int popup_w, popup_h;
// pixel of top of display relative to top of list
	int yposition;
// pixel of left display relative to first column
	int xposition;
// dimensions of a row in the list
	int row_height, row_ascent, row_descent;
// item cursor is over
	int highlighted_item;
// double click eliminator
	int last_selection1, last_selection2;
	int highlighted;
	int selection_mode;
	int display_format;
	int icon_position;
// Scrollbars are created as needed
	BC_ListBoxXScroll *xscrollbar;
	BC_ListBoxYScroll *yscrollbar;
	char query[BCTEXTLEN];
// Window containing the listbox
	BC_WindowBase *gui;
// Size of the popup if there is one
	char **column_titles;
	int *column_width;
	int default_column_width[1];
	int columns;
	int items_per_column;
	int view_h, view_w;
	int title_h;
	int active;
	int new_value;
	int need_xscroll, need_yscroll;
	int allow_drag;
	int allow_drag_scroll;
// Item being dragged
	int selection;
// Selection range being extended
	int selection_start, selection_end, selection_center;
// Background color of listbox
	int list_background;
	BC_Pixmap *bg_tile;
// Drag icon for text mode
	BC_Pixmap *drag_icon;
// Popup button
	BC_Pixmap *images[3];
// Background for drawing on
	BC_Pixmap *bg_surface;
// Default background picon
	BC_Pixmap *bg_pixmap;
	int status;
	int button_releases;
	int current_cursor;
// Starting coordinates of rectangle
	int rect_x1, rect_y1;
	int rect_x2, rect_y2;
// Gradually being expanded into a state of the list box.
	int current_operation;
// Division the cursor is operating on when resizing
	int highlighted_division;
// Window for dragging
	BC_DragWindow *drag_popup;
};




#endif
