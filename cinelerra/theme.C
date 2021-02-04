
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

#include "awindowgui.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "clip.h"
#include "colors.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "levelwindowgui.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "patchbay.h"
#include "playtransport.h"
#include "resourcepixmap.h"
#include "statusbar.h"
#include "theme.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "vwindowgui.h"
#include "zoombar.h"


#include <errno.h>
#include <string.h>

Theme::Theme()
 : BC_Theme()
{
	this->mwindow = 0;
	theme_title = DEFAULT_THEME;
	data_buffer = 0;
	contents_buffer = 0;
	last_image = 0;
	mtransport_margin = 0;
	toggle_margin = 0;

	BC_WindowBase::get_resources()->bg_color = BLOND;
	BC_WindowBase::get_resources()->button_up = 0xffc000;
	BC_WindowBase::get_resources()->button_highlighted = 0xffe000;
	BC_WindowBase::get_resources()->recursive_resizing = 0;
	audio_color = BLACK;
	fade_h = 22;
	meter_h = 17;
	mode_h = 30;
	pan_h = 32;
	pan_x = 50;
	play_h = 22;
	title_h = 23;

	preferences_category_overlap = 0;

	loadmode_w = 350;

#include "data/about_png.h"
	about_bg = new VFrame(about_png);
}


// Need to delete everything here
Theme::~Theme()
{
	flush_images();
}

void Theme::flush_images()
{
	if(data_buffer) delete [] data_buffer;
	if(contents_buffer) delete [] contents_buffer;
	data_buffer = 0;
	contents_buffer = 0;
	contents.remove_all();
}

void Theme::initialize()
{
	message_normal = BLACK;
	message_error = RED;
}

void Theme::draw_awindow_bg(AWindowGUI *gui)
{
	gui->clear_box(0, 0, mainsession->awindow_w, mainsession->awindow_h);
	gui->flash();
}

void Theme::draw_cwindow_bg(CWindowGUI *gui)
{
	gui->clear_box(0, 0, mainsession->cwindow_w, mainsession->cwindow_h);
	gui->flash();
}

void Theme::draw_lwindow_bg(LevelWindowGUI *gui)
{
	gui->clear_box(0, 0, mainsession->lwindow_w, mainsession->lwindow_h);
	gui->flash();
}


void Theme::draw_resource_bg(TrackCanvas *canvas,
	ResourcePixmap *pixmap, 
	int edit_x,
	int edit_w,
	int pixmap_x,
	int x1, 
	int y1, 
	int x2,
	int y2)
{
	VFrame *image;

	switch(master_edl->local_session->zoom_track)
	{
	case 1024: 
		image = get_image("resource1024");
		break;
	case 512:
		image = get_image("resource512");
		break;
	case 256:
		image = get_image("resource256");
		break;
	case 128: 
		image = get_image("resource128");
		break;
	case 64:
		image = get_image("resource64");
		break;
	default:
	case 32:
		image = get_image("resource32");
		break;
	}

	canvas->draw_3segmenth(x1, 
		y1, 
		x2 - x1, 
		edit_x - pixmap_x,
		edit_w,
		image,
		pixmap);
}

void Theme::get_awindow_sizes(AWindowGUI *gui)
{
	abuttons_x = 0; 
	abuttons_y = 0;
	afolders_x = 0;
	afolders_y = 0;
	afolders_w = mainsession->afolders_w;
	afolders_h = mainsession->awindow_h - afolders_y;
	adivider_x = afolders_x + afolders_w;
	adivider_y = 0;
	adivider_w = 5;
	adivider_h = afolders_h;
	alist_x = afolders_x + afolders_w + 5;
	alist_y = afolders_y;
	alist_w = mainsession->awindow_w - alist_x;
	alist_h = afolders_h;
}

void Theme::get_batchrender_sizes(BatchRenderGUI *gui,
	int w, 
	int h)
{
	batchrender_x1 = 5;
	batchrender_x2 = 300;
	batchrender_x3 = 400;
}

void Theme::get_plugindialog_sizes()
{
	int x = 10, y = 30;
	plugindialog_new_x = x;
	plugindialog_new_y = y;
	plugindialog_shared_x = mainsession->plugindialog_w / 3;
	plugindialog_shared_y = y;
	plugindialog_module_x = mainsession->plugindialog_w * 2 / 3;
	plugindialog_module_y = y;

	plugindialog_new_w = plugindialog_shared_x - plugindialog_new_x - 10;
	plugindialog_new_h = mainsession->plugindialog_h - 100;
	plugindialog_shared_w = plugindialog_module_x - plugindialog_shared_x - 10;
	plugindialog_shared_h = mainsession->plugindialog_h - 100;
	plugindialog_module_w = mainsession->plugindialog_w - plugindialog_module_x - 10;
	plugindialog_module_h = mainsession->plugindialog_h - 100;

	plugindialog_newattach_x = plugindialog_new_x + 20;
	plugindialog_newattach_y = plugindialog_new_y + plugindialog_new_h + 10;
	plugindialog_sharedattach_x = plugindialog_shared_x + 20;
	plugindialog_sharedattach_y = plugindialog_shared_y + plugindialog_shared_h + 10;
	plugindialog_moduleattach_x = plugindialog_module_x + 20;
	plugindialog_moduleattach_y = plugindialog_module_y + plugindialog_module_h + 10;
}

void Theme::get_menueffect_sizes(int use_list)
{
	if(use_list)
	{
		menueffect_list_x = 10;
		menueffect_list_y = 10;
		menueffect_list_w = mainsession->menueffect_w - 400;
		menueffect_list_h = mainsession->menueffect_h -
			menueffect_list_y -
			BC_OKButton::calculate_h() - 10;
	}
	else
	{
		menueffect_list_x = 0;
		menueffect_list_y = 10;
		menueffect_list_w = 0;
		menueffect_list_h = 0;
	}

	menueffect_file_x = menueffect_list_x + menueffect_list_w + 10;
	menueffect_file_y = 10;

	menueffect_tools_x = menueffect_file_x;
	menueffect_tools_y = menueffect_file_y + 20;
}
