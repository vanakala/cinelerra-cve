// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bcresources.h"
#include "cinelerra.h"
#include "clip.h"
#include "cursors.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "labels.h"
#include "labeledit.h"
#include "language.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "units.h"
#include "vframe.h"
#include "zoombar.h"


LabelGUI::LabelGUI(TimeBar *timebar,
	int pixel, int y, ptstime position,
	VFrame **data)
 : BC_Toggle(translate_pixel(pixel), y,
		data ? data : theme_global->label_toggle,
		0)
{
	this->timebar = timebar;
	this->pixel = pixel;
	this->position = position;
	this->label = 0;
}

int LabelGUI::get_y(TimeBar *timebar)
{
	return timebar->get_h() -
		theme_global->label_toggle[0]->get_h();
}

int LabelGUI::translate_pixel(int pixel)
{
	return (pixel - theme_global->label_toggle[0]->get_w() / 2);
}

void LabelGUI::reposition()
{
	reposition_window(translate_pixel(pixel), BC_Toggle::get_y());
}

int LabelGUI::button_press_event()
{
	int result = 0;

	if(this->is_event_win() && get_buttonpress() == 3)
	{
		if(label)
			timebar->label_edit->edit_label(label);
		result = 1;
	}
	else
	{
		result = BC_Toggle::button_press_event();
	}
	return result;
}

int LabelGUI::handle_event()
{
	timebar->select_label(position);
	return 1;
}


InPointGUI::InPointGUI(TimeBar *timebar,
	int pixel,
	ptstime position)
 : LabelGUI(timebar,
	pixel, 
	get_y(timebar),
	position, 
	theme_global->in_point)
{
}

int InPointGUI::get_y(TimeBar *timebar)
{
	return (timebar->get_h() - theme_global->in_point[0]->get_h());
}


OutPointGUI::OutPointGUI(TimeBar *timebar,
	int pixel, ptstime position)
 : LabelGUI(timebar, pixel, get_y(timebar),
	position, theme_global->out_point)
{
}

int OutPointGUI::get_y(TimeBar *timebar)
{
	return timebar->get_h() -
		theme_global->out_point[0]->get_h();
}


TimeBar::TimeBar(BC_WindowBase *gui,
	int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h)
{
	this->gui = gui;
	in_point = 0;
	out_point = 0;
	current_operation = TIMEBAR_NONE;
	label_edit = new LabelEdit(mwindow_global->awindow);
}

TimeBar::~TimeBar()
{
	delete in_point;
	delete out_point;
	delete label_edit;
	labels.remove_all_objects();
}

int TimeBar::position_to_pixel(ptstime position)
{
	get_edl_length();
	return round(position / time_per_pixel);
}

void TimeBar::update_labels()
{
	int output = 0;
	EDL *edl = get_edl();

	if(edl)
	{
		for(Label *current = edl->labels->first;
			current;
			current = NEXT)
		{
			int pixel = position_to_pixel(current->position);
			if(pixel >= 0 && pixel < get_w())
			{
// Create new label
				if(output >= labels.total)
				{
					LabelGUI *new_label;
					add_subwindow(new_label = 
						new LabelGUI(this,
							pixel, 
							LabelGUI::get_y(this),
							current->position));
					new_label->set_cursor(ARROW_CURSOR);
					new_label->set_tooltip(current->textstr, 1);
					new_label->label = current;
					labels.append(new_label);
				}
				else
// Reposition old label
				{
					LabelGUI *gui = labels.values[output];
					if(gui->pixel != pixel)
					{
						gui->pixel = pixel;
						gui->reposition();
					}
					else
					{
						gui->draw_face();
					}

					labels.values[output]->position = current->position;
					labels.values[output]->set_tooltip(current->textstr, 1);
					labels.values[output]->label = current;
				}
				update_highlight(labels.values[output], current->position);
				output++;
			}
		}
	}

// Delete excess labels
	while(labels.total > output)
		labels.remove_object();
}

void TimeBar::update_highlight(BC_Toggle *toggle, ptstime position)
{
	if(master_edl->equivalent(position,
			get_edl()->local_session->get_selectionstart(1)))
		toggle->update(1);
	else
		toggle->update(0);
}

