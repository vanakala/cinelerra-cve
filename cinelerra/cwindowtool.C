// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "automation.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "clip.h"
#include "condition.h"
#include "cpanel.h"
#include "cropauto.h"
#include "cropautos.h"
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
#include "theme.h"
#include "track.h"

struct tool_names CWindowMaskMode::modenames[] =
{
	{ N_("Multiply alpha"), MASK_MULTIPLY_ALPHA },
	{ N_("Subtract alpha"), MASK_SUBTRACT_ALPHA },
	{ N_("Hide surrounding"), MASK_MULTIPLY_COLOR },
	{ N_("Display surrounding"), MASK_SUBTRACT_COLOR },
	{ 0, 0 }
};

CWindowTool::CWindowTool(CWindowGUI *gui)
 : Thread(THREAD_SYNCHRONOUS)
{
	cwindowgui = gui;
	tool_gui = 0;
	done = 0;
	current_tool = CWINDOW_NONE;
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
			new_gui = new CWindowEyedropGUI(this);
			break;
		case CWINDOW_CROP:
			new_gui = new CWindowCropGUI(this);
			break;
		case CWINDOW_CAMERA:
			new_gui = new CWindowCameraGUI(this);
			break;
		case CWINDOW_PROJECTOR:
			new_gui = new CWindowProjectorGUI(this);
			break;
		case CWINDOW_RULER:
			new_gui = new CWindowRulerGUI(this);
			break;
		case CWINDOW_MASK:
			new_gui = new CWindowMaskGUI(this);
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

			if(edlsession->tool_window && mainsession->show_cwindow)
				tool_gui->show_window();

// Signal thread to run next tool GUI
			input_lock->unlock();
		}
	}
	else if(tool_gui)
		tool_gui->update();
}

void CWindowTool::stop_tool()
{
	if(tool_gui)
		tool_gui->set_done(0);
}

void CWindowTool::show_tool()
{
	if(tool_gui && edlsession->tool_window)
		tool_gui->show_window();
}

void CWindowTool::hide_tool()
{
	if(tool_gui && edlsession->tool_window)
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

int CWindowTool::update_show_window()
{
	int ret = 0;

	tool_gui_lock->lock("CWindowTool::update_show_window");
	if(tool_gui)
	{
		if(edlsession->tool_window)
		{
			tool_gui->allocate_autos();
			tool_gui->update();
			tool_gui->show_window();
			tool_gui->raise_window();
		}
		ret = 1;
	}
	tool_gui_lock->unlock();
	return ret;
}

void CWindowTool::raise_window()
{
	if(tool_gui)
		tool_gui->raise_window();
}

void CWindowTool::update_values()
{
	tool_gui_lock->lock("CWindowTool::update_values");
	if(tool_gui)
		tool_gui->update();
	tool_gui_lock->unlock();
}


CWindowToolGUI::CWindowToolGUI(CWindowTool *thread,
	const char *title,
	int w, 
	int h)
 : BC_Window(title, mainsession->ctool_x, mainsession->ctool_y,
	w, h, w, h, 0, 0, 1)
{
	this->thread = thread;
	current_operation = 0;
	set_icon(theme_global->get_image("cwindow_icon"));
}

void CWindowToolGUI::close_event()
{
	hide_window();
	edlsession->tool_window = 0;

	thread->cwindowgui->composite_panel->set_operation(edlsession->cwindow_operation);
}

void CWindowToolGUI::translation_event()
{
	mainsession->ctool_x = get_x();
	mainsession->ctool_y = get_y();
}

void CWindowToolGUI::get_keyframes(FloatAuto* &x_auto, FloatAuto* &y_auto,
	FloatAuto* &z_auto, int camera,
	int create_x, int create_y, int create_z)
{
	Track *track = mwindow_global->cwindow->calculate_affected_track();
	ptstime pos = master_edl->local_session->get_selectionstart(1);

	x_auto = 0;
	y_auto = 0;
	z_auto = 0;

	if(track)
	{
		mwindow_global->cwindow->calculate_affected_autos(&x_auto,
			&y_auto, &z_auto, track, camera,
			create_x, create_y, create_z);
	}

	if(edlsession->auto_keyframes)
	{
		if(x_auto && !PTSEQU(pos, x_auto->pos_time))
		{
			local_x->interpolate_from(x_auto, x_auto->next, pos, 0);
			x_auto = local_x;
		}

		if(y_auto && !PTSEQU(pos, y_auto->pos_time))
		{
			local_y->interpolate_from(y_auto, y_auto->next, pos, 0);
			y_auto = local_y;
		}

		if(z_auto && !PTSEQU(pos, z_auto->pos_time))
		{
			local_z->interpolate_from(z_auto, z_auto->next, pos, 0);
			z_auto = local_z;
		}
	}
}


CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, double value, int log_increment = 0)
 : BC_TumbleTextBox(gui, value, -65536.0, 65536.0, x, y, 100)
{
	this->gui = gui;
	set_log_floatincrement(log_increment);
}

