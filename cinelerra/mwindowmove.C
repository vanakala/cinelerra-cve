// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "automation.h"
#include "bcsignals.h"
#include "cinelerra.h"
#include "clip.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "datatype.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "labels.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "plugin.h"
#include "samplescroll.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "zoombar.h"


void MWindow::expand_sample(void)
{
	if(gui)
	{
		if(master_edl->local_session->zoom_time < MAX_ZOOM_TIME)
			zoom_time(master_edl->local_session->zoom_time * 2);
	}
}

void MWindow::zoom_in_sample(void)
{
	if(gui)
	{
		if(master_edl->local_session->zoom_time > MIN_ZOOM_TIME)
			zoom_time(master_edl->local_session->zoom_time / 2);
	}
}

void MWindow::zoom_time(ptstime zoom)
{
	master_edl->local_session->zoom_time = zoom;
	find_cursor();

	gui->get_scrollbars();

	if(!gui->samplescroll)
		master_edl->local_session->view_start_pts = 0;

	samplemovement(master_edl->local_session->view_start_pts, 1);
}

void MWindow::find_cursor(void)
{
	master_edl->local_session->view_start_pts =
		(master_edl->local_session->get_selectionend(1) +
		master_edl->local_session->get_selectionstart(1)) / 2 -
		((ptstime)gui->canvas->get_w() / 2 * master_edl->local_session->zoom_time);

	if(master_edl->local_session->view_start_pts < 0)
		master_edl->local_session->view_start_pts = 0;
}

void MWindow::fit_selection(void)
{
	ptstime selection_length, canvas_length, new_zoom;

	if(EQUIV(master_edl->local_session->get_selectionstart(1),
			master_edl->local_session->get_selectionend(1)))
		selection_length = master_edl->duration();
	else
		selection_length = (master_edl->local_session->get_selectionend(1) -
			master_edl->local_session->get_selectionstart(1));

	canvas_length = master_edl->local_session->zoom_time * gui->canvas->get_w();
	new_zoom = master_edl->local_session->zoom_time / (canvas_length / selection_length);

	CLAMP(new_zoom, MIN_ZOOM_TIME, MAX_ZOOM_TIME);
	zoom_time(new_zoom);
}

void MWindow::fit_autos(int doall)
{
	double min = 0, max = 0;
	ptstime start, end;
// Test all autos
	if(EQUIV(master_edl->local_session->get_selectionstart(1),
		master_edl->local_session->get_selectionend(1)))
	{
		start = 0;
		end = master_edl->duration();
	}
	else
// Test autos in highlighting only
	{
		start = master_edl->local_session->get_selectionstart(1);
		end = master_edl->local_session->get_selectionend(1);
	}

	int forstart = master_edl->local_session->zoombar_showautotype;
	int forend   = master_edl->local_session->zoombar_showautotype + 1;

	if(doall)
	{
		forstart = 0;
		forend   = AUTOGROUPTYPE_COUNT;
	}

	for(int i = forstart; i < forend; i++)
	{
// Adjust min and max
		master_edl->tracks->get_automation_extents(&min, &max, start, end, i);

		double range = max - min;

		switch(i)
		{
		case AUTOGROUPTYPE_AUDIO_FADE:
		case AUTOGROUPTYPE_VIDEO_FADE:
			if(range < 0.1)
			{
				min = MIN(min, master_edl->local_session->automation_mins[i]);
				max = MAX(max, master_edl->local_session->automation_maxs[i]);
			}
			break;

		case AUTOGROUPTYPE_ZOOM:
			if(range < 0.001)
			{
				min = floor(min * 50) / 100;
				max = floor(max * 200) / 100;
			}
			break;
		}

		if(!Automation::autogrouptypes[i].fixedrange)
		{
			master_edl->local_session->automation_mins[i] = min;
			master_edl->local_session->automation_maxs[i] = max;
		}
	}

// Show range in zoombar
	update_gui(WUPD_ZOOMBAR);

// Draw
	draw_canvas_overlays();
}

