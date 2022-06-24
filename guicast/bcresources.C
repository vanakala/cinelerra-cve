// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcdisplayinfo.h"
#include "bclistbox.inc"
#include "bcfontentry.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "colors.h"
#include "colormodels.inc"
#include "colorspaces.h"
#include "fonts.h"
#include "glthread.h"
#include "language.h"
#include "hashcache.h"
#include "mutex.h"
#include "tmpframecache.h"
#include "vframe.h"

#include <string.h>
#include <iconv.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>
#include <unistd.h>

VFrame* BC_Resources::menu_bg = 0;

int BC_Resources::locale_utf8 = 0;
int BC_Resources::little_endian = 0;
char BC_Resources::language[LEN_LANG] = {0};
char BC_Resources::region[LEN_LANG] = {0};
char BC_Resources::encoding[LEN_ENCOD] = {0};
const char *BC_Resources::wide_encoding = 0;
ArrayList<BC_FontEntry*> *BC_Resources::fontlist = 0;
const char *BC_Resources::fc_properties[] = { FC_SLANT, FC_WEIGHT, FC_WIDTH };
#define LEN_FCPROP (sizeof(BC_Resources::fc_properties) / sizeof(const char*))

BC_DisplayInfo *BC_Resources::display_info = 0;

// Size of RAM in kB
size_t BC_Resources::memory_size = 0;

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

const char* BC_Resources::small_font_xft = N_("Sans:pixelsize=10.66");
const char* BC_Resources::medium_font_xft = N_("Sans:pixelsize=13.33");
const char* BC_Resources::large_font_xft = N_("Sans:pixelsize=21.33");
const char* BC_Resources::small_b_font_xft = N_("Sans:bold:pixelsize=10.66");
const char* BC_Resources::medium_b_font_xft = N_("Sans:bold:pixelsize=13.33");
const char* BC_Resources::large_b_font_xft = N_("Sans:bold:pixelsize=21.33");

suffix_to_type_t BC_Resources::suffix_to_type[] = 
{
	{ "aac", ICON_SOUND },
	{ "ac3", ICON_SOUND },
	{ "dts", ICON_SOUND },
	{ "f4a", ICON_SOUND },
	{ "flac", ICON_SOUND },
	{ "m4a", ICON_SOUND },
	{ "mp2", ICON_SOUND },
	{ "mp3", ICON_SOUND },
	{ "mpc", ICON_SOUND },
	{ "oga", ICON_SOUND },
	{ "ogg", ICON_SOUND },
	{ "opus", ICON_SOUND },
	{ "ra", ICON_SOUND },
	{ "tta", ICON_SOUND },
	{ "vox", ICON_SOUND },
	{ "wav", ICON_SOUND },
	{ "wma", ICON_SOUND },
	{ "3gp", ICON_FILM },
	{ "avi", ICON_FILM },
	{ "bmp", ICON_FILM },
	{ "cr2", ICON_FILM },
	{ "dnxhd", ICON_FILM },
	{ "drc", ICON_FILM },
	{ "dv", ICON_FILM },
	{ "dvd", ICON_FILM },
	{ "exr", ICON_FILM },
	{ "f4v", ICON_FILM },
	{ "flv", ICON_FILM },
	{ "gif", ICON_FILM },
	{ "gxf", ICON_FILM },
	{ "h263", ICON_FILM },
	{ "h264", ICON_FILM },
	{ "h265", ICON_FILM },
	{ "hevc", ICON_FILM },
	{ "ifo", ICON_FILM },
	{ "jpeg", ICON_FILM },
	{ "jpg", ICON_FILM },
	{ "m2t", ICON_FILM },
	{ "m2ts", ICON_FILM },
	{ "m2v", ICON_FILM },
	{ "m4p", ICON_FILM },
	{ "m4v", ICON_FILM },
	{ "mkv", ICON_FILM },
	{ "mov", ICON_FILM },
	{ "mp4", ICON_FILM },
	{ "mpe", ICON_FILM },
	{ "mpeg", ICON_FILM },
	{ "mpg", ICON_FILM },
	{ "mpv", ICON_FILM },
	{ "mts", ICON_FILM },
	{ "mxf", ICON_FILM },
	{ "ogv", ICON_FILM },
	{ "pcm", ICON_FILM },
	{ "pgm", ICON_FILM },
	{ "png", ICON_FILM },
	{ "ppm", ICON_FILM },
	{ "qt", ICON_FILM },
	{ "rm", ICON_FILM },
	{ "rmvb", ICON_FILM },
	{ "rv", ICON_FILM },
	{ "swf", ICON_FILM },
	{ "tif", ICON_FILM },
	{ "tiff", ICON_FILM },
	{ "ts",  ICON_FILM },
	{ "vob", ICON_FILM },
	{ "vts", ICON_FILM },
	{ "webm", ICON_FILM },
	{ "webp", ICON_FILM },
	{ "wmv", ICON_FILM },
	{ "xml", ICON_FILM },
	{ "y4m", ICON_FILM },
	{ 0, 0 },
};