CWindowCoord::CWindowCoord(int x, int y, double value, CWindowToolGUI *gui)
 : BC_TumbleTextBox(gui, value, 0.0, 1.0, x, y, 100)
{
	this->gui = gui;
	set_increment(0.0001);
}

CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, int value)
 : BC_TumbleTextBox(gui, value, -65536, 65536, x, y, 100)
{
	this->gui = gui;
}

int CWindowCoord::handle_event()
{
	gui->event_caller = this;
	gui->handle_event();
	return 1;
}

CWindowCropBeforePlugins::CWindowCropBeforePlugins(CWindowCropGUI *gui,
	int x, int y)
 : BC_CheckBox(x, y, 1, _("Apply crop before plugins"))
{
	this->gui = gui;
}

int CWindowCropBeforePlugins::handle_event()
{
	CropAuto *keyframe;

	if(!(keyframe = gui->get_keyframe(0)))
		return 0;

	keyframe->apply_before_plugins = get_value();
	mwindow_global->sync_parameters();
	gui->thread->cwindowgui->canvas->draw_refresh();
	return 1;
}

CWindowCropGUI::CWindowCropGUI(CWindowTool *thread)
 : CWindowToolGUI(thread, MWindow::create_title(N_("Crop")),
	330, 100)
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
	add_subwindow(apply = new CWindowCropBeforePlugins(this, x, y));

	x += column1 + 5;
	y = 10;
	x1 = new CWindowCoord(x, y, 0, this);
	y += pad;
	width = new CWindowCoord(x, y, 0, this);

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
	y1 = new CWindowCoord(x, y, 0, this);
	y += pad;
	height = new CWindowCoord(x, y, 0, this);
	update();
}

int CWindowCropGUI::handle_event()
{
	double new_x1, new_y1;
	CropAuto *keyframe;
	Track *track;

	if(!(keyframe = get_keyframe(1)))
		return 0;

	new_x1 = atof(x1->get_text());
	new_y1 = atof(y1->get_text());

	if(!EQUIV(new_x1, keyframe->left))
	{
		keyframe->right = new_x1 + keyframe->right -
			keyframe->left;
		keyframe->left = new_x1;
	}
	if(!EQUIV(new_y1, keyframe->top))
	{
		keyframe->bottom = new_y1 + keyframe->bottom -
			keyframe->top;
		keyframe->top = new_y1;
	}
	keyframe->right = atof(width->get_text()) +
		keyframe->left;
	keyframe->bottom = atof(height->get_text()) +
		keyframe->top;
	CLAMP(keyframe->left, 0, 1.0);
	CLAMP(keyframe->right, 0, 1.0);
	CLAMP(keyframe->top, 0, 1.0);
	CLAMP(keyframe->bottom, 0, 1.0);
	update();
	mwindow_global->sync_parameters();
	thread->cwindowgui->canvas->draw_refresh();
	return 1;
}

void CWindowCropGUI::update()
{
	get_keyframe(0);
	x1->update(left);
	y1->update(top);
	width->update(right - left);
	height->update(bottom - top);
	apply->update(before_plugins);
}