void MWindow::change_currentautorange(int autogrouptype, int increment, int changemax)
{
	double val;

	if(changemax)
		val = master_edl->local_session->automation_maxs[autogrouptype];
	else
		val = master_edl->local_session->automation_mins[autogrouptype];

	if(increment)
	{
		switch(autogrouptype)
		{
		case AUTOGROUPTYPE_AUDIO_FADE:
			val += 2;
			break;
		case AUTOGROUPTYPE_VIDEO_FADE:
			val += 1;
			break;
		case AUTOGROUPTYPE_ZOOM:
			if(EQUIV(val,0))
				val = 0.001;
			else 
				val = val * 2;
			break;
		}
	}
	else 
	{ // decrement
		switch (autogrouptype)
		{
		case AUTOGROUPTYPE_AUDIO_FADE:
			val -= 2;
			break;
		case AUTOGROUPTYPE_VIDEO_FADE:
			val -= 1;
			break;
		case AUTOGROUPTYPE_ZOOM:
			if(val > 0)
				val = val / 2;
			break;
		}
	}

	AUTOMATIONVIEWCLAMPS(val, autogrouptype);

	if(changemax)
	{
		if(val > master_edl->local_session->automation_mins[autogrouptype])
			master_edl->local_session->automation_maxs[autogrouptype] = val;
	}
	else
	{
		if(val < master_edl->local_session->automation_maxs[autogrouptype])
			master_edl->local_session->automation_mins[autogrouptype] = val;
	}
}

void MWindow::expand_autos(int changeall, int domin, int domax)
{
	if(changeall)
	{
		for(int i = 0; i < AUTOGROUPTYPE_COUNT; i++)
		{
			if(domin)
				change_currentautorange(i, 1, 0);
			if(domax)
				change_currentautorange(i, 1, 1);
		}
	}
	else
	{
		if(domin)
			change_currentautorange(master_edl->local_session->zoombar_showautotype, 1, 0);
		if(domax)
			change_currentautorange(master_edl->local_session->zoombar_showautotype, 1, 1);
	}
	update_gui(WUPD_ZOOMBAR | WUPD_PATCHBAY);
	draw_canvas_overlays();
}

void MWindow::shrink_autos(int changeall, int domin, int domax)
{
	if(changeall)
	{
		for(int i = 0; i < AUTOGROUPTYPE_COUNT; i++)
		{
			if(domin)
				change_currentautorange(i, 0, 0);
			if(domax)
				change_currentautorange(i, 0, 1);
		}
	}
	else
	{
		if(domin)
			change_currentautorange(master_edl->local_session->zoombar_showautotype, 0, 0);
		if(domax)
			change_currentautorange(master_edl->local_session->zoombar_showautotype, 0, 1);
	}
	update_gui(WUPD_ZOOMBAR | WUPD_PATCHBAY);
	draw_canvas_overlays();
}

void MWindow::zoom_amp(int zoom_amp)
{
	master_edl->local_session->zoom_y = zoom_amp;
	update_gui(WUPD_CANVINCR | WUPD_PATCHBAY | WUPD_ZOOMBAR);
}

void MWindow::zoom_track(int zoom_track)
{
	master_edl->local_session->zoom_y =
		round((double)master_edl->local_session->zoom_y *
		zoom_track / master_edl->local_session->zoom_track);
	CLAMP(master_edl->local_session->zoom_y, MIN_AMP_ZOOM, MAX_AMP_ZOOM);
	master_edl->local_session->zoom_track = zoom_track;
	trackmovement(master_edl->local_session->track_start);
	update_gui(WUPD_ZOOMBAR);
}

void MWindow::trackmovement(int track_start)
{
	master_edl->local_session->track_start = track_start;
	if(master_edl->local_session->track_start < 0)
		master_edl->local_session->track_start = 0;
	master_edl->tracks->update_y_pixels(theme);
	update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_PATCHBAY);
}

void MWindow::move_up(int distance)
{
	if(!gui->trackscroll) return;
	if(distance == 0) distance = master_edl->local_session->zoom_track;
	master_edl->local_session->track_start -= distance;
	trackmovement(master_edl->local_session->track_start);
}

void MWindow::move_down(int distance)
{
	if(!gui->trackscroll) return;
	if(distance == 0) distance = master_edl->local_session->zoom_track;
	master_edl->local_session->track_start += distance;
	trackmovement(master_edl->local_session->track_start);
}

