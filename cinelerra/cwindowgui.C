#include "automation.h"
#include "autos.h"
#include "bezierauto.h"
#include "bezierautos.h"
#include "canvas.h"
#include "clip.h"
#include "cpanel.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "cursors.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "cwindowtool.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mainsession.h"
#include "maskauto.h"
#include "maskautos.h"
#include "meterpanel.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "mwindow.h"
#include "playtransport.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "vtrack.h"




CWindowGUI::CWindowGUI(MWindow *mwindow, CWindow *cwindow)
 : BC_Window(PROGRAM_NAME ": Compositor",
 	mwindow->session->cwindow_x, 
    mwindow->session->cwindow_y, 
    mwindow->session->cwindow_w, 
    mwindow->session->cwindow_h,
    100,
    100,
    1,
    1,
    1,
	BLACK,
	mwindow->edl->session->get_cwindow_display())
{
	this->mwindow = mwindow;
    this->cwindow = cwindow;
	affected_track = 0;
	affected_auto = 0;
	affected_point = 0;
	x_offset = 0;
	y_offset = 0;
	x_origin = 0;
	y_origin = 0;
	current_operation = CWINDOW_NONE;
	tool_panel = 0;
	translating_zoom = 0;
}

CWindowGUI::~CWindowGUI()
{
	if(tool_panel) delete tool_panel;
 	delete meters;
 	delete composite_panel;
 	delete canvas;
 	delete transport;
 	delete edit_panel;
 	delete zoom_panel;
}

int CWindowGUI::create_objects()
{
	set_icon(mwindow->theme->cwindow_icon);

	mwindow->theme->get_cwindow_sizes(this);
	mwindow->theme->draw_cwindow_bg(this);
	flash();

// Meters required by composite panel
	meters = new CWindowMeters(mwindow, 
		this,
		mwindow->theme->cmeter_x,
		mwindow->theme->cmeter_y,
		mwindow->theme->cmeter_h);
	meters->create_objects();


	composite_panel = new CPanel(mwindow, 
		this, 
		mwindow->theme->ccomposite_x,
		mwindow->theme->ccomposite_y,
		mwindow->theme->ccomposite_w,
		mwindow->theme->ccomposite_h);
	composite_panel->create_objects();

	canvas = new CWindowCanvas(mwindow, this);
	canvas->create_objects(mwindow->edl);


	add_subwindow(timebar = new CTimeBar(mwindow,
		this,
		mwindow->theme->ctimebar_x,
		mwindow->theme->ctimebar_y,
		mwindow->theme->ctimebar_w, 
		mwindow->theme->ctimebar_h));
	timebar->create_objects();

	add_subwindow(slider = new CWindowSlider(mwindow, 
		cwindow, 
		mwindow->theme->cslider_x,
		mwindow->theme->cslider_y, 
		mwindow->theme->cslider_w));

	transport = new CWindowTransport(mwindow, 
		this, 
		mwindow->theme->ctransport_x, 
		mwindow->theme->ctransport_y);
	transport->create_objects();
	transport->set_slider(slider);

	edit_panel = new CWindowEditing(mwindow, cwindow);
	edit_panel->set_meters(meters);
	edit_panel->create_objects();

// 	add_subwindow(clock = new MainClock(mwindow, 
// 		mwindow->theme->ctime_x, 
// 		mwindow->theme->ctime_y));

	zoom_panel = new CWindowZoom(mwindow, 
		this, 
		mwindow->theme->czoom_x, 
		mwindow->theme->czoom_y);
	zoom_panel->create_objects();
	zoom_panel->zoom_text->add_item(new BC_MenuItem(AUTO_ZOOM));
	if(!mwindow->edl->session->cwindow_scrollbars) zoom_panel->set_text(AUTO_ZOOM);

// 	destination = new CWindowDestination(mwindow, 
// 		this, 
// 		mwindow->theme->cdest_x,
// 		mwindow->theme->cdest_y);
// 	destination->create_objects();

// Must create after meter panel
	tool_panel = new CWindowTool(mwindow, this);
	tool_panel->Thread::start();
	
	set_operation(mwindow->edl->session->cwindow_operation);
	canvas->draw_refresh();


	return 0;
}

int CWindowGUI::translation_event()
{
	mwindow->session->cwindow_x = get_x();
	mwindow->session->cwindow_y = get_y();
	return 0;
}

int CWindowGUI::resize_event(int w, int h)
{
	mwindow->session->cwindow_x = get_x();
	mwindow->session->cwindow_y = get_y();
	mwindow->session->cwindow_w = w;
	mwindow->session->cwindow_h = h;

//printf("CWindowGUI::resize_event 1\n");
	mwindow->theme->get_cwindow_sizes(this);
	mwindow->theme->draw_cwindow_bg(this);
	flash();

//printf("CWindowGUI::resize_event 1\n");
	composite_panel->reposition_buttons(mwindow->theme->ccomposite_x,
		mwindow->theme->ccomposite_y);
//printf("CWindowGUI::resize_event 1\n");

	canvas->reposition_window(mwindow->edl,
		mwindow->theme->ccanvas_x,
		mwindow->theme->ccanvas_y,
		mwindow->theme->ccanvas_w,
		mwindow->theme->ccanvas_h);

	timebar->resize_event();

	slider->reposition_window(mwindow->theme->cslider_x,
		mwindow->theme->cslider_y, 
		mwindow->theme->cslider_w);
// Recalibrate pointer motion range
	slider->set_position();
//printf("CWindowGUI::resize_event 1\n");

	transport->reposition_buttons(mwindow->theme->ctransport_x, 
		mwindow->theme->ctransport_y);
//printf("CWindowGUI::resize_event 1\n");

	edit_panel->reposition_buttons(mwindow->theme->cedit_x, 
		mwindow->theme->cedit_y);
//printf("CWindowGUI::resize_event 1\n");

//	clock->reposition_window(mwindow->theme->ctime_x, 
//		mwindow->theme->ctime_y);
//printf("CWindowGUI::resize_event 1\n");

	zoom_panel->reposition_window(mwindow->theme->czoom_x, 
		mwindow->theme->czoom_y);
//printf("CWindowGUI::resize_event 1\n");

// 	destination->reposition_window(mwindow->theme->cdest_x,
// 		mwindow->theme->cdest_y);
//printf("CWindowGUI::resize_event 1\n");

	meters->reposition_window(mwindow->theme->cmeter_x,
		mwindow->theme->cmeter_y,
		mwindow->theme->cmeter_h);
//printf("CWindowGUI::resize_event 2\n");

	BC_WindowBase::resize_event(w, h);
	return 1;
}






// TODO
// Don't refresh the canvas in a load file operation which is going to
// refresh it anyway.
void CWindowGUI::set_operation(int value)
{
//printf("CWindowGUI::set_operation 1 %d\n", value);
	mwindow->edl->session->cwindow_operation = value;
//printf("CWindowGUI::set_operation 1 %d\n", value);

	composite_panel->set_operation(value);
//printf("CWindowGUI::set_operation 1 %d\n", value);
	edit_panel->update();

	tool_panel->start_tool(value);
	canvas->draw_refresh();
//printf("CWindowGUI::set_operation 2 %d\n", value);
}

void CWindowGUI::update_tool()
{
//printf("CWindowGUI::update_tool 1\n");
	tool_panel->update_values();
}

int CWindowGUI::close_event()
{
	hide_window();
	mwindow->session->show_cwindow = 0;
	mwindow->gui->lock_window("CWindowGUI::close_event");
	mwindow->gui->mainmenu->show_cwindow->set_checked(0);
	mwindow->gui->unlock_window();
	mwindow->save_defaults();
	return 1;
}


int CWindowGUI::keypress_event()
{
	int result = 0;

//printf("CWindowGUI::keypress_event 1\n");
	switch(get_keypress())
	{
		case 'w':
		case 'W':
			close_event();
			result = 1;
			break;
	}

//printf("CWindowGUI::keypress_event 1\n");
	if(!result) result = transport->keypress_event();

//printf("CWindowGUI::keypress_event 2\n");
	return result;
}

void CWindowGUI::drag_motion()
{
	if(get_hidden()) return;

	if(mwindow->session->current_operation == DRAG_ASSET ||
		mwindow->session->current_operation == DRAG_VTRANSITION ||
		mwindow->session->current_operation == DRAG_VEFFECT)
	{
		int old_status = mwindow->session->ccanvas_highlighted;
		int cursor_x = get_relative_cursor_x();
		int cursor_y = get_relative_cursor_y();

		mwindow->session->ccanvas_highlighted = get_cursor_over_window() &&
			cursor_x >= canvas->x &&
			cursor_x < canvas->x + canvas->w &&
			cursor_y >= canvas->y &&
			cursor_y < canvas->y + canvas->h;


		if(old_status != mwindow->session->ccanvas_highlighted)
			canvas->draw_refresh();
	}
}

int CWindowGUI::drag_stop()
{
	int result = 0;
	if(get_hidden()) return 0;

	if((mwindow->session->current_operation == DRAG_ASSET ||
		mwindow->session->current_operation == DRAG_VTRANSITION ||
		mwindow->session->current_operation == DRAG_VEFFECT) &&
		mwindow->session->ccanvas_highlighted)
	{
// Hide highlighting
		mwindow->session->ccanvas_highlighted = 0;
		canvas->draw_refresh();
		result = 1;
	}
	else
		return 0;

	if(mwindow->session->current_operation == DRAG_ASSET)
	{
		if(mwindow->session->drag_assets->total)
		{
			mwindow->gui->lock_window("CWindowGUI::drag_stop 1");
			mwindow->undo->update_undo_before(_("insert assets"), 
				LOAD_ALL);
			mwindow->clear(0);
			mwindow->load_assets(mwindow->session->drag_assets, 
				mwindow->edl->local_session->get_selectionstart(), 
				LOAD_PASTE,
				mwindow->session->track_highlighted,
				0,
				mwindow->edl->session->labels_follow_edits, 
				mwindow->edl->session->plugins_follow_edits);
		}

		if(mwindow->session->drag_clips->total)
		{
			mwindow->gui->lock_window("CWindowGUI::drag_stop 2");
			mwindow->undo->update_undo_before(_("insert assets"), 
				LOAD_ALL);
			mwindow->clear(0);
			mwindow->paste_edls(mwindow->session->drag_clips, 
				LOAD_PASTE, 
				mwindow->session->track_highlighted,
				mwindow->edl->local_session->get_selectionstart(),
				mwindow->edl->session->labels_follow_edits, 
				mwindow->edl->session->plugins_follow_edits);
		}

		if(mwindow->session->drag_assets->total ||
			mwindow->session->drag_clips->total)
		{
			mwindow->save_backup();
			mwindow->restart_brender();
			mwindow->gui->update(1, 1, 1, 1, 0, 1, 0);
			mwindow->undo->update_undo_after();
			mwindow->gui->unlock_window();
			mwindow->sync_parameters(LOAD_ALL);
		}
	}

	if(mwindow->session->current_operation == DRAG_VEFFECT)
	{
//printf("CWindowGUI::drag_stop 1\n");
		Track *affected_track = cwindow->calculate_affected_track();
//printf("CWindowGUI::drag_stop 2\n");

		mwindow->gui->lock_window("CWindowGUI::drag_stop 3");
		mwindow->insert_effects_cwindow(affected_track);
		mwindow->session->current_operation = NO_OPERATION;
		mwindow->gui->unlock_window();
	}

	if(mwindow->session->current_operation == DRAG_VTRANSITION)
	{
		Track *affected_track = cwindow->calculate_affected_track();
		mwindow->gui->lock_window("CWindowGUI::drag_stop 4");
		mwindow->paste_transition_cwindow(affected_track);
		mwindow->session->current_operation = NO_OPERATION;
		mwindow->gui->unlock_window();
	}

	return result;
}


