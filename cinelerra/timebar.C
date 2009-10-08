
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

#include "bcsignals.h"
#include "clip.h"
#include "cplayback.h"
#include "cursors.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "fonts.h"
#include "labels.h"
#include "labeledit.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "preferences.h"
#include "recordlabel.h"
#include "localsession.h"
#include "mainsession.h"
#include "theme.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "units.h"
#include "vframe.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "zoombar.h"


LabelGUI::LabelGUI(MWindow *mwindow, 
	TimeBar *timebar, 
	int64_t pixel, 
	int y, 
	double position,
	VFrame **data)
 : BC_Toggle(translate_pixel(mwindow, pixel), 
 		y, 
		data ? data : mwindow->theme->label_toggle,
		0)
{
	this->mwindow = mwindow;
	this->timebar = timebar;
	this->gui = 0;
	this->pixel = pixel;
	this->position = position;
	this->label = 0;
}

LabelGUI::~LabelGUI()
{
}

int LabelGUI::get_y(MWindow *mwindow, TimeBar *timebar)
{
	return timebar->get_h() - 
		mwindow->theme->label_toggle[0]->get_h();
}

int LabelGUI::translate_pixel(MWindow *mwindow, int pixel)
{
	int result = pixel - mwindow->theme->label_toggle[0]->get_w() / 2;
	return result;
}

void LabelGUI::reposition()
{
	reposition_window(translate_pixel(mwindow, pixel), BC_Toggle::get_y());
}

int LabelGUI::button_press_event()
{
	if (this->is_event_win() && get_buttonpress() == 3) {
		if (label)
			timebar->label_edit->edit_label(label);
	} else {
		BC_Toggle::button_press_event();
	}
	if (label)
		set_tooltip(this->label->textstr);
}

int LabelGUI::handle_event()
{
	timebar->select_label(position);
	return 1;
}








InPointGUI::InPointGUI(MWindow *mwindow, 
	TimeBar *timebar, 
	int64_t pixel, 
	double position)
 : LabelGUI(mwindow, 
 	timebar, 
	pixel, 
	get_y(mwindow, timebar), 
	position, 
	mwindow->theme->in_point)
{
//printf("InPointGUI::InPointGUI %d %d\n", pixel, get_y(mwindow, timebar));
}
InPointGUI::~InPointGUI()
{
}
int InPointGUI::get_y(MWindow *mwindow, TimeBar *timebar)
{
	int result;
	result = timebar->get_h() - 
		mwindow->theme->in_point[0]->get_h();
	return result;
}


OutPointGUI::OutPointGUI(MWindow *mwindow, 
	TimeBar *timebar, 
	int64_t pixel, 
	double position)
 : LabelGUI(mwindow, 
 	timebar, 
	pixel, 
	get_y(mwindow, timebar), 
	position, 
	mwindow->theme->out_point)
{
//printf("OutPointGUI::OutPointGUI %d %d\n", pixel, get_y(mwindow, timebar));
}
OutPointGUI::~OutPointGUI()
{
}
int OutPointGUI::get_y(MWindow *mwindow, TimeBar *timebar)
{
	return timebar->get_h() - 
		mwindow->theme->out_point[0]->get_h();
}


PresentationGUI::PresentationGUI(MWindow *mwindow, 
	TimeBar *timebar, 
	int64_t pixel, 
	double position)
 : LabelGUI(mwindow, timebar, pixel, get_y(mwindow, timebar), position)
{
}
PresentationGUI::~PresentationGUI()
{
}







TimeBar::TimeBar(MWindow *mwindow, 
	BC_WindowBase *gui,
	int x, 
	int y,
	int w,
	int h)
 : BC_SubWindow(x, y, w, h)
{
//printf("TimeBar::TimeBar %d %d %d %d\n", x, y, w, h);
	this->gui = gui;
	this->mwindow = mwindow;
	label_edit = new LabelEdit(mwindow, mwindow->awindow, 0);
}

TimeBar::~TimeBar()
{
	if(in_point) delete in_point;
	if(out_point) delete out_point;
	if(label_edit) delete label_edit;
	labels.remove_all_objects();
	presentations.remove_all_objects();
}

int TimeBar::create_objects()
{
	in_point = 0;
	out_point = 0;
	current_operation = TIMEBAR_NONE;
	update();
	return 0;
}