void MWindow::goto_end()
{
	ptstime new_view_start;

	if(master_edl->duration() > (ptstime)gui->canvas->get_w() *
			master_edl->local_session->zoom_time)
		new_view_start = master_edl->duration() -
			(gui->canvas->get_w() * master_edl->local_session->zoom_time) / 2;
	else
		new_view_start = 0;

	if(gui->shift_down())
		master_edl->local_session->set_selectionend(master_edl->duration());
	else
		master_edl->local_session->set_selection(master_edl->duration());

	gui->cursor->hide();
	samplemovement(new_view_start);
	update_gui(WUPD_PATCHBAY);
	update_plugin_guis();
	gui->cursor->show();
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
}

void MWindow::goto_start()
{
	if(gui->shift_down())
		master_edl->local_session->set_selectionstart(0);
	else
		master_edl->local_session->set_selection(0);

	gui->cursor->hide();
	samplemovement(0);
	update_gui(WUPD_PATCHBAY);
	update_plugin_guis();
	gui->cursor->show();
	cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
}

void MWindow::samplemovement(ptstime view_start, int force_redraw)
{
	ptstime old_pts = master_edl->local_session->view_start_pts;

	master_edl->local_session->view_start_pts = view_start;

	if(master_edl->local_session->view_start_pts < 0)
		master_edl->local_session->view_start_pts = 0;

	if(force_redraw || !PTSEQU(old_pts, master_edl->local_session->view_start_pts))
		update_gui(WUPD_CANVINCR | WUPD_TIMEBAR |
			WUPD_ZOOMBAR | WUPD_SCROLLBARS);
}

void MWindow::move_left(int distance)
{
	ptstime new_pts;

	if(!distance)
		distance = gui->canvas->get_w() / 10;
	new_pts = master_edl->local_session->view_start_pts -
		distance * master_edl->local_session->zoom_time;
	samplemovement(new_pts);
}

void MWindow::move_right(int distance)
{
	ptstime new_pts;

	if(!distance)
		distance = gui->canvas->get_w() / 10;
	new_pts = master_edl->local_session->view_start_pts +
		distance * master_edl->local_session->zoom_time;
	samplemovement(new_pts);
}

void MWindow::select_all(void)
{
	master_edl->local_session->set_selectionstart(0);
	master_edl->local_session->set_selectionend(master_edl->duration());
	update_gui(WUPD_CANVINCR | WUPD_TIMEBAR | WUPD_ZOOMBAR | WUPD_CLOCK);
	cwindow->update(WUPD_POSITION);
}

void MWindow::next_label(int shift_down)
{
	Label *current = master_edl->labels->next_label(
			master_edl->local_session->get_selectionstart(1));

	if(current)
	{
		master_edl->local_session->set_selectionend(current->position);
		if(!shift_down)
			master_edl->local_session->set_selectionstart(
				master_edl->local_session->get_selectionend(1));

		update_plugin_guis();
		update_gui(WUPD_PATCHBAY);

		if(master_edl->local_session->get_selectionend(1) >=
			master_edl->local_session->view_start_pts +
			gui->canvas->time_visible() ||
			master_edl->local_session->get_selectionend(1) <
			master_edl->local_session->view_start_pts)
		{
			samplemovement(master_edl->local_session->get_selectionend(1) -
				(gui->canvas->get_w() * master_edl->local_session->zoom_time /
				2));
		}
		else
			update_gui(WUPD_TIMEBAR | WUPD_CURSOR | WUPD_ZOOMBAR);

		cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	}
	else
	{
		goto_end();
	}
}

