
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
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "cpanel.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "cwindowtool.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "maskauto.h"
#include "maskautos.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "transportcommand.h"


CWindowTool::CWindowTool(MWindow *mwindow, CWindowGUI *gui)
 : Thread()
{
	this->mwindow = mwindow;
	this->gui = gui;
	tool_gui = 0;
	done = 0;
	current_tool = CWINDOW_NONE;
	set_synchronous(1);
	input_lock = new Condition(0, "CWindowTool::input_lock");
	output_lock = new Condition(1, "CWindowTool::output_lock");
	tool_gui_lock = new Mutex("CWindowTool::tool_gui_lock");
}

CWindowTool::~CWindowTool()
{
	done = 1;
	stop_tool();
	input_lock->unlock();
	Thread::join();
	delete input_lock;
	delete output_lock;
	delete tool_gui_lock;
}

void CWindowTool::start_tool(int operation)
{
	CWindowToolGUI *new_gui = 0;
	int result = 0;

	if(current_tool != operation)
	{
		current_tool = operation;
		switch(operation)
		{
		case CWINDOW_EYEDROP:
			new_gui = new CWindowEyedropGUI(mwindow, this);
			break;
		case CWINDOW_CROP:
			new_gui = new CWindowCropGUI(mwindow, this);
			break;
		case CWINDOW_CAMERA:
			new_gui = new CWindowCameraGUI(mwindow, this);
			break;
		case CWINDOW_PROJECTOR:
			new_gui = new CWindowProjectorGUI(mwindow, this);
			break;
		case CWINDOW_MASK:
			new_gui = new CWindowMaskGUI(mwindow, this);
			break;
		default:
			result = 1;
			stop_tool();
			break;
		}

		if(!result)
		{
			stop_tool();
// Wait for previous tool GUI to finish
			output_lock->lock("CWindowTool::start_tool");
			this->tool_gui = new_gui;

			if(mwindow->edl->session->tool_window &&
				mwindow->session->show_cwindow) tool_gui->show_window();
			tool_gui->flush();

// Signal thread to run next tool GUI
			input_lock->unlock();
		}
	}
	else
	if(tool_gui) 
		tool_gui->update();
}

void CWindowTool::stop_tool()
{
	if(tool_gui)
		tool_gui->set_done(0);
}

void CWindowTool::show_tool()
{
	if(tool_gui && mwindow->edl->session->tool_window)
		tool_gui->show_window();
}

void CWindowTool::hide_tool()
{
	if(tool_gui && mwindow->edl->session->tool_window)
		tool_gui->hide_window();
}

void CWindowTool::run()
{
	while(!done)
	{
		input_lock->lock("CWindowTool::run");
		if(!done)
		{
			tool_gui->run_window();
			tool_gui_lock->lock("CWindowTool::run");
			delete tool_gui;
			tool_gui = 0;
			tool_gui_lock->unlock();
		}
		output_lock->unlock();
	}
}

void CWindowTool::update_show_window()
{
	if(tool_gui)
	{
		if(mwindow->edl->session->tool_window) 
		{
			tool_gui->update();
			tool_gui->show_window();
		}
		else
			tool_gui->hide_window();

		tool_gui->flush();
	}
}

void CWindowTool::update_values()
{
	if(tool_gui)
	{
		tool_gui_lock->lock("CWindowTool::update_values");
		tool_gui->update();
		tool_gui->flush();
		tool_gui_lock->unlock();
	}
}


CWindowToolGUI::CWindowToolGUI(MWindow *mwindow, 
	CWindowTool *thread, 
	const char *title,
	int w, 
	int h)
 : BC_Window(title,
	mwindow->session->ctool_x,
	mwindow->session->ctool_y,
	w,
	h,
	w,
	h,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	current_operation = 0;
	set_icon(mwindow->theme->get_image("cwindow_icon"));
}

