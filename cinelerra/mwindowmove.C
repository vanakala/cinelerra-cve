
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

#include "automation.h"
#include "clip.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
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
#include "transportque.h"
#include "zoombar.h"


void MWindow::update_plugins()
{
// Show plugins which are visible and hide plugins which aren't
// Update plugin pointers in plugin servers
}


int MWindow::expand_sample(double fixed_sample)
{
	if(gui)
	{
		if(edl->local_session->zoom_sample < 0x100000)
		{
			int64_t new_zoom_sample = edl->local_session->zoom_sample * 2;
			int64_t view_start;
			if (fixed_sample < 0)
				view_start = -1;
			else
			{
				double viewstart_position = (double)edl->local_session->view_start * 
					edl->local_session->zoom_sample /
					edl->session->sample_rate * 2 - fixed_sample;
				view_start = Units::round(viewstart_position *
					edl->session->sample_rate /
					new_zoom_sample);
			}

			zoom_sample(new_zoom_sample, view_start);
		}
	}
	return 0;
}

int MWindow::zoom_in_sample(double fixed_sample)
{
	if(gui)
	{
		if(edl->local_session->zoom_sample > 1)
		{
			int64_t new_zoom_sample = edl->local_session->zoom_sample / 2;
			int64_t view_start;
			if (fixed_sample < 0)
				view_start = -1;
			else
			{
				double viewstart_position = (double)edl->local_session->view_start * 
					edl->local_session->zoom_sample /
					edl->session->sample_rate;
				viewstart_position = viewstart_position + (fixed_sample - viewstart_position) / 2;

				view_start = Units::round(viewstart_position *
					edl->session->sample_rate /
					new_zoom_sample);
			}
			
			zoom_sample(new_zoom_sample, view_start);
		}
	}
	return 0;
}

int MWindow::zoom_sample(int64_t zoom_sample, int64_t view_start)
{
	CLIP(zoom_sample, 1, 0x100000);
	edl->local_session->zoom_sample = zoom_sample;
	if (view_start < 0)
		find_cursor();
	else
		edl->local_session->view_start = view_start;
 			
	gui->get_scrollbars();

	if(!gui->samplescroll) edl->local_session->view_start = 0;
	samplemovement(edl->local_session->view_start);
	gui->zoombar->sample_zoom->update(zoom_sample);
	return 0;
}

void MWindow::find_cursor()
{
// 	if((edl->local_session->selectionend > 
// 		(double)gui->canvas->get_w() * 
// 		edl->local_session->zoom_sample / 
// 		edl->session->sample_rate) ||
// 		(edl->local_session->selectionstart > 
// 		(double)gui->canvas->get_w() * 
// 		edl->local_session->zoom_sample / 
// 		edl->session->sample_rate))
// 	{
		edl->local_session->view_start = 
			Units::round((edl->local_session->get_selectionend(1) + 
			edl->local_session->get_selectionstart(1)) / 
			2 *
			edl->session->sample_rate /
			edl->local_session->zoom_sample - 
			(double)gui->canvas->get_w() / 
			2);
// 	}
// 	else
// 		edl->local_session->view_start = 0;

//printf("MWindow::find_cursor %f\n", edl->local_session->view_start);
	if(edl->local_session->view_start < 0) edl->local_session->view_start = 0;
}


void MWindow::fit_selection()
{
	if(EQUIV(edl->local_session->get_selectionstart(1),
		edl->local_session->get_selectionend(1)))
	{
		double total_samples = edl->tracks->total_length() * 
			edl->session->sample_rate;
		for(edl->local_session->zoom_sample = 1; 
			gui->canvas->get_w() * edl->local_session->zoom_sample < total_samples; 
			edl->local_session->zoom_sample *= 2)
			;
	}
	else
	{
		double total_samples = (edl->local_session->get_selectionend(1) - 
			edl->local_session->get_selectionstart(1)) * 
			edl->session->sample_rate;
		for(edl->local_session->zoom_sample = 1; 
			gui->canvas->get_w() * edl->local_session->zoom_sample < total_samples; 
			edl->local_session->zoom_sample *= 2)
			;
	}

	edl->local_session->zoom_sample = MIN(0x100000, 
		edl->local_session->zoom_sample);
	zoom_sample(edl->local_session->zoom_sample);
}