void MWindow::prev_label(int shift_down)
{
	Label *current = master_edl->labels->prev_label(
			master_edl->local_session->get_selectionstart(1));

	if(current)
	{
		master_edl->local_session->set_selectionstart(current->position);
		if(!shift_down) 
			master_edl->local_session->set_selectionend(master_edl->local_session->get_selectionstart(1));

		update_plugin_guis();
		update_gui(WUPD_PATCHBAY);
// Scroll the display
		if(master_edl->local_session->get_selectionstart(1) >=
			master_edl->local_session->view_start_pts +
			gui->canvas->time_visible() ||
			master_edl->local_session->get_selectionstart(1) <
			master_edl->local_session->view_start_pts)
		{
			samplemovement(master_edl->local_session->get_selectionstart(1) -
				gui->canvas->get_w() * master_edl->local_session->zoom_time /
				2);
		}
		else
// Don't scroll the display
			update_gui(WUPD_TIMEBAR | WUPD_CURSOR | WUPD_ZOOMBAR);
		cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	}
	else
	{
		goto_start();
	}
}

void MWindow::next_edit_handle(int shift_down)
{
	ptstime position = master_edl->local_session->get_selectionend(1);
	ptstime new_position = INFINITY;

// Test for edit handles after cursor position
	for(Track *track = master_edl->first_track(); track; track = track->next)
	{
		if(track->record)
		{
			for(Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				ptstime edit_end = edit->end_pts();
				if(edit_end > position && edit_end < new_position)
					new_position = edit_end;
			}
		}
	}

	if(!isinf(new_position))
	{
		master_edl->local_session->set_selectionend(new_position);
		if(!shift_down) 
			master_edl->local_session->set_selectionstart(
				master_edl->local_session->get_selectionend(1));

		update_plugin_guis();
		update_gui(WUPD_PATCHBAY);

		if(master_edl->local_session->get_selectionend(1) >=
			master_edl->local_session->view_start_pts +
			gui->canvas->time_visible() ||
			master_edl->local_session->get_selectionend(1) < master_edl->local_session->view_start_pts)
		{
			samplemovement(master_edl->local_session->get_selectionend(1) -
				gui->canvas->get_w() *
				master_edl->local_session->zoom_time / 2);
		}
		else
			update_gui(WUPD_TIMEBAR | WUPD_CURSOR | WUPD_ZOOMBAR);

		cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	}
	else
		goto_end();
}

void MWindow::prev_edit_handle(int shift_down)
{
	ptstime position = master_edl->local_session->get_selectionstart(1);
	ptstime new_position = -1;

// Test for edit handles before cursor position
	for(Track *track = master_edl->first_track(); track; track = track->next)
	{
		if(track->record)
		{
			for(Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				ptstime edit_end = edit->get_pts();
				if (edit_end < position && edit_end > new_position)
					new_position = edit_end;
			}
		}
	}

	if(new_position != -1)
	{
		master_edl->local_session->set_selectionstart(new_position);

		if(!shift_down) 
			master_edl->local_session->set_selectionend(
				master_edl->local_session->get_selectionstart(1));

		update_plugin_guis();
		update_gui(WUPD_PATCHBAY);
// Scroll the display
		if(master_edl->local_session->get_selectionstart(1) >= master_edl->local_session->view_start_pts +
			gui->canvas->time_visible() ||
			master_edl->local_session->get_selectionstart(1) < master_edl->local_session->view_start_pts)
		{
			samplemovement((master_edl->local_session->get_selectionstart(1) -
				gui->canvas->get_w() * master_edl->local_session->zoom_time /
				2));
		}
		else
// Don't scroll the display
			update_gui(WUPD_TIMEBAR | WUPD_CURSOR | WUPD_ZOOMBAR);

		cwindow->update(WUPD_POSITION | WUPD_TIMEBAR);
	}
	else
		goto_start();
}

void MWindow::expand_y(void)
{
	int result = master_edl->local_session->zoom_y * 2;
	result = MIN(result, MAX_AMP_ZOOM);
	zoom_amp(result);
}

void MWindow::zoom_in_y(void)
{
	int result = master_edl->local_session->zoom_y / 2;
	result = MAX(result, MIN_AMP_ZOOM);
	zoom_amp(result);
}

void MWindow::expand_t(void)
{
	int result = master_edl->local_session->zoom_track * 2;

	result = MIN(result, MAX_TRACK_ZOOM);
	zoom_track(result);
}

void MWindow::zoom_in_t(void)
{
	int result = master_edl->local_session->zoom_track / 2;

	result = MAX(result, MIN_TRACK_ZOOM);
	zoom_track(result);
}