Mutex BC_Resources::fontconfig_lock("BC_Resources::fontconfig_lock");
HashCache BC_Resources::hash_cache;
char BC_Resources::working_directory[BCTEXTLEN];
TmpFrameCache BC_Resources::tmpframes;
int BC_Resources::interpolation_method = NEAREST_NEIGHBOR;
const char *BC_Resources::OpenGLStrings[3];
ColorSpaces BC_Resources::colorspaces;

BC_Resources::BC_Resources()
{
	glthread = 0;
	display_info = new BC_DisplayInfo("", 0);
	id_lock = new Mutex("BC_Resources::id_lock");
	create_window_lock = new Mutex("BC_Resources::create_window_lock", 1);

	id = 0;

	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
		filebox_history[i][0] = 0;

	little_endian = (*(const u_int32_t*)"\01\0\0\0") & 1;

	if(little_endian)
		wide_encoding = "UTF32LE";
	else
		wide_encoding = "UTF32BE";

	XftInitFtLibrary();

	use_xvideo = display_info->test_xvext();

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
	filebox_mode = 0;
	strcpy(filebox_filter, "*");
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

	drag_radius = 10;
	recursive_resizing = 1;
	find_memory_size();
	working_directory[0] = 0;
	if(!getcwd(working_directory, BCTEXTLEN))
	{
		perror("get current working directory");
		abort();
	}
}

BC_Resources::~BC_Resources()
{
	delete display_info;
}

void BC_Resources::find_memory_size()
{
	FILE *fp = fopen("/proc/meminfo", "r");
	char str[256];
	const char *keyw = "MemTotal:";
	int l = strlen(keyw);

	if(fp)
	{
		while(fgets(str, sizeof(str), fp))
		{
			if(strncmp(str, keyw, l) == 0)
			{
				memory_size = atoll(str + l);
				break;
			}
		}
		fclose(fp);
	}
	// Assume 512 Mb when size is not found
	if(memory_size == 0)
		memory_size = 524288;
}

void BC_Resources::initialize_display(BC_WindowBase *window)
{
// Test for shm.  Must come before yuv test
	init_shm(window);
}

void BC_Resources::init_shm(BC_WindowBase *window)
{
	use_shm = 1;

	if(!XShmQueryExtension(window->display))
		use_shm = 0;
	else
	{
		window->lock_window("BC_Resources::init_shm");
		XShmSegmentInfo test_shm;
		XImage *test_image;
		char *data;

		test_image = XShmCreateImage(window->display, window->vis, window->default_depth,
			ZPixmap, (char*)NULL, &test_shm, 5, 5);

		BC_Signals::set_catch_errors();

		test_shm.shmid = shmget(IPC_PRIVATE, 5 * test_image->bytes_per_line, (IPC_CREAT | 0600 ));
		if(test_shm.shmid != -1)
		{
			data = (char *)shmat(test_shm.shmid, NULL, 0);
			if(data != (void *)-1)
			{
				shmctl(test_shm.shmid, IPC_RMID, 0);
				test_shm.shmaddr = data;
				test_shm.readOnly = 0;

				if(XShmAttach(window->display, &test_shm))
					XSync(window->display, False);
				else
					use_shm = 0;
				shmdt(data);
			}
			else
				use_shm = 0;
		}
		else
			use_shm = 0;

		XDestroyImage(test_image);
		if(BC_Signals::reset_catch()) use_shm = 0;
		window->unlock_window();
	}
}

GLThread* BC_Resources::get_glthread()
{
	return glthread;
}

void BC_Resources::set_glthread(GLThread *glthread)
{
	this->glthread = glthread;
}

int BC_Resources::get_top_border()
{
	return display_info->get_top_border();
}

int BC_Resources::get_left_border()
{
	return display_info->get_left_border();
}

int BC_Resources::get_bg_color()
{
	return bg_color;
}

int BC_Resources::get_bg_shadow1()
{
	return bg_shadow1;
}