void TimeBar::update_highlights()
{
	ptstime selections[4];

	master_edl->local_session->get_selections(selections);

	for(int i = 0; i < labels.total; i++)
	{
		LabelGUI *label = labels.values[i];
		if(master_edl->equivalent(label->position,
					selections[0]) ||
				master_edl->equivalent(label->position,
					selections[1]))
			label->update(1);
		else
			label->update(0);
	}

	if(selections[2] >= 0)
	{
		if(master_edl->equivalent(selections[2], selections[0]) ||
			master_edl->equivalent(selections[2], selections[1]))
		{
			if(in_point)
				in_point->update(1);
		}
		else
		{
			if(in_point)
				in_point->update(0);
		}
	}
	if(selections[3] >= 0)
	{
		if(master_edl->equivalent(selections[3], selections[0]) ||
			master_edl->equivalent(selections[3], selections[1]))
		{
			if(out_point)
				out_point->update(1);
		}
		else
		{
			if(out_point)
				out_point->update(0);
		}
	}
}

void TimeBar::update_points()
{
	EDL *edl = get_edl();
	int pixel = 0;

	if(edl) pixel = position_to_pixel(edl->local_session->get_inpoint());

	if(in_point)
	{
		if(edl && 
			edl->local_session->inpoint_valid() && 
			pixel >= 0 && 
			pixel < get_w())
		{
			if(!EQUIV(edl->local_session->get_inpoint(), in_point->position) ||
				in_point->pixel != pixel)
			{
				in_point->pixel = pixel;
				in_point->position = edl->local_session->get_inpoint();
				in_point->reposition();
			}
			else
				in_point->draw_face();
		}
		else
		{
			delete in_point;
			in_point = 0;
		}
	}
	else
	if(edl && edl->local_session->inpoint_valid() && 
		pixel >= 0 && pixel < get_w())
	{
		add_subwindow(in_point = new InPointGUI(this, pixel,
			edl->local_session->get_inpoint()));
		in_point->set_cursor(ARROW_CURSOR);
	}
	if(in_point)
		update_highlight(in_point, in_point->position);

	pixel = 0;
	if(edl) pixel = position_to_pixel(edl->local_session->get_outpoint());

	if(out_point)
	{
		if(edl && edl->local_session->outpoint_valid() &&
			pixel >= 0 && pixel < get_w())
		{
			if(!EQUIV(edl->local_session->get_outpoint(), out_point->position) ||
				out_point->pixel != pixel)
			{
				out_point->pixel = pixel;
				out_point->position = edl->local_session->get_outpoint();
				out_point->reposition();
			}
			else
				out_point->draw_face();
		}
		else
		{
			delete out_point;
			out_point = 0;
		}
	}
	else
	if(edl && edl->local_session->outpoint_valid() &&
		pixel >= 0 && pixel < get_w())
	{
		add_subwindow(out_point = new OutPointGUI(this, pixel,
			edl->local_session->get_outpoint()));
		out_point->set_cursor(ARROW_CURSOR);
		update_highlight(out_point, out_point->position);
	}
	if(out_point)
		update_highlight(out_point, out_point->position);
}

void TimeBar::update(int fast)
{
	draw_time();
	if(!fast)
	{
		update_labels();
		update_points();
	}
	flash();
}

EDL* TimeBar::get_edl()
{
	return master_edl;
}

void TimeBar::draw_range()
{
	EDL *edl;
	int x1 = 0, x2 = 0;

	if(edl = get_edl())
	{
		get_preview_pixels(x1, x2);

		draw_3segmenth(0, 0, x1, theme_global->timebar_view_data);
		draw_top_background(get_parent(), x1, 0, x2 - x1, get_h());
		draw_3segmenth(x2, 0, get_w() - x2, theme_global->timebar_view_data);

		set_color(BLACK);
		draw_line(x1, 0, x1, get_h());
		draw_line(x2, 0, x2, get_h());

		int pixel = position_to_pixel(
			edl->local_session->get_selectionstart(1));
// Draw insertion point position if this timebar to a window which 
// has something other than the master EDL.
		set_color(RED);
		draw_line(pixel, 0, pixel, get_h());
	}
	else
		draw_top_background(get_parent(), 0, 0, get_w(), get_h());
}

void TimeBar::get_edl_length()
{
	edl_length = 0;

	if(get_edl())
		edl_length = get_edl()->total_length();

	if(!EQUIV(edl_length, 0))
		time_per_pixel = edl_length / get_w();
	else
		time_per_pixel = 0;
}

void TimeBar::get_preview_pixels(int &x1, int &x2)
{
	EDL *edl;

	x1 = 0;
	x2 = 0;

	get_edl_length();

	if(edl = get_edl())
	{
		if(!EQUIV(edl_length, 0))
		{
			if(edl->local_session->preview_end <= 0 ||
					edl->local_session->preview_end > edl_length)
				edl->local_session->preview_end = edl_length;
			if(edl->local_session->preview_start >
					edl->local_session->preview_end)
				edl->local_session->preview_start = 0;
			x1 = round(edl->local_session->preview_start / time_per_pixel);
			x2 = round(edl->local_session->preview_end / time_per_pixel);
		}
		else
		{
			x1 = 0;
			x2 = get_w();
		}
	}
}