void CWindowToolGUI::close_event()
{
	hide_window();
	flush();
	mwindow->edl->session->tool_window = 0;

	thread->gui->composite_panel->set_operation(mwindow->edl->session->cwindow_operation);
	thread->gui->flush();
}

void CWindowToolGUI::translation_event()
{
	mwindow->session->ctool_x = get_x();
	mwindow->session->ctool_y = get_y();
}


CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, float value, int log_increment = 0)
 : BC_TumbleTextBox(gui, 
		(float)value,
		(float)-65536,
		(float)65536,
		x, 
		y, 
		100)
{
	this->gui = gui;
	set_log_floatincrement(log_increment);
}

CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, int value)
 : BC_TumbleTextBox(gui, 
		value,
		-65536,
		65536,
		x, 
		y, 
		100)
{
	this->gui = gui;
}

int CWindowCoord::handle_event()
{
	gui->event_caller = this;
	gui->handle_event();
	return 1;
}


CWindowCropOK::CWindowCropOK(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Do it"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CWindowCropOK::handle_event()
{
	mwindow->crop_video();
	return 1;
}

int CWindowCropOK::keypress_event()
{
	if(get_keypress() == 0xd) 
	{
		handle_event();
		return 1;
	}
	return 0;
}


CWindowCropGUI::CWindowCropGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
	thread,
	PROGRAM_NAME ": Crop",
	330,
	100)
{
	int x = 10, y = 10;
	BC_Title *title;
	int column1 = 0;
	int pad = MAX(BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1), 
		BC_Title::calculate_h(this, "X")) + 5;

	add_subwindow(title = new BC_Title(x, y, _("X1:")));
	column1 = MAX(column1, title->get_w());
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("W:")));
	column1 = MAX(column1, title->get_w());
	y += pad;
	add_subwindow(new CWindowCropOK(mwindow, this, x, y));

	x += column1 + 5;
	y = 10;
	x1 = new CWindowCoord(this, x, y, mwindow->edl->session->crop_x1);
	x1->create_objects();
	y += pad;
	width = new CWindowCoord(this,
		x, 
		y, 
		mwindow->edl->session->crop_x2 - 
			mwindow->edl->session->crop_x1);
	width->create_objects();

	x += x1->get_w() + 10;
	y = 10;
	int column2 = 0;
	add_subwindow(title = new BC_Title(x, y, _("Y1:")));
	column2 = MAX(column2, title->get_w());
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("H:")));
	column2 = MAX(column2, title->get_w());
	y += pad;

	y = 10;
	x += column2 + 5;
	y1 = new CWindowCoord(this, x, y, mwindow->edl->session->crop_y1);
	y1->create_objects();
	y += pad;
	height = new CWindowCoord(this,
		x, 
		y, 
		mwindow->edl->session->crop_y2 - 
			mwindow->edl->session->crop_y1);
	height->create_objects();
}

int CWindowCropGUI::handle_event()
{
	int new_x1, new_y1;
	new_x1 = atol(x1->get_text());
	new_y1 = atol(y1->get_text());
	if(new_x1 != mwindow->edl->session->crop_x1)
	{
		mwindow->edl->session->crop_x2 = new_x1 +
			mwindow->edl->session->crop_x2 - 
			mwindow->edl->session->crop_x1;
		mwindow->edl->session->crop_x1 = new_x1;
	}
	if(new_y1 != mwindow->edl->session->crop_y1)
	{
		mwindow->edl->session->crop_y2 = new_y1 +
			mwindow->edl->session->crop_y2 -
			mwindow->edl->session->crop_y1;
		mwindow->edl->session->crop_y1 = atol(y1->get_text());
	}
	mwindow->edl->session->crop_x2 = atol(width->get_text()) + 
		mwindow->edl->session->crop_x1;
	mwindow->edl->session->crop_y2 = atol(height->get_text()) + 
		mwindow->edl->session->crop_y1;
	update();
	mwindow->cwindow->gui->canvas->draw_refresh();
	return 1;
}