void MWindow::fit_autos(int doall)
{
	float min = 0, max = 0;
	double start, end;

// Test all autos
	if(EQUIV(edl->local_session->get_selectionstart(1),
		edl->local_session->get_selectionend(1)))
	{
		start = 0;
		end = edl->tracks->total_length();
	}
	else
// Test autos in highlighting only
	{
		start = edl->local_session->get_selectionstart(1);
		end = edl->local_session->get_selectionend(1);
	}

	int forstart = edl->local_session->zoombar_showautotype;
	int forend   = edl->local_session->zoombar_showautotype + 1;
	
	if (doall) {
		forstart = 0;
		forend   = AUTOGROUPTYPE_COUNT;
	}

	for (int i = forstart; i < forend; i++)
	{
// Adjust min and max
		edl->tracks->get_automation_extents(&min, &max, start, end, i);
//printf("MWindow::fit_autos %d %f %f results in ", i, min, max);

		float range = max - min;
		switch (i) 
		{
		case AUTOGROUPTYPE_AUDIO_FADE:
		case AUTOGROUPTYPE_VIDEO_FADE:
			if (range < 0.1) {
				min = MIN(min, edl->local_session->automation_mins[i]);
				max = MAX(max, edl->local_session->automation_maxs[i]);
			}
			break;
		case AUTOGROUPTYPE_ZOOM:
			if (range < 0.001) {
				min = floor(min*50)/100;
				max = floor(max*200)/100;
			}
			break;
		case AUTOGROUPTYPE_X:
		case AUTOGROUPTYPE_Y:
			if (range < 5) {
				min = floor((min+max)/2) - 50;
				max = floor((min+max)/2) + 50;
			}
			break;
		}
//printf("%f %f\n", min, max);
		if (!Automation::autogrouptypes_fixedrange[i]) 
		{
			edl->local_session->automation_mins[i] = min;
			edl->local_session->automation_maxs[i] = max;
		}
	}

// Show range in zoombar
	gui->zoombar->update();

// Draw
	gui->canvas->draw_overlays();
	gui->canvas->flash();
}


void MWindow::change_currentautorange(int autogrouptype, int increment, int changemax) {
	float val;
	if (changemax) {
		val = edl->local_session->automation_maxs[autogrouptype];
	} else {
		val = edl->local_session->automation_mins[autogrouptype];
	}

	if (increment) 
	{
		switch (autogrouptype) {
		case AUTOGROUPTYPE_AUDIO_FADE:
			val += 2;
			break;
		case AUTOGROUPTYPE_VIDEO_FADE:
			val += 1;
			break;
		case AUTOGROUPTYPE_ZOOM:
			if (val == 0) 
				val = 0.001;
			else 
				val = val*2;
			break;
		case AUTOGROUPTYPE_X:
		case AUTOGROUPTYPE_Y:
			val = floor(val + 5);
			break;
		}
	} 
	else 
	{ // decrement
		switch (autogrouptype) {
		case AUTOGROUPTYPE_AUDIO_FADE:
			val -= 2;
			break;
		case AUTOGROUPTYPE_VIDEO_FADE:
			val -= 1;
			break;
		case AUTOGROUPTYPE_ZOOM:
			if (val > 0) val = val/2;
			break;
		case AUTOGROUPTYPE_X:
		case AUTOGROUPTYPE_Y:
			val = floor(val-5);
			break;
		}
	}

	AUTOMATIONVIEWCLAMPS(val, autogrouptype);

	if (changemax) {
		if (val > edl->local_session->automation_mins[autogrouptype])
			edl->local_session->automation_maxs[autogrouptype] = val;
	}
	else
	{
		if (val < edl->local_session->automation_maxs[autogrouptype])
			edl->local_session->automation_mins[autogrouptype] = val;
	}
}