int TimeBar::test_preview(int buttonpress)
{
	int result = 0;
	int x1, x2;
	EDL *edl;

	get_preview_pixels(x1, x2);

	if(edl = get_edl())
	{
// Inside left handle
		if(cursor_inside() &&
			get_cursor_x() >= x1 - HANDLE_W &&
			get_cursor_x() < x1 + HANDLE_W &&
// Ignore left handle if both handles are up against the left side
			x2 > HANDLE_W)
		{
			if(buttonpress)
			{
				current_operation = TIMEBAR_DRAG_LEFT;
				start_position = edl->local_session->preview_start;
				start_cursor_x = get_cursor_x();
				result = 1;
			}
			else
			if(get_cursor() != LEFT_CURSOR)
			{
				result = 1;
				set_cursor(LEFT_CURSOR);
			}
		}
		else
// Inside right handle
		if(cursor_inside() &&
			get_cursor_x() >= x2 - HANDLE_W &&
			get_cursor_x() < x2 + HANDLE_W &&
// Ignore right handle if both handles are up against the right side
			x1 < get_w() - HANDLE_W)
		{
			if(buttonpress)
			{
				current_operation = TIMEBAR_DRAG_RIGHT;
				start_position = edl->local_session->preview_end;
				start_cursor_x = get_cursor_x();
				result = 1;
			}
			else
			if(get_cursor() != RIGHT_CURSOR)
			{
				result = 1;
				set_cursor(RIGHT_CURSOR);
			}
		}
		else
		if(cursor_inside() &&
			get_cursor_x() >= x1 &&
			get_cursor_x() < x2)
		{
			if(buttonpress)
			{
				current_operation = TIMEBAR_DRAG_CENTER;
				starting_start_position = edl->local_session->preview_start;
				starting_end_position = edl->local_session->preview_end;
				start_cursor_x = get_cursor_x();
				result = 1;
			}
			else
			{
				result = 1;
				set_cursor(HSEPARATE_CURSOR);
			}
		}
		else
		{
// Trap all buttonpresses inside timebar
			if(cursor_inside() && buttonpress)
				result = 1;

			if(get_cursor() == LEFT_CURSOR ||
				get_cursor() == RIGHT_CURSOR)
			{
				result = 1;
				set_cursor(ARROW_CURSOR);
			}
		}
	}

	return result;
}

int TimeBar::move_preview(int &redraw)
{
	EDL *edl = get_edl();
	int result = 0;

	if(current_operation == TIMEBAR_DRAG_LEFT)
	{
		edl->local_session->preview_start =
			start_position +
			time_per_pixel * (get_cursor_x() - start_cursor_x);
		CLAMP(edl->local_session->preview_start, 0,
			edl->local_session->preview_end);
		result = 1;
	}
	else
	if(current_operation == TIMEBAR_DRAG_RIGHT)
	{
		edl->local_session->preview_end =
			start_position +
			time_per_pixel * (get_cursor_x() - start_cursor_x);
		CLAMP(edl->local_session->preview_end,
			edl->local_session->preview_start,
			edl_length);
		result = 1;
	}
	else
	if(current_operation == TIMEBAR_DRAG_CENTER)
	{
		edl->local_session->preview_start =
			starting_start_position +
			time_per_pixel * (get_cursor_x() - start_cursor_x);
		edl->local_session->preview_end =
			starting_end_position +
			time_per_pixel * (get_cursor_x() - start_cursor_x);
		if(edl->local_session->preview_start < 0)
		{
			edl->local_session->preview_end -= edl->local_session->preview_start;
			edl->local_session->preview_start = 0;
		}
		else
		if(edl->local_session->preview_end > edl_length)
		{
			edl->local_session->preview_start -= edl->local_session->preview_end - edl_length;
			edl->local_session->preview_end = edl_length;
		}
		result = 1;
	}

	if(result)
	{
		update_preview();
		redraw = 1;
	}

	return result;
}

int TimeBar::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
// Change time format
		if(ctrl_down())
		{
			if(get_buttonpress() == 1)
				mwindow_global->next_time_format();
			else
			if(get_buttonpress() == 3)
				mwindow_global->prev_time_format();
			return 1;
		}
		else
		if(!test_preview(1))
		{

// Select region between two labels
			if(get_double_click())
			{
				ptstime position = (ptstime)get_cursor_x() *
					master_edl->local_session->zoom_time +
					master_edl->local_session->view_start_pts;
// Test labels
				select_region(position);
				return 1;
			}
			else
// Reposition highlight cursor
			if(is_event_win() && cursor_inside())
			{
				update_cursor();
				mwindow_global->gui->canvas->activate();
				return 1;
			}
		}
	}
	return 0;
}