int BC_Resources::get_bg_shadow2()
{
	return bg_shadow2;
}

int BC_Resources::get_bg_light1()
{
	return bg_light1;
}

int BC_Resources::get_bg_light2()
{
	return bg_light2;
}

int BC_Resources::get_id()
{
	id_lock->lock("BC_Resources::get_id");
	int result = id++;
	id_lock->unlock();
	return result;
}

void BC_Resources::get_window_borders(int *left, int *right, int *top, int *bottom)
{
	*left = display_info->get_left_border();
	*right = display_info->get_right_border();
	*top = display_info->get_top_border();
	*bottom = display_info->get_bottom_border();
}

void BC_Resources::get_abs_cursor(int *x, int *y)
{
	display_info->get_abs_cursor(x, y);
}

void BC_Resources::get_root_size(int *width, int *height)
{
	display_info->get_root_size(width, height);
}

int BC_Resources::init_fontconfig(const char *search_path, int options)
{
	if(!fontlist)
	{
		FcPattern *pat;
		FcFontSet *fs;
		FcObjectSet *os;
		FcChar8 *strvalue;
		int intvalue;
		FcPattern *font;
		BC_FontEntry *entry;
		int i;

		fontlist = new ArrayList<BC_FontEntry*>;

		if(!FcInit())
			return 1;

		FcConfigAppFontAddDir(0, (const FcChar8*)search_path);
		FcConfigSetRescanInterval(0, 0);
		pat = FcPatternCreate();
		os = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_FOUNDRY, FC_WEIGHT,
			FC_WIDTH, FC_SLANT, FC_SPACING, FC_STYLE, (char *)0);

		FcPatternAddBool(pat, FC_SCALABLE, true);

		if((options & FLFC_LANGUAGE) && language[0])
		{
			char langstr[LEN_LANG * 3];
			strcpy(langstr, language);

			if(region[0])
			{
				strcat(langstr, "-");
				strcat(langstr, region);
			}

			FcLangSet *ls =  FcLangSetCreate();
			if(FcLangSetAdd(ls, (const FcChar8*)langstr))
				if(FcPatternAddLangSet(pat, FC_LANG, ls))
			FcLangSetDestroy(ls);
		}

		fs = FcFontList(0, pat, os);
		FcPatternDestroy(pat);
		FcObjectSetDestroy(os);

		for(i = 0; i < fs->nfont; i++)
		{
			font = fs->fonts[i];
			entry = new BC_FontEntry;

			if(FcPatternGetString(font, FC_FILE, 0, &strvalue) == FcResultMatch)
			{
				entry->path = new char[strlen((char*)strvalue) + 1];
				strcpy(entry->path, (char*)strvalue);
			}

			if(FcPatternGetString(font, FC_FOUNDRY, 0, &strvalue) == FcResultMatch)
			{
				entry->foundry = new char[strlen((char*)strvalue) + 1];
				strcpy(entry->foundry, (char *)strvalue);
			}

			if(FcPatternGetString(font, FC_FAMILY, 0, &strvalue) == FcResultMatch)
			{
				entry->family = new char[strlen((char*)strvalue) + 2];
				strcpy(entry->family, (char*)strvalue);
			}

			if(FcPatternGetInteger(font, FC_SLANT, 0, &intvalue) == FcResultMatch)
			{
				switch(intvalue)
				{
				case FC_SLANT_ROMAN:
				default:
					entry->style |= FL_SLANT_ROMAN;
					break;

				case FC_SLANT_ITALIC:
					entry->style |= FL_SLANT_ITALIC;
					break;

				case FC_SLANT_OBLIQUE:
					entry->style |= FL_SLANT_OBLIQUE;
					break;
				}
			}

			if(FcPatternGetInteger(font, FC_WEIGHT, 0, &intvalue) == FcResultMatch)
			{
				switch(intvalue)
				{
				case FC_WEIGHT_THIN:
					entry->style |= FL_WEIGHT_THIN;
					break;

				case FC_WEIGHT_EXTRALIGHT:
					entry->style |= FL_WEIGHT_EXTRALIGHT;
					break;

				case FC_WEIGHT_LIGHT:
					entry->style |= FL_WEIGHT_LIGHT;
					break;

				case FC_WEIGHT_BOOK:
					entry->style |= FL_WEIGHT_BOOK;
					break;

				case FC_WEIGHT_NORMAL:
				default:
					entry->style |= FL_WEIGHT_NORMAL;
					break;

				case FC_WEIGHT_MEDIUM:
					entry->style |= FL_WEIGHT_MEDIUM;
					break;

				case FC_WEIGHT_DEMIBOLD:
					entry->style |= FL_WEIGHT_DEMIBOLD;
					break;

				case FC_WEIGHT_BOLD:
					entry->style |= FL_WEIGHT_BOLD;
					break;

				case FC_WEIGHT_EXTRABOLD:
					entry->style |= FL_WEIGHT_EXTRABOLD;
					break;

				case FC_WEIGHT_BLACK:
					entry->style |= FL_WEIGHT_BLACK;
					break;

				case FC_WEIGHT_EXTRABLACK:
					entry->style |= FL_WEIGHT_EXTRABLACK;
					break;
				}
			}

			if(FcPatternGetInteger(font, FC_WIDTH, 0, &intvalue) == FcResultMatch)
			{
				switch(intvalue)
				{
				case FC_WIDTH_ULTRACONDENSED:
					entry->style |= FL_WIDTH_ULTRACONDENSED;
					break;

				case FC_WIDTH_EXTRACONDENSED:
					entry->style |= FL_WIDTH_EXTRACONDENSED;
					break;

				case FC_WIDTH_CONDENSED:
					entry->style |= FL_WIDTH_CONDENSED;
					break;

				case FC_WIDTH_SEMICONDENSED:
					entry->style = FL_WIDTH_SEMICONDENSED;
					break;

				case FC_WIDTH_NORMAL:
				default:
					entry->style |= FL_WIDTH_NORMAL;
					break;

				case FC_WIDTH_SEMIEXPANDED:
					entry->style |= FL_WIDTH_SEMIEXPANDED;
					break;

				case FC_WIDTH_EXPANDED:
					entry->style |= FL_WIDTH_EXPANDED;
					break;

				case FC_WIDTH_EXTRAEXPANDED:
					entry->style |= FL_WIDTH_EXTRAEXPANDED;
					break;

				case FC_WIDTH_ULTRAEXPANDED:
					entry->style |= FL_WIDTH_ULTRAEXPANDED;
					break;
				}
			}

			if(FcPatternGetInteger(font, FC_SPACING, 0, &intvalue) == FcResultMatch)
			{
				switch(intvalue)
				{
				case FC_PROPORTIONAL:
				default:
					entry->style |= FL_PROPORTIONAL;
					break;

				case FC_DUAL:
					entry->style |= FL_DUAL;
					break;

				case FC_MONO:
					entry->style |= FL_MONO;
					break;

				case FC_CHARCELL:
					entry->style |= FL_CHARCELL;
					break;
				}
			}
			if(entry->foundry && strcmp(entry->foundry, "unknown"))
			{
				char tempstr[BCTEXTLEN];
				sprintf(tempstr, "%s (%s)", entry->family, entry->foundry);
				entry->displayname = new char[strlen(tempstr) + 1];
				strcpy(entry->displayname, tempstr);
			}
			else
			{
				entry->displayname = new char[strlen(entry->family) + 1];
				strcpy(entry->displayname, entry->family);
			}
			fontlist->append(entry);
		}
		FcFontSetDestroy(fs);
	}
	return 0;
}

