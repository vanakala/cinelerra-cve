#include "automation.h"
#include "bezierauto.h"
#include "bezierautos.h"
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
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "transportque.h"


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

//printf("CWindowTool::start_tool 1\n");
	if(current_tool != operation)
	{
		current_tool = operation;
		switch(operation)
		{
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

//printf("CWindowTool::start_tool 1\n");


		if(!result)
		{
			stop_tool();
// Wait for previous tool GUI to finish
			output_lock->lock("CWindowTool::start_tool");
			this->tool_gui = new_gui;
			tool_gui->create_objects();
// Signal thread to run next tool GUI
			input_lock->unlock();
		}
//printf("CWindowTool::start_tool 1\n");
	}
	else
	if(tool_gui) 
	{
		tool_gui->lock_window("CWindowTool::start_tool");
		tool_gui->update();
		tool_gui->unlock_window();
	}

//printf("CWindowTool::start_tool 2\n");

}


void CWindowTool::stop_tool()
{
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::stop_tool");
		tool_gui->set_done(0);
		tool_gui->unlock_window();
	}
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
		tool_gui->lock_window("CWindowTool::update_show_window");

		if(mwindow->edl->session->tool_window) 
		{
			tool_gui->update();
			tool_gui->show_window();
		}
		else
			tool_gui->hide_window();
		tool_gui->flush();

		tool_gui->unlock_window();
	}
}

void CWindowTool::update_values()
{
	tool_gui_lock->lock("CWindowTool::update_values");
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::update_values");
		tool_gui->update();
		tool_gui->flush();
		tool_gui->unlock_window();
	}
	tool_gui_lock->unlock();
}







CWindowToolGUI::CWindowToolGUI(MWindow *mwindow, 
	CWindowTool *thread, 
	char *title,
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
}

CWindowToolGUI::~CWindowToolGUI()
{
}

int CWindowToolGUI::close_event()
{
	hide_window();
	flush();
	mwindow->edl->session->tool_window = 0;
	thread->gui->lock_window("CWindowToolGUI::close_event");
	thread->gui->composite_panel->set_operation(mwindow->edl->session->cwindow_operation);
	thread->gui->flush();
	thread->gui->unlock_window();
	return 1;
}

int CWindowToolGUI::keypress_event()
{
	if(get_keypress() == 'w' || get_keypress() == 'W')
		return close_event();
	return 0;
}

int CWindowToolGUI::translation_event()
{
	mwindow->session->ctool_x = get_x();
	mwindow->session->ctool_y = get_y();
	return 0;
}






CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, float value)
 : BC_TumbleTextBox(gui, 
		(float)value,
		(float)-65536,
		(float)65536,
		x, 
		y, 
		100)
{
	this->gui = gui;
}

CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, int value)
 : BC_TumbleTextBox(gui, 
		(int64_t)value,
		(int64_t)-65536,
		(int64_t)65536,
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
}


CWindowCropGUI::~CWindowCropGUI()
{
}

void CWindowCropGUI::create_objects()
{
	int x = 10, y = 10;
	BC_TumbleTextBox *textbox;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x, y, _("X1:")));
	x += title->get_w();
	x1 = new CWindowCoord(thread->tool_gui, x, y, mwindow->edl->session->crop_x1);
	x1->create_objects();
	x += x1->get_w() + 10;

	add_subwindow(title = new BC_Title(x, y, _("Y1:")));
	x += title->get_w();
	y1 = new CWindowCoord(thread->tool_gui, x, y, mwindow->edl->session->crop_y1);
	y1->create_objects();
	y += y1->get_h() + 5;
	x = 10;

	add_subwindow(title = new BC_Title(x, y, _("X2:")));
	x += title->get_w();
	x2 = new CWindowCoord(thread->tool_gui, x, y, mwindow->edl->session->crop_x2);
	x2->create_objects();
	x += x2->get_w() + 10;

	add_subwindow(title = new BC_Title(x, y, _("Y2:")));
	x += title->get_w();
	y2 = new CWindowCoord(thread->tool_gui, x, y, mwindow->edl->session->crop_y2);
	y2->create_objects();
	y += y2->get_h() + 5;
	x = 10;
	add_subwindow(new CWindowCropOK(mwindow, thread->tool_gui, x, y));

	if(mwindow->edl->session->tool_window) show_window();
	flush();
}

