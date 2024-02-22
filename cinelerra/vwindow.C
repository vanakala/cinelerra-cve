// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "clipedit.h"
#include "bcclipboard.h"
#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "localsession.h"
#include "mainclock.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "transportcommand.inc"
#include "vplayback.h"
#include "vtimebar.h"
#include "vtracking.h"
#include "vwindow.h"
#include "vwindowgui.h"

#include <unistd.h>

VWindow::VWindow()
 : Thread()
{
	gui = new VWindowGUI(this);

	playback_engine = new VPlayback(this, gui->canvas);

// Start command loop
	gui->transport->set_engine(playback_engine);
	playback_cursor = new VTracking(this);
	vedlsession = new EDLSession();
	vedlsession->copy(edlsession);
	vwindow_edl->this_edlsession = vedlsession;
}

VWindow::~VWindow()
{
	delete playback_engine;
	delete playback_cursor;
	delete vedlsession;
}

void VWindow::run()
{
	gui->run_window();
}

void VWindow::change_source()
{
	gui->canvas->clear_canvas();
	if(vwindow_edl)
	{
		gui->change_source(vwindow_edl->local_session->clip_title);
		update_position(1, 1);
	}
}

void VWindow::change_source(Asset *asset)
{
	int stream;
	char title[BCTEXTLEN];
	FileSystem fs;
	fs.extract_name(title, asset->path);

	gui->canvas->clear_canvas();

// Generate EDL off of main EDL for cutting
	vwindow_edl->reset_instance();
	vwindow_edl->update_assets(asset);
	vwindow_edl->init_edl();
	vwindow_edl->id = vwindow_edl->next_id();
	vwindow_edl->this_edlsession = vedlsession;
	vedlsession->copy(edlsession);
	if((stream = asset->get_stream_ix(STRDSC_AUDIO)) >= 0)
		vedlsession->audio_channels = asset->streams[stream].channels;
	else
		vedlsession->audio_channels = 0;
	if((stream = asset->get_stream_ix(STRDSC_VIDEO)) >= 0)
	{
		vedlsession->sample_aspect_ratio = asset->streams[stream].sample_aspect_ratio;
		vedlsession->frame_rate = asset->streams[stream].frame_rate;
		vedlsession->output_w = asset->streams[stream].width;
		vedlsession->output_h = asset->streams[stream].height;
	}
// Update GUI
	gui->change_source(title);
	update_position(1, 1);
}

void VWindow::change_source(EDL *edl)
{
// EDLs are identical
	if(edl && vwindow_edl &&
		edl->id == vwindow_edl->id) return;

	gui->canvas->clear_canvas();

	vwindow_edl->reset_instance();

	if(edl)
	{
		vwindow_edl->copy_all(edl);
		vwindow_edl->id = edl->id;
		if(edl->this_edlsession)
			vedlsession->copy(edl->this_edlsession);
		else
			vedlsession->copy(edlsession);
// Update GUI
		gui->change_source(edl->local_session->clip_title);
		update_position(1, 1);
	}
	else
	{
		gui->change_source(0);
		gui->clock->update(0);
		gui->canvas->release_refresh_frame();
		gui->canvas->draw_refresh();
	}
}

void VWindow::remove_source(Asset *asset)
{
	if(!asset)
	{
		change_source((EDL*)0);
		return;
	}

	if(vwindow_edl->assets->total)
	{
		for(int i = 0; i < vwindow_edl->assets->total; i++)
		{
			if(vwindow_edl->assets->values[i] == asset)
			{
				vwindow_edl->assets->remove(asset);
				break;
			}
		}
	}
	if(vwindow_edl->assets->total)
		vwindow_edl->id = vwindow_edl->next_id();
	else
	{
		vwindow_edl->reset_instance();
		gui->change_source(0);
		gui->clock->update(0);
		gui->canvas->release_refresh_frame();
		gui->canvas->draw_refresh();
	}
}

void VWindow::goto_start()
{
	vwindow_edl->local_session->set_selection(0);
	update_position(0, 1);
}

void VWindow::goto_end()
{
	ptstime position = vwindow_edl->duration();

	vwindow_edl->local_session->set_selection(position);
	update_position(0, 1);
}

void VWindow::update(int options)
{
	if(options & WUPD_ACHANNELS)
	{
		gui->meters->set_meters(vedlsession->audio_channels,
			edlsession->vwindow_meter);
		gui->resize_event(gui->get_w(), gui->get_h());
	}
}

void VWindow::update_position(int use_slider, int update_slider)
{
	if(use_slider)
		vwindow_edl->local_session->set_selection(gui->slider->get_value());

	if(update_slider)
		gui->slider->set_position();

	gui->timebar->update();
	playback_engine->send_command(CURRENT_FRAME);

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

void VWindow::copy()
{
	ptstime start = vwindow_edl->local_session->get_selectionstart();
	ptstime end = vwindow_edl->local_session->get_selectionend();

	if(PTSEQU(start, end))
		return;

	FileXML file;
	EDL edl(0);

	edl.copy(vwindow_edl, start, end);
	edl.save_xml(&file);
	mwindow_global->gui->get_clipboard()->to_clipboard(file.string,
		strlen(file.string),
		SECONDARY_SELECTION);
}

int VWindow::stop_playback()
{
	playback_engine->send_command(STOP);

	for(int i = 0; playback_engine->is_playing_back && i < 5; i++)
		usleep(50000);

	return playback_engine->is_playing_back;
}

VFrame *VWindow::get_window_icon()
{
	return theme_global->get_image("vwindow_icon");
}
