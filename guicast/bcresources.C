
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

#include "bcdisplayinfo.h"
#include "bcipc.h"
#include "bclistbox.inc"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bcwindowbase.h"
#include "colors.h"
#include "colormodels.h"
#include "fonts.h"
#include "language.h"
#include "vframe.h"

#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <unistd.h>
 




int BC_Resources::error = 0;

VFrame* BC_Resources::bg_image = 0;
VFrame* BC_Resources::menu_bg = 0;

#include "images/file_film_png.h"
#include "images/file_folder_png.h"
#include "images/file_sound_png.h"
#include "images/file_unknown_png.h"
#include "images/file_column_png.h"
VFrame* BC_Resources::type_to_icon[] = 
{
	new VFrame(file_folder_png),
	new VFrame(file_unknown_png),
	new VFrame(file_film_png),
	new VFrame(file_sound_png),
	new VFrame(file_column_png)
};

char* BC_Resources::small_font = N_("-*-helvetica-medium-r-normal-*-10-*");
char* BC_Resources::small_font2 = N_("-*-helvetica-medium-r-normal-*-11-*");
char* BC_Resources::medium_font = N_("-*-helvetica-bold-r-normal-*-14-*");
char* BC_Resources::medium_font2 = N_("-*-helvetica-bold-r-normal-*-14-*");
char* BC_Resources::large_font = N_("-*-helvetica-bold-r-normal-*-18-*");
char* BC_Resources::large_font2 = N_("-*-helvetica-bold-r-normal-*-20-*");

char* BC_Resources::small_fontset = "6x12,*";
char* BC_Resources::medium_fontset = "7x14,*";
char* BC_Resources::large_fontset = "8x16,*";

char* BC_Resources::small_font_xft = N_("-*-luxi sans-*-r-*-*-12-*-*-*-*-*-*-*");
char* BC_Resources::small_font_xft2 = N_("-microsoft-verdana-*-*-*-*-*-*-*-*-*-*-*-*");
char* BC_Resources::medium_font_xft = N_("-*-luxi sans-*-r-*-*-16-*-*-*-*-*-*-*");
char* BC_Resources::medium_font_xft2 = N_("-microsoft-verdana-*-*-*-*-*-*-*-*-*-*-*-*");
char* BC_Resources::large_font_xft = N_("-*-luxi sans-bold-r-*-*-20-*-*-*-*-*-*-*");
char* BC_Resources::large_font_xft2 = N_("-microsoft-verdana-*-*-*-*-*-*-*-*-*-*-*-*");

suffix_to_type_t BC_Resources::suffix_to_type[] = 
{
	{ "m2v", ICON_FILM },
	{ "mov", ICON_FILM },
	{ "mp2", ICON_SOUND },
	{ "mp3", ICON_SOUND },
	{ "mpg", ICON_FILM },
	{ "vob", ICON_FILM },
	{ "wav", ICON_SOUND }
};

BC_Signals* BC_Resources::signal_handler = 0;


int BC_Resources::x_error_handler(Display *display, XErrorEvent *event)
{
// 	char string[1024];
// 	XGetErrorText(event->display, event->error_code, string, 1024);
// 	printf("BC_Resources::x_error_handler: error_code=%d opcode=%d,%d %s\n", 
// 		event->error_code, 
// 		event->request_code,
// 		event->minor_code,
// 		string);


	BC_Resources::error = 1;
// This bug only happens in 32 bit mode.
	if(sizeof(long) == 4)
		BC_WindowBase::get_resources()->use_xft = 0;
	return 0;
}