void CWindowCropGUI::handle_event()
{
	mwindow->edl->session->crop_x1 = atol(x1->get_text());
	mwindow->edl->session->crop_y1 = atol(y1->get_text());
	mwindow->edl->session->crop_x2 = atol(x2->get_text());
	mwindow->edl->session->crop_y2 = atol(y2->get_text());
	mwindow->cwindow->gui->lock_window("CWindowCropGUI::handle_event");
	mwindow->cwindow->gui->canvas->draw_refresh();
	mwindow->cwindow->gui->unlock_window();
}

void CWindowCropGUI::update()
{
	x1->update((int64_t)mwindow->edl->session->crop_x1);
	y1->update((int64_t)mwindow->edl->session->crop_y1);
	x2->update((int64_t)mwindow->edl->session->crop_x2);
	y2->update((int64_t)mwindow->edl->session->crop_y2);
}












CWindowCameraGUI::CWindowCameraGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
 	thread,
	PROGRAM_NAME ": Camera",
	170,
	170)
{
}
CWindowCameraGUI::~CWindowCameraGUI()
{
}

void CWindowCameraGUI::create_objects()
{
	int x = 10, y = 10, x1;
	Track *track = mwindow->cwindow->calculate_affected_track();
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	BC_Title *title;
	BC_Button *button;

	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->camera_autos, 
			0);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->czoom_autos, 
			0);
	}

	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w();
	this->x = new CWindowCoord(this, 
		x, 
		y, 
		keyframe ? keyframe->center_x : (float)0);
	this->x->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w();
	this->y = new CWindowCoord(this, 
		x, 
		y, 
		keyframe ? keyframe->center_y : (float)0);
	this->y->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Z:")));
	x += title->get_w();
	this->z = new CWindowCoord(this, 
		x, 
		y, 
		zoom_keyframe ? zoom_keyframe->value : (float)1);
	this->z->create_objects();

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



	if(mwindow->edl->session->tool_window) show_window();
	flush();
}

void CWindowCameraGUI::update_preview()
{
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);

	mwindow->cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
	mwindow->cwindow->gui->lock_window("CWindowCameraGUI::update_preview");
	mwindow->cwindow->gui->canvas->draw_refresh();
	mwindow->cwindow->gui->unlock_window();
}


void CWindowCameraGUI::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		if(event_caller == z)
			zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->czoom_autos,
				1);
		else
			keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->camera_autos,
				1);
	}

	if(keyframe)
	{
		keyframe->center_x = atof(x->get_text());
		keyframe->center_y = atof(y->get_text());
		update_preview();
	}
	else
	if(zoom_keyframe)
	{
		float zoom = atof(z->get_text());
		if(zoom > 10) zoom = 10; 
		else
		if(zoom < 0) zoom = 0;
// Doesn't allow user to enter from scratch
// 		if(zoom != atof(z->get_text())) 
// 			z->update(zoom);

		zoom_keyframe->value = zoom;
		mwindow->gui->lock_window("CWindowCameraGUI::handle_event");
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
		mwindow->gui->unlock_window();
		update_preview();
	}
}

void CWindowCameraGUI::update()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->camera_autos, 
			0);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->czoom_autos, 
			0);
	}

	if(keyframe)
	{
		x->update(keyframe->center_x);
		y->update(keyframe->center_y);
	}

	if(zoom_keyframe)
	{
		z->update(zoom_keyframe->value);
	}
}

BezierAuto* CWindowCameraGUI::get_keyframe()
{
	BezierAuto *keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->camera_autos);
	return keyframe;
}



CWindowCameraLeft::CWindowCameraLeft(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->left_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}
int CWindowCameraLeft::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->camera_autos, 1);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->czoom_autos, 0);
	}

	if(keyframe)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(mwindow->edl->local_session->selectionstart,
			w,
			h);

		if(w && h)
		{
			keyframe->center_x = 
				(double)track->track_w / zoom_keyframe->value / 2 - 
				(double)w / 2;
			gui->update();
			gui->update_preview();
		}
	}

	return 1;
}