CropAuto *CWindowCropGUI::get_keyframe(int create_it)
{
	Track *track;
	CropAuto *keyframe;
	CropAutos *autos;

	ptstime pos = master_edl->local_session->get_selectionstart(1);

	track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
		keyframe = (CropAuto*)mwindow_global->cwindow->calculate_affected_auto(
			AUTOMATION_CROP, pos, track, create_it);
	else
		return 0;

	if(keyframe)
	{
		if(edlsession->auto_keyframes &&
				!PTSEQU(pos, keyframe->pos_time))
		{
			CropAutos *autos = (CropAutos*)track->automation->have_autos(AUTOMATION_CROP);
			autos->get_values(pos, &left, &right, &top, &bottom);
		}
		else
		{
			top = keyframe->top;
			bottom = keyframe->bottom;
			left = keyframe->left;
			right = keyframe->right;
		}
		before_plugins = keyframe->apply_before_plugins;
	}
	return keyframe;
}

void CWindowCropGUI::allocate_autos()
{
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track && !track->automation->have_autos(AUTOMATION_CROP))
		track->automation->get_autos(AUTOMATION_CROP);
}

CWindowEyedropGUI::CWindowEyedropGUI(CWindowTool *thread)
 : CWindowToolGUI(thread, MWindow::create_title(N_("Color")),
	150, 150)
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
	int r, g, b;

	master_edl->local_session->get_picker_rgb(&r, &g, &b);

	r >>= 8;
	g >>= 8;
	b >>= 8;
	red->update(r);
	green->update(g);
	blue->update(b);
	sample->set_color((r << 16) | (g << 8) | b);
	sample->draw_box(0, 0, sample->get_w(), sample->get_h());
	sample->set_color(BLACK);
	sample->draw_rectangle(0, 0, sample->get_w(), sample->get_h());
	sample->flash();
}

// Buttons to control Keyframe-Tangent-Mode for Projector or Camera
const _TGD Camera_Tan_Smooth =
{
	TGNT_SMOOTH,
	true,
	"tan_smooth",
	N_("\"smooth\" Tangent on current Camera Keyframes")
};

const _TGD Camera_Tan_Linear =
{
	TGNT_LINEAR,
	true,
	"tan_linear",
	N_("\"linear\" Tangent on current Camera Keyframes")
};

const _TGD Projector_Tan_Smooth =
{
	TGNT_SMOOTH,
	false,
	"tan_smooth",
	N_("\"smooth\" Tangent on current Projector Keyframes")
};

const _TGD Projector_Tan_Linear =
{
	TGNT_LINEAR,
	false,
	"tan_linear",
	N_("\"linear\" Tangent on current Projector Keyframes")
};

CWindowTangentToggle::CWindowTangentToggle(_TGD mode,
	CWindowToolGUI *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set(mode.icon_id), false), cfg(mode)
{
	this->gui = gui;
	set_tooltip(_(cfg.tooltip));
}

void CWindowTangentToggle::check_toggle_state(FloatAuto *x, FloatAuto *y, FloatAuto *z)
{
// the toggle state is only set to ON if all
// three automation lines have the same tangent mode.
// For mixed states the toggle stays off.
	set_value(x->tangent_mode == this->cfg.mode &&
		y->tangent_mode == this->cfg.mode &&
		z->tangent_mode == this->cfg.mode,
		true); // redraw to show new state
}

int CWindowTangentToggle::handle_event()
{
	FloatAuto *x=0, *y=0, *z=0;
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
	{
		mwindow_global->cwindow->calculate_affected_autos(&x, &y, &z,
			track, cfg.use_camera, 0, 0, 0); // don't create new keyframe

		if(x)
			x->change_tangent_mode(cfg.mode);
		if(y)
			y->change_tangent_mode(cfg.mode);
		if(z)
			z->change_tangent_mode(cfg.mode);

		gui->update();
		gui->update_preview();
	}
	return 1;
}

CWindowCameraGUI::CWindowCameraGUI(CWindowTool *thread)
 : CWindowCamProjGUI(thread, MWindow::create_title(N_("Camera")), 1)
{
}

CWindowProjectorGUI::CWindowProjectorGUI(CWindowTool *thread)
 : CWindowCamProjGUI(thread, MWindow::create_title(N_("Projector")), 0)
{
}