void MWindow::expand_autos(int changeall, int domin, int domax)
{
	if (changeall)
		for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
			if (domin) change_currentautorange(i, 1, 0);
			if (domax) change_currentautorange(i, 1, 1);
		}
	else
	{
		if (domin) change_currentautorange(edl->local_session->zoombar_showautotype, 1, 0);
		if (domax) change_currentautorange(edl->local_session->zoombar_showautotype, 1, 1);
	}
	gui->zoombar->update_autozoom();
	gui->canvas->draw_overlays();
	gui->patchbay->update();
	gui->canvas->flash();
}

void MWindow::shrink_autos(int changeall, int domin, int domax)
{
	if (changeall)
		for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
			if (domin) change_currentautorange(i, 0, 0);
			if (domax) change_currentautorange(i, 0, 1);
		}
	else
	{
		if (domin) change_currentautorange(edl->local_session->zoombar_showautotype, 0, 0);
		if (domax) change_currentautorange(edl->local_session->zoombar_showautotype, 0, 1);
	}
	gui->zoombar->update_autozoom();
	gui->canvas->draw_overlays();
	gui->patchbay->update();
	gui->canvas->flash();
}


void MWindow::zoom_amp(int64_t zoom_amp)
{
	edl->local_session->zoom_y = zoom_amp;
	gui->canvas->draw(0, 0);
	gui->canvas->flash();
	gui->patchbay->update();
	gui->flush();
}

void MWindow::zoom_track(int64_t zoom_track)
{
	edl->local_session->zoom_y = (int64_t)((float)edl->local_session->zoom_y * 
		zoom_track / 
		edl->local_session->zoom_track);
	CLAMP(edl->local_session->zoom_y, MIN_AMP_ZOOM, MAX_AMP_ZOOM);
	edl->local_session->zoom_track = zoom_track;
	trackmovement(edl->local_session->track_start);
//printf("MWindow::zoom_track %d %d\n", edl->local_session->zoom_y, edl->local_session->zoom_track);
}

void MWindow::trackmovement(int track_start)
{
	edl->local_session->track_start = track_start;
	if(edl->local_session->track_start < 0) edl->local_session->track_start = 0;
	edl->tracks->update_y_pixels(theme);
	gui->get_scrollbars();
	gui->canvas->draw(0, 0);
	gui->patchbay->update();
	gui->canvas->flash();
	gui->flush();
}

void MWindow::move_up(int64_t distance)
{
	if(!gui->trackscroll) return;
	if(distance == 0) distance = edl->local_session->zoom_track;
	edl->local_session->track_start -= distance;
	trackmovement(edl->local_session->track_start);
}

void MWindow::move_down(int64_t distance)
{
	if(!gui->trackscroll) return;
	if(distance == 0) distance = edl->local_session->zoom_track;
	edl->local_session->track_start += distance;
	trackmovement(edl->local_session->track_start);
}

int MWindow::goto_end()
{
	int64_t old_view_start = edl->local_session->view_start;

	if(edl->tracks->total_length() > (double)gui->canvas->get_w() * 
		edl->local_session->zoom_sample / 
		edl->session->sample_rate)
	{
		edl->local_session->view_start = 
			Units::round(edl->tracks->total_length() * 
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				gui->canvas->get_w() / 
				2);
	}
	else
	{
		edl->local_session->view_start = 0;
	}

	if(gui->shift_down())
	{
		edl->local_session->set_selectionend(edl->tracks->total_length());
	}
	else
	{
		edl->local_session->set_selectionstart(edl->tracks->total_length());
		edl->local_session->set_selectionend(edl->tracks->total_length());
	}

	if(edl->local_session->view_start != old_view_start) 
		samplemovement(edl->local_session->view_start);

	update_plugin_guis();
	gui->patchbay->update();
	gui->cursor->update();
	gui->canvas->activate();
	gui->zoombar->update();
	cwindow->update(1, 0, 0, 0, 1);
	return 0;
}