CWindowCameraCenter::CWindowCameraCenter(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->center_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}
int CWindowCameraCenter::handle_event()
{
	BezierAuto *keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->camera_autos);

	if(keyframe)
	{
		keyframe->center_x = 0;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowCameraRight::CWindowCameraRight(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->right_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}
int CWindowCameraRight::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->camera_autos, 1);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->czoom_autos, 0);
	}

	if(keyframe)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(mwindow->edl->local_session->selectionstart,
			w,
			h);

		if(w && h)
		{
			keyframe->center_x = -((double)track->track_w / zoom_keyframe->value / 2 - 
				(double)w / 2);
			gui->update();
			gui->update_preview();
		}
	}

	return 1;
}


CWindowCameraTop::CWindowCameraTop(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->top_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}
int CWindowCameraTop::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->camera_autos, 1);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->czoom_autos, 0);
	}

	if(keyframe)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(mwindow->edl->local_session->selectionstart,
			w,
			h);

		if(w && h)
		{
			keyframe->center_y = (double)track->track_h / zoom_keyframe->value / 2 - 
				(double)h / 2;
			gui->update();
			gui->update_preview();
		}
	}

	return 1;
}


CWindowCameraMiddle::CWindowCameraMiddle(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->middle_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}
int CWindowCameraMiddle::handle_event()
{
	BezierAuto *keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->camera_autos);

	if(keyframe)
	{
		keyframe->center_y = 0;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowCameraBottom::CWindowCameraBottom(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->bottom_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}
int CWindowCameraBottom::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->camera_autos, 1);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->czoom_autos, 0);
	}

	if(keyframe)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(mwindow->edl->local_session->selectionstart,
			w,
			h);

		if(w && h)
		{
			keyframe->center_y = -((double)track->track_h / zoom_keyframe->value / 2 - 
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
}
CWindowProjectorGUI::~CWindowProjectorGUI()
{
}
void CWindowProjectorGUI::create_objects()
{
	int x = 10, y = 10, x1;
	Track *track = mwindow->cwindow->calculate_affected_track();
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	BC_Title *title;
	BC_Button *button;

	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->projector_autos, 
			0);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->pzoom_autos, 
			0);
	}

	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w();
	this->x = new CWindowCoord(this, 
		x, 
		y, 
		keyframe ? keyframe->center_x : (float)0);
	this->x->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w();
	this->y = new CWindowCoord(this, 
		x, 
		y, 
		keyframe ? keyframe->center_y : (float)0);
	this->y->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Z:")));
	x += title->get_w();
	this->z = new CWindowCoord(this, 
		x, 
		y, 
		zoom_keyframe ? zoom_keyframe->value : (float)1);
	this->z->create_objects();

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


	if(mwindow->edl->session->tool_window) show_window();
	flush();
}

void CWindowProjectorGUI::update_preview()
{
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
	mwindow->cwindow->gui->lock_window("CWindowProjectorGUI::update_preview");
	mwindow->cwindow->gui->canvas->draw_refresh();
	mwindow->cwindow->gui->unlock_window();
}

void CWindowProjectorGUI::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		if(event_caller == z)
			zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->pzoom_autos,
				1);
		else
			keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->projector_autos,
				1);
	}

	if(keyframe)
	{
		keyframe->center_x = atof(x->get_text());
		keyframe->center_y = atof(y->get_text());
		update_preview();
	}
	else
	if(zoom_keyframe)
	{
		float zoom = atof(z->get_text());
		if(zoom > 10000) zoom = 10000; 
		else 
		if(zoom < 0) zoom = 0;
// 		if (zoom != atof(z->get_text())) 
// 			z->update(zoom);
		zoom_keyframe->value = zoom;
		mwindow->gui->lock_window("CWindowProjectorGUI::handle_event");
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
		mwindow->gui->unlock_window();
		update_preview();
	}
}

void CWindowProjectorGUI::update()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->projector_autos, 
			0);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->pzoom_autos, 
			0);
	}

	if(keyframe)
	{
		x->update(keyframe->center_x);
		y->update(keyframe->center_y);
	}

	if(zoom_keyframe)
	{
		z->update(zoom_keyframe->value);
	}
}

BezierAuto* CWindowProjectorGUI::get_keyframe()
{
	BezierAuto *keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->projector_autos);
	return keyframe;
}






































