// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bcresources.h"
#include "cinelerra.h"
#include "cwindow.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "keys.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "patchbay.h"
#include "samplescroll.h"
#include "statusbar.h"
#include "theme.h"
#include "trackcanvas.h"
#include "trackscroll.h"
#include "tracks.h"
#include "zoombar.h"

#include <stdarg.h>

// the main window uses its own private colormap for video
MWindowGUI::MWindowGUI()
 : BC_Window(PROGRAM_NAME,
		mainsession->mwindow_x,
		mainsession->mwindow_y,
		mainsession->mwindow_w,
		mainsession->mwindow_h,
		100,
		100,
		1,
		1,
		1)
{
	mbuttons = 0;
	statusbar = 0;
	zoombar = 0;
	samplescroll = 0;
	trackscroll = 0;
	cursor = 0;
	canvas = 0;
	cursor = 0;
	patchbay = 0;
	timebar = 0;
	mainclock = 0;
}

MWindowGUI::~MWindowGUI()
{
	delete mbuttons;
	delete statusbar;
	delete zoombar;
	delete samplescroll;
	delete trackscroll;
	delete cursor;
	delete patchbay;
	delete timebar;
	delete mainclock;
}

void MWindowGUI::get_scrollbars()
{
	view_w = theme_global->mcanvas_w;
	view_h = theme_global->mcanvas_h;

	if(canvas && (view_w != canvas->get_w() || view_h != canvas->get_h()))
	{
		canvas->reposition_window(theme_global->mcanvas_x,
			theme_global->mcanvas_y,
			view_w,
			view_h);
	}

	if(!samplescroll)
		add_subwindow(samplescroll = new SampleScroll(
			theme_global->mhscroll_x,
			theme_global->mhscroll_y,
			theme_global->mhscroll_w));
	else
		samplescroll->resize_event();

	samplescroll->set_position();

	if(!trackscroll)
		add_subwindow(trackscroll = new TrackScroll());
	else
		trackscroll->resize_event();

	trackscroll->set_position(view_h);
}

void MWindowGUI::show()
{
	set_icon(mwindow_global->get_window_icon());

	add_subwindow(mainmenu = new MainMenu(this));

	theme_global->get_mwindow_sizes(this, get_w(), get_h());
	theme_global->draw_mwindow_bg(this);

	add_subwindow(mbuttons = new MButtons());
	mbuttons->show();

	add_subwindow(timebar = new MTimeBar(this,
		theme_global->mtimebar_x,
		theme_global->mtimebar_y,
		theme_global->mtimebar_w,
		theme_global->mtimebar_h));
	timebar->update();

	add_subwindow(patchbay = new PatchBay());
	patchbay->show();

	get_scrollbars();

	cursor = new MainCursor(this);

	add_subwindow(canvas = new TrackCanvas(this));
	canvas->show();

	add_subwindow(zoombar = new ZoomBar());
	zoombar->show();

	add_subwindow(statusbar = new StatusBar());
	statusbar->show();

	add_subwindow(mainclock = new MainClock(
		theme_global->mclock_x,
		theme_global->mclock_y,
		theme_global->mclock_w));
	mainclock->set_frame_offset(
		edlsession->get_frame_offset());
	mainclock->update(0);

	canvas->activate();
}

void MWindowGUI::focus_out_event()
{
	cursor->focus_out_event();
}

void MWindowGUI::resize_event(int w, int h)
{
	mainsession->mwindow_location(-1, -1, w, h);
	theme_global->get_mwindow_sizes(this, w, h);
	theme_global->draw_mwindow_bg(this);
	flash();
	mainmenu->reposition_window(0, 0, w, mainmenu->get_h());
	mbuttons->resize_event();
	statusbar->resize_event();
	timebar->resize_event();
	patchbay->resize_event();
	get_scrollbars();
	canvas->resize_event();
	zoombar->resize_event();
}

int MWindowGUI::visible(int x1, int x2, int view_x1, int view_x2)
{
	return (x1 >= view_x1 && x1 < view_x2) ||
		(x2 > view_x1 && x2 <= view_x2) ||
		(x1 <= view_x1 && x2 >= view_x2);
}

// Drag motion called from other window
void MWindowGUI::drag_motion()
{
	if(!get_hidden())
		canvas->drag_motion();
}

void MWindowGUI::drag_stop()
{
	if(!get_hidden())
		canvas->drag_stop();
}

void MWindowGUI::repeat_event(int duration)
{
	cursor->repeat_event(duration);
}

void MWindowGUI::translation_event()
{
	if(mainsession->mwindow_location(get_x(), get_y(), get_w(), get_h()))
	{
		reposition_window(mainsession->mwindow_x,
			mainsession->mwindow_y,
			mainsession->mwindow_w,
			mainsession->mwindow_h);
		resize_event(mainsession->mwindow_w,
			mainsession->mwindow_h);
	}
}

void MWindowGUI::save_defaults(BC_Hash *defaults)
{
	BC_Resources *resources = get_resources();
	char string[BCTEXTLEN];

	defaults->delete_key("MWINDOWWIDTH");
	defaults->delete_key("MWINDOWHEIGHT");
	mainmenu->save_defaults(defaults);

	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
	{
		sprintf(string, "FILEBOX_HISTORY%d", i);
		defaults->update(string, resources->filebox_history[i]);
	}
	defaults->update("FILEBOX_MODE", resources->filebox_mode);
	defaults->update("FILEBOX_W", resources->filebox_w);
	defaults->update("FILEBOX_H", resources->filebox_h);
	defaults->update("FILEBOX_FILTER", resources->filebox_filter);
}