int64_t TimeBar::position_to_pixel(double position)
{
	get_edl_length();
	return (int64_t)(position / time_per_pixel);
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
			int64_t pixel = position_to_pixel(current->position);

			if(pixel >= 0 && pixel < get_w())
			{
// Create new label
				if(output >= labels.total)
				{
					LabelGUI *new_label;
					add_subwindow(new_label = 
						new LabelGUI(mwindow, 
							this, 
							pixel, 
							LabelGUI::get_y(mwindow, this), 
							current->position));
					new_label->set_cursor(ARROW_CURSOR);
					new_label->set_tooltip(current->textstr);
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
					labels.values[output]->set_tooltip(current->textstr);
					labels.values[output]->label = current;
				}

				if(edl->local_session->get_selectionstart(1) <= current->position &&
					edl->local_session->get_selectionend(1) >= current->position)
					labels.values[output]->update(1);
				else
				if(labels.values[output]->get_value())
					labels.values[output]->update(0);

				output++;
			}
		}
	}

// Delete excess labels
	while(labels.total > output)
	{
		labels.remove_object();
	}
}

void TimeBar::update_highlights()
{
	for(int i = 0; i < labels.total; i++)
	{
		LabelGUI *label = labels.values[i];
		if(mwindow->edl->equivalent(label->position, 
				mwindow->edl->local_session->get_selectionstart(1)) ||
			mwindow->edl->equivalent(label->position, 
				mwindow->edl->local_session->get_selectionend(1)))
		{
			if(!label->get_value()) label->update(1);
		}
		else
			if(label->get_value()) label->update(0);
	}

	if(mwindow->edl->equivalent(mwindow->edl->local_session->get_inpoint(), 
			mwindow->edl->local_session->get_selectionstart(1)) ||
		mwindow->edl->equivalent(mwindow->edl->local_session->get_inpoint(), 
			mwindow->edl->local_session->get_selectionend(1)))
	{
		if(in_point) in_point->update(1);
	}
	else
		if(in_point) in_point->update(0);

	if(mwindow->edl->equivalent(mwindow->edl->local_session->get_outpoint(), 
			mwindow->edl->local_session->get_selectionstart(1)) ||
		mwindow->edl->equivalent(mwindow->edl->local_session->get_outpoint(), 
			mwindow->edl->local_session->get_selectionend(1)))
	{
		if(out_point) out_point->update(1);
	}
	else
		if(out_point) out_point->update(0);
}

void TimeBar::update_points()
{
	EDL *edl = get_edl();
	int64_t pixel;

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
			{
				in_point->draw_face();
			}
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
		add_subwindow(in_point = new InPointGUI(mwindow, 
			this, 
			pixel, 
			edl->local_session->get_inpoint()));
		in_point->set_cursor(ARROW_CURSOR);
	}

	if(edl) pixel = position_to_pixel(edl->local_session->get_outpoint());

	if(out_point)
	{
		if(edl &&
			edl->local_session->outpoint_valid() && 
			pixel >= 0 && 
			pixel < get_w())
		{
			if(!EQUIV(edl->local_session->get_outpoint(), out_point->position) ||
				out_point->pixel != pixel) 
			{
				out_point->pixel = pixel;
				out_point->position = edl->local_session->get_outpoint();
				out_point->reposition();
			}
			else
			{
				out_point->draw_face();
			}
		}
		else
		{
			delete out_point;
			out_point = 0;
		}
	}
	else
	if(edl && 
		edl->local_session->outpoint_valid() && 
		pixel >= 0 && pixel < get_w())
	{
		add_subwindow(out_point = new OutPointGUI(mwindow, 
			this, 
			pixel, 
			edl->local_session->get_outpoint()));
		out_point->set_cursor(ARROW_CURSOR);
	}
}

void TimeBar::update_presentations()
{
}


void TimeBar::update(int do_range, int do_others)
{
	draw_time();
// Need to redo these when range is drawn to get the background updated.
	update_labels();
	update_points();
	update_presentations();
	flash();
}



int TimeBar::delete_project()
{
//	labels->delete_all();
	return 0;
}

int TimeBar::save(FileXML *xml)
{
//	labels->save(xml);
	return 0;
}




void TimeBar::draw_time()
{
}

EDL* TimeBar::get_edl()
{
	return mwindow->edl;
}



void TimeBar::draw_range()
{
	int x1 = 0, x2 = 0;
	if(get_edl())
	{
		get_preview_pixels(x1, x2);

//printf("TimeBar::draw_range %f %d %d\n", edl_length, x1, x2);
		draw_3segmenth(0, 0, x1, mwindow->theme->timebar_view_data);
		draw_top_background(get_parent(), x1, 0, x2 - x1, get_h());
		draw_3segmenth(x2, 0, get_w() - x2, mwindow->theme->timebar_view_data);

		set_color(BLACK);
		draw_line(x1, 0, x1, get_h());
		draw_line(x2, 0, x2, get_h());
		
		EDL *edl;
		if(edl = get_edl())
		{
			int64_t pixel = position_to_pixel(
				edl->local_session->get_selectionstart(1));
// Draw insertion point position if this timebar beint64_ts to a window which 
// has something other than the master EDL.
			set_color(RED);
			draw_line(pixel, 0, pixel, get_h());
		}
	}
	else
		draw_top_background(get_parent(), 0, 0, get_w(), get_h());
}