CWindowProjectorLeft::CWindowProjectorLeft(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->left_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}
int CWindowProjectorLeft::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->projector_autos, 1);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->pzoom_autos, 0);
	}
	if(keyframe)
	{
		keyframe->center_x = (double)track->track_w * zoom_keyframe->value / 2 - 
			(double)mwindow->edl->session->output_w / 2;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorCenter::CWindowProjectorCenter(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->center_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}
int CWindowProjectorCenter::handle_event()
{
	BezierAuto *keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->projector_autos);

	if(keyframe)
	{
		keyframe->center_x = 0;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorRight::CWindowProjectorRight(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->right_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}
int CWindowProjectorRight::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->projector_autos, 1);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->pzoom_autos, 0);
	}

	if(keyframe)
	{
		keyframe->center_x = -((double)track->track_w * zoom_keyframe->value / 2 - 
			(double)mwindow->edl->session->output_w / 2);
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorTop::CWindowProjectorTop(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->top_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}
int CWindowProjectorTop::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->projector_autos, 1);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->pzoom_autos, 0);
	}

	if(keyframe)
	{
		keyframe->center_y = (double)track->track_h * zoom_keyframe->value / 2 - 
			(double)mwindow->edl->session->output_h / 2;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorMiddle::CWindowProjectorMiddle(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->middle_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}
int CWindowProjectorMiddle::handle_event()
{
	BezierAuto *keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->projector_autos);

	if(keyframe)
	{
		keyframe->center_y = 0;
		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowProjectorBottom::CWindowProjectorBottom(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->bottom_justify)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}
int CWindowProjectorBottom::handle_event()
{
	BezierAuto *keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		keyframe = (BezierAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->projector_autos, 1);
		zoom_keyframe = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->pzoom_autos, 0);
	}

	if(keyframe)
	{
		keyframe->center_y = -((double)track->track_h * zoom_keyframe->value / 2 - 
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
	char *text)
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

char* CWindowMaskMode::mode_to_text(int mode)
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

int CWindowMaskMode::text_to_mode(char *text)
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
		((MaskAuto*)track->automation->mask_autos->default_auto)->mode = 
			text_to_mode(get_text());
	}

//printf("CWindowMaskMode::handle_event 1\n");
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
		MaskAutos *mask_autos = track->automation->mask_autos;
		for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto;
			current; )
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


			if(current == (MaskAuto*)mask_autos->default_auto)
				current = (MaskAuto*)mask_autos->first;
			else
				current = (MaskAuto*)NEXT;
		}
		gui->update();
		gui->update_preview();
	}


// 	((CWindowMaskGUI*)gui)->get_keyframe(track, 
// 		keyframe, 
// 		mask, 
// 		point,
// 		0);

// Need to apply to every keyframe
	
// 	if(keyframe)
// 	{
// 		for(int i = mwindow->cwindow->gui->affected_point;
// 			i < mask->points.total - 1;
// 			i++)
// 		{
// 			*mask->points.values[i] = *mask->points.values[i + 1];
// 		}
// 		
// 		if(mask->points.total)
// 		{
// 			mask->points.remove_object(mask->points.values[mask->points.total - 1]);
// 		}
// 
// 		gui->update();
// 		gui->update_preview();
// 	}

	return 1;
}

int CWindowMaskDelete::keypress_event()
{
	if(get_keypress() == BACKSPACE ||
		get_keypress() == DELETE) 
		return handle_event();
	return 0;
}


CWindowMaskCycleNext::CWindowMaskCycleNext(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Cycle next"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int CWindowMaskCycleNext::handle_event()
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

	MaskPoint *temp;

// Should apply to all keyframes
	if(keyframe && mask->points.total)
	{
		temp = mask->points.values[0];

		for(int i = 0; i < mask->points.total - 1; i++)
		{
			mask->points.values[i] = mask->points.values[i + 1];
		}
		mask->points.values[mask->points.total - 1] = temp;

		mwindow->cwindow->gui->affected_point--;
		if(mwindow->cwindow->gui->affected_point < 0)
			mwindow->cwindow->gui->affected_point = mask->points.total - 1;

		gui->update();
		gui->update_preview();
	}
	
	return 1;
}