CWindowEditing::CWindowEditing(MWindow *mwindow, CWindow *cwindow)
 : EditPanel(mwindow, 
		cwindow->gui, 
		mwindow->theme->cedit_x, 
		mwindow->theme->cedit_y,
		mwindow->edl->session->editing_mode, 
		0,
		1,
		0, 
		0,
		1,
		1,
		1,
		1,
		1,
		0,
		1,
		1,
		1,
		0,
		1)
{
	this->mwindow = mwindow;
	this->cwindow = cwindow;
}

void CWindowEditing::set_inpoint()
{
	mwindow->set_inpoint(0);
}

void CWindowEditing::set_outpoint()
{
	mwindow->set_outpoint(0);
}





CWindowMeters::CWindowMeters(MWindow *mwindow, CWindowGUI *gui, int x, int y, int h)
 : MeterPanel(mwindow, 
		gui,
		x,
		y,
		h,
		mwindow->edl->session->audio_channels,
		mwindow->edl->session->cwindow_meter)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMeters::~CWindowMeters()
{
}

int CWindowMeters::change_status_event()
{
//printf("CWindowMeters::change_status_event 1 %d\n", gui->meters->use_meters);
	mwindow->edl->session->cwindow_meter = use_meters;
	mwindow->theme->get_cwindow_sizes(gui);
	gui->resize_event(gui->get_w(), gui->get_h());
	return 1;
}



#define MIN_ZOOM 0.25
#define MAX_ZOOM 4.0

CWindowZoom::CWindowZoom(MWindow *mwindow, CWindowGUI *gui, int x, int y)
 : ZoomPanel(mwindow, 
 	gui, 
	(long)mwindow->edl->session->cwindow_zoom, 
	x, 
	y,
	100, 
	MIN_ZOOM, 
	MAX_ZOOM, 
	ZOOM_PERCENTAGE)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowZoom::~CWindowZoom()
{
}

int CWindowZoom::handle_event()
{
	if(!strcasecmp(AUTO_ZOOM, get_text()))
	{
		mwindow->edl->session->cwindow_scrollbars = 0;
	}
	else
	{
		mwindow->edl->session->cwindow_scrollbars = 1;
	}

//printf("CWindowZoom::handle_event 1 %d %d\n", gui->canvas->get_xscroll(), gui->canvas->get_yscroll());
	gui->canvas->update_zoom(gui->canvas->get_xscroll(), 
		gui->canvas->get_yscroll(), 
		get_value());
//printf("CWindowZoom::handle_event 2 %d %d\n", gui->canvas->get_xscroll(), gui->canvas->get_yscroll());
	gui->canvas->reposition_window(mwindow->edl, 
		mwindow->theme->ccanvas_x,
		mwindow->theme->ccanvas_y,
		mwindow->theme->ccanvas_w,
		mwindow->theme->ccanvas_h);
//printf("CWindowZoom::handle_event 3 %d %d\n", gui->canvas->get_xscroll(), gui->canvas->get_yscroll());
	gui->canvas->draw_refresh();
//printf("CWindowZoom::handle_event 4 %d %d\n", gui->canvas->get_xscroll(), gui->canvas->get_yscroll());
	return 1;
}



CWindowSlider::CWindowSlider(MWindow *mwindow, CWindow *cwindow, int x, int y, int pixels)
 : BC_PercentageSlider(x, 
			y,
			0,
			pixels, 
			pixels, 
			0, 
			1, 
			0)
{
	this->mwindow = mwindow;
	this->cwindow = cwindow;
}

CWindowSlider::~CWindowSlider()
{
}

int CWindowSlider::handle_event()
{
	unlock_window();
	cwindow->playback_engine->interrupt_playback(1);
	lock_window("CWindowSlider::handle_event 1");
	
	mwindow->gui->lock_window("CWindowSlider::handle_event 2");
	mwindow->select_point((double)get_value());
	mwindow->gui->unlock_window();
	return 1;
}

void CWindowSlider::set_position()
{
	double new_length = mwindow->edl->tracks->total_playable_length();
	if(mwindow->edl->local_session->preview_end <= 0 ||
		mwindow->edl->local_session->preview_end > new_length)
		mwindow->edl->local_session->preview_end = new_length;
	if(mwindow->edl->local_session->preview_start > 
		mwindow->edl->local_session->preview_end)
		mwindow->edl->local_session->preview_start = 0;



	update(mwindow->theme->cslider_w, 
		mwindow->edl->local_session->selectionstart, 
		mwindow->edl->local_session->preview_start, 
		mwindow->edl->local_session->preview_end);
}


int CWindowSlider::increase_value()
{
	unlock_window();
	cwindow->gui->transport->handle_transport(SINGLE_FRAME_FWD);
	lock_window("CWindowSlider::increase_value");
	return 1;
}

int CWindowSlider::decrease_value()
{
	unlock_window();
	cwindow->gui->transport->handle_transport(SINGLE_FRAME_REWIND);
	lock_window("CWindowSlider::decrease_value");
	return 1;
}


// CWindowDestination::CWindowDestination(MWindow *mwindow, CWindowGUI *cwindow, int x, int y)
//  : BC_PopupTextBox(cwindow, 
//  	&cwindow->destinations, 
// 	cwindow->destinations.values[cwindow->cwindow->destination]->get_text(),
// 	x, 
// 	y, 
// 	70, 
// 	200)
// {
// 	this->mwindow = mwindow;
// 	this->cwindow = cwindow;
// }
// 
// CWindowDestination::~CWindowDestination()
// {
// }
// 
// int CWindowDestination::handle_event()
// {
// 	return 1;
// }


CWindowTransport::CWindowTransport(MWindow *mwindow, 
	CWindowGUI *gui, 
	int x, 
	int y)
 : PlayTransport(mwindow, 
	gui, 
	x, 
	y)
{
	this->gui = gui;
}

EDL* CWindowTransport::get_edl()
{
	return mwindow->edl;
}

void CWindowTransport::goto_start()
{
	gui->unlock_window();
	handle_transport(REWIND, 1);

	mwindow->gui->lock_window("CWindowTransport::goto_start 1");
	mwindow->goto_start();
	mwindow->gui->unlock_window();

	gui->lock_window("CWindowTransport::goto_start 2");
}

void CWindowTransport::goto_end()
{
	gui->unlock_window();
	handle_transport(GOTO_END, 1);

	mwindow->gui->lock_window("CWindowTransport::goto_end 1");
	mwindow->goto_end();
	mwindow->gui->unlock_window();

	gui->lock_window("CWindowTransport::goto_end 2");
}