void CWindowCropGUI::update()
{
	x1->update((int64_t)mwindow->edl->session->crop_x1);
	y1->update((int64_t)mwindow->edl->session->crop_y1);
	width->update((int64_t)mwindow->edl->session->crop_x2 - 
		mwindow->edl->session->crop_x1);
	height->update((int64_t)mwindow->edl->session->crop_y2 - 
		mwindow->edl->session->crop_y1);
}


CWindowEyedropGUI::CWindowEyedropGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
	thread,
	PROGRAM_NAME ": Color",
	150,
	150)
{
	int x = 10;
	int y = 10;
	int x2 = 70;
	BC_Title *title1, *title2, *title3;
	add_subwindow(title1 = new BC_Title(x, y, "Red:"));
	y += title1->get_h() + 5;
	add_subwindow(title2 = new BC_Title(x, y, "Green:"));
	y += title2->get_h() + 5;
	add_subwindow(title3 = new BC_Title(x, y, "Blue:"));

	add_subwindow(red = new BC_Title(x2, title1->get_y(), "0"));
	add_subwindow(green = new BC_Title(x2, title2->get_y(), "0"));
	add_subwindow(blue = new BC_Title(x2, title3->get_y(), "0"));

	y = blue->get_y() + blue->get_h() + 5;
	add_subwindow(sample = new BC_SubWindow(x, y, 50, 50));
	update();
}

void CWindowEyedropGUI::update()
{
	red->update(mwindow->edl->local_session->red);
	green->update(mwindow->edl->local_session->green);
	blue->update(mwindow->edl->local_session->blue);
	int r = (int)(CLIP(mwindow->edl->local_session->red, 0, 1) * 0xff);
	int g = (int)(CLIP(mwindow->edl->local_session->green, 0, 1) * 0xff);
	int b = (int)(CLIP(mwindow->edl->local_session->blue, 0, 1) * 0xff);
	sample->set_color((r << 16) | (g << 8) | b);
	sample->draw_box(0, 0, sample->get_w(), sample->get_h());
	sample->set_color(BLACK);
	sample->draw_rectangle(0, 0, sample->get_w(), sample->get_h());
	sample->flash();
}


CWindowCameraGUI::CWindowCameraGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
	thread,
	PROGRAM_NAME ": Camera",
	170,
	170)
{
	int x = 10, y = 10, x1;
	Track *track = mwindow->cwindow->calculate_affected_track();
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	BC_Title *title;
	BC_Button *button;

	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			&y_auto,
			&z_auto,
			track,
			1,
			0,
			0,
			0);
	}

	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w();
	this->x = new CWindowCoord(this, 
		x, 
		y, 
		x_auto ? x_auto->value : (float)0);
	this->x->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w();
	this->y = new CWindowCoord(this, 
		x, 
		y, 
		y_auto ? y_auto->value : (float)0);
	this->y->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Z:")));
	x += title->get_w();
	this->z = new CWindowCoord(this, 
		x, 
		y, 
		z_auto ? z_auto->value : (float)1,
		1);
	this->z->create_objects();
	this->z->set_boundaries((float).0001, (float)256.0);

	y += 30;
	x1 = 10;
	add_subwindow(button = new CWindowCameraLeft(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraCenter(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraRight(mwindow, this, x1, y));

	y += button->get_h();
	x1 = 10;
	add_subwindow(button = new CWindowCameraTop(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraMiddle(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraBottom(mwindow, this, x1, y));
}

void CWindowCameraGUI::update_preview()
{
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);

	mwindow->cwindow->playback_engine->send_command(CURRENT_FRAME, mwindow->edl);
	mwindow->cwindow->gui->canvas->draw_refresh();
}

int CWindowCameraGUI::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		if(event_caller == x)
		{
			x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_CAMERA_X],
				1);
			if(x_auto)
			{
				x_auto->value = atof(x->get_text());
				update_preview();
			}
		}
		else
		if(event_caller == y)
		{
			y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_CAMERA_Y],
				1);
			if(y_auto)
			{
				y_auto->value = atof(y->get_text());
				update_preview();
			}
		}
		else
		if(event_caller == z)
		{
			z_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_CAMERA_Z],
				1);
			if(z_auto)
			{
				float zoom = atof(z->get_text());
				if(zoom > 10) zoom = 10; 
				else
				if(zoom < 0) zoom = 0;

				z_auto->value = zoom;
				mwindow->gui->canvas->draw_overlays();
				mwindow->gui->canvas->flash();
				update_preview();
			}
		}
	}
	return 1;
}

