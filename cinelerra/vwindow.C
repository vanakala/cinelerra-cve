
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

#include "asset.h"
#include "assets.h"
#include "clipedit.h"
#include "bcclipboard.h"
#include "bchash.h"
#include "bcresources.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "tracks.h"
#include "transportcommand.inc"
#include "vplayback.h"
#include "vtimebar.h"
#include "vtracking.h"
#include "vwindow.h"
#include "vwindowgui.h"


VWindow::VWindow(MWindow *mwindow) : Thread()
{
	this->mwindow = mwindow;
	asset = 0;

	gui = new VWindowGUI(mwindow, this);

	playback_engine = new VPlayback(mwindow, this, gui->canvas);

// Start command loop
	gui->transport->set_engine(playback_engine);
	playback_cursor = new VTracking(mwindow, this);
	clip_edit = new ClipEdit(mwindow, 0, this);
}

VWindow::~VWindow()
{
	delete playback_engine;
	delete playback_cursor;
	delete_edl();
	delete clip_edit;
}

void VWindow::delete_edl()
{
	if(mwindow->edl->vwindow_edl && !mwindow->edl->vwindow_edl_shared)
	{
		delete mwindow->edl->vwindow_edl;
		mwindow->edl->vwindow_edl = 0;
		mwindow->edl->vwindow_edl_shared = 0;
	}

	if(asset) Garbage::delete_object(asset);
	asset = 0;
}

void VWindow::run()
{
	gui->run_window();
}

EDL* VWindow::get_edl()
{
	return mwindow->edl->vwindow_edl;
}

Asset* VWindow::get_asset()
{
	return this->asset;
}

void VWindow::change_source()
{
	gui->canvas->clear_canvas();
	if(mwindow->edl->vwindow_edl)
	{
		gui->change_source(get_edl(), get_edl()->local_session->clip_title);
		update_position(CHANGE_ALL, 1, 1);
	}
	else
	{
		if(asset) Garbage::delete_object(asset);
		asset = 0;
		mwindow->edl->vwindow_edl_shared = 0;
	}
}

void VWindow::change_source(Asset *asset)
{
	char title[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(title, asset->path);

	delete_edl();
	gui->canvas->clear_canvas();

// Generate EDL off of main EDL for cutting
	this->asset = new Asset;
	*this->asset = *asset;
	mwindow->edl->vwindow_edl = new EDL(mwindow->edl);
	mwindow->edl->vwindow_edl_shared = 0;
	mwindow->asset_to_edl(mwindow->edl->vwindow_edl, asset);

// Update GUI
	gui->change_source(mwindow->edl->vwindow_edl, title);
	update_position(CHANGE_ALL, 1, 1);
}

void VWindow::change_source(EDL *edl)
{
// EDLs are identical
	if(edl && mwindow->edl->vwindow_edl && 
		edl->id == mwindow->edl->vwindow_edl->id) return;

	delete_edl();
	gui->canvas->clear_canvas();

	if(edl)
	{
		this->asset = 0;
		mwindow->edl->vwindow_edl = edl;
// in order not to later delete edl if it is shared
		mwindow->edl->vwindow_edl_shared = 1;

// Update GUI
		gui->change_source(edl, edl->local_session->clip_title);
		update_position(CHANGE_ALL, 1, 1);
	}
	else
		gui->change_source(edl, _("Viewer"));
}

void VWindow::remove_source()
{
	delete_edl();
	gui->change_source(0, 0);
	gui->clock->update(0);
	gui->canvas->draw_refresh();
}

void VWindow::goto_start()
{
	if(get_edl())
	{
		get_edl()->local_session->set_selection(0);
		update_position(CHANGE_NONE, 
			0, 
			1);
	}
}

void VWindow::goto_end()
{
	if(get_edl())
	{
		ptstime position = get_edl()->tracks->total_length();
		get_edl()->local_session->set_selection(position);
		update_position(CHANGE_NONE, 
			0, 
			1);
	}
}

void VWindow::update_position(int change_type, 
	int use_slider, 
	int update_slider)
{
	EDL *edl = get_edl();
	if(edl)
	{
		Asset *asset = edl->assets->first;
		if(use_slider) 
			edl->local_session->set_selection(gui->slider->get_value());

		if(update_slider)
		{
			gui->slider->set_position();
		}

		gui->timebar->update();
		playback_engine->send_command(CURRENT_FRAME, edl, change_type);

		gui->clock->update(edl->local_session->get_selectionstart(1));
	}
}

void VWindow::set_inpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->set_inpoint(edl->local_session->get_selectionstart(1));
		gui->timebar->update();
	}
}

void VWindow::set_outpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->set_outpoint(edl->local_session->get_selectionstart(1));
		gui->timebar->update();
	}
}

void VWindow::clear_inpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->local_session->unset_inpoint();
		gui->timebar->update();
	}
}

void VWindow::clear_outpoint()
{
	EDL *edl = get_edl();
	if(edl)
	{
		edl->local_session->unset_outpoint();
		gui->timebar->update();
	}
}

void VWindow::copy()
{
	EDL *edl = get_edl();
	if(edl)
	{
		ptstime start = edl->local_session->get_selectionstart();
		ptstime end = edl->local_session->get_selectionend();
		FileXML file;
		edl->copy(start,
			end,
			0,
			0,
			0,
			&file,
			mwindow->plugindb,
			"",
			1);
		mwindow->gui->get_clipboard()->to_clipboard(file.string,
			strlen(file.string),
			SECONDARY_SELECTION);
	}
}