void TimeBar::select_label(double position)
{
}



int TimeBar::draw()
{
	return 0;
}

void TimeBar::get_edl_length()
{
	edl_length = 0;

	if(get_edl())
	{
//printf("TimeBar::get_edl_length 1 %f\n", get_edl()->tracks->total_playable_length());
		edl_length = get_edl()->tracks->total_playable_length();
	}

//printf("TimeBar::get_edl_length 2\n");
	if(!EQUIV(edl_length, 0))
	{
//printf("TimeBar::get_edl_length 3\n");
		time_per_pixel = edl_length / get_w();
//printf("TimeBar::get_edl_length 4\n");
	}
	else
	{
		time_per_pixel = 0;
	}
//printf("TimeBar::get_edl_length 5\n");
}

int TimeBar::get_preview_pixels(int &x1, int &x2)
{
	x1 = 0;
	x2 = 0;

	get_edl_length();

	if(get_edl())
	{
		if(!EQUIV(edl_length, 0))
		{
			if(get_edl()->local_session->preview_end <= 0 ||
				get_edl()->local_session->preview_end > edl_length)
				get_edl()->local_session->preview_end = edl_length;
			if(get_edl()->local_session->preview_start > 
				get_edl()->local_session->preview_end)
				get_edl()->local_session->preview_start = 0;
			x1 = (int)(get_edl()->local_session->preview_start / time_per_pixel);
			x2 = (int)(get_edl()->local_session->preview_end / time_per_pixel);
		}
		else
		{
			x1 = 0;
			x2 = get_w();
		}
	}
// printf("TimeBar::get_preview_pixels %f %f %d %d\n", 
// 	get_edl()->local_session->preview_start,
// 	get_edl()->local_session->preview_end,
// 	x1, 
// 	x2);
	return 0;
}


int TimeBar::test_preview(int buttonpress)
{
	int result = 0;
	int x1, x2;

	get_preview_pixels(x1, x2);
//printf("TimeBar::test_preview %d %d %d\n", x1, x2, get_cursor_x());

	if(get_edl())
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
				start_position = get_edl()->local_session->preview_start;
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
				start_position = get_edl()->local_session->preview_end;
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
				starting_start_position = get_edl()->local_session->preview_start;
				starting_end_position = get_edl()->local_session->preview_end;
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
	int result = 0;

	if(current_operation == TIMEBAR_DRAG_LEFT)
	{
		get_edl()->local_session->preview_start = 
			start_position + 
			time_per_pixel * (get_cursor_x() - start_cursor_x);
		CLAMP(get_edl()->local_session->preview_start, 
			0, 
			get_edl()->local_session->preview_end);
		result = 1;
	}
	else
	if(current_operation == TIMEBAR_DRAG_RIGHT)
	{
		get_edl()->local_session->preview_end = 
			start_position + 
			time_per_pixel * (get_cursor_x() - start_cursor_x);
		CLAMP(get_edl()->local_session->preview_end, 
			get_edl()->local_session->preview_start, 
			edl_length);
		result = 1;
	}
	else
	if(current_operation == TIMEBAR_DRAG_CENTER)
	{
		get_edl()->local_session->preview_start = 
			starting_start_position +
			time_per_pixel * (get_cursor_x() - start_cursor_x);
		get_edl()->local_session->preview_end = 
			starting_end_position +
			time_per_pixel * (get_cursor_x() - start_cursor_x);
		if(get_edl()->local_session->preview_start < 0)
		{
			get_edl()->local_session->preview_end -= get_edl()->local_session->preview_start;
			get_edl()->local_session->preview_start = 0;
		}
		else
		if(get_edl()->local_session->preview_end > edl_length)
		{
			get_edl()->local_session->preview_start -= get_edl()->local_session->preview_end - edl_length;
			get_edl()->local_session->preview_end = edl_length;
		}
		result = 1;
	}

//printf("TimeBar::move_preview %f %f\n", get_edl()->local_session->preview_start, get_edl()->local_session->preview_end);

	if(result)
	{
		update_preview();
		redraw = 1;
	}

	return result;
}

void TimeBar::update_preview()
{
}

int TimeBar::samplemovement()
{
	return 0;
}

void TimeBar::stop_playback()
{
}