void CWindowCameraGUI::update()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			&y_auto,
			&z_auto,
			track,
			1,
			0,
			0,
			0);
	}

	if(x_auto)
		x->update(x_auto->value);
	if(y_auto)
		y->update(y_auto->value);
	if(z_auto)
		z->update(z_auto->value);
}


CWindowCameraLeft::CWindowCameraLeft(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("left_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}

int CWindowCameraLeft::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			0,
			&z_auto,
			track,
			1,
			1,
			0,
			0);
	}

	if(x_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			x_auto->value = 
				(double)track->track_w / z_auto->value / 2 - 
				(double)w / 2;
			gui->update();
			gui->update_preview();
		}
	}
	return 1;
}


CWindowCameraCenter::CWindowCameraCenter(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("center_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}

int CWindowCameraCenter::handle_event()
{
	FloatAuto *x_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_X],
			1);

	if(x_auto)
	{
		x_auto->value = 0;
		gui->update();
		gui->update_preview();
	}
	return 1;
}


CWindowCameraRight::CWindowCameraRight(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("right_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}

int CWindowCameraRight::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			0,
			&z_auto,
			track,
			1,
			1,
			0,
			0);
	}

	if(x_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			x_auto->value = -((double)track->track_w / z_auto->value / 2 - 
				(double)w / 2);
			gui->update();
			gui->update_preview();
		}
	}

	return 1;
}


CWindowCameraTop::CWindowCameraTop(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("top_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}

int CWindowCameraTop::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(0,
			&y_auto,
			&z_auto,
			track,
			1,
			0,
			1,
			0);
	}

	if(y_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			y_auto->value = (double)track->track_h / z_auto->value / 2 - 
				(double)h / 2;
			gui->update();
			gui->update_preview();
		}
	}

	return 1;
}


CWindowCameraMiddle::CWindowCameraMiddle(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("middle_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}

int CWindowCameraMiddle::handle_event()
{
	FloatAuto *y_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_Y], 1);

	if(y_auto)
	{
		y_auto->value = 0;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowCameraBottom::CWindowCameraBottom(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("bottom_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}

int CWindowCameraBottom::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(0,
			&y_auto,
			&z_auto,
			track,
			1,
			0,
			1,
			0);
	}

	if(y_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			y_auto->value = -((double)track->track_h / z_auto->value / 2 - 
				(double)h / 2);
			gui->update();
			gui->update_preview();
		}
	}

	return 1;
}


CWindowProjectorGUI::CWindowProjectorGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
	thread,
	PROGRAM_NAME ": Projector",
	170,
	170)
{
	int x = 10, y = 10, x1;
	Track *track = mwindow->cwindow->calculate_affected_track();
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	BC_Title *title;
	BC_Button *button;

	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			&y_auto,
			&z_auto,
			track,
			0,
			0,
			0,
			0);
	}

	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w();
	this->x = new CWindowCoord(this, 
		x, 
		y, 
		x_auto ? x_auto->value : (float)0);
	this->x->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w();
	this->y = new CWindowCoord(this, 
		x, 
		y, 
		y_auto ? y_auto->value : (float)0);
	this->y->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Z:")));
	x += title->get_w();
	this->z = new CWindowCoord(this, 
		x, 
		y, 
		z_auto ? z_auto->value : (float)1,
		1);
	this->z->create_objects();
	this->z->set_boundaries((float).0001, (float)256.0);

	y += 30;
	x1 = 10;
	add_subwindow(button = new CWindowProjectorLeft(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorCenter(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorRight(mwindow, this, x1, y));

	y += button->get_h();
	x1 = 10;
	add_subwindow(button = new CWindowProjectorTop(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorMiddle(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorBottom(mwindow, this, x1, y));
}

void CWindowProjectorGUI::update_preview()
{
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);

	mwindow->cwindow->playback_engine->send_command(CURRENT_FRAME, mwindow->edl);
	mwindow->cwindow->gui->canvas->draw_refresh();
}

int CWindowProjectorGUI::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		if(event_caller == x)
		{
			x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_PROJECTOR_X],
				1);
			if(x_auto)
			{
				x_auto->value = atof(x->get_text());
				update_preview();
			}
		}
		else
		if(event_caller == y)
		{
			y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_PROJECTOR_Y],
				1);
			if(y_auto)
			{
				y_auto->value = atof(y->get_text());
				update_preview();
			}
		}
		else
		if(event_caller == z)
		{
			z_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_PROJECTOR_Z],
				1);
			if(z_auto)
			{
				float zoom = atof(z->get_text());
				if(zoom > 10000) zoom = 10000; 
				else 
				if(zoom < 0) zoom = 0;
				z_auto->value = zoom;

				mwindow->gui->canvas->draw_overlays();
				mwindow->gui->canvas->flash();

				update_preview();
			}
		}
	}
	return 1;
}