CWindowCamProjGUI::CWindowCamProjGUI(CWindowTool *thread,
	const char *tooltitle, int camera)
 : CWindowToolGUI(thread, tooltitle, 170, 170)
{
	int x = 10, y = 10, x1;
	BC_Title *title;
	BC_Button *button;

	is_camera = camera;
	local_x = new FloatAuto(0, 0);
	local_y = new FloatAuto(0, 0);
	local_z = new FloatAuto(0, 0);

	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w();
	this->x = new CWindowCoord(this, x, y, 0);
	y += 30;

	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w();
	this->y = new CWindowCoord(this, x, y, 0);
	y += 30;

	x = 10;
	add_subwindow(title = new BC_Title(x, y, _("Z:")));
	x += title->get_w();
	this->z = new CWindowCoord(this, x, y, 1.0, 1);
	this->z->set_boundaries(.0001, 256.0);

	y += 30;
	x1 = 10;
	add_subwindow(button = new CWindowCPLeft(this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCPCenter(this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCPRight(this, x1, y));

	y += button->get_h();
	x1 = 10;
	add_subwindow(button = new CWindowCPTop(this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCPMiddle(this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCPBottom(this, x1, y));

// additional Buttons to control the tangent mode of the "current" keyframe
	x1 += button->get_w() + 15;
	add_subwindow(this->t_smooth = new CWindowTangentToggle(
		is_camera ? Camera_Tan_Smooth : Projector_Tan_Smooth, this, x1, y));
	x1 += button->get_w();
	add_subwindow(this->t_linear = new CWindowTangentToggle(
		is_camera ? Camera_Tan_Linear : Projector_Tan_Linear, this, x1, y));

// fill in current auto keyframe values, set toggle states.
	this->update();
}

CWindowCamProjGUI::~CWindowCamProjGUI()
{
	delete local_x;
	delete local_y;
	delete local_z;
}

void CWindowCamProjGUI::update_preview()
{
	mwindow_global->sync_parameters();

	mwindow_global->draw_canvas_overlays();
	thread->cwindowgui->canvas->draw_refresh();
}

int CWindowCamProjGUI::handle_event()
{
	FloatAuto *x_auto;
	FloatAuto *y_auto;
	FloatAuto *z_auto;

	int create_x = event_caller == x;
	int create_y = event_caller == y;
	int create_z = event_caller == z;

	get_keyframes(x_auto, y_auto, z_auto, is_camera,
		create_x, create_y, create_z);

	if(create_x && x_auto)
		x_auto->set_value(atof(x->get_text()));
	else if(create_y && y_auto)
		y_auto->set_value(atof(y->get_text()));
	else if(create_z && z_auto)
	{
		double zoom = atof(z->get_text());
		double zmax = is_camera ? 10 : 10000;

		if(zoom > zmax)
			zoom = zmax;
		else
		if(zoom < 0)
			zoom = 0;

		z_auto->set_value(zoom);
	}
	update();
	update_preview();
	return 1;
}

void CWindowCamProjGUI::update()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;

	get_keyframes(x_auto, y_auto, z_auto, is_camera, 0, 0, 0);

	if(x_auto)
		x->update((int)round(x_auto->get_value()));
	if(y_auto)
		y->update((int)round(y_auto->get_value()));
	if(z_auto)
		z->update(z_auto->get_value());

	if(x_auto && y_auto && z_auto)
	{
		t_smooth->check_toggle_state(x_auto, y_auto, z_auto);
		t_linear->check_toggle_state(x_auto, y_auto, z_auto);
	}
}


CWindowCPLeft::CWindowCPLeft(CWindowCamProjGUI *gui, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("left_justify"))
{
	this->gui = gui;
	set_tooltip(_("Left justify"));
}

int CWindowCPLeft::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	int do_update = 0;
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
		mwindow_global->cwindow->calculate_affected_autos(&x_auto, 0, &z_auto,
			track, gui->is_camera, 1, 0, 0);

	if(x_auto && z_auto)
	{
		if(gui->is_camera)
		{
			int w = 0, h = 0;
			track->get_source_dimensions(
				master_edl->local_session->get_selectionstart(1),
				w, h);

			if(w && h)
			{
				x_auto->set_value((double)track->track_w /
					z_auto->get_value() / 2 - (double)w / 2);
				do_update = 1;
			}
		}
		else
		{
			x_auto->set_value((double)track->track_w * z_auto->get_value() / 2 -
				(double)edlsession->output_w / 2);
			do_update = 1;
		}

		if(do_update)
		{
			gui->update();
			gui->update_preview();
		}
	}
	return 1;
}


CWindowCPCenter::CWindowCPCenter(CWindowCamProjGUI *gui, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("center_justify"))
{
	this->gui = gui;
	set_tooltip(_("Center horizontal"));
}

int CWindowCPCenter::handle_event()
{
	FloatAuto *x_auto = 0;
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
		x_auto = (FloatAuto*)mwindow_global->cwindow->calculate_affected_auto(
			gui->is_camera ? AUTOMATION_CAMERA_X : AUTOMATION_PROJECTOR_X,
			master_edl->local_session->get_selectionstart(1),
			track, 1);

	if(x_auto)
	{
		x_auto->set_value(0);
		gui->update();
		gui->update_preview();
	}
	return 1;
}


CWindowCPRight::CWindowCPRight(CWindowCamProjGUI *gui, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("right_justify"))
{
	this->gui = gui;
	set_tooltip(_("Right justify"));
}

int CWindowCPRight::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	int do_update = 0;
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
		mwindow_global->cwindow->calculate_affected_autos(&x_auto, 0,
			&z_auto, track, gui->is_camera,
			1, 0, 0);

	if(x_auto && z_auto)
	{
		if(gui->is_camera)
		{
			int w = 0, h = 0;

			track->get_source_dimensions(
				master_edl->local_session->get_selectionstart(1),
				w, h);

			if(w && h)
			{
				x_auto->set_value(-((double)track->track_w /
					z_auto->get_value() / 2 - (double)w / 2));
				do_update = 1;
			}
		}
		else
		{
			x_auto->set_value(-((double)track->track_w * z_auto->get_value() / 2 -
				(double)edlsession->output_w / 2));
			do_update = 1;
		}

		if(do_update)
		{
			gui->update();
			gui->update_preview();
		}
	}
	return 1;
}


CWindowCPTop::CWindowCPTop(CWindowCamProjGUI *gui, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("top_justify"))
{
	this->gui = gui;
	set_tooltip(_("Top justify"));
}

int CWindowCPTop::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	int do_update = 0;
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
		mwindow_global->cwindow->calculate_affected_autos(0, &y_auto,
			&z_auto, track, gui->is_camera,
			0, 1, 0);

	if(y_auto && z_auto)
	{
		if(gui->is_camera)
		{
			int w = 0, h = 0;

			track->get_source_dimensions(
				master_edl->local_session->get_selectionstart(1),
				w, h);

			if(w && h)
			{
				y_auto->set_value((double)track->track_h /
					z_auto->get_value() / 2 - (double)h / 2);
				do_update = 1;
			}
		}
		else
		{
			y_auto->set_value((double)track->track_h * z_auto->get_value() / 2 -
				(double)edlsession->output_h / 2);
			do_update = 1;
		}

		if(do_update)
		{
			gui->update();
			gui->update_preview();
		}
	}
	return 1;
}


