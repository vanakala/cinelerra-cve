#include "bcdisplayinfo.h"
#include "bcipc.h"
#include "bclistbox.inc"
#include "bcresources.h"
#include "bcsignals.h"
#include "bctheme.h"
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

char* BC_Resources::small_font = N_("-*-helvetica-medium-r-normal-*-10-*");
char* BC_Resources::small_font2 = N_("-*-helvetica-medium-r-normal-*-11-*");
char* BC_Resources::medium_font = N_("-*-helvetica-bold-r-normal-*-14-*");
char* BC_Resources::medium_font2 = N_("-*-helvetica-bold-r-normal-*-14-*");
char* BC_Resources::large_font = N_("-*-helvetica-bold-r-normal-*-18-*");
char* BC_Resources::large_font2 = N_("-*-helvetica-bold-r-normal-*-20-*");

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

static VFrame* type_to_icon = 0;

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

	static VFrame* default_checkbox_images = 0;
	static VFrame* default_radial_images = 0;
	static VFrame* default_label_images =  0;

 	listbox_button = 0;
	listbox_bg = 0;

	static VFrame* default_filebox_text_images = 0; 
	static VFrame* default_filebox_icons_images = 0;
	static VFrame* default_filebox_updir_images = 0;
	static VFrame* default_filebox_newfolder_images = 0; 

	listbox_expand = 0;
	listbox_column = 0;
	listbox_up =0;
	listbox_dn = 0;

 	horizontal_slider_data = 0;
 	vertical_slider_data = 0;


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

	menu_light = LTCYAN;
	menu_highlighted = LTBLUE;
	menu_down = MDCYAN;
	menu_up = MECYAN;
	menu_shadow = DKCYAN;
	menu_popup_bg = 0;
	menu_title_bg = 0;
	menu_item_bg = 0;

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

	toggle_highlight_bg = 0;
	toggle_text_margin = 0;

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

 	filebox_text_images = 0;
 	filebox_icons_images = 0;
 	filebox_updir_images = 0;
 	filebox_newfolder_images = 0;

	filebox_sortcolumn = 0;
	filebox_sortorder = BC_ListBox::SORT_ASCENDING;

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

	listboxitemselected_color = BLUE;

	audiovideo_color = RED;

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