int MWindow::goto_start()
{
	int64_t old_view_start = edl->local_session->view_start;

	edl->local_session->view_start = 0;
	if(gui->shift_down())
	{
		edl->local_session->set_selectionstart(0);
	}
	else
	{
		edl->local_session->set_selectionstart(0);
		edl->local_session->set_selectionend(0);
	}

	if(edl->local_session->view_start != old_view_start)
		samplemovement(edl->local_session->view_start);

	update_plugin_guis();
	gui->patchbay->update();
	gui->cursor->update();
	gui->canvas->activate();
	gui->zoombar->update();
	cwindow->update(1, 0, 0, 0, 1);
	return 0;
}

int MWindow::samplemovement(int64_t view_start)
{
	edl->local_session->view_start = view_start;
	if(edl->local_session->view_start < 0) edl->local_session->view_start = 0;
	gui->canvas->draw();
	gui->cursor->show();
	gui->canvas->flash();
	gui->timebar->update();
	gui->zoombar->update();

	if(gui->samplescroll) gui->samplescroll->set_position();
	return 0;
}

int MWindow::move_left(int64_t distance)
{
	if(!distance) 
		distance = gui->canvas->get_w() / 
			10;
	edl->local_session->view_start -= distance;
	if(edl->local_session->view_start < 0) edl->local_session->view_start = 0;
	samplemovement(edl->local_session->view_start);
	return 0;
}

int MWindow::move_right(int64_t distance)
{
	if(!distance) 
		distance = gui->canvas->get_w() / 
			10;
	edl->local_session->view_start += distance;
	samplemovement(edl->local_session->view_start);
	return 0;
}

void MWindow::select_all()
{
	edl->local_session->set_selectionstart(0);
	edl->local_session->set_selectionend(edl->tracks->total_length());
	gui->update(0, 1, 1, 1, 0, 1, 0);
	gui->canvas->activate();
	cwindow->update(1, 0, 0);
}

int MWindow::next_label(int shift_down)
{
	Label *current = edl->labels->next_label(
			edl->local_session->get_selectionstart(1));

	if(current)
	{

		edl->local_session->set_selectionend(current->position);
		if(!shift_down) 
			edl->local_session->set_selectionstart(
				edl->local_session->get_selectionend(1));

		update_plugin_guis();
		gui->patchbay->update();
		if(edl->local_session->get_selectionend(1) >= 
			(double)edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate + 
			gui->canvas->time_visible() ||
			edl->local_session->get_selectionend(1) < (double)edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->get_selectionend(1) *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				gui->canvas->get_w() / 
				2));
		}
		else
		{
			gui->timebar->update();
			gui->cursor->hide(0);
			gui->cursor->draw(1);
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
		}
		cwindow->update(1, 0, 0, 0, 1);
	}
	else
	{
		goto_end();
	}
	return 0;
}

int MWindow::prev_label(int shift_down)
{
	Label *current = edl->labels->prev_label(
			edl->local_session->get_selectionstart(1));;

	if(current)
	{
		edl->local_session->set_selectionstart(current->position);
		if(!shift_down) 
			edl->local_session->set_selectionend(edl->local_session->get_selectionstart(1));

		update_plugin_guis();
		gui->patchbay->update();
// Scroll the display
		if(edl->local_session->get_selectionstart(1) >= edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate + 
			gui->canvas->time_visible() 
		||
			edl->local_session->get_selectionstart(1) < edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->get_selectionstart(1) *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				gui->canvas->get_w() / 
				2));
		}
		else
// Don't scroll the display
		{
			gui->timebar->update();
			gui->cursor->hide(0);
			gui->cursor->draw(1);
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
		}
		cwindow->update(1, 0, 0, 0, 1);
	}
	else
	{
		goto_start();
	}
	return 0;
}