CWindowCanvas::CWindowCanvas(MWindow *mwindow, CWindowGUI *gui)
 : Canvas(gui,
		mwindow->theme->ccanvas_x,
		mwindow->theme->ccanvas_y,
		mwindow->theme->ccanvas_w,
		mwindow->theme->ccanvas_h,
		0,
		0,
		mwindow->edl->session->cwindow_scrollbars,
		1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void CWindowCanvas::update_zoom(int x, int y, float zoom)
{
	use_scrollbars = mwindow->edl->session->cwindow_scrollbars;

	mwindow->edl->session->cwindow_xscroll = x;
	mwindow->edl->session->cwindow_yscroll = y;
	mwindow->edl->session->cwindow_zoom = zoom;
}

int CWindowCanvas::get_xscroll()
{
	return mwindow->edl->session->cwindow_xscroll;
}

int CWindowCanvas::get_yscroll()
{
	return mwindow->edl->session->cwindow_yscroll;
}


float CWindowCanvas::get_zoom()
{
	return mwindow->edl->session->cwindow_zoom;
}

void CWindowCanvas::draw_refresh()
{
	if(!canvas->video_is_on())
	{
//printf("CWindowCanvas::draw_refresh 1 %f\n", mwindow->edl->session->cwindow_zoom);
		canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
//printf("CWindowCanvas::draw_refresh 2\n");

		if(refresh_frame)
		{
			int in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h;
//printf("CWindowCanvas::draw_refresh 3\n");
			get_transfers(mwindow->edl, 
				in_x, 
				in_y, 
				in_w, 
				in_h, 
				out_x, 
				out_y, 
				out_w, 
				out_h);


// printf("CWindowCanvas::draw_refresh %d %d %d %d -> %d %d %d %d\n", in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h);
// 			canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());


//printf("CWindowCanvas::draw_refresh 5\n");
			if(out_w > 0 && out_h > 0 && in_w > 0 && in_h > 0)
				canvas->draw_vframe(refresh_frame,
						out_x, 
						out_y, 
						out_w, 
						out_h,
						in_x, 
						in_y, 
						in_w, 
						in_h,
						0);
//printf("CWindowCanvas::draw_refresh 6\n");
		}
		else
		{
//printf("CWindowCanvas::draw_refresh 7\n");
			canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
//printf("CWindowCanvas::draw_refresh 8\n");
		}

//printf("CWindowCanvas::draw_refresh 9\n");
		draw_overlays();
		canvas->flash();
		canvas->flush();
//printf("CWindowCanvas::draw_refresh 10\n");
	}
}

#define CROPHANDLE_W 10
#define CROPHANDLE_H 10

void CWindowCanvas::draw_crophandle(int x, int y)
{
	canvas->draw_box(x, y, CROPHANDLE_W, CROPHANDLE_H);
}

#define CONTROL_W 10
#define CONTROL_H 10
#define FIRST_CONTROL_W 20
#define FIRST_CONTROL_H 20
#undef BC_INFINITY
#define BC_INFINITY 65536
#ifndef SQR
#define SQR(x) ((x) * (x))
#endif

int CWindowCanvas::do_mask(int &redraw, 
		int &rerender, 
		int button_press, 
		int cursor_motion,
		int draw)
{
// Retrieve points from top recordable track
//printf("CWindowCanvas::do_mask 1\n");
	Track *track = gui->cwindow->calculate_affected_track();
//printf("CWindowCanvas::do_mask 2\n");

	if(!track) return 0;
//printf("CWindowCanvas::do_mask 3\n");

	MaskAutos *mask_autos = track->automation->mask_autos;
	long position = track->to_units(mwindow->edl->local_session->selectionstart,
		0);
	ArrayList<MaskPoint*> points;
	mask_autos->get_points(&points, mwindow->edl->session->cwindow_mask,
		position, 
		PLAY_FORWARD);
//printf("CWindowCanvas::do_mask 4\n");

// Translate mask to projection
	BezierAutos *projector_autos = track->automation->projector_autos;
	FloatAutos *projector_zooms = track->automation->pzoom_autos;
	BezierAuto *before = 0, *after = 0;
	FloatAuto *zoom_before = 0, *zoom_after = 0;
	float projector_x, projector_y, projector_z;
// Projector zooms relative to the center of the track output.
	float half_track_w = (float)track->track_w / 2;
	float half_track_h = (float)track->track_h / 2;
	projector_autos->get_center(projector_x, 
			projector_y, 
			projector_z, 
			position, 
			PLAY_FORWARD, 
			&before, 
			&after);
	projector_z = projector_zooms->get_value(position,
		PLAY_FORWARD,
		zoom_before,
		zoom_after);
// printf("CWindowCanvas::do_mask 1 %f %f %f\n", projector_x, 
// 			projector_y, 
// 			projector_z);


// Get position of cursor relative to mask
	float mask_cursor_x = get_cursor_x();
	float mask_cursor_y = get_cursor_y();
	canvas_to_output(mwindow->edl, 0, mask_cursor_x, mask_cursor_y);

	mask_cursor_x -= projector_x;
	mask_cursor_y -= projector_y;
	mask_cursor_x = half_track_w + (mask_cursor_x - half_track_w) / projector_z;
	mask_cursor_y = half_track_h + (mask_cursor_y - half_track_h) / projector_z;

// Fix cursor origin
	if(button_press)
	{
		gui->x_origin = mask_cursor_x;
		gui->y_origin = mask_cursor_y;
	}

	int result = 0;
// Points of closest line
	int shortest_point1 = -1;
	int shortest_point2 = -1;
// Closest point
	int shortest_point = -1;
// Distance to closest line
	float shortest_line_distance = BC_INFINITY;
// Distance to closest point
	float shortest_point_distance = BC_INFINITY;
	int selected_point = -1;
	int selected_control_point = -1;
	float selected_control_point_distance = BC_INFINITY;
	ArrayList<int> x_points;
	ArrayList<int> y_points;

	if(!cursor_motion)
	{
		if(draw)
		{
			canvas->set_color(WHITE);
			canvas->set_inverse();
		}
//printf("CWindowCanvas::do_mask 1 %d\n", points.total);

// Never draw closed polygon and a closed
// polygon is harder to add points to.
		for(int i = 0; i < points.total && !result; i++)
		{
			MaskPoint *point1 = points.values[i];
			MaskPoint *point2 = (i >= points.total - 1) ? 
				points.values[0] : 
				points.values[i + 1];
			float x0, x1, x2, x3;
			float y0, y1, y2, y3;
			float old_x, old_y, x, y;
			int segments = (int)(sqrt(SQR(point1->x - point2->x) + SQR(point1->y - point2->y)));

//printf("CWindowCanvas::do_mask 1 %f, %f -> %f, %f projectorz=%f\n",
//point1->x, point1->y, point2->x, point2->y, projector_z);
			for(int j = 0; j <= segments && !result; j++)
			{
//printf("CWindowCanvas::do_mask 1 %f, %f -> %f, %f\n", x0, y0, x3, y3);
				x0 = point1->x;
				y0 = point1->y;
				x1 = point1->x + point1->control_x2;
				y1 = point1->y + point1->control_y2;
				x2 = point2->x + point2->control_x1;
				y2 = point2->y + point2->control_y1;
				x3 = point2->x;
				y3 = point2->y;

				float t = (float)j / segments;
				float tpow2 = t * t;
				float tpow3 = t * t * t;
				float invt = 1 - t;
				float invtpow2 = invt * invt;
				float invtpow3 = invt * invt * invt;

				x = (        invtpow3 * x0
					+ 3 * t     * invtpow2 * x1
					+ 3 * tpow2 * invt     * x2 
					+     tpow3            * x3);
				y = (        invtpow3 * y0 
					+ 3 * t     * invtpow2 * y1
					+ 3 * tpow2 * invt     * y2 
					+     tpow3            * y3);

				x = half_track_w + (x - half_track_w) * projector_z + projector_x;
				y = half_track_h + (y - half_track_h) * projector_z + projector_y;


// Test new point addition
				if(button_press)
				{
					float line_distance = 
						sqrt(SQR(x - mask_cursor_x) + SQR(y - mask_cursor_y));

//printf("CWindowCanvas::do_mask 1 x=%f mask_cursor_x=%f y=%f mask_cursor_y=%f %f %f %d, %d\n", 
//x, mask_cursor_x, y, mask_cursor_y, line_distance, shortest_line_distance, shortest_point1, shortest_point2);
					if(line_distance < shortest_line_distance || 
						shortest_point1 < 0)
					{
						shortest_line_distance = line_distance;
						shortest_point1 = i;
						shortest_point2 = (i >= points.total - 1) ? 0 : (i + 1);
//printf("CWindowCanvas::do_mask 2 %f %f %d, %d\n", line_distance, shortest_line_distance, shortest_point1, shortest_point2);
					}


					float point_distance1 = 
						sqrt(SQR(point1->x - mask_cursor_x) + SQR(point1->y - mask_cursor_y));
					float point_distance2 = 
						sqrt(SQR(point2->x - mask_cursor_x) + SQR(point2->y - mask_cursor_y));

					if(point_distance1 < shortest_point_distance || 
						shortest_point < 0)
					{
						shortest_point_distance = point_distance1;
						shortest_point = i;
					}

					if(point_distance2 < shortest_point_distance || 
						shortest_point < 0)
					{
						shortest_point_distance = point_distance2;
						shortest_point = (i >= points.total - 1) ? 0 : (i + 1);
					}
				}

				output_to_canvas(mwindow->edl, 0, x, y);


#define TEST_BOX(cursor_x, cursor_y, target_x, target_y) \
	(cursor_x >= target_x - CONTROL_W / 2 && \
	cursor_x < target_x + CONTROL_W / 2 && \
	cursor_y >= target_y - CONTROL_H / 2 && \
	cursor_y < target_y + CONTROL_H / 2)

// Test existing point selection
				if(button_press)
				{
					float canvas_x = half_track_w + (x0 - half_track_w) * projector_z + projector_x;
					float canvas_y = half_track_h + (y0 - half_track_h) * projector_z + projector_y;
					int cursor_x = get_cursor_x();
					int cursor_y = get_cursor_y();

// Test first point
					if(gui->shift_down())
					{
						float control_x = half_track_w + (x1 - half_track_w) * projector_z + projector_x;
						float control_y = half_track_h + (y1 - half_track_h) * projector_z + projector_y;
						output_to_canvas(mwindow->edl, 0, control_x, control_y);

						float distance = 
							sqrt(SQR(control_x - cursor_x) + SQR(control_y - cursor_y));

						if(distance < selected_control_point_distance)
						{
							selected_point = i;
							selected_control_point = 1;
							selected_control_point_distance = distance;
						}
					}
					else
					{
						output_to_canvas(mwindow->edl, 0, canvas_x, canvas_y);
						if(!gui->ctrl_down())
						{
							if(TEST_BOX(cursor_x, cursor_y, canvas_x, canvas_y))
							{
								selected_point = i;
							}
						}
						else
						{
							selected_point = shortest_point;
						}
					}

// Test second point
					canvas_x = half_track_w + (x3 - half_track_w) * projector_z + projector_x;
					canvas_y = half_track_h + (y3 - half_track_h) * projector_z + projector_y;
					if(gui->shift_down())
					{
						float control_x = half_track_w + (x2 - half_track_w) * projector_z + projector_x;
						float control_y = half_track_h + (y2 - half_track_h) * projector_z + projector_y;
						output_to_canvas(mwindow->edl, 0, control_x, control_y);

						float distance = 
							sqrt(SQR(control_x - cursor_x) + SQR(control_y - cursor_y));

//printf("CWindowCanvas::do_mask %d %f %f\n", i, distance, selected_control_point_distance);
						if(distance < selected_control_point_distance)
						{
							selected_point = (i < points.total - 1 ? i + 1 : 0);
							selected_control_point = 0;
							selected_control_point_distance = distance;
						}
					}
					else
					if(i < points.total - 1)
					{
						output_to_canvas(mwindow->edl, 0, canvas_x, canvas_y);
						if(!gui->ctrl_down())
						{
							if(TEST_BOX(cursor_x, cursor_y, canvas_x, canvas_y))
							{
								selected_point = (i < points.total - 1 ? i + 1 : 0);
							}
						}
						else
						{
							selected_point = shortest_point;
						}
					}
				}



				if(j > 0)
				{
// Draw joining line
					if(draw)
					{
						x_points.append((int)x);
						y_points.append((int)y);
					}

					if(j == segments)
					{




						if(draw)
						{
// Draw second anchor
							if(i < points.total - 1)
							{
								if(i == gui->affected_point - 1)
									canvas->draw_disc((int)x - CONTROL_W / 2, 
										(int)y - CONTROL_W / 2, 
										CONTROL_W, 
										CONTROL_W);
								else
									canvas->draw_circle((int)x - CONTROL_W / 2, 
										(int)y - CONTROL_W / 2, 
										CONTROL_W, 
										CONTROL_W);
// char string[BCTEXTLEN];
// sprintf(string, "%d", (i < points.total - 1 ? i + 1 : 0));
// canvas->draw_text((int)x + CONTROL_W, (int)y + CONTROL_W, string);
							}

// Draw second control point.  Discard x2 and y2 after this.
							x2 = half_track_w + (x2 - half_track_w) * projector_z + projector_x;
							y2 = half_track_h + (y2 - half_track_h) * projector_z + projector_y;
							output_to_canvas(mwindow->edl, 0, x2, y2);
							canvas->draw_line((int)x, (int)y, (int)x2, (int)y2);
							canvas->draw_rectangle((int)x2 - CONTROL_W / 2,
								(int)y2 - CONTROL_H / 2,
								CONTROL_W,
								CONTROL_H);
						}
					}
				}
				else
				{


// Draw first anchor
					if(i == 0 && draw)
					{
						canvas->draw_disc((int)x - FIRST_CONTROL_W / 2, 
							(int)y - FIRST_CONTROL_H / 2, 
							FIRST_CONTROL_W, 
							FIRST_CONTROL_H);
					}

// Draw first control point.  Discard x1 and y1 after this.
					if(draw)
					{
						x1 = half_track_w + (x1 - half_track_w) * projector_z + projector_x;
						y1 = half_track_h + (y1 - half_track_h) * projector_z + projector_y;
						output_to_canvas(mwindow->edl, 0, x1, y1);
						canvas->draw_line((int)x, (int)y, (int)x1, (int)y1);
						canvas->draw_rectangle((int)x1 - CONTROL_W / 2,
							(int)y1 - CONTROL_H / 2,
							CONTROL_W,
							CONTROL_H);
					
						x_points.append((int)x);
						y_points.append((int)y);
					}
				}
//printf("CWindowCanvas::do_mask 1\n");

				old_x = x;
				old_y = y;
			}
		}
//printf("CWindowCanvas::do_mask 1\n");

		if(draw)
		{
			canvas->draw_polygon(&x_points, &y_points);
			canvas->set_opaque();
		}
//printf("CWindowCanvas::do_mask 1\n");
	}







	if(button_press && !result)
	{
//printf("CWindowCanvas::do_mask 5\n");
		gui->affected_track = gui->cwindow->calculate_affected_track();
// Get current keyframe
		if(gui->affected_track)
			gui->affected_auto = 
				gui->cwindow->calculate_affected_auto(gui->affected_track->automation->mask_autos);

		MaskAuto *keyframe = (MaskAuto*)gui->affected_auto;
		SubMask *mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);

//printf("CWindowCanvas::do_mask 6 %d %d\n", gui->alt_down(), mask->points.total);

// Translate entire keyframe
		if(gui->alt_down() && mask->points.total)
		{
			gui->current_operation = CWINDOW_MASK_TRANSLATE;
			gui->affected_point = 0;
		}
		else
// Existing point or control point was selected
		if(selected_point >= 0)
		{
			gui->affected_point = selected_point;

			if(selected_control_point == 0)
				gui->current_operation = CWINDOW_MASK_CONTROL_IN;
			else
			if(selected_control_point == 1)
				gui->current_operation = CWINDOW_MASK_CONTROL_OUT;
			else
				gui->current_operation = mwindow->edl->session->cwindow_operation;
		}
		else
// No existing point or control point was selected so create a new one
		if(!gui->shift_down() && !gui->alt_down())
		{
// Create the template
			MaskPoint *point = new MaskPoint;
			point->x = mask_cursor_x;
			point->y = mask_cursor_y;
			point->control_x1 = 0;
			point->control_y1 = 0;
			point->control_x2 = 0;
			point->control_y2 = 0;

			mwindow->undo->update_undo_before(_("mask point"), LOAD_AUTOMATION);

			if(shortest_point2 < shortest_point1)
			{
				shortest_point2 ^= shortest_point1;
				shortest_point1 ^= shortest_point2;
				shortest_point2 ^= shortest_point1;
			}






//printf("CWindowCanvas::do_mask 1 %f %f %d %d\n", 
//	shortest_line_distance, shortest_point_distance, shortest_point1, shortest_point2);
//printf("CWindowCanvas::do_mask %d %d\n", shortest_point1, shortest_point2);

// Append to end of list
			if(labs(shortest_point1 - shortest_point2) > 1)
			{
// Need to apply the new point to every keyframe
				for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto;
					current; )
				{
					SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
					MaskPoint *new_point = new MaskPoint;
					submask->points.append(new_point);
					*new_point = *point;
					if(current == (MaskAuto*)mask_autos->default_auto)
						current = (MaskAuto*)mask_autos->first;
					else
						current = (MaskAuto*)NEXT;
				}

				gui->affected_point = mask->points.total - 1;
				result = 1;
			}
			else
// Insert between 2 points, shifting back point 2
			if(shortest_point1 >= 0 && shortest_point2 >= 0)
			{
				for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto;
					current; )
				{
					SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
					MaskPoint *new_point = new MaskPoint;
					submask->points.append(0);
					for(int i = submask->points.total - 1; 
						i > shortest_point2; 
						i--)
						submask->points.values[i] = submask->points.values[i - 1];
					submask->points.values[shortest_point2] = new_point;

					*new_point = *point;
					if(current == (MaskAuto*)mask_autos->default_auto)
						current = (MaskAuto*)mask_autos->first;
					else
						current = (MaskAuto*)NEXT;
				}


				gui->affected_point = shortest_point2;
				result = 1;
			}
//printf("CWindowCanvas::do_mask 3 %d\n", mask->points.total);






// Create the first point.
			if(!result)
			{
//printf("CWindowCanvas::do_mask 1\n");
				for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto;
					current; )
				{
					SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
					MaskPoint *new_point = new MaskPoint;
					submask->points.append(new_point);
					*new_point = *point;
					if(current == (MaskAuto*)mask_autos->default_auto)
						current = (MaskAuto*)mask_autos->first;
					else
						current = (MaskAuto*)NEXT;
				}

//printf("CWindowCanvas::do_mask 2\n");
// Create a second point if none existed before
				if(mask->points.total < 2)
				{
					for(MaskAuto *current = (MaskAuto*)mask_autos->default_auto;
						current; )
					{
						SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
						MaskPoint *new_point = new MaskPoint;
						submask->points.append(new_point);
						*new_point = *point;
						if(current == (MaskAuto*)mask_autos->default_auto)
							current = (MaskAuto*)mask_autos->first;
						else
							current = (MaskAuto*)NEXT;
					}
				}
				gui->affected_point = mask->points.total - 1;
//printf("CWindowCanvas::do_mask 3 %d\n", mask->points.total);
			}



