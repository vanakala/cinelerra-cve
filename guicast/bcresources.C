#include "bcdisplayinfo.h"
#include "bcipc.h"
#include "bclistbox.inc"
#include "bcresources.h"
#include "bcsignals.h"
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
char* BC_Resources::medium_font = N_("-*-helvetica-bold-r-normal-*-14-*");
char* BC_Resources::large_font = N_("-*-helvetica-bold-r-normal-*-18-*");

char* BC_Resources::small_fontset = "6x12,*";
char* BC_Resources::medium_fontset = "7x14,*";
char* BC_Resources::large_fontset = "8x16,*";

char* BC_Resources::small_font_xft = N_("-microsoft-verdana-*-*-*-*-*-*-*-*-*-*-*-*");
char* BC_Resources::medium_font_xft = N_("-microsoft-verdana-*-*-*-*-*-*-*-*-*-*-*-*");
char* BC_Resources::large_font_xft = N_("-microsoft-verdana-*-*-*-*-*-*-*-*-*-*-*-*");

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
	char string[1024];
	XGetErrorText(event->display, event->error_code, string, 1024);
//	printf("BC_Resources::x_error_handler: %s\n", string);
	BC_Resources::error = 1;
	return 0;
}



BC_Resources::BC_Resources()
{
	display_info = new BC_DisplayInfo("", 0);

#ifdef HAVE_XFT
	XftInitFtLibrary();
#endif

	use_xvideo = 1;
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
#include "images/checkbox_down_png.h"
#include "images/checkbox_checkedhi_png.h"
#include "images/checkbox_up_png.h"
#include "images/checkbox_uphi_png.h"
	static VFrame* default_checkbox_images[] =  
	{
		new VFrame(checkbox_up_png),
		new VFrame(checkbox_uphi_png),
		new VFrame(checkbox_checked_png),
		new VFrame(checkbox_down_png),
		new VFrame(checkbox_checkedhi_png)
	};

#include "images/radial_checked_png.h"
#include "images/radial_down_png.h"
#include "images/radial_checkedhi_png.h"
#include "images/radial_up_png.h"
#include "images/radial_uphi_png.h"
	static VFrame* default_radial_images[] =  
	{
		new VFrame(radial_up_png),
		new VFrame(radial_uphi_png),
		new VFrame(radial_checked_png),
		new VFrame(radial_down_png),
		new VFrame(radial_checkedhi_png)
	};

	static VFrame* default_label_images[] =  
	{
		new VFrame(radial_up_png),
		new VFrame(radial_uphi_png),
		new VFrame(radial_checked_png),
		new VFrame(radial_down_png),
		new VFrame(radial_checkedhi_png)
	};


#include "images/file_text_up_png.h"
#include "images/file_text_uphi_png.h"
#include "images/file_text_dn_png.h"
#include "images/file_icons_up_png.h"
#include "images/file_icons_uphi_png.h"
#include "images/file_icons_dn_png.h"
#include "images/file_newfolder_up_png.h"
#include "images/file_newfolder_uphi_png.h"
#include "images/file_newfolder_dn_png.h"
#include "images/file_updir_up_png.h"
#include "images/file_updir_uphi_png.h"
#include "images/file_updir_dn_png.h"
	static VFrame* default_filebox_text_images[] = 
	{
		new VFrame(file_text_up_png),
		new VFrame(file_text_uphi_png),
		new VFrame(file_text_dn_png)
	};

	static VFrame* default_filebox_icons_images[] = 
	{
		new VFrame(file_icons_up_png),
		new VFrame(file_icons_uphi_png),
		new VFrame(file_icons_dn_png)
	};

	static VFrame* default_filebox_updir_images[] =  
	{
		new VFrame(file_updir_up_png),
		new VFrame(file_updir_uphi_png),
		new VFrame(file_updir_dn_png)
	};

	static VFrame* default_filebox_newfolder_images[] = 
	{
		new VFrame(file_newfolder_up_png),
		new VFrame(file_newfolder_uphi_png),
		new VFrame(file_newfolder_dn_png)
	};

#include "images/listbox_button_dn_png.h"
#include "images/listbox_button_hi_png.h"
#include "images/listbox_button_up_png.h"
	static VFrame* default_listbox_button[] = 
	{
		new VFrame(listbox_button_up_png),
		new VFrame(listbox_button_hi_png),
		new VFrame(listbox_button_dn_png)
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









#include "images/horizontal_slider_bg_up_png.h"
#include "images/horizontal_slider_bg_hi_png.h"
#include "images/horizontal_slider_bg_dn_png.h"
#include "images/horizontal_slider_fg_up_png.h"
#include "images/horizontal_slider_fg_hi_png.h"
#include "images/horizontal_slider_fg_dn_png.h"
	static VFrame *default_horizontal_slider_data[] = 
	{
		new VFrame(horizontal_slider_fg_up_png),
		new VFrame(horizontal_slider_fg_hi_png),
		new VFrame(horizontal_slider_fg_dn_png),
		new VFrame(horizontal_slider_bg_up_png),
		new VFrame(horizontal_slider_bg_hi_png),
		new VFrame(horizontal_slider_bg_dn_png),
	};

#include "images/vertical_slider_bg_up_png.h"
#include "images/vertical_slider_bg_hi_png.h"
#include "images/vertical_slider_bg_dn_png.h"
#include "images/vertical_slider_fg_up_png.h"
#include "images/vertical_slider_fg_hi_png.h"
#include "images/vertical_slider_fg_dn_png.h"
	static VFrame *default_vertical_slider_data[] = 
	{
		new VFrame(vertical_slider_fg_up_png),
		new VFrame(vertical_slider_fg_hi_png),
		new VFrame(vertical_slider_fg_dn_png),
		new VFrame(vertical_slider_bg_up_png),
		new VFrame(vertical_slider_bg_hi_png),
		new VFrame(vertical_slider_bg_dn_png),
	};
	horizontal_slider_data = default_horizontal_slider_data;
	vertical_slider_data = default_vertical_slider_data;

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


#include "images/pan_up_png.h"
#include "images/pan_hi_png.h"
#include "images/pan_popup_png.h"
#include "images/pan_channel_png.h"
#include "images/pan_stick_png.h"
#include "images/pan_channel_small_png.h"
#include "images/pan_stick_small_png.h"
	static VFrame* default_pan_data[] = 
	{
		new VFrame(pan_up_png),
		new VFrame(pan_hi_png),
		new VFrame(pan_popup_png),
		new VFrame(pan_channel_png),
		new VFrame(pan_stick_png),
		new VFrame(pan_channel_small_png),
		new VFrame(pan_stick_small_png)
	};
	pan_data = default_pan_data;
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

#include "images/tumble_bottomdn_png.h"
#include "images/tumble_topdn_png.h"
#include "images/tumble_hi_png.h"
#include "images/tumble_up_png.h"
	static VFrame* default_tumbler_data[] = 
	{
		new VFrame(tumble_up_png),
		new VFrame(tumble_hi_png),
		new VFrame(tumble_bottomdn_png),
		new VFrame(tumble_topdn_png)
	};

#include "images/xmeter_normal_png.h"
#include "images/xmeter_green_png.h"
#include "images/xmeter_red_png.h"
#include "images/xmeter_yellow_png.h"
#include "images/over_horiz_png.h"
#include "images/ymeter_normal_png.h"
#include "images/ymeter_green_png.h"
#include "images/ymeter_red_png.h"
#include "images/ymeter_yellow_png.h"
#include "images/over_vertical_png.h"
	static VFrame* default_xmeter_data[] =
	{
		new VFrame(xmeter_normal_png),
		new VFrame(xmeter_green_png),
		new VFrame(xmeter_red_png),
		new VFrame(xmeter_yellow_png),
		new VFrame(over_horiz_png)
	};

	static VFrame* default_ymeter_data[] =
	{
		new VFrame(ymeter_normal_png),
		new VFrame(ymeter_green_png),
		new VFrame(ymeter_red_png),
		new VFrame(ymeter_yellow_png),
		new VFrame(over_vertical_png)
	};

#include "images/generic_up_png.h"
#include "images/generic_hi_png.h"
#include "images/generic_dn_png.h"
	
	static VFrame* default_generic_button_data[] = 
	{
		new VFrame(generic_up_png),
		new VFrame(generic_hi_png),
		new VFrame(generic_dn_png)
	};
	
	generic_button_images = default_generic_button_data;




#include "images/hscroll_handle_up_png.h"
#include "images/hscroll_handle_hi_png.h"
#include "images/hscroll_handle_dn_png.h"
#include "images/hscroll_handle_bg_png.h"
#include "images/hscroll_left_up_png.h"
#include "images/hscroll_left_hi_png.h"
#include "images/hscroll_left_dn_png.h"
#include "images/hscroll_right_up_png.h"
#include "images/hscroll_right_hi_png.h"
#include "images/hscroll_right_dn_png.h"
#include "images/vscroll_handle_up_png.h"
#include "images/vscroll_handle_hi_png.h"
#include "images/vscroll_handle_dn_png.h"
#include "images/vscroll_handle_bg_png.h"
#include "images/vscroll_left_up_png.h"
#include "images/vscroll_left_hi_png.h"
#include "images/vscroll_left_dn_png.h"
#include "images/vscroll_right_up_png.h"
#include "images/vscroll_right_hi_png.h"
#include "images/vscroll_right_dn_png.h"
	static VFrame *default_hscroll_data[] = 
	{
		new VFrame(hscroll_handle_up_png), 
		new VFrame(hscroll_handle_hi_png), 
		new VFrame(hscroll_handle_dn_png), 
		new VFrame(hscroll_handle_bg_png), 
		new VFrame(hscroll_left_up_png), 
		new VFrame(hscroll_left_hi_png), 
		new VFrame(hscroll_left_dn_png), 
		new VFrame(hscroll_right_up_png), 
		new VFrame(hscroll_right_hi_png), 
		new VFrame(hscroll_right_dn_png)
	};
	static VFrame *default_vscroll_data[] = 
	{
		new VFrame(vscroll_handle_up_png), 
		new VFrame(vscroll_handle_hi_png), 
		new VFrame(vscroll_handle_dn_png), 
		new VFrame(vscroll_handle_bg_png), 
		new VFrame(vscroll_left_up_png), 
		new VFrame(vscroll_left_hi_png), 
		new VFrame(vscroll_left_dn_png), 
		new VFrame(vscroll_right_up_png), 
		new VFrame(vscroll_right_hi_png), 
		new VFrame(vscroll_right_dn_png)
	};
	hscroll_data = default_hscroll_data;
	vscroll_data = default_vscroll_data;




	use_shm = -1;

// Initialize
	bg_color = MEGREY;
	bg_shadow1 = DKGREY;
	bg_shadow2 = BLACK;
	bg_light1 = WHITE;
	bg_light2 = bg_color;

	button_light = WHITE;           // bright corner
	button_highlighted = LTGREY;  // face when highlighted
	button_down = MDGREY;         // face when down
	button_up = MEGREY;           // face when up
	button_shadow = DKGREY;       // dark corner

	tumble_data = default_tumbler_data;
	tumble_duration = 150;

	ok_images = default_ok_images;
	cancel_images = default_cancel_images;
	usethis_button_images = default_usethis_images;

	checkbox_images = default_checkbox_images;
	radial_images = default_radial_images;
	label_images = default_label_images;

	menu_light = LTCYAN;
	menu_highlighted = LTBLUE;
	menu_down = MDCYAN;
	menu_up = MECYAN;
	menu_shadow = DKCYAN;

	text_default = BLACK;
	text_background = WHITE;
	highlight_inverse = WHITE ^ BLUE;
	text_highlight = BLUE;

// Delays must all be different for repeaters
	double_click = 300;
	blink_rate = 250;
	scroll_repeat = 150;
	tooltip_delay = 1000;
	tooltip_bg_color = YELLOW;
	tooltips_enabled = 1;

	filebox_mode = LISTBOX_TEXT;
	sprintf(filebox_filter, "*");
	filebox_w = 640;
	filebox_h = 480;
	filebox_columntype[0] = FILEBOX_NAME;
	filebox_columntype[1] = FILEBOX_SIZE;
	filebox_columntype[2] = FILEBOX_DATE;
	filebox_columnwidth[0] = 200;
	filebox_columnwidth[1] = 100;
	filebox_columnwidth[2] = 100;

	filebox_text_images = default_filebox_text_images;
	filebox_icons_images = default_filebox_icons_images;
	filebox_updir_images = default_filebox_updir_images;
	filebox_newfolder_images = default_filebox_newfolder_images;

	filebox_sortcolumn = 0;
	filebox_sortorder = BC_ListBox::SORT_ASCENDING;


	pot_images = default_pot_images;
	pot_x1 = pot_images[0]->get_w() / 2 - 2;
	pot_y1 = pot_images[0]->get_h() / 2 - 2;;
	pot_r = pot_x1;

	progress_images = default_progress_images;

	xmeter_images = default_xmeter_data;
	ymeter_images = default_ymeter_data;
	meter_font = SMALLFONT_3D;
	meter_font_color = RED;
	meter_title_w = 20;
	meter_3d = 1;
	medium_7segment = default_medium_7segment;


	use_fontset = 0;

// Xft has priority over font set
#ifdef HAVE_XFT
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
	bc_init_ipc();

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