int MWindow::next_edit_handle(int shift_down)
{
	double position = edl->local_session->get_selectionend(1);
	Units::fix_double(&position);
	double new_position = INFINITY;
// Test for edit handles after cursor position
	for (Track *track = edl->tracks->first; track; track = track->next)
	{
		if (track->record)
		{
			for (Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				double edit_end = track->from_units(edit->startproject + edit->length);
				Units::fix_double(&edit_end);
				if (edit_end > position && edit_end < new_position)
					new_position = edit_end;
			}
		}
	}

	if(new_position != INFINITY)
	{

		edl->local_session->set_selectionend(new_position);
printf("MWindow::next_edit_handle %d\n", shift_down);
		if(!shift_down) 
			edl->local_session->set_selectionstart(
				edl->local_session->get_selectionend(1));

		update_plugin_guis();
		gui->patchbay->update();
		if(edl->local_session->get_selectionend(1) >= 
			(double)edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate + 
			gui->canvas->time_visible() ||
			edl->local_session->get_selectionend(1) < (double)edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->get_selectionend(1) *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				gui->canvas->get_w() / 
				2));
		}
		else
		{
			gui->timebar->update();
			gui->cursor->hide(0);
			gui->cursor->draw(1);
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
		}
		cwindow->update(1, 0, 0, 0, 1);
	}
	else
	{
		goto_end();
	}
	return 0;
}

int MWindow::prev_edit_handle(int shift_down)
{
	double position = edl->local_session->get_selectionstart(1);
	double new_position = -1;
	Units::fix_double(&position);
// Test for edit handles before cursor position
	for (Track *track = edl->tracks->first; track; track = track->next)
	{
		if (track->record)
		{
			for (Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				double edit_end = track->from_units(edit->startproject);
				Units::fix_double(&edit_end);
				if (edit_end < position && edit_end > new_position)
					new_position = edit_end;
			}
		}
	}

	if(new_position != -1)
	{

		edl->local_session->set_selectionstart(new_position);
printf("MWindow::next_edit_handle %d\n", shift_down);
		if(!shift_down) 
			edl->local_session->set_selectionend(edl->local_session->get_selectionstart(1));

		update_plugin_guis();
		gui->patchbay->update();
// Scroll the display
		if(edl->local_session->get_selectionstart(1) >= edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate + 
			gui->canvas->time_visible() 
		||
			edl->local_session->get_selectionstart(1) < edl->local_session->view_start *
			edl->local_session->zoom_sample /
			edl->session->sample_rate)
		{
			samplemovement((int64_t)(edl->local_session->get_selectionstart(1) *
				edl->session->sample_rate /
				edl->local_session->zoom_sample - 
				gui->canvas->get_w() / 
				2));
		}
		else
// Don't scroll the display
		{
			gui->timebar->update();
			gui->cursor->hide(0);
			gui->cursor->draw(1);
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
		}
		cwindow->update(1, 0, 0, 0, 1);
	}
	else
	{
		goto_start();
	}
	return 0;
}








int MWindow::expand_y()
{
	int result = edl->local_session->zoom_y * 2;
	result = MIN(result, MAX_AMP_ZOOM);
	zoom_amp(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::zoom_in_y()
{
	int result = edl->local_session->zoom_y / 2;
	result = MAX(result, MIN_AMP_ZOOM);
	zoom_amp(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::expand_t()
{
	int result = edl->local_session->zoom_track * 2;
	result = MIN(result, MAX_TRACK_ZOOM);
	zoom_track(result);
	gui->zoombar->update();
	return 0;
}

int MWindow::zoom_in_t()
{
	int result = edl->local_session->zoom_track / 2;
	result = MAX(result, MIN_TRACK_ZOOM);
	zoom_track(result);
	gui->zoombar->update();
	return 0;
}