//printf("CWindowCanvas::do_mask 2 %d %d %f %f\n", get_cursor_x(), get_cursor_y(), gui->affected_point->x, gui->affected_point->y);
			gui->current_operation = mwindow->edl->session->cwindow_operation;
// Delete the template
			delete point;
			mwindow->undo->update_undo_after();
//printf("CWindowCanvas::do_mask 4\n");
		}

		result = 1;
		redraw = 1;
	}
//printf("CWindowCanvas::do_mask 7\n");

	if(button_press && result)
	{
		MaskAuto *keyframe = (MaskAuto*)gui->affected_auto;
		SubMask *mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);
		MaskPoint *point = mask->points.values[gui->affected_point];
		gui->center_x = point->x;
		gui->center_y = point->y;
		gui->control_in_x = point->control_x1;
		gui->control_in_y = point->control_y1;
		gui->control_out_x = point->control_x2;
		gui->control_out_y = point->control_y2;
// printf("CWindowCanvas::do_mask 1 %p %p %d\n", 
// 	gui->affected_auto, 
// 	gui->affected_point,
// 	gui->current_operation);
	}

//printf("CWindowCanvas::do_mask 8\n");
	if(cursor_motion)
	{
		MaskAuto *keyframe = (MaskAuto*)gui->affected_auto;
		SubMask *mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);
		if(gui->affected_point < mask->points.total)
		{
			MaskPoint *point = mask->points.values[gui->affected_point];
// 			float cursor_x = get_cursor_x();
// 			float cursor_y = get_cursor_y();
// 			canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
			float cursor_x = mask_cursor_x;
			float cursor_y = mask_cursor_y;
//printf("CWindowCanvas::do_mask 9 %d %d\n", mask->points.total, gui->affected_point);

			float last_x = point->x;
			float last_y = point->y;
			float last_control_x1 = point->control_x1;
			float last_control_y1 = point->control_y1;
			float last_control_x2 = point->control_x2;
			float last_control_y2 = point->control_y2;


			switch(gui->current_operation)
			{
				case CWINDOW_MASK:
					point->x = cursor_x - gui->x_origin + gui->center_x;
					point->y = cursor_y - gui->y_origin + gui->center_y;
					break;

				case CWINDOW_MASK_CONTROL_IN:
					point->control_x1 = cursor_x - gui->x_origin + gui->control_in_x;
					point->control_y1 = cursor_y - gui->y_origin + gui->control_in_y;
					break;

				case CWINDOW_MASK_CONTROL_OUT:
					point->control_x2 = cursor_x - gui->x_origin + gui->control_out_x;
					point->control_y2 = cursor_y - gui->y_origin + gui->control_out_y;
					break;

				case CWINDOW_MASK_TRANSLATE:
					for(int i = 0; i < mask->points.total; i++)
					{
						mask->points.values[i]->x += cursor_x - gui->x_origin;
						mask->points.values[i]->y += cursor_y - gui->y_origin;
					}
					gui->x_origin = cursor_x;
					gui->y_origin = cursor_y;
					break;
			}


			if( !EQUIV(last_x, point->x) ||
				!EQUIV(last_y, point->y) ||
				!EQUIV(last_control_x1, point->control_x1) ||
				!EQUIV(last_control_y1, point->control_y1) ||
				!EQUIV(last_control_x2, point->control_x2) ||
				!EQUIV(last_control_y2, point->control_y2))
			{
				mwindow->undo->update_undo_before(_("tweek"), LOAD_AUTOMATION);
				rerender = 1;
				redraw = 1;
			}
		}
		result = 1;
	}
//printf("CWindowCanvas::do_mask 2 %d %d %d\n", result, rerender, redraw);

	points.remove_all_objects();
//printf("CWindowCanvas::do_mask 20\n");
	return result;
}

