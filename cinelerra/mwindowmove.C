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


void MWindow::fit_autos()
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

// Adjust min and max
	edl->tracks->get_automation_extents(&min, &max, start, end);
//printf("MWindow::fit_autos %f %f\n", min, max);

// Pad
	float range = max - min;
// No automation visible
	if(range < 0.001)
	{
		min -= 1;
		max += 1;
	}
	float pad = range * 0.33;
	min -= pad;
	max += pad;
	edl->local_session->automation_min = min;
	edl->local_session->automation_max = max;

// Show range in zoombar
	gui->zoombar->update();

// Draw
	gui->canvas->draw_overlays();
	gui->canvas->flash();
}


void MWindow::expand_autos()
{
	float range = edl->local_session->automation_max - 
		edl->local_session->automation_min;
	float center = range / 2 + 
		edl->local_session->automation_min;
	if(EQUIV(range, 0)) range = 0.002;
	edl->local_session->automation_min = center - range;
	edl->local_session->automation_max = center + range;
	gui->zoombar->update_autozoom();
	gui->canvas->draw_overlays();
	gui->canvas->flash();
}

void MWindow::shrink_autos()
{
	float range = edl->local_session->automation_max - 
		edl->local_session->automation_min;
	float center = range / 2 + 
		edl->local_session->automation_min;
	float new_range = range / 4;
	edl->local_session->automation_min = center - new_range;
	edl->local_session->automation_max = center + new_range;
	gui->zoombar->update_autozoom();
	gui->canvas->draw_overlays();
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
	cwindow->update(1, 0, 0, 0, 0);
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
	cwindow->update(1, 0, 0, 0, 0);
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

int MWindow::next_label()
{
	Label *current = edl->labels->next_label(
			edl->local_session->get_selectionstart(1));

	if(current)
	{

		edl->local_session->set_selectionend(current->position);
		if(!gui->shift_down()) 
			edl->local_session->set_selectionstart(
				edl->local_session->get_selectionend(1));

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
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
		{
			update_plugin_guis();
			gui->patchbay->update();
			gui->timebar->update();
			gui->cursor->hide();
			gui->cursor->draw();
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
			cwindow->update(1, 0, 0);
		}
	}
	else
	{
		goto_end();
	}
	return 0;
}

int MWindow::prev_label()
{
	Label *current = edl->labels->prev_label(
			edl->local_session->get_selectionstart(1));;

	if(current)
	{
		edl->local_session->set_selectionstart(current->position);
		if(!gui->shift_down()) 
			edl->local_session->set_selectionend(edl->local_session->get_selectionstart(1));

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
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
// Don't scroll the display
		{
			update_plugin_guis();
			gui->patchbay->update();
			gui->timebar->update();
			gui->cursor->hide();
			gui->cursor->draw();
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
			cwindow->update(1, 0, 0);
		}
	}
	else
	{
		goto_start();
	}
	return 0;
}






int MWindow::next_edit_handle()
{
	double position = edl->local_session->get_selectionend(1);
	double new_position = INFINITY;
// Test for edit handles after cursor position
	for (Track *track = edl->tracks->first; track; track = track->next)
	{
		if (track->record)
		{
			for (Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				double edit_end = track->from_units(edit->startproject + edit->length);
				if (edit_end > position && edit_end < new_position)
					new_position = edit_end;
			}
		}
	}

	if(new_position != INFINITY)
	{

		edl->local_session->set_selectionend(new_position);
		if(!gui->shift_down()) 
			edl->local_session->set_selectionstart(
				edl->local_session->get_selectionend(1));

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
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
		{
			update_plugin_guis();
			gui->patchbay->update();
			gui->timebar->update();
			gui->cursor->hide();
			gui->cursor->draw();
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
			cwindow->update(1, 0, 0);
		}
	}
	else
	{
		goto_end();
	}
	return 0;
}

int MWindow::prev_edit_handle()
{
	double position = edl->local_session->get_selectionstart(1);
	double new_position = -1;
// Test for edit handles before cursor position
	for (Track *track = edl->tracks->first; track; track = track->next)
	{
		if (track->record)
		{
			for (Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				double edit_end = track->from_units(edit->startproject);
				if (edit_end < position && edit_end > new_position)
					new_position = edit_end;
			}
		}
	}

	if(new_position != -1)
	{

		edl->local_session->set_selectionstart(new_position);
		if(!gui->shift_down()) 
			edl->local_session->set_selectionend(edl->local_session->get_selectionstart(1));

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
			cwindow->update(1, 0, 0, 0, 0);
		}
		else
// Don't scroll the display
		{
			update_plugin_guis();
			gui->patchbay->update();
			gui->timebar->update();
			gui->cursor->hide();
			gui->cursor->draw();
			gui->zoombar->update();
			gui->canvas->flash();
			gui->flush();
			cwindow->update(1, 0, 0);
		}
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