CWindowMaskCyclePrev::CWindowMaskCyclePrev(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Cycle prev"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int CWindowMaskCyclePrev::handle_event()
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

// Should apply to all keyframes
	MaskPoint *temp;
	if(keyframe && mask->points.total)
	{
		temp = mask->points.values[mask->points.total - 1];

		for(int i = mask->points.total - 1; i > 0; i--)
		{
			mask->points.values[i] = mask->points.values[i - 1];
		}
		mask->points.values[0] = temp;

		mwindow->cwindow->gui->affected_point++;
		if(mwindow->cwindow->gui->affected_point >= mask->points.total)
			mwindow->cwindow->gui->affected_point = 0;

		gui->update();
		gui->update_preview();
	}
	return 1;
}


CWindowMaskNumber::CWindowMaskNumber(MWindow *mwindow, 
	CWindowToolGUI *gui, 
	int x, 
	int y)
 : BC_TumbleTextBox(gui, 
		(int64_t)mwindow->edl->session->cwindow_mask,
		(int64_t)0,
		(int64_t)SUBMASKS - 1,
		x, 
		y, 
		100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskNumber::~CWindowMaskNumber()
{
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
		(int64_t)0,
		(int64_t)0,
		(int64_t)0xff,
		x, 
		y, 
		100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
CWindowMaskFeather::~CWindowMaskFeather()
{
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

CWindowMaskValue::~CWindowMaskValue()
{
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











CWindowMaskGUI::CWindowMaskGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, 
 	thread,
	PROGRAM_NAME ": Mask",
	330,
	280)
{
	this->mwindow = mwindow;
	this->thread = thread;
}
CWindowMaskGUI::~CWindowMaskGUI()
{
	delete number;
	delete feather;
}

void CWindowMaskGUI::create_objects()
{
//printf("CWindowMaskGUI::create_objects 1\n");
	int x = 10, y = 10;
	MaskAuto *keyframe = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->mask_autos, 0);
//printf("CWindowMaskGUI::create_objects 1\n");

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	add_subwindow(mode = new CWindowMaskMode(mwindow, 
		this, 
		x + title->get_w(), 
		y,
		""));
//printf("CWindowMaskGUI::create_objects 1\n");
	mode->create_objects();
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Value:")));
	add_subwindow(value = new CWindowMaskValue(mwindow, this, x + 50, y));
	y += 30;
	add_subwindow(delete_point = new CWindowMaskDelete(mwindow, this, x, y));
	y += 30;
//	add_subwindow(next_point = new CWindowMaskCycleNext(mwindow, this, x, y));
//	y += 30;
//	add_subwindow(prev_point = new CWindowMaskCyclePrev(mwindow, this, x, y));
//	y += 40;
	add_subwindow(new BC_Title(x, y, _("Mask number:")));
	number = new CWindowMaskNumber(mwindow, 
		this, 
		x + 110, 
		y);
//printf("CWindowMaskGUI::create_objects 1\n");
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
//printf("CWindowMaskGUI::create_objects 1\n");

	update();
	if(mwindow->edl->session->tool_window) show_window();
	flush();
//printf("CWindowMaskGUI::create_objects 2\n");
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
		keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->mask_autos, create_it);
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
//printf("CWindowMaskGUI::update 1\n");
	get_keyframe(track, 
		keyframe, 
		mask,
		point,
		0);

//printf("CWindowMaskGUI::update 1\n");
	if(point)
	{
		x->update(point->x);
		y->update(point->y);
	}
//printf("CWindowMaskGUI::update 1\n");

	if(mask)
	{
		feather->update((int64_t)keyframe->feather);
		value->update((int64_t)keyframe->value);
	}
//printf("CWindowMaskGUI::update 1\n");

	number->update((int64_t)mwindow->edl->session->cwindow_mask);

//printf("CWindowMaskGUI::update 1\n");
	if(track)
	{
		mode->set_text(
			CWindowMaskMode::mode_to_text(((MaskAuto*)track->automation->mask_autos->default_auto)->mode));
	}
//printf("CWindowMaskGUI::update 2\n");
}

void CWindowMaskGUI::handle_event()
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
}

void CWindowMaskGUI::update_preview()
{
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
	mwindow->cwindow->gui->lock_window("CWindowMaskGUI::update_preview");
	mwindow->cwindow->gui->canvas->draw_refresh();
	mwindow->cwindow->gui->unlock_window();
}










