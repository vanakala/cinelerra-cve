
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
#include "loadmode.inc"
#include "mainclock.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
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
	delete clip_edit;
}

void VWindow::run()
{
	gui->run_window();
}

Asset* VWindow::get_asset()
{
	return this->asset;
}

void VWindow::change_source()
{
	gui->canvas->clear_canvas();
	if(vwindow_edl)
	{
		gui->change_source(vwindow_edl->local_session->clip_title);
		update_position(CHANGE_ALL, 1, 1);
	}
	else
		asset = 0;
}

void VWindow::change_source(Asset *asset)
{
	char title[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(title, asset->path);

	gui->canvas->clear_canvas();

// Generate EDL off of main EDL for cutting
	this->asset = asset;
	vwindow_edl->reset_instance();
	vwindow_edl->update_assets(asset);
	vwindow_edl->init_edl();

// Update GUI
	gui->change_source(title);
	update_position(CHANGE_ALL, 1, 1);
}

void VWindow::change_source(EDL *edl)
{
// EDLs are identical
	if(edl && vwindow_edl &&
		edl->id == vwindow_edl->id) return;

	gui->canvas->clear_canvas();

	if(edl)
	{
		this->asset = 0;
		vwindow_edl->reset_instance();
		vwindow_edl->copy_all(edl);
// Update GUI
		gui->change_source(edl->local_session->clip_title);
		update_position(CHANGE_ALL, 1, 1);
	}
	else
		gui->change_source(0);
}

void VWindow::remove_source()
{
	gui->change_source(0);
	gui->clock->update(0);
	gui->canvas->release_refresh_frame();
	gui->canvas->draw_refresh();
}

void VWindow::goto_start()
{
	vwindow_edl->local_session->set_selection(0);
	update_position(CHANGE_NONE, 0, 1);
}

void VWindow::goto_end()
{
	ptstime position = vwindow_edl->total_length();

	vwindow_edl->local_session->set_selection(position);
	update_position(CHANGE_NONE, 0, 1);
}

void VWindow::update(int options)
{
	if(options & WUPD_ACHANNELS)
	{
		gui->meters->set_meters(edlsession->audio_channels,
			edlsession->vwindow_meter);
		gui->resize_event(gui->get_w(), gui->get_h());
	}
}

void VWindow::update_position(int change_type, 
	int use_slider, 
	int update_slider)
{
	if(use_slider)
		vwindow_edl->local_session->set_selection(gui->slider->get_value());

	if(update_slider)
		gui->slider->set_position();

	gui->timebar->update();
	playback_engine->send_command(CURRENT_FRAME, vwindow_edl, change_type);

	gui->clock->update(vwindow_edl->local_session->get_selectionstart(1));
}

void VWindow::set_inpoint()
{
	vwindow_edl->set_inpoint(vwindow_edl->local_session->get_selectionstart(1));
	gui->timebar->update();
}

void VWindow::set_outpoint()
{
	vwindow_edl->set_outpoint(vwindow_edl->local_session->get_selectionstart(1));
	gui->timebar->update();
}

void VWindow::clear_inpoint()
{
	vwindow_edl->local_session->unset_inpoint();
	gui->timebar->update();
}

void VWindow::clear_outpoint()
{
	vwindow_edl->local_session->unset_outpoint();
	gui->timebar->update();
}

void VWindow::copy()
{
	ptstime start = vwindow_edl->local_session->get_selectionstart();
	ptstime end = vwindow_edl->local_session->get_selectionend();

	if(PTSEQU(start, end))
		return;

	FileXML file;
	EDL edl(0);

	edl.copy(vwindow_edl, start, end);
	edl.save_xml(&file, "", 0, 0);
	mwindow->gui->get_clipboard()->to_clipboard(file.string,
		strlen(file.string),
		SECONDARY_SELECTION);
}

int VWindow::stop_playback()
{
	playback_engine->send_command(STOP, 0, CMDOPT_WAITTRACKING);

	for(int i = 0; playback_engine->is_playing_back && i < 5; i++)
		usleep(50000);

	return playback_engine->is_playing_back;
}
