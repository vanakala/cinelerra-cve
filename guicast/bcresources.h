// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCRESOURCES_H
#define BCRESOURCES_H

// Global objects for the user interface

#include "bcdisplayinfo.inc"
#include "bcfilebox.h"
#include "bcfontentry.inc"
#include "bcresources.inc"
#include "bcsignals.inc"
#include "bcwindowbase.inc"
#include "colorspaces.inc"
#include "glthread.inc"
#include "hashcache.inc"
#include "tmpframecache.inc"
#include "vframe.inc"

#include <X11/Xlib.h>

typedef struct
{
	const char *suffix;
	int icon_type;
} suffix_to_type_t;


class BC_Resources
{
public:
	BC_Resources(); // The window parameter is used to get the display information initially
	~BC_Resources();

	void initialize_display(BC_WindowBase *window);

// Get unique ID
	int get_id();
	int get_bg_color();          // window backgrounds
	int get_bg_shadow1();        // border for windows
	int get_bg_shadow2();
	int get_bg_light1();
	int get_bg_light2();
// Get window border size created by window manager
	int get_top_border();
	int get_left_border();
// Get GLThread for OpenGL
	GLThread* get_glthread();
// Called by user after synchronous thread is created.
	void set_glthread(GLThread *glthread);

// These values should be changed before the first window is created.
// colors
	int bg_color;          // window backgrounds
	int bg_shadow1;        // border for windows
	int bg_shadow2;
	int bg_light1;
	int bg_light2;
	int default_text_color;
	int disabled_text_color;

// beveled box colors
	int button_light;
	int button_highlighted;
	int button_down;
	int button_up;
	int button_shadow;
	int button_uphighlighted;

// highlighting
	int highlight_inverse;

// 3D box colors for menus
	int menu_light;
	int menu_highlighted;
	int menu_down;
	int menu_up;
	int menu_shadow;
// If these are nonzero, they override the menu backgrounds.
	VFrame *menu_popup_bg;
	VFrame **menu_title_bg;
	VFrame *menu_bar_bg;
	VFrame **popupmenu_images;

// Minimum menu width
	int min_menu_w;
// Menu bar text color
	int menu_title_text;
// color of popup title
	int popup_title_text;
// Right and left margin for text not including triangle space.
	int popupmenu_margin;
// Right margin for triangle not including text margin.
	int popupmenu_triangle_margin;
// color for item text
	int menu_item_text;
// Override the menu item background if nonzero.
	VFrame **menu_item_bg;

// color for progress text
	int progress_text;

	int menu_highlighted_fontcolor;

// ms for double click
	long double_click;
// ms for cursor flash
	int blink_rate;
// ms for scroll repeats
	int scroll_repeat;
// ms before tooltip
	int tooltip_delay;
	int tooltip_bg_color;
	int tooltips_enabled;

	int audiovideo_color;

// default color of text
	int text_default;
// background color of textboxes and list boxes
	int text_border1;
	int text_border2;
	int text_border2_hi;
	int text_background;
	int text_background_hi;
	int text_background_noborder_hi;
	int text_border3;
	int text_border3_hi;
	int text_border4;
	int text_highlight;
	int text_inactive_highlight;

// Optional background for highlighted text in toggle
	VFrame *toggle_highlight_bg;
	int toggle_text_margin;

// Background images
	static VFrame *menu_bg;

// Buttons
	VFrame **ok_images;
	VFrame **cancel_images;
	VFrame **filebox_text_images;
	VFrame **filebox_icons_images;
	VFrame **filebox_updir_images;
	VFrame **filebox_newfolder_images;
	VFrame **filebox_descend_images;
	VFrame **filebox_delete_images;
	VFrame **filebox_reload_images;

// Generic button images
	VFrame **generic_button_images;
// Generic button text margin
	int generic_button_margin;
	VFrame **usethis_button_images;

// Toggles
	VFrame **checkbox_images;
	VFrame **radial_images;
	VFrame **label_images;

	VFrame **tumble_data;
	int tumble_duration;

// Horizontal bar
	VFrame *bar_data;

// Listbox
	VFrame *listbox_bg;
	VFrame **listbox_button;
	VFrame **listbox_expand;
	VFrame **listbox_column;
	VFrame *listbox_up;
	VFrame *listbox_dn;
// Margin for titles in addition to listbox border
	int listbox_title_margin;
	int listbox_title_color;
	int listbox_title_hotspot;
	int listbox_border1;
	int listbox_border2_hi;
	int listbox_border2;
	int listbox_border3_hi;
	int listbox_border3;
	int listbox_border4;
// Selected row color
	int listbox_selected;
// Highlighted row color
	int listbox_highlighted;
// Inactive row color
	int listbox_inactive;
// Default text color
	int listbox_text;

// Sliders
	VFrame **horizontal_slider_data;
	VFrame **vertical_slider_data;
	VFrame **hscroll_data;