void CWindowProjectorGUI::update()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			&y_auto,
			&z_auto,
			track,
			0,
			0,
			0,
			0);
	}

	if(x_auto)
		x->update(x_auto->value);
	if(y_auto)
		y->update(y_auto->value);
	if(z_auto)
		z->update(z_auto->value);
}


CWindowProjectorLeft::CWindowProjectorLeft(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("left_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}

int CWindowProjectorLeft::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			0,
			&z_auto,
			track,
			0,
			1,
			0,
			0);
	}
	if(x_auto && z_auto)
	{
		x_auto->value = (double)track->track_w * z_auto->value / 2 - 
			(double)mwindow->edl->session->output_w / 2;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorCenter::CWindowProjectorCenter(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("center_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}

int CWindowProjectorCenter::handle_event()
{
	FloatAuto *x_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_X],
			1);

	if(x_auto)
	{
		x_auto->value = 0;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorRight::CWindowProjectorRight(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("right_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}

int CWindowProjectorRight::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(&x_auto,
			0,
			&z_auto,
			track,
			0,
			1,
			0,
			0);
	}

	if(x_auto && z_auto)
	{
		x_auto->value = -((double)track->track_w * z_auto->value / 2 - 
			(double)mwindow->edl->session->output_w / 2);
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorTop::CWindowProjectorTop(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("top_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}

int CWindowProjectorTop::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(0,
			&y_auto,
			&z_auto,
			track,
			0,
			0,
			1,
			0);
	}

	if(y_auto && z_auto)
	{
		y_auto->value = (double)track->track_h * z_auto->value / 2 - 
			(double)mwindow->edl->session->output_h / 2;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorMiddle::CWindowProjectorMiddle(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("middle_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}

int CWindowProjectorMiddle::handle_event()
{
	FloatAuto *y_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_Y], 1);

	if(y_auto)
	{
		y_auto->value = 0;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorBottom::CWindowProjectorBottom(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("bottom_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}

int CWindowProjectorBottom::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->cwindow->calculate_affected_autos(0,
			&y_auto,
			&z_auto,
			track,
			0,
			0,
			1,
			0);
	}

	if(y_auto && z_auto)
	{
		y_auto->value = -((double)track->track_h * z_auto->value / 2 - 
			(double)mwindow->edl->session->output_h / 2);
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowMaskMode::CWindowMaskMode(MWindow *mwindow, 
	CWindowToolGUI *gui, 
	int x, 
	int y,
	const char *text)
 : BC_PopupMenu(x,
	y,
	200,
	text,
	1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void CWindowMaskMode::create_objects()
{
	add_item(new BC_MenuItem(mode_to_text(MASK_MULTIPLY_ALPHA)));
	add_item(new BC_MenuItem(mode_to_text(MASK_SUBTRACT_ALPHA)));
}

const char* CWindowMaskMode::mode_to_text(int mode)
{
	switch(mode)
	{
		case MASK_MULTIPLY_ALPHA:
			return _("Multiply alpha");
			break;

		case MASK_SUBTRACT_ALPHA:
			return _("Subtract alpha");
			break;
	}

	return _("Subtract alpha");
}

int CWindowMaskMode::text_to_mode(const char *text)
{
	if(!strcasecmp(text, _("Multiply alpha")))
		return MASK_MULTIPLY_ALPHA;
	else
	if(!strcasecmp(text, _("Subtract alpha")))
		return MASK_SUBTRACT_ALPHA;

	return MASK_SUBTRACT_ALPHA;
}

int CWindowMaskMode::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	((CWindowMaskGUI*)gui)->get_keyframe(track, 
		keyframe, 
		mask,
		point,
		0);

	if(track)
	{
		((MaskAuto*)track->automation->autos[AUTOMATION_MASK]->first)->mode = 
			text_to_mode(get_text());
	}

	gui->update_preview();
	return 1;
}


CWindowMaskDelete::CWindowMaskDelete(MWindow *mwindow, 
	CWindowToolGUI *gui, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CWindowMaskDelete::handle_event()
{
	MaskAuto *keyframe;
	Track *track = mwindow->cwindow->calculate_affected_track();
	MaskPoint *point;
	SubMask *mask;

	if(track)
	{
		MaskAutos *mask_autos = (MaskAutos*)track->automation->autos[AUTOMATION_MASK];
		for(MaskAuto *current = (MaskAuto*)mask_autos->first;
			current; current = (MaskAuto*)NEXT)
		{
			SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);

			for(int i = mwindow->cwindow->gui->affected_point;
				i < submask->points.total - 1;
				i++)
			{
				*submask->points.values[i] = *submask->points.values[i + 1];
			}

			if(submask->points.total)
			{
				submask->points.remove_object(
					submask->points.values[submask->points.total - 1]);
			}
		}
		gui->update();
		gui->update_preview();
	}
	return 1;
}

int CWindowMaskDelete::keypress_event()
{
	if(get_keypress() == BACKSPACE ||
		get_keypress() == DELETE) 
		return handle_event();
	return 0;
}


CWindowMaskNumber::CWindowMaskNumber(MWindow *mwindow, 
	CWindowToolGUI *gui, 
	int x, 
	int y)
 : BC_TumbleTextBox(gui, 
		mwindow->edl->session->cwindow_mask,
		0,
		SUBMASKS - 1,
		x, 
		y, 
		100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CWindowMaskNumber::handle_event()
{
	mwindow->edl->session->cwindow_mask = atol(get_text());
	gui->update();
	gui->update_preview();
	return 1;
}


CWindowMaskFeather::CWindowMaskFeather(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui, 
		0,
		0,
		0xff,
		x, 
		y, 
		100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CWindowMaskFeather::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	((CWindowMaskGUI*)gui)->get_keyframe(track, 
		keyframe,
		mask, 
		point,
		1);

	keyframe->feather = atof(get_text());
	gui->update_preview();
	return 1;
}


CWindowMaskValue::CWindowMaskValue(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_ISlider(x, 
		y,
		0,
		200,
		200,
		0,
		100,
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CWindowMaskValue::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	((CWindowMaskGUI*)gui)->get_keyframe(track, 
		keyframe,
		mask, 
		point,
		1);

	keyframe->value = get_value();
	gui->update_preview();
	return 1;
}



CWindowMaskBeforePlugins::CWindowMaskBeforePlugins(CWindowToolGUI *gui, int x, int y)
 : BC_CheckBox(x, 
	y, 
	1, 
	_("Apply mask before plugins"))
{
	this->gui = gui;
}

int CWindowMaskBeforePlugins::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	((CWindowMaskGUI*)gui)->get_keyframe(track, 
		keyframe,
		mask, 
		point,
		1);

	if (keyframe) {
		keyframe->apply_before_plugins = get_value();
		gui->update_preview();
	}
	return 1;
}


CWindowMaskGUI::CWindowMaskGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
	thread,
	PROGRAM_NAME ": Mask",
	330,
	280)
{
	int x = 10, y = 10;
	MaskAuto *keyframe = 0;
	BC_Title *title;
	Track *track = mwindow->cwindow->calculate_affected_track();

	this->mwindow = mwindow;
	this->thread = thread;

	if(track)
		keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->autos[AUTOMATION_MASK], 0);

	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	add_subwindow(mode = new CWindowMaskMode(mwindow, 
		this, 
		x + title->get_w(), 
		y,
		""));
	mode->create_objects();
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Value:")));
	add_subwindow(value = new CWindowMaskValue(mwindow, this, x + 50, y));
	y += 30;
	add_subwindow(delete_point = new CWindowMaskDelete(mwindow, this, x, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Mask number:")));
	number = new CWindowMaskNumber(mwindow, 
		this, 
		x + 110, 
		y);
	number->create_objects();
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Feather:")));
	feather = new CWindowMaskFeather(mwindow,
		this,
		x + 110,
		y);
	feather->create_objects();
	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w();
	this->x = new CWindowCoord(this, 
		x, 
		y, 
		(float)0.0);
	this->x->create_objects();
	x += 150;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w();
	this->y = new CWindowCoord(this, 
		x, 
		y, 
		(float)0.0);
	this->y->create_objects();

	y += 30;
	add_subwindow(this->apply_before_plugins = new CWindowMaskBeforePlugins(this, 
		10, 
		y));

	update();
}

CWindowMaskGUI::~CWindowMaskGUI()
{
	delete number;
	delete feather;
}

void CWindowMaskGUI::get_keyframe(Track* &track, 
	MaskAuto* &keyframe, 
	SubMask* &mask, 
	MaskPoint* &point,
	int create_it)
{
	keyframe = 0;
	track = mwindow->cwindow->calculate_affected_track();
	if(track)
		keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->autos[AUTOMATION_MASK], create_it);
	else
		keyframe = 0;

	if(keyframe)
		mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);
	else
		mask = 0;

	point = 0;
	if(keyframe)
	{
		if(mwindow->cwindow->gui->affected_point < mask->points.total &&
			mwindow->cwindow->gui->affected_point >= 0)
		{
			point =  mask->points.values[mwindow->cwindow->gui->affected_point];
		}
	}
}

void CWindowMaskGUI::update()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

	get_keyframe(track, 
		keyframe, 
		mask,
		point,
		0);

	if(!keyframe)
		return;

	if(point)
	{
		x->update(point->x);
		y->update(point->y);
	}

	if(mask)
	{
		feather->update((int64_t)keyframe->feather);
		value->update((int64_t)keyframe->value);
		apply_before_plugins->update((int64_t)keyframe->apply_before_plugins);
	}

	number->update((int64_t)mwindow->edl->session->cwindow_mask);

	if(track)
	{
		mode->set_text(
			CWindowMaskMode::mode_to_text(((MaskAuto*)track->automation->autos[AUTOMATION_MASK]->first)->mode));
	}
}

int CWindowMaskGUI::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	get_keyframe(track, 
		keyframe, 
		mask,
		point,
		0);

	if(point)
	{
		point->x = atof(x->get_text());
		point->y = atof(y->get_text());
	}

	update_preview();
	return 1;
}

void CWindowMaskGUI::update_preview()
{
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->cwindow->playback_engine->send_command(CURRENT_FRAME, mwindow->edl);
	mwindow->cwindow->gui->canvas->draw_refresh();
}