void MWindowGUI::load_defaults(BC_Hash *defaults)
{
	BC_Resources *resources = get_resources();
	char string[BCTEXTLEN];

	for(int i = 0; i < FILEBOX_HISTORY_SIZE; i++)
	{
		sprintf(string, "FILEBOX_HISTORY%d", i);
		resources->filebox_history[i][0] = 0;
		defaults->get(string, resources->filebox_history[i]);
	}
	resources->filebox_mode = defaults->get("FILEBOX_MODE", resources->filebox_mode);
	resources->filebox_w = defaults->get("FILEBOX_W", resources->filebox_w);
	resources->filebox_h = defaults->get("FILEBOX_H", resources->filebox_h);
	defaults->get("FILEBOX_FILTER", resources->filebox_filter);
}

int MWindowGUI::keypress_event()
{
	int result = 0;
	result = mbuttons->keypress_event();

	if(!result)
	{
		switch(get_keypress())
		{
		case 'e':
			mwindow_global->toggle_editing_mode();
			result = 1;
			break;
		case LEFT:
			if(!ctrl_down()) 
			{
				if(alt_down())
				{
					mbuttons->transport->handle_transport(STOP);
					mwindow_global->prev_edit_handle(shift_down());
				}
				else
					mwindow_global->move_left(); 
				result = 1;
			}
			break;
		case RIGHT:
			if(!ctrl_down()) 
			{
				if(alt_down())
				{
					mbuttons->transport->handle_transport(STOP);
					mwindow_global->next_edit_handle(shift_down());
				}
				else
					mwindow_global->move_right(); 
				result = 1;
			}
			break;

		case UP:
			if(ctrl_down() && !alt_down())
			{
				mwindow_global->expand_y();
				result = 1;
			}
			else
			if(!ctrl_down() && alt_down())
			{
				mwindow_global->expand_autos(0,1,1);
				result = 1;
			}
			else
			if(ctrl_down() && alt_down())
			{
				mwindow_global->expand_autos(1,1,1);
				result = 1;
			}
			else
			{
				mwindow_global->expand_sample();
				result = 1;
			}
			break;

		case DOWN:
			if(ctrl_down() && !alt_down())
			{
				mwindow_global->zoom_in_y();
				result = 1;
			}
			else
			if(!ctrl_down() && alt_down())
			{
				mwindow_global->shrink_autos(0,1,1);
				result = 1;
			}
			else
			if(ctrl_down() && alt_down())
			{
				mwindow_global->shrink_autos(1,1,1);
				result = 1;
			}
			else
			{
				mwindow_global->zoom_in_sample();
				result = 1;
			}
			break;

		case PGUP:
			if(!ctrl_down())
			{
				mwindow_global->move_up();
				result = 1;
			}
			else
			{
				mwindow_global->expand_t();
				result = 1;
			}
			break;

		case PGDN:
			if(!ctrl_down())
			{
				mwindow_global->move_down();
				result = 1;
			}
			else
			{
				mwindow_global->zoom_in_t();
				result = 1;
			}
			break;

		case TAB:
		case LEFTTAB:
			int cursor_x, cursor_y;

			canvas->get_relative_cursor_pos(&cursor_x, &cursor_y);

			if(get_keypress() == TAB)
			{
// Switch the record button
				for(Track *track = master_edl->first_track(); track; track = track->next)
				{
					int track_x, track_y, track_w, track_h;
					canvas->track_dimensions(track, track_x, track_y, track_w, track_h);

					if(cursor_y >= track_y && 
						cursor_y < track_y + track_h)
					{
						if (track->record)
							track->record = 0;
						else
							track->record = 1;
						result = 1; 
						break;
					}
				}
			}
			else
			{
				Track *this_track = 0;
				for(Track *track = master_edl->first_track(); track; track = track->next)
				{
					int track_x, track_y, track_w, track_h;
					canvas->track_dimensions(track, track_x, track_y, track_w, track_h);

					if(cursor_y >= track_y &&
						cursor_y < track_y + track_h)
					{
						// This is our track
						this_track = track;
						break;
					}
				}

				int total_selected = master_edl->total_toggled(Tracks::RECORD);

// nothing previously selected
				if(total_selected == 0)
				{
					master_edl->set_all_toggles(Tracks::RECORD,
						1);
				}
				else
				if(total_selected == 1)
				{
// this patch was previously the only one on
					if(this_track && this_track->record)
					{
						master_edl->set_all_toggles(Tracks::RECORD,
							1);
					}
// another patch was previously the only one on
					else
					{
						master_edl->set_all_toggles(Tracks::RECORD,
							0);
						if(this_track)
							this_track->record = 1;

					}
				}
				else
				if(total_selected > 1)
				{
					master_edl->set_all_toggles(Tracks::RECORD,
						0);
					if(this_track)
						this_track->record = 1;
				}

			}

			mwindow_global->update_gui(WUPD_CANVINCR | WUPD_PATCHBAY | WUPD_BUTTONBAR);
			mwindow_global->cwindow->update(WUPD_OVERLAYS | WUPD_TOOLWIN);

			result = 1;
			break;
		}

// since things under cursor have changed...
		if(result) 
			cursor_motion_event(); 
	}

	return result;
}

void MWindowGUI::close_event() 
{
	mainmenu->quit();
}

int MWindowGUI::menu_h()
{
	return mainmenu->get_h();
}