CWindowCPMiddle::CWindowCPMiddle(CWindowCamProjGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow_global->theme->get_image_set("middle_justify"))
{
	this->gui = gui;
	set_tooltip(_("Center vertical"));
}

int CWindowCPMiddle::handle_event()
{
	FloatAuto *y_auto = 0;
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
		y_auto = (FloatAuto*)mwindow_global->cwindow->calculate_affected_auto(
			gui->is_camera ? AUTOMATION_CAMERA_Y : AUTOMATION_PROJECTOR_Y,
			master_edl->local_session->get_selectionstart(1),
			track, 1);

	if(y_auto)
	{
		y_auto->set_value(0);
		gui->update();
		gui->update_preview();
	}
	return 1;
}


CWindowCPBottom::CWindowCPBottom(CWindowCamProjGUI *gui, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("bottom_justify"))
{
	this->gui = gui;
	set_tooltip(_("Bottom justify"));
}

int CWindowCPBottom::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	int do_update;

	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
		mwindow_global->cwindow->calculate_affected_autos(0, &y_auto, &z_auto,
			track, gui->is_camera, 0, 1, 0);

	if(y_auto && z_auto)
	{
		if(gui->is_camera)
		{
			int w = 0, h = 0;
			track->get_source_dimensions(
				master_edl->local_session->get_selectionstart(1),
				w,
				h);

			if(w && h)
			{
				y_auto->set_value(-((double)track->track_h /
					z_auto->get_value() / 2 - (double)h / 2));
				do_update = 1;
			}
		}
		else
		{
			y_auto->set_value(-((double)track->track_h * z_auto->get_value() / 2 -
				(double)edlsession->output_h / 2));
			do_update = 1;
		}

		if(do_update)
		{
			gui->update();
			gui->update_preview();
		}
	}
	return 1;
}