#define STYLE_MATCH(fst, stl, msk) ((fst) & (msk) & (stl)) && \
	!((fst) & ~(style) &msk)

BC_FontEntry *BC_Resources::find_fontentry(const char *displayname, int style,
	int mask, int preferred)
{
	BC_FontEntry *entry, *style_match, *preferred_match;

	if(!fontlist)
		return 0;

	style_match = 0;
	preferred_match = 0;

	if(displayname)
	{
		for(int i = 0; i < fontlist->total; i++)
		{
			entry = fontlist->values[i];

			if(strcmp(entry->displayname, displayname) == 0 &&
					STYLE_MATCH(entry->style, style, mask))
			{
				if(!style_match)
					style_match = entry;
				if(!preferred_match && (entry->style & preferred))
					preferred_match = entry;
			}
		}
		if(preferred_match)
			return preferred_match;

		if(style_match)
			return style_match;
	}

// No exact match - assume normal width font
	style |= FL_WIDTH_NORMAL;
	mask |= FL_WIDTH_MASK;
	style_match = 0;
	preferred_match = 0;

	for(int i = 0; i < fontlist->total; i++)
	{
		entry = fontlist->values[i];

		if(STYLE_MATCH(entry->style, style, mask))
		{
			if(!style_match)
				style_match = entry;

			if(!preferred_match && (entry->style & preferred))
				preferred_match = entry;

			if(!strncasecmp(displayname, entry->family,
					strlen(entry->family)))
				return entry;
		}
	}

	if(preferred_match)
		return preferred_match;

	return style_match;
}