int TimeBar::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
// Change time format
		if(ctrl_down())
		{
			if(get_buttonpress() == 1)
				mwindow->next_time_format();
			else
			if(get_buttonpress() == 2)
				mwindow->prev_time_format();
			return 1;
		}
		else
		if(test_preview(1))
		{
		}
		else
		{
			stop_playback();

// Select region between two labels
			if(get_double_click())
			{
				double position = (double)get_cursor_x() * 
					mwindow->edl->local_session->zoom_sample / 
					mwindow->edl->session->sample_rate + 
					(double)mwindow->edl->local_session->view_start *
					mwindow->edl->local_session->zoom_sample / 
					mwindow->edl->session->sample_rate;
// Test labels
				select_region(position);
				return 1;
			}
			else
// Reposition highlight cursor
			if(is_event_win() && cursor_inside())
			{
				update_cursor();
				mwindow->gui->canvas->activate();
				return 1;
			}
		}
	}
	return 0;
}

int TimeBar::repeat_event(int64_t duration)
{
	if(!mwindow->gui->canvas->drag_scroll) return 0;
	if(duration != BC_WindowBase::get_resources()->scroll_repeat) return 0;

	int distance = 0;
	int x_movement = 0;
	int relative_cursor_x = mwindow->gui->canvas->get_relative_cursor_x();
	if(current_operation == TIMEBAR_DRAG)
	{
		if(relative_cursor_x >= mwindow->gui->canvas->get_w())
		{
			distance = relative_cursor_x - mwindow->gui->canvas->get_w();
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
			mwindow->samplemovement(mwindow->edl->local_session->view_start + 
				distance);
		}
		return 1;
	}
	return 0;
}

int TimeBar::cursor_motion_event()
{
	int result = 0;
	int redraw = 0;

	switch(current_operation)
	{
		case TIMEBAR_DRAG:
		{
			update_cursor();
//printf("TimeBar::cursor_motion_event 1\n");
			int relative_cursor_x = mwindow->gui->canvas->get_relative_cursor_x();
			if(relative_cursor_x >= mwindow->gui->canvas->get_w() || 
				relative_cursor_x < 0)
			{
				mwindow->gui->canvas->start_dragscroll();
			}
			else
			if(relative_cursor_x < mwindow->gui->canvas->get_w() && 
				relative_cursor_x >= 0)
			{
				mwindow->gui->canvas->stop_dragscroll();
			}
			result = 1;
//printf("TimeBar::cursor_motion_event 10\n");
			break;
		}


		case TIMEBAR_DRAG_LEFT:
		case TIMEBAR_DRAG_RIGHT:
		case TIMEBAR_DRAG_CENTER:
			result = move_preview(redraw);
			break;

		default:
//printf("TimeBar::cursor_motion_event 20\n");
			result = test_preview(0);
//printf("TimeBar::cursor_motion_event 30\n");
			break;
	}

	if(redraw)
	{
		update();
	}

	return result;
}

int TimeBar::button_release_event()
{
//printf("TimeBar::button_release_event %d\n", current_operation);
	int result = 0;
	switch(current_operation)
	{
		case TIMEBAR_DRAG:
			mwindow->undo->update_undo(_("select"), LOAD_SESSION, 0, 0);
			mwindow->gui->canvas->stop_dragscroll();
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
	double position = (double)get_cursor_x() * 
		mwindow->edl->local_session->zoom_sample / 
		mwindow->edl->session->sample_rate + 
		(double)mwindow->edl->local_session->view_start * 
		mwindow->edl->local_session->zoom_sample / 
		mwindow->edl->session->sample_rate;
	
	position = mwindow->edl->align_to_frame(position, 0);
	position = MAX(0, position);
	current_operation = TIMEBAR_DRAG;

	mwindow->select_point(position);
	update_highlights();
}


int TimeBar::select_region(double position)
{
	Label *start = 0, *end = 0, *current;
	for(current = mwindow->edl->labels->first; current; current = NEXT)
	{
		if(current->position > position)
		{
			end = current;
			break;
		}
	}

	for(current = mwindow->edl->labels->last ; current; current = PREVIOUS)
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
			mwindow->edl->local_session->set_selectionstart(0);
		else
			mwindow->edl->local_session->set_selectionstart(start->position);

		if(!end)
			mwindow->edl->local_session->set_selectionend(mwindow->edl->tracks->total_length());
		else
			mwindow->edl->local_session->set_selectionend(end->position);
	}
	else
	if(end || start)
	{
		mwindow->edl->local_session->set_selectionstart(start->position);
		mwindow->edl->local_session->set_selectionend(start->position);
	}

// Que the CWindow
	mwindow->cwindow->update(1, 0, 0);
	mwindow->gui->cursor->hide(0);
	mwindow->gui->cursor->draw(1);
	mwindow->gui->canvas->flash();
	mwindow->gui->canvas->activate();
	mwindow->gui->zoombar->update();
	mwindow->undo->update_undo(_("select"), LOAD_SESSION, 0, 0);
	update_highlights();
	return 0;
}




int TimeBar::delete_arrows()
{
	return 0;
}