CWindowMaskMode::CWindowMaskMode(CWindowMaskGUI *gui, int x, int y)
 : BC_PopupMenu(x, y, 220, modenames[MASK_SUBTRACT_ALPHA].text,
	POPUPMENU_USE_COORDS)
{
	this->gui = gui;
	add_item(new BC_MenuItem(name(MASK_MULTIPLY_ALPHA)));
	add_item(new BC_MenuItem(name(MASK_SUBTRACT_ALPHA)));
	add_item(new BC_MenuItem(name(MASK_MULTIPLY_COLOR)));
	add_item(new BC_MenuItem(name(MASK_SUBTRACT_COLOR)));
}

const char* CWindowMaskMode::name(int mode)
{
	for(int i = 0; modenames[i].text; i++)
	{
		if(modenames[i].value == mode)
			return _(modenames[i].text);
	}
	return modenames[0].text;
}

int CWindowMaskMode::mode(const char *text)
{
	for(int i = 0; modenames[i].text; i++)
	{
		if(strcmp(_(modenames[i].text), text) == 0)
			return modenames[i].value;
	}
	return MASK_SUBTRACT_ALPHA;
}

int CWindowMaskMode::handle_event()
{
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
	{
		MaskAutos *mautos = (MaskAutos*)track->automation->have_autos(AUTOMATION_MASK);

		if(mautos)
		{
			mautos->set_mode(mode(get_text()));
			gui->update_preview();
			return 1;
		}
	}
	return 0;
}


CWindowMaskDelete::CWindowMaskDelete(CWindowToolGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->gui = gui;
}

int CWindowMaskDelete::handle_event()
{
	MaskAuto *keyframe;
	Track *track = mwindow_global->cwindow->calculate_affected_track();
	MaskPoint *point;
	SubMask *mask;

	if(track)
	{
		MaskAutos *mask_autos = (MaskAutos *)track->automation->have_autos(AUTOMATION_MASK);
		if(mask_autos)
		{
			for(MaskAuto *current = (MaskAuto*)mask_autos->first;
				current; current = (MaskAuto*)NEXT)
			{
				SubMask *submask = current->get_submask(edlsession->cwindow_mask);

				for(int i = gui->thread->cwindowgui->affected_point;
						i < submask->points.total - 1; i++)
					*submask->points.values[i] = *submask->points.values[i + 1];

				if(submask->points.total)
					submask->points.remove_object(
						submask->points.values[submask->points.total - 1]);
			}
			gui->update();
			gui->update_preview();
		}
	}
	return 1;
}

int CWindowMaskDelete::keypress_event()
{
	if(get_keypress() == BACKSPACE || get_keypress() == DELETE)
		return handle_event();
	return 0;
}


CWindowMaskNumber::CWindowMaskNumber(CWindowToolGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui, edlsession->cwindow_mask, 0,
	1, x, y, 100)
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

	this->gui = gui;
	((CWindowMaskGUI*)gui)->get_keyframe(track, keyframe, mask, point, 0);

	if(keyframe)
		set_boundaries(0, keyframe->masks.total);
}