size_t BC_Resources::encode(const char *from_enc, const char *to_enc,
	char *input, char *output, int output_length, int input_length)
{
	size_t inbytes, outbytes = 0;
	iconv_t cd;
	char *outbase = output;

	if(!from_enc || *from_enc == 0)
		from_enc = "UTF-8";

	if(!to_enc || *to_enc == 0)
		to_enc = "UTF-8";

	if(input_length < 0)
		inbytes = strlen(input);
	else
		inbytes = input_length;

	if(strcmp(from_enc, to_enc) && inbytes)
	{
		if((cd = iconv_open(to_enc, from_enc)) == (iconv_t)-1)
		{
			printf(_("Conversion from %s to %s is not available"),
				from_enc, to_enc);
			return 0;
		}

		outbytes = output_length - 1;

		iconv(cd, &input, &inbytes, &output, &outbytes);

		iconv_close(cd);
		inbytes = output - outbase;
	}
	else if(inbytes)
	{
		memcpy(output,  input, inbytes);
		outbytes -= inbytes;
	}

	for(int i = 0; i < 4; i++)
	{
		output[i] = 0;
		if(outbytes-- == 0)
			break;
	}
	return inbytes;
}

void BC_Resources::encode_to_utf8(char *buffer, int buflen)
{
	char lbuf[buflen];

	if(BC_Resources::locale_utf8)
		return;

	BC_Resources::encode(BC_Resources::encoding, 0, buffer, lbuf, buflen);
	strcpy(buffer, lbuf);
}

int BC_Resources::find_font_by_char(FT_ULong char_code, char *path_new, const FT_Face oldface)
{
	FcPattern *font, *ofont;
	FcChar8 *file;
	int result = 0;

	*path_new = 0;

	// Do not search control codes
	if(char_code < ' ')
		return 0;

	if(ofont = FcFreeTypeQueryFace(oldface, (const FcChar8*)"", 4097, 0))
	{
		if(font = find_similar_font(char_code, ofont))
		{
			if(FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
			{
				strcpy(path_new, (char*)file);
				result = 1;
			}
			FcPatternDestroy(font);
		}
		FcPatternDestroy(ofont);
	}
	return result;
}

FcPattern* BC_Resources::find_similar_font(FT_ULong char_code, FcPattern *oldfont)
{
	FcPattern *pat, *font;
	FcFontSet *fs;
	FcObjectSet *os;
	FcCharSet *fcs;
	FcChar8 *file;
	double dval;
	int ival;

	// Do not search control codes
	if(char_code < ' ')
		return 0;

	fontconfig_lock.lock("BC_Resources::find_similar_font");
	pat = FcPatternCreate();
	os = FcObjectSetBuild(FC_FILE, FC_CHARSET, FC_SCALABLE, FC_FAMILY,
		FC_SLANT, FC_WEIGHT, FC_WIDTH, (char *)0);

	FcPatternAddBool(pat, FC_SCALABLE, true);
	fcs = FcCharSetCreate();
	if(FcCharSetAddChar(fcs, char_code))
		FcPatternAddCharSet(pat, FC_CHARSET, fcs);
	FcCharSetDestroy(fcs);
	for(int i = 0; i < LEN_FCPROP; i++)
	{
		if(FcPatternGetInteger(oldfont, fc_properties[i], 0, &ival) == FcResultMatch)
			FcPatternAddInteger(pat, fc_properties[i], ival);
	}
	fs = FcFontList(0, pat, os);

	for(int i = LEN_FCPROP - 1; i >= 0 && fs->nfont == 0; i--)
	{
		FcFontSetDestroy(fs);
		FcPatternDel(pat, fc_properties[i]);
		fs = FcFontList(0, pat, os);
	}
	FcPatternDestroy(pat);
	FcObjectSetDestroy(os);

	pat = 0;

	for (int i = 0; i < fs->nfont; i++)
	{
		font = fs->fonts[i];
		if(FcPatternGetCharSet(font, FC_CHARSET, 0, &fcs) == FcResultMatch)
		{
			if(FcCharSetHasChar(fcs, char_code))
			{
				pat =  FcPatternDuplicate(font);
				break;
			}
		}
	}
	FcFontSetDestroy(fs);
	fontconfig_lock.unlock();

	return pat;
}