	VFrame **vscroll_data;
// Minimum pixels in handle
	int scroll_minhandle;

// Pans
	VFrame **pan_data;
	int pan_text_color;

// Pots
	VFrame **pot_images;
	int pot_x1, pot_y1, pot_r;
// Amount of deflection of pot when down
	int pot_offset;
	int pot_needle_color;

// Meters
	VFrame **xmeter_images, **ymeter_images;
	int meter_font;
	int meter_font_color;
	int meter_title_w;
	int meter_3d;

// Progress bar
	VFrame **progress_images;

// Motion required to start a drag
	int drag_radius;

// Filebox
	static suffix_to_type_t suffix_to_type[];
	static VFrame *type_to_icon[TOTAL_ICONS];
// Display mode for fileboxes
	int filebox_mode;
// Filter currently used in filebox
	char filebox_filter[BCTEXTLEN];
// History of submitted files
	char filebox_history[FILEBOX_HISTORY_SIZE][BCTEXTLEN];
// filebox size
	int filebox_w;
	int filebox_h;
// Column types for filebox
	int filebox_columntype[FILEBOX_COLUMNS];
	int filebox_columnwidth[FILEBOX_COLUMNS];
	int filebox_sortcolumn;
	int filebox_sortorder;
// Column types for filebox in directory mode
	int dirbox_columntype[FILEBOX_COLUMNS];
	int dirbox_columnwidth[FILEBOX_COLUMNS];
	int dirbox_sortcolumn;
	int dirbox_sortorder;
// Bottom margin between list and window
	int filebox_margin;
	int dirbox_margin;
	int directory_color;
	int file_color;

// fonts
	static const char *large_font_xft;
	static const char *medium_font_xft;
	static const char *small_font_xft;
	static const char *large_b_font_xft;
	static const char *medium_b_font_xft;
	static const char *small_b_font_xft;
	VFrame **medium_7segment;

//clock
	int draw_clock_background;

// This must be constitutive since applications access the private members here.
// Current locale is utf8
	static int locale_utf8;
// Byte order is little_endian
	static int little_endian;
// Language and region
	static char language[LEN_LANG];
	static char region[LEN_LANG];
	static char encoding[LEN_ENCOD];
	static const char *wide_encoding;
	static ArrayList<BC_FontEntry*> *fontlist;
	static int init_fontconfig(const char *search_path, int options = 0);
	static BC_FontEntry *find_fontentry(const char *displayname, int style,
		int mask, int preferred_style = 0);
	static FcPattern* find_similar_font(FT_ULong char_code, FcPattern *oldfont);
	static size_t encode(const char *from_enc, const char *to_enc,
		char *input, char *output, int output_length, int input_length = -1);
	// encode to utf8 on place
	static void encode_to_utf8(char *buffer, int buflen);
	static int find_font_by_char(FT_ULong char_code, char *path_new, const FT_Face oldface);
	static void get_abs_cursor(int *x, int *y);
	static void get_root_size(int *width, int *height);
	static void get_window_borders(int *left, int *right, int *top, int *bottom);

// Available display extensions
	int use_shm;

// If the program uses recursive_resizing
	int recursive_resizing;
// Work around X server bugs
	int use_xvideo;
// Seems to help if only 1 window is created at a time.
	Mutex *create_window_lock;
//	Cache of parameters
	static HashCache hash_cache;
// Current directory
	static char working_directory[BCTEXTLEN];
// Temporary frames
	static TmpFrameCache tmpframes;
// Size of RAM in kB
	static size_t memory_size;
// Interpolation method for scaling
	static int interpolation_method;
// OpenGL version strings (version, vendor, renderer)
	static const char *OpenGLStrings[];
// Colorspace conversion
	static ColorSpaces colorspaces;

private:
// Test for availability of shared memory pixmaps
	void init_shm(BC_WindowBase *window);
	void init_sizes(BC_WindowBase *window);
	void find_memory_size();

	static BC_DisplayInfo *display_info;
	VFrame **list_pointers[100];
	int list_lengths[100];
	int list_total;
	static const char *fc_properties[];

	Mutex *id_lock;
	static Mutex fontconfig_lock;

	GLThread *glthread;

	int id;
};

#endif