int CWindowMaskNumber::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	int num;

	((CWindowMaskGUI*)gui)->get_keyframe(track, keyframe, mask, point, 1);
	if(keyframe)
	{
		num = atoi(get_text());
		if(keyframe->masks.total <= num)
		{
			if(keyframe->masks.values[keyframe->masks.total - 1]->points.total)
			{
				((MaskAutos*)keyframe->autos)->new_submask();
				set_boundaries(0, keyframe->masks.total);
			}
			num = keyframe->masks.total - 1;
		}
		edlsession->cwindow_mask = num;
		gui->update();
		gui->update_preview();
		return 1;
	}
	return 0;
}


CWindowMaskFeather::CWindowMaskFeather(CWindowToolGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui, 0, 0, 0xff, x, y, 100)
{
	this->gui = gui;
}

int CWindowMaskFeather::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

	((CWindowMaskGUI*)gui)->get_keyframe(track, keyframe, mask, point, 1);

	keyframe->feather = atoi(get_text());
	gui->update_preview();
	return 1;
}


CWindowMaskValue::CWindowMaskValue(CWindowToolGUI *gui, int x, int y)
 : BC_ISlider(x, y, 0, 200, 200, 0, 100, 0)
{
	this->gui = gui;
}

int CWindowMaskValue::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

	((CWindowMaskGUI*)gui)->get_keyframe(track, keyframe, mask, point, 1);

	keyframe->value = get_value();
	gui->update_preview();
	return 1;
}


CWindowMaskBeforePlugins::CWindowMaskBeforePlugins(CWindowToolGUI *gui, int x, int y)
 : BC_CheckBox(x, y, 1, _("Apply mask before plugins"))
{
	this->gui = gui;
}

int CWindowMaskBeforePlugins::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

	((CWindowMaskGUI*)gui)->get_keyframe(track, keyframe, mask, point, 1);

	if(keyframe)
	{
		keyframe->apply_before_plugins = get_value();
		gui->update_preview();
	}
	return 1;
}


CWindowMaskGUI::CWindowMaskGUI(CWindowTool *thread)
 : CWindowToolGUI(thread, MWindow::create_title(N_("Mask")), 330, 280)
{
	int x = 10, y = 10;
	BC_Title *title;
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	this->thread = thread;
	localauto = new MaskAuto(0, 0);

	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	add_subwindow(mode = new CWindowMaskMode(this,
		x + title->get_w(), y));
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Value:")));
	add_subwindow(value = new CWindowMaskValue(this, x + 80, y));
	y += 30;
	add_subwindow(delete_point = new CWindowMaskDelete(this, x, y));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Mask number:")));
	number = new CWindowMaskNumber(this, x + 110, y);
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Feather:")));
	feather = new CWindowMaskFeather(this, x + 110, y);
	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("X:")));
	x += title->get_w();
	this->x = new CWindowCoord(x, y, 0.0, this);
	x += 150;
	add_subwindow(title = new BC_Title(x, y, _("Y:")));
	x += title->get_w();
	this->y = new CWindowCoord(x, y, 0.0, this);

	y += 30;
	add_subwindow(this->apply_before_plugins = new CWindowMaskBeforePlugins(
		this, 10, y));

	update();
}

CWindowMaskGUI::~CWindowMaskGUI()
{
	delete number;
	delete feather;
	delete localauto;
}

void CWindowMaskGUI::allocate_autos()
{
	Track *track = mwindow_global->cwindow->calculate_affected_track();

	if(track && !track->automation->have_autos(AUTOMATION_MASK))
		track->automation->get_autos(AUTOMATION_MASK);
}

void CWindowMaskGUI::get_keyframe(Track* &track, MaskAuto* &keyframe,
	SubMask* &mask, MaskPoint* &point, int create_it)
{
	ptstime pos = master_edl->local_session->get_selectionstart(1);

	track = mwindow_global->cwindow->calculate_affected_track();

	if(track)
		keyframe = (MaskAuto*)mwindow_global->cwindow->calculate_affected_auto(
			AUTOMATION_MASK, pos, track, create_it);
	else
		keyframe = 0;

	if(edlsession->auto_keyframes && keyframe &&
			!PTSEQU(pos, keyframe->pos_time))
	{
		localauto->interpolate_from(keyframe, keyframe->next, master_edl->local_session->get_selectionstart(1), 0);
		keyframe = localauto;
	}

	if(keyframe)
		mask = keyframe->get_submask(edlsession->cwindow_mask);
	else
		mask = 0;

	point = 0;
	if(mask)
	{
		if(mwindow_global->cwindow->gui->affected_point < mask->points.total &&
				mwindow_global->cwindow->gui->affected_point >= 0)
			point = mask->points.values[mwindow_global->cwindow->gui->affected_point];
	}
}