void CWindowCanvas::draw_overlays()
{
//printf("CWindowCanvas::draw_overlays 1\n");
	if(mwindow->edl->session->safe_regions)
	{
		draw_safe_regions();
	}

	if(mwindow->edl->session->cwindow_scrollbars)
	{
// Always draw output rectangle
		float x1, y1, x2, y2;
		x1 = 0;
		x2 = mwindow->edl->session->output_w;
		y1 = 0;
		y2 = mwindow->edl->session->output_h;
		output_to_canvas(mwindow->edl, 0, x1, y1);
		output_to_canvas(mwindow->edl, 0, x2, y2);

		canvas->set_inverse();
		canvas->set_color(WHITE);

		canvas->draw_rectangle((int)x1, 
				(int)y1, 
				(int)(x2 - x1), 
				(int)(y2 - y1));

		canvas->set_opaque();
	}

	if(mwindow->session->ccanvas_highlighted)
	{
		canvas->set_color(WHITE);
		canvas->set_inverse();
		canvas->draw_rectangle(0, 0, canvas->get_w(), canvas->get_h());
		canvas->draw_rectangle(1, 1, canvas->get_w() - 2, canvas->get_h() - 2);
		canvas->set_opaque();
	}

	int temp1 = 0, temp2 = 0;
//printf("CWindowCanvas::draw_overlays 1 %d\n", mwindow->edl->session->cwindow_operation);
	switch(mwindow->edl->session->cwindow_operation)
	{
		case CWINDOW_CAMERA:
			draw_bezier(1);
			break;

		case CWINDOW_PROJECTOR:
			draw_bezier(0);
			break;

		case CWINDOW_CROP:
			draw_crop();
			break;

		case CWINDOW_MASK:
			do_mask(temp1, temp2, 0, 0, 1);
			break;
	}
//printf("CWindowCanvas::draw_overlays 2\n");
}

void CWindowCanvas::draw_safe_regions()
{
	float action_x1, action_x2, action_y1, action_y2;
	float title_x1, title_x2, title_y1, title_y2;

	action_x1 = mwindow->edl->session->output_w / 2 - mwindow->edl->session->output_w / 2 * 0.9;
	action_x2 = mwindow->edl->session->output_w / 2 + mwindow->edl->session->output_w / 2 * 0.9;
	action_y1 = mwindow->edl->session->output_h / 2 - mwindow->edl->session->output_h / 2 * 0.9;
	action_y2 = mwindow->edl->session->output_h / 2 + mwindow->edl->session->output_h / 2 * 0.9;
	title_x1 = mwindow->edl->session->output_w / 2 - mwindow->edl->session->output_w / 2 * 0.8;
	title_x2 = mwindow->edl->session->output_w / 2 + mwindow->edl->session->output_w / 2 * 0.8;
	title_y1 = mwindow->edl->session->output_h / 2 - mwindow->edl->session->output_h / 2 * 0.8;
	title_y2 = mwindow->edl->session->output_h / 2 + mwindow->edl->session->output_h / 2 * 0.8;

	output_to_canvas(mwindow->edl, 0, action_x1, action_y1);
	output_to_canvas(mwindow->edl, 0, action_x2, action_y2);
	output_to_canvas(mwindow->edl, 0, title_x1, title_y1);
	output_to_canvas(mwindow->edl, 0, title_x2, title_y2);

	canvas->set_inverse();
	canvas->set_color(WHITE);

	canvas->draw_rectangle((int)action_x1, 
			(int)action_y1, 
			(int)(action_x2 - action_x1), 
			(int)(action_y2 - action_y1));
	canvas->draw_rectangle((int)title_x1, 
			(int)title_y1, 
			(int)(title_x2 - title_x1), 
			(int)(title_y2 - title_y1));

	canvas->set_opaque();
}

void CWindowCanvas::reset_keyframe(int do_camera)
{
	BezierAuto *translate_keyframe = 0;
	FloatAuto *zoom_keyframe = 0;
	Track *affected_track = 0;

	affected_track = gui->cwindow->calculate_affected_track();

	if(affected_track)
	{
		if(do_camera)
		{
			translate_keyframe = (BezierAuto*)gui->cwindow->calculate_affected_auto(
				affected_track->automation->camera_autos);
			zoom_keyframe = (FloatAuto*)gui->cwindow->calculate_affected_auto(
				affected_track->automation->czoom_autos);
		}
		else
		{
			translate_keyframe = (BezierAuto*)gui->cwindow->calculate_affected_auto(
				affected_track->automation->projector_autos);
			zoom_keyframe = (FloatAuto*)gui->cwindow->calculate_affected_auto(
				affected_track->automation->pzoom_autos);
		}

		translate_keyframe->center_x = 0;
		translate_keyframe->center_y = 0;
		translate_keyframe->center_z = 1;
		zoom_keyframe->value = 1;

		mwindow->sync_parameters(CHANGE_PARAMS);
		gui->update_tool();
// 		gui->cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
// 			CHANGE_NONE,
// 			mwindow->edl,
// 			1);
	}
}

void CWindowCanvas::reset_camera()
{
	reset_keyframe(1);
}

void CWindowCanvas::reset_projector()
{
	reset_keyframe(0);
}