BC_Resources::BC_Resources()
{
	synchronous = 0;
	display_info = new BC_DisplayInfo("", 0);
	id_lock = new Mutex("BC_Resources::id_lock");
	create_window_lock = new Mutex("BC_Resources::create_window_lock", 1);
	id = 0;

	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
		filebox_history[i][0] = 0;

#ifdef HAVE_XFT
	XftInitFtLibrary();
#endif

	use_xvideo = 1;


#include "images/bar_png.h"
	static VFrame* default_bar = new VFrame(bar_png);
	bar_data = default_bar;



#include "images/cancel_up_png.h"
#include "images/cancel_hi_png.h"
#include "images/cancel_dn_png.h"
	static VFrame* default_cancel_images[] = 
	{
		new VFrame(cancel_up_png),
		new VFrame(cancel_hi_png),
		new VFrame(cancel_dn_png)
	};

#include "images/ok_up_png.h"
#include "images/ok_hi_png.h"
#include "images/ok_dn_png.h"
	static VFrame* default_ok_images[] = 
	{
		new VFrame(ok_up_png),
		new VFrame(ok_hi_png),
		new VFrame(ok_dn_png)
	};

#include "images/usethis_up_png.h"
#include "images/usethis_uphi_png.h"
#include "images/usethis_dn_png.h"
	static VFrame* default_usethis_images[] = 
	{
		new VFrame(usethis_up_png),
		new VFrame(usethis_uphi_png),
		new VFrame(usethis_dn_png)
	};


#include "images/checkbox_checked_png.h"
#include "images/checkbox_dn_png.h"
#include "images/checkbox_checkedhi_png.h"
#include "images/checkbox_up_png.h"
#include "images/checkbox_hi_png.h"
	static VFrame* default_checkbox_images[] =  
	{
		new VFrame(checkbox_up_png),
		new VFrame(checkbox_hi_png),
		new VFrame(checkbox_checked_png),
		new VFrame(checkbox_dn_png),
		new VFrame(checkbox_checkedhi_png)
	};

#include "images/radial_checked_png.h"
#include "images/radial_dn_png.h"
#include "images/radial_checkedhi_png.h"
#include "images/radial_up_png.h"
#include "images/radial_hi_png.h"
	static VFrame* default_radial_images[] =  
	{
		new VFrame(radial_up_png),
		new VFrame(radial_hi_png),
		new VFrame(radial_checked_png),
		new VFrame(radial_dn_png),
		new VFrame(radial_checkedhi_png)
	};

	static VFrame* default_label_images[] =  
	{
		new VFrame(radial_up_png),
		new VFrame(radial_hi_png),
		new VFrame(radial_checked_png),
		new VFrame(radial_dn_png),
		new VFrame(radial_checkedhi_png)
	};


#include "images/file_text_up_png.h"
#include "images/file_text_hi_png.h"
#include "images/file_text_dn_png.h"
#include "images/file_icons_up_png.h"
#include "images/file_icons_hi_png.h"
#include "images/file_icons_dn_png.h"
#include "images/file_newfolder_up_png.h"
#include "images/file_newfolder_hi_png.h"
#include "images/file_newfolder_dn_png.h"
#include "images/file_updir_up_png.h"
#include "images/file_updir_hi_png.h"
#include "images/file_updir_dn_png.h"
#include "images/file_delete_up_png.h"
#include "images/file_delete_hi_png.h"
#include "images/file_delete_dn_png.h"
#include "images/file_reload_up_png.h"
#include "images/file_reload_hi_png.h"
#include "images/file_reload_dn_png.h"
	static VFrame* default_filebox_text_images[] = 
	{
		new VFrame(file_text_up_png),
		new VFrame(file_text_hi_png),
		new VFrame(file_text_dn_png)
	};

	static VFrame* default_filebox_icons_images[] = 
	{
		new VFrame(file_icons_up_png),
		new VFrame(file_icons_hi_png),
		new VFrame(file_icons_dn_png)
	};

	static VFrame* default_filebox_updir_images[] =  
	{
		new VFrame(file_updir_up_png),
		new VFrame(file_updir_hi_png),
		new VFrame(file_updir_dn_png)
	};

	static VFrame* default_filebox_newfolder_images[] = 
	{
		new VFrame(file_newfolder_up_png),
		new VFrame(file_newfolder_hi_png),
		new VFrame(file_newfolder_dn_png)
	};

	static VFrame* default_filebox_delete_images[] = 
	{
		new VFrame(file_delete_up_png),
		new VFrame(file_delete_hi_png),
		new VFrame(file_delete_dn_png)
	};

	static VFrame* default_filebox_reload_images[] =
	{
		new VFrame(file_reload_up_png),
		new VFrame(file_reload_hi_png),
		new VFrame(file_reload_dn_png)
	};

#include "images/listbox_button_dn_png.h"
#include "images/listbox_button_hi_png.h"
#include "images/listbox_button_up_png.h"
#include "images/listbox_button_disabled_png.h"
	static VFrame* default_listbox_button[] = 
	{
		new VFrame(listbox_button_up_png),
		new VFrame(listbox_button_hi_png),
		new VFrame(listbox_button_dn_png),
		new VFrame(listbox_button_disabled_png)
	};
	listbox_button = default_listbox_button;

#include "images/list_bg_png.h"
	static VFrame* default_listbox_bg = new VFrame(list_bg_png);
	listbox_bg = default_listbox_bg;

#include "images/listbox_expandchecked_png.h"
#include "images/listbox_expandcheckedhi_png.h"
#include "images/listbox_expanddn_png.h"
#include "images/listbox_expandup_png.h"
#include "images/listbox_expanduphi_png.h"
	static VFrame* default_listbox_expand[] = 
	{
		new VFrame(listbox_expandup_png),
		new VFrame(listbox_expanduphi_png),
		new VFrame(listbox_expandchecked_png),
		new VFrame(listbox_expanddn_png),
		new VFrame(listbox_expandcheckedhi_png),
	};
	listbox_expand = default_listbox_expand;

#include "images/listbox_columnup_png.h"
#include "images/listbox_columnhi_png.h"
#include "images/listbox_columndn_png.h"
	static VFrame* default_listbox_column[] = 
	{
		new VFrame(listbox_columnup_png),
		new VFrame(listbox_columnhi_png),
		new VFrame(listbox_columndn_png)
	};
	listbox_column = default_listbox_column;


#include "images/listbox_up_png.h"
#include "images/listbox_dn_png.h"
	listbox_up = new VFrame(listbox_up_png);
	listbox_dn = new VFrame(listbox_dn_png);
	listbox_title_margin = 0;
	listbox_title_color = BLACK;
	listbox_title_hotspot = 5;
	
	listbox_border1 = DKGREY;
	listbox_border2_hi = RED;
	listbox_border2 = BLACK;
	listbox_border3_hi = RED;
	listbox_border3 = MEGREY;
	listbox_border4 = WHITE;
	listbox_selected = BLUE;
	listbox_highlighted = LTGREY;
	listbox_inactive = WHITE;
	listbox_text = BLACK;

#include "images/pot_hi_png.h"
#include "images/pot_up_png.h"
#include "images/pot_dn_png.h"
	static VFrame *default_pot_images[] = 
	{
		new VFrame(pot_up_png),
		new VFrame(pot_hi_png),
		new VFrame(pot_dn_png)
	};

#include "images/progress_up_png.h"
#include "images/progress_hi_png.h"
	static VFrame* default_progress_images[] = 
	{
		new VFrame(progress_up_png),
		new VFrame(progress_hi_png)
	};

	pan_data = 0;
	pan_text_color = YELLOW;

#include "images/7seg_small/0_png.h"
#include "images/7seg_small/1_png.h"
#include "images/7seg_small/2_png.h"
#include "images/7seg_small/3_png.h"
#include "images/7seg_small/4_png.h"
#include "images/7seg_small/5_png.h"
#include "images/7seg_small/6_png.h"
#include "images/7seg_small/7_png.h"
#include "images/7seg_small/8_png.h"
#include "images/7seg_small/9_png.h"
#include "images/7seg_small/colon_png.h"
#include "images/7seg_small/period_png.h"
#include "images/7seg_small/a_png.h"
#include "images/7seg_small/b_png.h"
#include "images/7seg_small/c_png.h"
#include "images/7seg_small/d_png.h"
#include "images/7seg_small/e_png.h"
#include "images/7seg_small/f_png.h"
#include "images/7seg_small/space_png.h"
#include "images/7seg_small/dash_png.h"
	static VFrame* default_medium_7segment[] = 
	{
		new VFrame(_0_png),
		new VFrame(_1_png),
		new VFrame(_2_png),
		new VFrame(_3_png),
		new VFrame(_4_png),
		new VFrame(_5_png),
		new VFrame(_6_png),
		new VFrame(_7_png),
		new VFrame(_8_png),
		new VFrame(_9_png),
		new VFrame(colon_png),
		new VFrame(period_png),
		new VFrame(a_png),
		new VFrame(b_png),
		new VFrame(c_png),
		new VFrame(d_png),
		new VFrame(e_png),
		new VFrame(f_png),
		new VFrame(space_png),
		new VFrame(dash_png)
	};

	generic_button_margin = 15;
	draw_clock_background=1;

	use_shm = -1;

// Initialize
	bg_color = ORANGE;
	bg_shadow1 = DKGREY;
	bg_shadow2 = BLACK;
	bg_light1 = WHITE;
	bg_light2 = bg_color;

	default_text_color = BLACK;
	disabled_text_color = MEGREY;

	button_light = WHITE;           // bright corner
	button_highlighted = 0xffe000;  // face when highlighted
	button_down = MDGREY;         // face when down
	button_up = 0xffc000;           // face when up
	button_shadow = DKGREY;       // dark corner
	button_uphighlighted = RED;   // upper side when highlighted

	tumble_data = 0;
	tumble_duration = 150;

	ok_images = default_ok_images;
	cancel_images = default_cancel_images;
	usethis_button_images = default_usethis_images;
	filebox_descend_images = default_ok_images;

	menu_light = LTCYAN;
	menu_highlighted = LTBLUE;
	menu_down = MDCYAN;
	menu_up = MECYAN;
	menu_shadow = DKCYAN;
	menu_popup_bg = 0;
	menu_title_bg = 0;
	menu_item_bg = 0;
	menu_bar_bg = 0;
	menu_title_bg = 0;
	popupmenu_images = 0;
	popupmenu_margin = 10;
	popupmenu_triangle_margin = 10;

	min_menu_w = 0;
	menu_title_text = BLACK;
	popup_title_text = BLACK;
	menu_item_text = BLACK;
	menu_highlighted_fontcolor = BLACK;
	progress_text = BLACK;



	text_default = BLACK;
	highlight_inverse = WHITE ^ BLUE;
	text_background = WHITE;
	text_background_hi = LTYELLOW;
	text_background_noborder_hi = LTGREY;
	text_background_noborder = -1;
	text_border1 = DKGREY;
	text_border2 = BLACK;
	text_border2_hi = RED;
	text_border3 = MEGREY;
	text_border3_hi = LTPINK;
	text_border4 = WHITE;
	text_highlight = BLUE;
	text_inactive_highlight = MEGREY;

	toggle_highlight_bg = 0;
	toggle_text_margin = 0;

// Delays must all be different for repeaters
	double_click = 300;
	blink_rate = 250;
	scroll_repeat = 150;
	tooltip_delay = 1000;
	tooltip_bg_color = YELLOW;
	tooltips_enabled = 1;

	filebox_margin = 110;
	dirbox_margin = 90;
	filebox_mode = LISTBOX_TEXT;
	sprintf(filebox_filter, "*");
	filebox_w = 640;
	filebox_h = 480;
	filebox_columntype[0] = FILEBOX_NAME;
	filebox_columntype[1] = FILEBOX_SIZE;
	filebox_columntype[2] = FILEBOX_DATE;
	filebox_columntype[3] = FILEBOX_EXTENSION;
	filebox_columnwidth[0] = 200;
	filebox_columnwidth[1] = 100;
	filebox_columnwidth[2] = 100;
	filebox_columnwidth[3] = 100;
	dirbox_columntype[0] = FILEBOX_NAME;
	dirbox_columntype[1] = FILEBOX_DATE;
	dirbox_columnwidth[0] = 200;
	dirbox_columnwidth[1] = 100;

	filebox_text_images = default_filebox_text_images;
	filebox_icons_images = default_filebox_icons_images;
	filebox_updir_images = default_filebox_updir_images;
	filebox_newfolder_images = default_filebox_newfolder_images;
	filebox_delete_images = default_filebox_delete_images;
	filebox_reload_images = default_filebox_reload_images;
	directory_color = BLUE;
	file_color = BLACK;

	filebox_sortcolumn = 0;
	filebox_sortorder = BC_ListBox::SORT_ASCENDING;
	dirbox_sortcolumn = 0;
	dirbox_sortorder = BC_ListBox::SORT_ASCENDING;


	pot_images = default_pot_images;
	pot_offset = 2;
	pot_x1 = pot_images[0]->get_w() / 2 - pot_offset;
	pot_y1 = pot_images[0]->get_h() / 2 - pot_offset;
	pot_r = pot_x1;
	pot_needle_color = BLACK;

	progress_images = default_progress_images;

	xmeter_images = 0;
	ymeter_images = 0;
	meter_font = SMALLFONT_3D;
	meter_font_color = RED;
	meter_title_w = 20;
	meter_3d = 1;
	medium_7segment = default_medium_7segment;

	audiovideo_color = RED;

	use_fontset = 0;

// Xft has priority over font set
#ifdef HAVE_XFT
// But Xft dies in 32 bit mode after some amount of drawing.
	use_xft = 1;
#else
	use_xft = 0;
#endif


	drag_radius = 10;
	recursive_resizing = 1;

	
}