void CWindowMaskGUI::update()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

	get_keyframe(track, keyframe, mask, point, 0);

	if(track)
	{
		MaskAutos *mautos = (MaskAutos*)track->automation->have_autos(AUTOMATION_MASK);
		if(mautos)
			mode->set_text(CWindowMaskMode::name(mautos->get_mode()));
	}
	if(!keyframe)
		return;

	if(point)
	{
		x->update(point->submask_x);
		y->update(point->submask_y);
	}

	if(mask)
	{
		feather->update(keyframe->feather);
		value->update(keyframe->value);
		apply_before_plugins->update(keyframe->apply_before_plugins);
	}

	number->update(edlsession->cwindow_mask);
}

int CWindowMaskGUI::handle_event()
{
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

	get_keyframe(track, keyframe, mask, point, 1);

	if(point)
	{
		point->submask_x = atof(x->get_text());
		point->submask_y = atof(y->get_text());
	}

	update_preview();
	return 1;
}

void CWindowMaskGUI::update_preview()
{
	mwindow_global->sync_parameters();
	thread->cwindowgui->canvas->draw_refresh();
}

CWindowRulerGUI::CWindowRulerGUI(CWindowTool *thread)
 : CWindowToolGUI(thread, MWindow::create_title(N_("Ruler")), 320, 240)
{
	int x = 10, y = 10;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x, y, "Current:"));
	add_subwindow(current = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 5;

	add_subwindow(title = new BC_Title(x, y, "Point 1:"));
	add_subwindow(point1 = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 5;

	add_subwindow(title = new BC_Title(x, y, "Point 2:"));
	add_subwindow(point2 = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 5;

	add_subwindow(title = new BC_Title(x, y, "Distance:"));
	add_subwindow(distance = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 5;
	add_subwindow(title = new BC_Title(x, y, "Angle:"));
	add_subwindow(angle = new BC_Title(x + title->get_w() + 10, y, ""));
	y += title->get_h() + 10;

	char string[BCTEXTLEN];
	sprintf(string, _("Press Ctrl to lock ruler to the\nnearest 45%c angle."), 0xb0);
	add_subwindow(title = new BC_Title(x,
		y,
		string));
	y += title->get_h() + 10;
	sprintf(string, _("Press Alt to translate the ruler."));
	add_subwindow(title = new BC_Title(x,
		y,
		string));
	update();
}

void CWindowRulerGUI::update()
{
	double distance_value =
		sqrt(SQR(edlsession->ruler_x2 - edlsession->ruler_x1) +
		SQR(edlsession->ruler_y2 - edlsession->ruler_y1));
	double angle_value = atan((edlsession->ruler_y2 - edlsession->ruler_y1) /
		(edlsession->ruler_x2 - edlsession->ruler_x1)) * 360 / 2 / M_PI;

	if(EQUIV(distance_value, 0.0))
		angle_value = 0.0;
	else if(angle_value < 0)
		angle_value *= -1;

	char string[BCTEXTLEN];

	sprintf(string, "%d, %d", mainsession->cwindow_output_x,
		mainsession->cwindow_output_y);
	current->update(string);

	sprintf(string, "%.0f, %.0f", edlsession->ruler_x1,
		edlsession->ruler_y1);
	point1->update(string);

	sprintf(string, "%.0f, %.0f", edlsession->ruler_x2,
		edlsession->ruler_y2);
	point2->update(string);

	sprintf(string, "%0.01f pixels", distance_value);
	distance->update(string);
	sprintf(string, "%0.02f %c", angle_value, 0xb0);
	angle->update(string);
}