int CWindowCanvas::test_crop(int button_press, int &redraw)
{
	int result = 0;
	int handle_selected = -1;
	float x1 = mwindow->edl->session->crop_x1;
	float y1 = mwindow->edl->session->crop_y1;
	float x2 = mwindow->edl->session->crop_x2;
	float y2 = mwindow->edl->session->crop_y2;
	float cursor_x = get_cursor_x();
	float cursor_y = get_cursor_y();
	float canvas_x1 = x1;
	float canvas_y1 = y1;
	float canvas_x2 = x2;
	float canvas_y2 = y2;
	float canvas_cursor_x = cursor_x;
	float canvas_cursor_y = cursor_y;

	canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
// Use screen normalized coordinates for hot spot tests.
	output_to_canvas(mwindow->edl, 0, canvas_x1, canvas_y1);
	output_to_canvas(mwindow->edl, 0, canvas_x2, canvas_y2);

	if(gui->current_operation == CWINDOW_CROP)
	{
		handle_selected = gui->crop_handle;
	}
	else
	if(canvas_cursor_x >= canvas_x1 && canvas_cursor_x < canvas_x1 + CROPHANDLE_W &&
		canvas_cursor_y >= canvas_y1 && canvas_cursor_y < canvas_y1 + CROPHANDLE_H)
	{
		handle_selected = 0;
		gui->crop_origin_x = x1;
		gui->crop_origin_y = y1;
	}
	else
	if(canvas_cursor_x >= canvas_x2 - CROPHANDLE_W && canvas_cursor_x < canvas_x2 &&
		canvas_cursor_y >= canvas_y1 && canvas_cursor_y < canvas_y1 + CROPHANDLE_H)
	{
		handle_selected = 1;
		gui->crop_origin_x = x2;
		gui->crop_origin_y = y1;
	}
	else
	if(canvas_cursor_x >= canvas_x1 && canvas_cursor_x < canvas_x1 + CROPHANDLE_W &&
		canvas_cursor_y >= canvas_y2 - CROPHANDLE_H && canvas_cursor_y < canvas_y2)
	{
		handle_selected = 2;
		gui->crop_origin_x = x1;
		gui->crop_origin_y = y2;
	}
	else
	if(canvas_cursor_x >= canvas_x2 - CROPHANDLE_W && canvas_cursor_x < canvas_x2 &&
		canvas_cursor_y >= canvas_y2 - CROPHANDLE_H && canvas_cursor_y < canvas_y2)
	{
		handle_selected = 3;
		gui->crop_origin_x = x2;
		gui->crop_origin_y = y2;
	}
	else
// Start new box
	{
		gui->crop_origin_x = cursor_x;
		gui->crop_origin_y = cursor_y;
	}

// printf("test crop %d %d\n", 
// 	gui->current_operation,
// 	handle_selected);

// Start dragging.
	if(button_press)
	{
		gui->current_operation = CWINDOW_CROP;
		gui->crop_handle = handle_selected;
		gui->x_origin = cursor_x;
		gui->y_origin = cursor_y;
		result = 1;

		if(handle_selected < 0) 
		{
			x2 = x1 = cursor_x;
			y2 = y1 = cursor_y;
			mwindow->edl->session->crop_x1 = (int)x1;
			mwindow->edl->session->crop_y1 = (int)y1;
			mwindow->edl->session->crop_x2 = (int)x2;
			mwindow->edl->session->crop_y2 = (int)y2;
			redraw = 1;
		}
	}
    else
// Update dragging
	if(gui->current_operation == CWINDOW_CROP)
	{
		float x_difference, y_difference;
		if(gui->crop_handle >= 0)
		{
			float zoom_x, zoom_y, conformed_w, conformed_h;
			get_zooms(mwindow->edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
			x_difference = cursor_x - gui->x_origin;
		}

		switch(gui->crop_handle)
		{
			case -1:
				x1 = gui->crop_origin_x;
				y1 = gui->crop_origin_y;
				x2 = gui->crop_origin_x;
				y2 = gui->crop_origin_y;
				if(cursor_x < gui->x_origin)
				{
					if(cursor_y < gui->y_origin)
					{
						x1 = cursor_x;
						y1 = cursor_y;
					}
					else
					if(cursor_y >= gui->y_origin)
					{
						x1 = cursor_x;
						y2 = cursor_y;
					}
				}
				else
				if(cursor_x  >= gui->x_origin)
				{
					if(cursor_y < gui->y_origin)
					{
						y1 = cursor_y;
						x2 = cursor_x;
					}
					else
					if(cursor_y >= gui->y_origin)
					{
						x2 = cursor_x;
						y2 = cursor_y;
					}
				}

// printf("test crop %d %d %d %d\n", 
// 	mwindow->edl->session->crop_x1,
// 	mwindow->edl->session->crop_y1,
// 	mwindow->edl->session->crop_x2,
// 	mwindow->edl->session->crop_y2);
				break;
			case 0:
				x1 = cursor_x - gui->x_origin + gui->crop_origin_x;
				y1 = cursor_y - gui->y_origin + gui->crop_origin_y;
				break;
			case 1:
				x2 = cursor_x - gui->x_origin + gui->crop_origin_x;
				y1 = cursor_y - gui->y_origin + gui->crop_origin_y;
				break;
			case 2:
				x1 = cursor_x - gui->x_origin + gui->crop_origin_x;
				y2 = cursor_y - gui->y_origin + gui->crop_origin_y;
				break;
			case 3:
				x2 = cursor_x - gui->x_origin + gui->crop_origin_x;
				y2 = cursor_y - gui->y_origin + gui->crop_origin_y;
				break;
		}

		if(!EQUIV(mwindow->edl->session->crop_x1, x1) ||
			!EQUIV(mwindow->edl->session->crop_x2, x2) ||
			!EQUIV(mwindow->edl->session->crop_y1, y1) ||
			!EQUIV(mwindow->edl->session->crop_y2, y2))
		{
			mwindow->edl->session->crop_x1 = (int)x1;
			mwindow->edl->session->crop_y1 = (int)y1;
			mwindow->edl->session->crop_x2 = (int)x2;
			mwindow->edl->session->crop_y2 = (int)y2;
			result = 1;
			redraw = 1;
		}
	}
	else
// Update cursor font
	if(handle_selected >= 0)
	{
		switch(handle_selected)
		{
			case 0:
				set_cursor(UPLEFT_RESIZE);
				break;
			case 1:
				set_cursor(UPRIGHT_RESIZE);
				break;
			case 2:
				set_cursor(DOWNLEFT_RESIZE);
				break;
			case 3:
				set_cursor(DOWNRIGHT_RESIZE);
				break;
		}
		result = 1;
	}
	else
	{
		set_cursor(ARROW_CURSOR);
	}
#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))
	
	if(redraw)
	{
		CLAMP(mwindow->edl->session->crop_x1, 0, mwindow->edl->calculate_output_w(0));
		CLAMP(mwindow->edl->session->crop_x2, 0, mwindow->edl->calculate_output_w(0));
		CLAMP(mwindow->edl->session->crop_y1, 0, mwindow->edl->calculate_output_h(0));
		CLAMP(mwindow->edl->session->crop_y2, 0, mwindow->edl->calculate_output_h(0));
// printf("CWindowCanvas::test_crop %d %d %d %d\n", 
// 	mwindow->edl->session->crop_x2,
// 	mwindow->edl->session->crop_y2,
// 	mwindow->edl->calculate_output_w(0), 
// 	mwindow->edl->calculate_output_h(0));
	}
	return result;
}


void CWindowCanvas::draw_crop()
{
	canvas->set_inverse();
	canvas->set_color(WHITE);

	float x1 = mwindow->edl->session->crop_x1;
	float y1 = mwindow->edl->session->crop_y1;
	float x2 = mwindow->edl->session->crop_x2;
	float y2 = mwindow->edl->session->crop_y2;

	output_to_canvas(mwindow->edl, 0, x1, y1);
	output_to_canvas(mwindow->edl, 0, x2, y2);

	if(x2 - x1 && y2 - y1)
		canvas->draw_rectangle((int)x1, 
			(int)y1, 
			(int)(x2 - x1), 
			(int)(y2 - y1));

	draw_crophandle((int)x1, (int)y1);
	draw_crophandle((int)x2 - CROPHANDLE_W, (int)y1);
	draw_crophandle((int)x1, (int)y2 - CROPHANDLE_H);
	draw_crophandle((int)x2 - CROPHANDLE_W, (int)y2 - CROPHANDLE_H);
	canvas->set_opaque();
}



#define BEZIER_W 20
#define BEZIER_H 20

int CWindowCanvas::do_bezier_center(BezierAuto *current, 
	BezierAutos *camera_autos,
	BezierAutos *projector_autos, 
	FloatAutos *czoom_autos,
	FloatAutos *pzoom_autos,
	int camera, 
	int draw)
{
	float center_x = 0, center_y = 0, center_z = 0;
	BezierAuto *before = 0, *after = 0;
	FloatAuto *previous = 0, *next = 0;
	VTrack *track = (VTrack*)current->autos->track;
	long position = track->to_units(
				mwindow->edl->local_session->selectionstart, 
				0);

// Get center of current frame.  Draw everything relative to this.
	if(camera)
	{
		camera_autos->get_center(center_x, 
			center_y, 
			center_z, 
			(float)position,
			PLAY_FORWARD, 
			&before, 
			&after);
		center_z = czoom_autos->get_value(position,
			PLAY_FORWARD,
			previous,
			next);

		float projector_x, projector_y, projector_z;
		before = after = 0;
		projector_autos->get_center(projector_x, 
			projector_y, 
			projector_z,
			(float)position,
			PLAY_FORWARD,
			&before,
			&after);
		previous = next = 0;
		projector_z = pzoom_autos->get_value(position,
			PLAY_FORWARD,
			previous,
			next);

		center_x -= projector_x;
		center_y -= projector_y;
		center_z = projector_z;
//printf("CWindowCanvas::do_bezier_center 1 %p %f %f\n", 
//current, projector_y, center_y);
	}
	else
	{
		center_z = pzoom_autos->get_value(current->position,
			PLAY_FORWARD,
			previous,
			next);
	}



	float auto_x = current->center_x - center_x + mwindow->edl->session->output_w / 2;
	float auto_y = current->center_y - center_y + mwindow->edl->session->output_h / 2;
	float track_x1 = auto_x - track->track_w / 2 * center_z;
	float track_y1 = auto_y - track->track_h / 2 * center_z;
	float track_x2 = track_x1 + track->track_w * center_z;
	float track_y2 = track_y1 + track->track_h * center_z;
	float control_in_x = auto_x + current->control_in_x;
	float control_in_y = auto_y + current->control_in_y;
	float control_out_x = auto_x + current->control_out_x;
	float control_out_y = auto_y + current->control_out_y;
	int control_point = 0;


	output_to_canvas(mwindow->edl, 0, auto_x, auto_y);
	output_to_canvas(mwindow->edl, 0, control_out_x, control_out_y);
	output_to_canvas(mwindow->edl, 0, control_in_x, control_in_y);
	output_to_canvas(mwindow->edl, 0, track_x1, track_y1);
	output_to_canvas(mwindow->edl, 0, track_x2, track_y2);

#define DRAW_THING(offset) \
	canvas->draw_line((int)control_in_x + offset,  \
		(int)control_in_y + offset,  \
		(int)auto_x + offset,  \
		(int)auto_y + offset); \
	canvas->draw_line((int)control_out_x + offset,  \
		(int)control_out_y + offset,  \
		(int)auto_x + offset,  \
		(int)auto_y + offset); \
	canvas->draw_rectangle((int)control_in_x - CONTROL_W / 2 + offset,  \
		(int)control_in_y - CONTROL_H / 2 + offset, \
		CONTROL_W, \
		CONTROL_H); \
	canvas->draw_rectangle((int)control_out_x - CONTROL_W / 2 + offset,  \
		(int)control_out_y - CONTROL_H / 2 + offset, \
		CONTROL_W, \
		CONTROL_H); \
 \
	if(0) \
	{ \
		canvas->draw_line((int)auto_x - BEZIER_W / 2,  \
			(int)auto_y - BEZIER_H / 2 + offset, \
			(int)auto_x + BEZIER_W / 2, \
			(int)auto_y + BEZIER_H / 2 + offset); \
		canvas->draw_line((int)auto_x - BEZIER_W / 2,  \
			(int)auto_y + BEZIER_H / 2 + offset, \
			(int)auto_x + BEZIER_W / 2, \
			(int)auto_y - BEZIER_H / 2 + offset); \
	} \
	else \
	{ \
		canvas->draw_rectangle((int)track_x1 + offset, \
			(int)track_y1 + offset, \
			(int)(track_x2 - track_x1), \
			(int)(track_y2 - track_y1)); \
		canvas->draw_line((int)track_x1 + offset,  \
			(int)track_y1 + offset, \
			(int)track_x2 + offset, \
			(int)track_y2 + offset); \
		canvas->draw_line((int)track_x2 + offset,  \
			(int)track_y1 + offset, \
			(int)track_x1 + offset, \
			(int)track_y2 + offset); \
	}

	if(draw)
	{
		canvas->set_color(BLACK);
		DRAW_THING(1);

		if(current->position > position)
			canvas->set_color(GREEN);
		else
			canvas->set_color(RED);

		DRAW_THING(0);
// printf("CWindowCanvas::do_bezier_center 2 %f,%f %f,%f\n", 
// control_in_x, 
// control_in_y, 
// control_out_x,
// control_out_y);
	}
	else
	{
		int cursor_x = get_cursor_x();
		int cursor_y = get_cursor_y();
		
#ifndef SQR
#define SQR(x) ((x) * (x))
#endif

		float distance1 = sqrt(SQR(cursor_x - control_in_x) + 
			SQR(cursor_y - control_in_y));
		float distance2 = sqrt(SQR(cursor_x - control_out_x) + 
			SQR(cursor_y - control_out_y));
// printf("CWindowCanvas::do_bezier_center 3 %f,%f %f,%f\n", 
// control_in_x, 
// control_in_y, 
// control_out_x,
// control_out_y);

		control_point = distance2 < distance1;
	}
	
	return control_point;
}



void CWindowCanvas::draw_bezier_joining(BezierAuto *first, 
	BezierAuto *last, 
	BezierAutos *camera_autos,
	BezierAutos *projector_autos, 
	FloatAutos *czoom_autos,
	FloatAutos *pzoom_autos,
	int camera)
{
	if(first == last) return;
	
	float center_x = 0, center_y = 0, center_z = 0;
	BezierAuto *before = 0, *after = 0;
	long position = first->autos->track->to_units(
					mwindow->edl->local_session->selectionstart, 
					0);

// Get center of current position.  Draw everything relative to this.
	if(camera)
	{
		camera_autos->get_center(center_x, 
				center_y, 
				center_z, 
				(float)position, 
				PLAY_FORWARD, 
				&before, 
				&after);

		float projector_x, projector_y, projector_z;
		before = after = 0;
		projector_autos->get_center(projector_x, 
			projector_y, 
			projector_z,
			(float)position,
			PLAY_FORWARD,
			&before,
			&after);

		center_x -= projector_x;
		center_y -= projector_y;
	}

//	int segments = 10;
	int segments = MAX(canvas->get_w(), canvas->get_h());
	int step = (last->position - first->position) / segments;
	float old_x, old_y;
	if(step < 1) step = 1;

	for(long frame = first->position; 
		frame < last->position; 
		frame += step)
	{
		float new_x, new_y, new_z;
		float x1, y1, x2, y2;

		((BezierAutos*)(first->autos))->get_center(new_x, 
			new_y, 
			new_z, 
			frame, 
			PLAY_FORWARD, 
			&before, 
			&after);

		if(frame == first->position)
		{
			old_x = new_x;
			old_y = new_y;
		}

		x1 = old_x - center_x + mwindow->edl->session->output_w / 2;
		y1 = old_y - center_y + mwindow->edl->session->output_h / 2;
		x2 = new_x - center_x + mwindow->edl->session->output_w / 2;
		y2 = new_y - center_y + mwindow->edl->session->output_h / 2;
		output_to_canvas(mwindow->edl, 0, x1, y1);
		output_to_canvas(mwindow->edl, 0, x2, y2);

		canvas->set_color(BLACK);
		canvas->draw_line((int)x1 + 1, (int)y1 + 1, (int)x2 + 1, (int)y2 + 1);

		if(frame >= position)
			canvas->set_color(GREEN);
		else
			canvas->set_color(RED);

		canvas->draw_line((int)x1, (int)y1, (int)x2, (int)y2);
		old_x = new_x;
		old_y = new_y;
	}
}



void CWindowCanvas::draw_bezier(int do_camera)
{
	Track *track = gui->cwindow->calculate_affected_track();
	BezierAutos *autos, *camera_autos, *projector_autos;
	BezierAuto *first = 0, *mid = 0, *last = 0;
	BezierAuto *before = 0, *after = 0;
	FloatAutos *czoom_autos, *pzoom_autos;

// No track at initialization
	if(!track) return;

	camera_autos = track->automation->camera_autos;
	projector_autos = track->automation->projector_autos;
	czoom_autos = track->automation->czoom_autos;
	pzoom_autos = track->automation->pzoom_autos;

	if(do_camera)
		autos = track->automation->camera_autos;
	else
		autos = track->automation->projector_autos;


	long position = track->to_units(mwindow->edl->local_session->selectionstart, 
					0);


// Rules for which autos to draw:
//
// No automation besides default:
//     mid = default
//
// Automation on or after current position only
//     mid = first, last = next
//
// Automation before current position only
//     mid = last, first = previous
// 
// Automation on current position only
//     mid = first
//
// Automation before and after current position
//     first = previous, mid = next, last = next->next

	for(first = (BezierAuto*)autos->last; 
		first; 
		first = (BezierAuto*)first->previous)
		if(first->position < position)
			break;

	if(first)
	{
		mid = (BezierAuto*)first->next;
		if(!mid)
		{
			mid = first;
			first = (BezierAuto*)first->previous;
		}
	}
	else
		mid = (BezierAuto*)autos->first;

	if(mid)
		last = (BezierAuto*)mid->next;
	else
		mid = (BezierAuto*)autos->default_auto;

	if(!last)
	{
		last = mid;
		mid = first;
		if(mid) first = (BezierAuto*)mid->previous;
	}
	
	if(!first)
	{
		first = mid;
		mid = last;
		if(mid) last = (BezierAuto*)mid->next;
	}

//printf("draw_bezier 1 %p %p %p\n", first, mid, last);



// Draw joining lines
	if(first && mid)
		draw_bezier_joining(first, 
			mid, 
			camera_autos, 
			projector_autos, 
			czoom_autos,
			pzoom_autos,
			do_camera);

	if(mid && last)
		draw_bezier_joining(mid, 
			last, 
			camera_autos, 
			projector_autos, 
			czoom_autos,
			pzoom_autos,
			do_camera);

//printf("draw_bezier 2 %p %p %p\n", first, mid, last);


// Draw centers of autos
	if(first) 
		do_bezier_center(first, 
			camera_autos, 
			projector_autos, 
			czoom_autos,
			pzoom_autos,
			do_camera, 
			1);
	if(mid) 
		do_bezier_center(mid, 
			camera_autos, 
			projector_autos, 
			czoom_autos,
			pzoom_autos,
			do_camera, 
			1);
	if(last) 
		do_bezier_center(last, 
			camera_autos, 
			projector_autos, 
			czoom_autos,
			pzoom_autos,
			do_camera, 
			1);

//printf("draw_bezier 3 %p %p %p\n", first, mid, last);



}



int CWindowCanvas::test_bezier(int button_press, 
	int &redraw, 
	int &redraw_canvas,
	int &rerender,
	int do_camera)
{
	int result = 0;

// Processing drag operation.
// Create keyframe during first cursor motion.
	if(!button_press)
	{

		float cursor_x = get_cursor_x();
		float cursor_y = get_cursor_y();
		canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);

		if(gui->current_operation == CWINDOW_CAMERA ||
			gui->current_operation == CWINDOW_PROJECTOR)
		{
			if(!gui->ctrl_down() && gui->shift_down() && !gui->translating_zoom)
			{
				gui->translating_zoom = 1;
				gui->affected_auto = 0;
			}
			else
			if(!gui->ctrl_down() && !gui->shift_down() && gui->translating_zoom)
			{
				gui->translating_zoom = 0;
				gui->affected_auto = 0;
			}

// Get target keyframe
			BezierAutos *camera_autos = gui->affected_track->automation->camera_autos;
			BezierAutos *projector_autos = gui->affected_track->automation->projector_autos;
			FloatAutos *czoom_autos = gui->affected_track->automation->czoom_autos;
			FloatAutos *pzoom_autos = gui->affected_track->automation->pzoom_autos;
			float last_center_x;
			float last_center_y;
			float last_center_z;


			if(!gui->affected_auto)
			{
				mwindow->undo->update_undo_before(_("keyframe"), LOAD_AUTOMATION);
				if(mwindow->edl->session->cwindow_operation == CWINDOW_CAMERA)
				{
					if(gui->translating_zoom)
					{
						gui->affected_auto = 
							gui->cwindow->calculate_affected_auto((Autos*)czoom_autos, 1);
					}
					else
						gui->affected_auto = 
							gui->cwindow->calculate_affected_auto((Autos*)camera_autos, 1);
				}
				else
				if(mwindow->edl->session->cwindow_operation == CWINDOW_PROJECTOR)
				{
					if(gui->translating_zoom)
					{
						gui->affected_auto = 
							gui->cwindow->calculate_affected_auto((Autos*)pzoom_autos, 1);
					}
					else
						gui->affected_auto = 
							gui->cwindow->calculate_affected_auto((Autos*)projector_autos, 1);
				}

				calculate_origin();
				
				if(gui->translating_zoom)
				{
					gui->center_z = ((FloatAuto*)gui->affected_auto)->value;
				}
				else
				{
					gui->center_x = ((BezierAuto*)gui->affected_auto)->center_x;
					gui->center_y = ((BezierAuto*)gui->affected_auto)->center_y;
				}

				rerender = 1;
				redraw = 1;
			}

			BezierAuto *bezier_keyframe = (BezierAuto*)gui->affected_auto;
			FloatAuto *zoom_keyframe = (FloatAuto*)gui->affected_auto;

			if(gui->translating_zoom)
			{
				last_center_z = zoom_keyframe->value;
			}
			else
			{
				last_center_x = bezier_keyframe->center_x;
				last_center_y = bezier_keyframe->center_y;
			}

			if(gui->translating_zoom)
			{
				zoom_keyframe->value = gui->center_z + (cursor_y - gui->y_origin) / 128;
				if(!EQUIV(last_center_z, zoom_keyframe->value))
				{
					mwindow->undo->update_undo_before(_("tweek"), LOAD_AUTOMATION);
					rerender = 1;
					redraw = 1;
					redraw_canvas = 1;
				}
			}
			else
			{
				bezier_keyframe->center_x = gui->center_x + cursor_x - gui->x_origin;
				bezier_keyframe->center_y = gui->center_y + cursor_y - gui->y_origin;
				if(!EQUIV(last_center_x,  bezier_keyframe->center_x) ||
				   	!EQUIV(last_center_y, bezier_keyframe->center_y))
				{
					mwindow->undo->update_undo_before(_("tweek"), LOAD_AUTOMATION);
					rerender = 1;
					redraw = 1;
				}
			}
		}
		else
		if(gui->current_operation == CWINDOW_CAMERA_CONTROL_IN ||
			gui->current_operation == CWINDOW_PROJECTOR_CONTROL_IN)
		{
			BezierAuto *bezier_keyframe = (BezierAuto*)gui->affected_auto;
			float last_control_in_x = bezier_keyframe->control_in_x;
			float last_control_in_y = bezier_keyframe->control_in_y;
			bezier_keyframe->control_in_x = gui->control_in_x + cursor_x - gui->x_origin;
			bezier_keyframe->control_in_y = gui->control_in_y + cursor_y - gui->y_origin;

			if(!EQUIV(last_control_in_x, bezier_keyframe->control_in_x) ||
				!EQUIV(last_control_in_y, bezier_keyframe->control_in_y))
			{
				mwindow->undo->update_undo_before(_("tweek"), LOAD_AUTOMATION);
				rerender = 1;
				redraw = 1;
			}
		}
		else
		if(gui->current_operation == CWINDOW_CAMERA_CONTROL_OUT ||
			gui->current_operation == CWINDOW_PROJECTOR_CONTROL_OUT)
		{
			BezierAuto *bezier_keyframe = (BezierAuto*)gui->affected_auto;
			float last_control_out_x = bezier_keyframe->control_out_x;
			float last_control_out_y = bezier_keyframe->control_out_y;
			bezier_keyframe->control_out_x = gui->control_out_x + cursor_x - gui->x_origin;
			bezier_keyframe->control_out_y = gui->control_out_y + cursor_y - gui->y_origin;

			if(!EQUIV(last_control_out_x, bezier_keyframe->control_out_x) ||
				!EQUIV(last_control_out_y, bezier_keyframe->control_out_y))
			{
				mwindow->undo->update_undo_before(_("tweek"), LOAD_AUTOMATION);
				rerender = 1;
				redraw = 1;
			}
		}

		result = 1;
	}
	else
// Begin drag operation.  Don't create keyframe here.
	{
// Get affected track off of the first recordable video track.
// Calculating based on the alpha channel would require rendering
// each layer and its effects and trapping the result in VirtualVNode before
// compositing onto the output.
		gui->affected_track = gui->cwindow->calculate_affected_track();
		gui->affected_auto = 0;

		if(gui->affected_track)
		{
			BezierAutos *camera_autos = gui->affected_track->automation->camera_autos;
			BezierAutos *projector_autos = gui->affected_track->automation->projector_autos;
			FloatAutos *czoom_autos = gui->affected_track->automation->czoom_autos;
			FloatAutos *pzoom_autos = gui->affected_track->automation->pzoom_autos;


			if(!gui->ctrl_down())
			{
				gui->current_operation = 
					mwindow->edl->session->cwindow_operation;
				gui->affected_auto = 0;
			}
			else
			{
// Get nearest control point
				if(mwindow->edl->session->cwindow_operation == CWINDOW_CAMERA)
					gui->affected_auto = 
						gui->cwindow->calculate_affected_auto(camera_autos, 0);
				else
				if(mwindow->edl->session->cwindow_operation == CWINDOW_PROJECTOR)
					gui->affected_auto = 
						gui->cwindow->calculate_affected_auto(projector_autos, 0);

//printf("CWindowCanvas::test_bezier 1 %d\n", gui->current_operation);
				int control_point = do_bezier_center(
					(BezierAuto*)gui->affected_auto, 
					camera_autos,
					projector_autos,
					czoom_autos,
					pzoom_autos,
					mwindow->edl->session->cwindow_operation == CWINDOW_CAMERA,
					0);

//printf("CWindowCanvas::test_bezier 2 %d\n", control_point);
				if(control_point == 0)
				{
					if(mwindow->edl->session->cwindow_operation == CWINDOW_CAMERA)
						gui->current_operation = CWINDOW_CAMERA_CONTROL_IN;
					else
						gui->current_operation = CWINDOW_PROJECTOR_CONTROL_IN;
				}
				else
				{
					if(mwindow->edl->session->cwindow_operation == CWINDOW_CAMERA)
						gui->current_operation = CWINDOW_CAMERA_CONTROL_OUT;
					else
						gui->current_operation = CWINDOW_PROJECTOR_CONTROL_OUT;
				}

				BezierAuto *keyframe = (BezierAuto*)gui->affected_auto;
				gui->center_x = keyframe->center_x;
				gui->center_y = keyframe->center_y;
				gui->center_z = keyframe->center_z;
				gui->control_in_x = keyframe->control_in_x;
				gui->control_in_y = keyframe->control_in_y;
				gui->control_out_x = keyframe->control_out_x;
				gui->control_out_y = keyframe->control_out_y;
			}


			result = 1;
		}
	}
	
	return result;
}

