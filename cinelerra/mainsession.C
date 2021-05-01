
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
#include "bcresources.h"
#include "clip.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "mainsession.h"
#include "meterpanel.h"
#include "auto.h"
#include "ruler.inc"

MainSession::MainSession()
{
	changes_made = 0;
	filename[0] = 0;
	track_highlighted = 0;
	vcanvas_highlighted = 0;
	ccanvas_highlighted = 0;
	edit_highlighted = 0;
	current_operation = NO_OPERATION;
	drag_pluginservers = new ArrayList<PluginServer*>;
	drag_plugin = 0;
	drag_handle = HANDLE_MAIN;
	drag_assets = new ArrayList<Asset*>;
	drag_auto_gang = new ArrayList<Auto*>;
	drag_clips = new ArrayList<EDL*>;
	drag_edits = new ArrayList<Edit*>;
	drag_edit = 0;
	clip_number = 1;
	brender_end = 0;
	cwindow_controls = 1;
	gwindow_x = 0;
	gwindow_y = 0;
	show_gwindow = 0;
	current_tip = 0;
	cwindow_fullscreen = 0;
	rwindow_fullscreen = 0;
	vwindow_fullscreen = 0;
}

MainSession::~MainSession()
{
	delete drag_pluginservers;
	delete drag_assets;
	delete drag_auto_gang;
	delete drag_clips;
	delete drag_edits;
}

void MainSession::boundaries()
{
	lwindow_x = MAX(0, lwindow_x);
	lwindow_y = MAX(0, lwindow_y);
	mwindow_x = MAX(0, mwindow_x);
	mwindow_y = MAX(0, mwindow_y);
	cwindow_x = MAX(0, cwindow_x);
	cwindow_y = MAX(0, cwindow_y);
	vwindow_x = MAX(0, vwindow_x);
	vwindow_y = MAX(0, vwindow_y);
	awindow_x = MAX(0, awindow_x);
	awindow_y = MAX(0, awindow_y);
	gwindow_x = MAX(0, gwindow_x);
	gwindow_y = MAX(0, gwindow_y);
	ruler_x = MAX(0, ruler_x);
	ruler_y = MAX(0, ruler_y);
	cwindow_controls = CLIP(cwindow_controls, 0, 1);
}

void MainSession::default_window_positions()
{
// Get defaults based on root window size
	int border_left, border_right, border_top, border_bottom;
	int root_w, root_h;
	int root_x = 0;
	int root_y = 0;

	BC_Resources::get_root_size(&root_w, &root_h);

	BC_Resources::get_window_borders(&border_left, &border_right,
		&border_top, &border_bottom);

// Wider than 16:9, narrower than dual head
	if((float)root_w / root_h > 1.8) root_w /= 2;

	vwindow_x = root_x;
	vwindow_y = root_y;
	vwindow_w = root_w / 2 - border_left - border_right;
	vwindow_h = root_h * 6 / 10 - border_top - border_bottom;

	cwindow_x = root_x + root_w / 2;
	cwindow_y = root_y;
	cwindow_w = vwindow_w;
	cwindow_h = vwindow_h;

	ctool_x = cwindow_x + cwindow_w / 2;
	ctool_y = cwindow_y + cwindow_h / 2;

	mwindow_x = root_x;
	mwindow_y = vwindow_y + vwindow_h + border_top + border_bottom;
	mwindow_w = root_w * 2 / 3 - border_left - border_right;
	mwindow_h = root_h - mwindow_y - border_top - border_bottom;

	awindow_x = mwindow_x + border_left + border_right + mwindow_w;
	awindow_y = mwindow_y;
	awindow_w = root_x + root_w - awindow_x - border_left - border_right;
	awindow_h = mwindow_h;

	ewindow_w = 640;
	ewindow_h = 240;

	lwindow_w = 100;
	lwindow_y = 0;
	lwindow_x = root_w - lwindow_w;
	lwindow_h = mwindow_y;

	batchrender_w = 680;
	batchrender_h = 340;
	batchrender_x = root_w / 2 - batchrender_w / 2;
	batchrender_y = root_h / 2 - batchrender_h / 2;

	ruler_x = root_w / 4;
	ruler_y = root_h / 4;
	ruler_length = RULER_LENGTH;
	ruler_orientation = RULER_HORIZ;
}