void TimeBar::repeat_event(int duration)
{
	if(!mwindow_global->gui->canvas->drag_scroll) return;
	if(duration != BC_WindowBase::get_resources()->scroll_repeat) return;

	int distance = 0;
	int x_movement = 0;
	int relative_cursor_x, relative_cursor_y;

	mwindow_global->gui->canvas->get_relative_cursor_pos(&relative_cursor_x,
		&relative_cursor_y);

	if(current_operation == TIMEBAR_DRAG)
	{
		if(relative_cursor_x >= mwindow_global->gui->canvas->get_w())
		{
			distance = relative_cursor_x - mwindow_global->gui->canvas->get_w();
			x_movement = 1;
		}
		else
		if(relative_cursor_x < 0)
		{
			distance = relative_cursor_x;
			x_movement = 1;
		}

		if(x_movement)
		{
			update_cursor();
			mwindow_global->samplemovement(master_edl->local_session->view_start_pts +
				distance * master_edl->local_session->zoom_time);
		}
	}
}

int TimeBar::cursor_motion_event()
{
	int relative_cursor_x, relative_cursor_y;
	int result = 0;
	int redraw = 0;

	switch(current_operation)
	{
	case TIMEBAR_DRAG:
		update_cursor();
		mwindow_global->gui->canvas->get_relative_cursor_pos(&relative_cursor_x,
			&relative_cursor_y);
		if(relative_cursor_x >= mwindow_global->gui->canvas->get_w() ||
			relative_cursor_x < 0)
		{
			mwindow_global->gui->canvas->start_dragscroll();
		}
		else
		if(relative_cursor_x < mwindow_global->gui->canvas->get_w() && 
			relative_cursor_x >= 0)
		{
			mwindow_global->gui->canvas->stop_dragscroll();
		}
		result = 1;
		break;

	case TIMEBAR_DRAG_LEFT:
	case TIMEBAR_DRAG_RIGHT:
	case TIMEBAR_DRAG_CENTER:
		result = move_preview(redraw);
		break;

	default:
		result = test_preview(0);
		break;
	}

	if(redraw)
		update();

	return result;
}

int TimeBar::button_release_event()
{
	int result = 0;
	switch(current_operation)
	{
	case TIMEBAR_DRAG:
		mwindow_global->undo->update_undo(_("select"), LOAD_SESSION, 0, 0);
		mwindow_global->gui->canvas->stop_dragscroll();
		current_operation = TIMEBAR_NONE;
		result = 1;
		break;

	default:
		if(current_operation != TIMEBAR_NONE)
		{
			current_operation = TIMEBAR_NONE;
			result = 1;
		}
		break;
	}
	return result;
}

// Update the selection cursor during a dragging operation
void TimeBar::update_cursor()
{
	ptstime position = (double)get_cursor_x() *
		master_edl->local_session->zoom_time +
		master_edl->local_session->view_start_pts;
	position = master_edl->align_to_frame(position);
	position = MAX(0, position);
	current_operation = TIMEBAR_DRAG;

	mwindow_global->select_point(position);
	update_highlights();
}

void TimeBar::select_region(ptstime position)
{
	Label *start = 0, *end = 0, *current;

	for(current = master_edl->labels->first; current; current = NEXT)
	{
		if(current->position > position)
		{
			end = current;
			break;
		}
	}

	for(current = master_edl->labels->last ; current; current = PREVIOUS)
	{
		if(current->position <= position)
		{
			start = current;
			break;
		}
	}

// Select region
	if(end != start)
	{
		if(!start)
			master_edl->local_session->set_selectionstart(0);
		else
			master_edl->local_session->set_selectionstart(start->position);

		if(!end)
			master_edl->local_session->set_selectionend(master_edl->total_length());
		else
			master_edl->local_session->set_selectionend(end->position);
	}
	else
	if(end || start)
	{
		master_edl->local_session->set_selectionstart(start->position);
		master_edl->local_session->set_selectionend(start->position);
	}

// Que the CWindow
	mwindow_global->cwindow->update(WUPD_POSITION);
	mwindow_global->gui->cursor->update();
	mwindow_global->gui->canvas->flash();
	mwindow_global->gui->canvas->activate();
	mwindow_global->gui->zoombar->update();
	mwindow_global->undo->update_undo(_("select"), LOAD_SESSION, 0, 0);
	update_highlights();
}