int CWindowCanvas::test_zoom(int &redraw)
{
	int result = 0;
	float zoom = get_zoom();
	float x;
	float y;

	if(!mwindow->edl->session->cwindow_scrollbars)
	{
		mwindow->edl->session->cwindow_scrollbars = 1;
		zoom = 1.0;
		x = 0;
		y = 0;
	}
	else
	{
		x = get_cursor_x();
		y = get_cursor_y();
		canvas_to_output(mwindow->edl, 
				0, 
				x, 
				y);

//printf("CWindowCanvas::test_zoom 1 %f %f\n", x, y);

// Zoom out
		if(get_buttonpress() == 5 ||
			gui->ctrl_down() || 
			gui->shift_down())
		{
			if(zoom > MIN_ZOOM)
			{
				zoom /= 2;
				x -= w_visible * 2 / 2;
				y -= h_visible * 2 / 2;
			}
			else
			{
				x -= w_visible / 2;
				y -= h_visible / 2;
			}
		}
		else
// Zoom in
		{
			if(zoom < MAX_ZOOM)
			{
				zoom *= 2;
				x -= w_visible / 2 / 2;
				y -= h_visible / 2 / 2;
			}
			else
			{
				x -= w_visible / 2;
				y -= h_visible / 2;
			}
		}
	}

	int x_i = (int)x;
	int y_i = (int)y;
//	check_boundaries(mwindow->edl, x_i, y_i, zoom);

//printf("CWindowCanvas::test_zoom 2 %d %d\n", x_i, y_i);

	update_zoom(x_i, 
			y_i, 
			zoom);
	reposition_window(mwindow->edl, 
			mwindow->theme->ccanvas_x,
			mwindow->theme->ccanvas_y,
			mwindow->theme->ccanvas_w,
			mwindow->theme->ccanvas_h);
	redraw = 1;
	result = 1;

	
	gui->zoom_panel->update(zoom);
	
	return result;
}