void MainSession::load_defaults(BC_Hash *defaults)
{
// Setup main windows
	default_window_positions();

	vwindow_x = defaults->get("VWINDOW_X", vwindow_x);
	vwindow_y = defaults->get("VWINDOW_Y", vwindow_y);
	vwindow_w = defaults->get("VWINDOW_W", vwindow_w);
	vwindow_h = defaults->get("VWINDOW_H", vwindow_h);


	cwindow_x = defaults->get("CWINDOW_X", cwindow_x);
	cwindow_y = defaults->get("CWINDOW_Y", cwindow_y);
	cwindow_w = defaults->get("CWINDOW_W", cwindow_w);
	cwindow_h = defaults->get("CWINDOW_H", cwindow_h);

	ctool_x = defaults->get("CTOOL_X", ctool_x);
	ctool_y = defaults->get("CTOOL_Y", ctool_y);

	gwindow_x = defaults->get("GWINDOW_X", gwindow_x);
	gwindow_y = defaults->get("GWINDOW_Y", gwindow_y);

	mwindow_x = defaults->get("MWINDOW_X", mwindow_x);
	mwindow_y = defaults->get("MWINDOW_Y", mwindow_y);
	mwindow_w = defaults->get("MWINDOW_W", mwindow_w);
	mwindow_h = defaults->get("MWINDOW_H", mwindow_h);

	lwindow_x = defaults->get("LWINDOW_X", lwindow_x);
	lwindow_y = defaults->get("LWINDOW_Y", lwindow_y);
	lwindow_w = defaults->get("LWINDOW_W", lwindow_w);
	lwindow_h = defaults->get("LWINDOW_H", lwindow_h);


	awindow_x = defaults->get("AWINDOW_X", awindow_x);
	awindow_y = defaults->get("AWINDOW_Y", awindow_y);
	awindow_w = defaults->get("AWINDOW_W", awindow_w);
	awindow_h = defaults->get("AWINDOW_H", awindow_h);

	ewindow_w = defaults->get("EWINDOW_W", ewindow_w);
	ewindow_h = defaults->get("EWINDOW_H", ewindow_h);

// Other windows
	afolders_w = defaults->get("ABINS_W", 100);

	batchrender_x = defaults->get("BATCHRENDER_X", batchrender_x);
	batchrender_y = defaults->get("BATCHRENDER_Y", batchrender_y);
	batchrender_w = defaults->get("BATCHRENDER_W", batchrender_w);
	batchrender_h = defaults->get("BATCHRENDER_H", batchrender_h);

	ruler_x = defaults->get("RULER_X", ruler_x);
	ruler_y = defaults->get("RULER_Y", ruler_y);
	ruler_length = defaults->get("RULER_LENGTH", ruler_length);
	ruler_orientation = defaults->get("RULER_ORIENTATION", ruler_orientation);

	show_vwindow = defaults->get("SHOW_VWINDOW", 1);
	show_awindow = defaults->get("SHOW_AWINDOW", 1);
	show_cwindow = defaults->get("SHOW_CWINDOW", 1);
	show_lwindow = defaults->get("SHOW_LWINDOW", 0);
	show_gwindow = defaults->get("SHOW_GWINDOW", 0);
	show_ruler = defaults->get("SHOW_RULER", 0);

	cwindow_controls = defaults->get("CWINDOW_CONTROLS", cwindow_controls);

	plugindialog_w = defaults->get("PLUGINDIALOG_W", 510);
	plugindialog_h = defaults->get("PLUGINDIALOG_H", 415);
	menueffect_w = defaults->get("MENUEFFECT_W", 580);
	menueffect_h = defaults->get("MENUEFFECT_H", 350);

	current_tip = defaults->get("CURRENT_TIP", current_tip);

	boundaries();
}

void MainSession::save_defaults(BC_Hash *defaults)
{
// Window positions
	defaults->update("MWINDOW_X", mwindow_x);
	defaults->update("MWINDOW_Y", mwindow_y);
	defaults->update("MWINDOW_W", mwindow_w);
	defaults->update("MWINDOW_H", mwindow_h);

	defaults->update("LWINDOW_X", lwindow_x);
	defaults->update("LWINDOW_Y", lwindow_y);
	defaults->update("LWINDOW_W", lwindow_w);
	defaults->update("LWINDOW_H", lwindow_h);

	defaults->update("VWINDOW_X", vwindow_x);
	defaults->update("VWINDOW_Y", vwindow_y);
	defaults->update("VWINDOW_W", vwindow_w);
	defaults->update("VWINDOW_H", vwindow_h);

	defaults->update("CWINDOW_X", cwindow_x);
	defaults->update("CWINDOW_Y", cwindow_y);
	defaults->update("CWINDOW_W", cwindow_w);
	defaults->update("CWINDOW_H", cwindow_h);

	defaults->update("CTOOL_X", ctool_x);
	defaults->update("CTOOL_Y", ctool_y);

	defaults->update("GWINDOW_X", gwindow_x);
	defaults->update("GWINDOW_Y", gwindow_y);

	defaults->update("AWINDOW_X", awindow_x);
	defaults->update("AWINDOW_Y", awindow_y);
	defaults->update("AWINDOW_W", awindow_w);
	defaults->update("AWINDOW_H", awindow_h);

	defaults->update("EWINDOW_W", ewindow_w);
	defaults->update("EWINDOW_H", ewindow_h);

	defaults->update("ABINS_W", afolders_w);

	defaults->delete_key("RMONITOR_X");
	defaults->delete_key("RMONITOR_Y");
	defaults->delete_key("RMONITOR_W");
	defaults->delete_key("RMONITOR_H");

	defaults->delete_key("RWINDOW_X");
	defaults->delete_key("RWINDOW_Y");
	defaults->delete_key("RWINDOW_W");
	defaults->delete_key("RWINDOW_H");

	defaults->update("BATCHRENDER_X", batchrender_x);
	defaults->update("BATCHRENDER_Y", batchrender_y);
	defaults->update("BATCHRENDER_W", batchrender_w);
	defaults->update("BATCHRENDER_H", batchrender_h);

	defaults->update("RULER_X", ruler_x);
	defaults->update("RULER_Y", ruler_y);
	defaults->update("RULER_LENGTH", ruler_length);
	defaults->update("RULER_ORIENTATION", ruler_orientation);

	defaults->update("SHOW_VWINDOW", show_vwindow);
	defaults->update("SHOW_AWINDOW", show_awindow);
	defaults->update("SHOW_CWINDOW", show_cwindow);
	defaults->update("SHOW_LWINDOW", show_lwindow);
	defaults->update("SHOW_GWINDOW", show_gwindow);
	defaults->update("SHOW_RULER", show_ruler);

	defaults->update("CWINDOW_CONTROLS", cwindow_controls);

	defaults->update("PLUGINDIALOG_W", plugindialog_w);
	defaults->update("PLUGINDIALOG_H", plugindialog_h);

	defaults->update("MENUEFFECT_W", menueffect_w);
	defaults->update("MENUEFFECT_H", menueffect_h);

	defaults->update("CURRENT_TIP", current_tip);
}

size_t MainSession::get_size()
{
	return sizeof(*this);
}