BC_Resources::~BC_Resources()
{
}

int BC_Resources::initialize_display(BC_WindowBase *window)
{
// Set up IPC cleanup handlers
//	bc_init_ipc();

// Test for shm.  Must come before yuv test
	init_shm(window);
	return 0;
}


int BC_Resources::init_shm(BC_WindowBase *window)
{
	use_shm = 1;
	XSetErrorHandler(BC_Resources::x_error_handler);

	if(!XShmQueryExtension(window->display)) use_shm = 0;
	else
	{
		XShmSegmentInfo test_shm;
		XImage *test_image;
		unsigned char *data;
		test_image = XShmCreateImage(window->display, window->vis, window->default_depth,
                ZPixmap, (char*)NULL, &test_shm, 5, 5);

		test_shm.shmid = shmget(IPC_PRIVATE, 5 * test_image->bytes_per_line, (IPC_CREAT | 0777 ));
		data = (unsigned char *)shmat(test_shm.shmid, NULL, 0);
    	shmctl(test_shm.shmid, IPC_RMID, 0);
		BC_Resources::error = 0;
 	   	XShmAttach(window->display, &test_shm);
    	XSync(window->display, False);
		if(BC_Resources::error) use_shm = 0;
		XDestroyImage(test_image);
		shmdt(test_shm.shmaddr);
	}
//	XSetErrorHandler(0);
	return 0;
}




BC_Synchronous* BC_Resources::get_synchronous()
{
	return synchronous;
}

void BC_Resources::set_synchronous(BC_Synchronous *synchronous)
{
	this->synchronous = synchronous;
}







int BC_Resources::get_top_border()
{
	return display_info->get_top_border();
}

int BC_Resources::get_left_border()
{
	return display_info->get_left_border();
}

int BC_Resources::get_right_border()
{
	return display_info->get_right_border();
}

int BC_Resources::get_bottom_border()
{
	return display_info->get_bottom_border();
}


int BC_Resources::get_bg_color() { return bg_color; }

int BC_Resources::get_bg_shadow1() { return bg_shadow1; }

int BC_Resources::get_bg_shadow2() { return bg_shadow2; }

int BC_Resources::get_bg_light1() { return bg_light1; }

int BC_Resources::get_bg_light2() { return bg_light2; }


int BC_Resources::get_id()
{
	id_lock->lock("BC_Resources::get_id");
	int result = id++;
	id_lock->unlock();
	return result;
}


void BC_Resources::set_signals(BC_Signals *signal_handler)
{
	BC_Resources::signal_handler = signal_handler;
}

BC_Signals* BC_Resources::get_signals()
{
	return BC_Resources::signal_handler;
}