void CWindowCanvas::calculate_origin()
{
	gui->x_origin = get_cursor_x();
	gui->y_origin = get_cursor_y();
//printf("CWindowCanvas::calculate_origin 1 %f %f\n", gui->x_origin, gui->y_origin);
	canvas_to_output(mwindow->edl, 0, gui->x_origin, gui->y_origin);
//printf("CWindowCanvas::calculate_origin 2 %f %f\n", gui->x_origin, gui->y_origin);
}


int CWindowCanvas::cursor_leave_event()
{
	set_cursor(ARROW_CURSOR);
	return 1;
}

int CWindowCanvas::cursor_enter_event()
{
	int redraw = 0;
	switch(mwindow->edl->session->cwindow_operation)
	{
		case CWINDOW_CAMERA:
		case CWINDOW_PROJECTOR:
			set_cursor(MOVE_CURSOR);
			break;
		case CWINDOW_ZOOM:
			set_cursor(MOVE_CURSOR);
			break;
		case CWINDOW_CROP:
			test_crop(0, redraw);
			break;
		case CWINDOW_PROTECT:
			set_cursor(ARROW_CURSOR);
			break;
		case CWINDOW_MASK:
			set_cursor(CROSS_CURSOR);
			break;
	}
	return 1;
}

int CWindowCanvas::cursor_motion_event()
{
	int redraw = 0, result = 0, rerender = 0, redraw_canvas = 0;


	switch(gui->current_operation)
	{
		case CWINDOW_SCROLL:
		{
			float zoom = get_zoom();
			float cursor_x = get_cursor_x();
			float cursor_y = get_cursor_y();

			float zoom_x, zoom_y, conformed_w, conformed_h;
			get_zooms(mwindow->edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
			cursor_x = (float)cursor_x / zoom_x + gui->x_offset;
			cursor_y = (float)cursor_y / zoom_y + gui->y_offset;



			int x = (int)(gui->x_origin - cursor_x + gui->x_offset);
			int y = (int)(gui->y_origin - cursor_y + gui->y_offset);


			update_zoom(x, 
				y, 
				zoom);
			update_scrollbars();
			redraw = 1;
			result = 1;
			break;
		}

		case CWINDOW_CAMERA:
		case CWINDOW_CAMERA_CONTROL_IN:
		case CWINDOW_CAMERA_CONTROL_OUT:
			result = test_bezier(0, redraw, redraw_canvas, rerender, 1);
			break;

		case CWINDOW_PROJECTOR:
		case CWINDOW_PROJECTOR_CONTROL_IN:
		case CWINDOW_PROJECTOR_CONTROL_OUT:
			result = test_bezier(0, redraw, redraw_canvas, rerender, 0);
			break;


		case CWINDOW_CROP:
//printf("CWindowCanvas::cursor_motion_event 1 %d %d\n", x, y);
			result = test_crop(0, redraw);
			break;

		case CWINDOW_MASK:
		case CWINDOW_MASK_CONTROL_IN:
		case CWINDOW_MASK_CONTROL_OUT:
		case CWINDOW_MASK_TRANSLATE:
			result = do_mask(redraw, 
				rerender, 
				0, 
				1,
				0);
			break;
	}



	if(!result)
	{
		switch(mwindow->edl->session->cwindow_operation)
		{
			case CWINDOW_CROP:
				result = test_crop(0, redraw);
				break;
		}
	}


// If the window is never unlocked before calling send_command the
// display shouldn't get stuck on the old video frame although it will
// flicker between the old video frame and the new video frame.

	if(redraw)
	{
		draw_refresh();
		gui->update_tool();
	}

	if(redraw_canvas)
	{
		mwindow->gui->lock_window("CWindowCanvas::cursor_motion_event 1");
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
		mwindow->gui->unlock_window();
	}

	if(rerender)
	{
		mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_PARAMS);
		gui->cwindow->playback_engine->que->send_command(CURRENT_FRAME, 
			CHANGE_NONE,
			mwindow->edl,
			1);
		if(!redraw) gui->update_tool();
	}
	return result;
}

int CWindowCanvas::button_press_event()
{
	int result = 0;
	int redraw = 0;
	int redraw_canvas = 0;
	int rerender = 0;

	if(Canvas::button_press_event()) return 1;

	gui->translating_zoom = gui->shift_down(); 

	calculate_origin();
//printf("CWindowCanvas::button_press_event 2 %f %f\n", gui->x_origin, gui->y_origin, gui->x_origin, gui->y_origin);

	float zoom_x, zoom_y, conformed_w, conformed_h;
	get_zooms(mwindow->edl, 0, zoom_x, zoom_y, conformed_w, conformed_h);
	gui->x_offset = get_x_offset(mwindow->edl, 0, zoom_x, conformed_w, conformed_h);
	gui->y_offset = get_y_offset(mwindow->edl, 0, zoom_y, conformed_w, conformed_h);

// Scroll view
	if(get_buttonpress() == 2)
	{
		gui->current_operation = CWINDOW_SCROLL;
		result = 1;
	}
	else
// Adjust parameter
	{
		switch(mwindow->edl->session->cwindow_operation)
		{
			case CWINDOW_CAMERA:
				result = test_bezier(1, redraw, redraw_canvas, rerender, 1);
				break;

			case CWINDOW_PROJECTOR:
				result = test_bezier(1, redraw, redraw_canvas, rerender, 0);
				break;

			case CWINDOW_ZOOM:
				result = test_zoom(redraw);
				break;

			case CWINDOW_CROP:
				result = test_crop(1, redraw);
				break;

			case CWINDOW_MASK:
				if(get_buttonpress() == 1)
					result = do_mask(redraw, rerender, 1, 0, 0);
				break;
		}
	}

	if(redraw)
	{
		draw_refresh();
		gui->update_tool();
	}
	return result;
}

int CWindowCanvas::button_release_event()
{
	int result = 0;

	switch(gui->current_operation)
	{
		case CWINDOW_SCROLL:
			result = 1;
			break;
	}

	gui->current_operation = CWINDOW_NONE;
	mwindow->undo->update_undo_after();
//printf("CWindowCanvas::button_release_event %d\n", result);
	return result;
}

void CWindowCanvas::zoom_resize_window(float percentage)
{
	int canvas_w, canvas_h;
	calculate_sizes(mwindow->edl->get_aspect_ratio(), 
		mwindow->edl->calculate_output_w(0), 
		mwindow->edl->calculate_output_h(0), 
		percentage,
		canvas_w,
		canvas_h);
	int new_w, new_h;
	new_w = canvas_w + (gui->get_w() - mwindow->theme->ccanvas_w);
	new_h = canvas_h + (gui->get_h() - mwindow->theme->ccanvas_h);
	gui->resize_window(new_w, new_h);
	gui->resize_event(new_w, new_h);
}
