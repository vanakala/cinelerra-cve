#include "awindowgui.h"
#include "awindow.h"
#include "bcsignals.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "defaults.h"
#include "editpopup.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "keys.h"
#include "language.h"
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
#include "pluginpopup.h"
#include "samplescroll.h"
#include "statusbar.h"
#include "theme.h"
#include "trackcanvas.h"
#include "trackscroll.h"
#include "tracks.h"
#include "transitionpopup.h"
#include "vwindowgui.h"
#include "vwindow.h"
#include "zoombar.h"

// the main window uses its own private colormap for video
MWindowGUI::MWindowGUI(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Program", 
 		mwindow->session->mwindow_x, 
		mwindow->session->mwindow_y, 
		mwindow->session->mwindow_w, 
		mwindow->session->mwindow_h, 
		100,
		100,
		1,
		1,
		1)
{
	this->mwindow = mwindow;
	samplescroll = 0;
	trackscroll = 0;
	cursor = 0;
	canvas = 0;
}


MWindowGUI::~MWindowGUI()
{
	delete mbuttons;
	delete statusbar;
	delete zoombar;
	if(samplescroll) delete samplescroll;
	if(trackscroll) delete trackscroll;
	delete cursor;
	delete patchbay;
	delete timebar;
	delete mainclock;
	delete plugin_menu;
	delete transition_menu;
}

void MWindowGUI::get_scrollbars()
{
//printf("MWindowGUI::get_scrollbars 1\n");
	int64_t h_needed = mwindow->edl->get_tracks_height(mwindow->theme);
	int64_t w_needed = mwindow->edl->get_tracks_width();
	int need_xscroll = 0;
	int need_yscroll = 0;
	view_w = mwindow->theme->mcanvas_w;
	view_h = mwindow->theme->mcanvas_h;
//printf("MWindowGUI::get_scrollbars 1\n");

// Scrollbars are constitutive
	need_xscroll = need_yscroll = 1;
	view_h = mwindow->theme->mcanvas_h - BC_ScrollBar::get_span(SCROLL_VERT);
	view_w = mwindow->theme->mcanvas_w - BC_ScrollBar::get_span(SCROLL_HORIZ);

// 	for(int i = 0; i < 2; i++)
// 	{
// 		if(w_needed > view_w)
// 		{
// 			need_xscroll = 1;
// 			view_h = mwindow->theme->mcanvas_h - SCROLL_SPAN;
// 		}
// 		else
// 			need_xscroll = 0;
// 
// 		if(h_needed > view_h)
// 		{
// 			need_yscroll = 1;
// 			view_w = mwindow->theme->mcanvas_w - SCROLL_SPAN;
// 		}
// 		else
// 			need_yscroll = 0;
// 	}
//printf("MWindowGUI::get_scrollbars 1\n");

	if(canvas && (view_w != canvas->get_w() || view_h != canvas->get_h()))
	{
		canvas->reposition_window(mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y,
			view_w,
			view_h);
	}
//printf("MWindowGUI::get_scrollbars 1\n");

	if(need_xscroll)
	{
		if(!samplescroll)
			add_subwindow(samplescroll = new SampleScroll(mwindow, 
				this, 
				view_w));
		else
			samplescroll->resize_event();

		samplescroll->set_position();
	}
	else
	{
		if(samplescroll) delete samplescroll;
		samplescroll = 0;
		mwindow->edl->local_session->view_start = 0;
	}
//printf("MWindowGUI::get_scrollbars 1\n");

	if(need_yscroll)
	{
//printf("MWindowGUI::get_scrollbars 1.1 %p %p\n", this, canvas);
		if(!trackscroll)
			add_subwindow(trackscroll = new TrackScroll(mwindow, 
				this,
				view_h));
		else
			trackscroll->resize_event();


//printf("MWindowGUI::get_scrollbars 1.2\n");
		trackscroll->update_length(mwindow->edl->get_tracks_height(mwindow->theme),
			mwindow->edl->local_session->track_start,
			view_h);
//printf("MWindowGUI::get_scrollbars 1.3\n");
	}
	else
	{
		if(trackscroll) delete trackscroll;
		trackscroll = 0;
		mwindow->edl->local_session->track_start = 0;
	}
//printf("get_scrollbars 2 %d %d\n", need_xscroll, w_needed);
}

int MWindowGUI::create_objects()
{
//printf("MWindowGUI::create_objects 1\n");
	set_icon(mwindow->theme->mwindow_icon);
	
//printf("MWindowGUI::create_objects 1\n");

	cursor = 0;
	add_subwindow(mainmenu = new MainMenu(mwindow, this));
//printf("MWindowGUI::create_objects 1\n");

	mwindow->theme->get_mwindow_sizes(this, get_w(), get_h());
	mwindow->theme->draw_mwindow_bg(this);
	mainmenu->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	add_subwindow(mbuttons = new MButtons(mwindow, this));
	mbuttons->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	add_subwindow(mwindow->timebar = timebar = new MTimeBar(mwindow, 
		this,
		mwindow->theme->mtimebar_x,
 		mwindow->theme->mtimebar_y,
		mwindow->theme->mtimebar_w,
		mwindow->theme->mtimebar_h));
	timebar->create_objects();
//printf("MWindowGUI::create_objects 2\n");

	add_subwindow(mwindow->patches = patchbay = new PatchBay(mwindow, this));
	patchbay->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	get_scrollbars();

//printf("MWindowGUI::create_objects 1\n");
	mwindow->gui->add_subwindow(canvas = new TrackCanvas(mwindow, this));
	canvas->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	add_subwindow(zoombar = new ZoomBar(mwindow, this));
	zoombar->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	add_subwindow(statusbar = new StatusBar(mwindow, this));
	statusbar->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	add_subwindow(mainclock = new MainClock(mwindow, 
		mwindow->theme->mclock_x,
 		mwindow->theme->mclock_y,
		mwindow->theme->mclock_w));
	mainclock->update(0);

//printf("MWindowGUI::create_objects 1\n");

	cursor = new MainCursor(mwindow, this);
	cursor->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	add_subwindow(edit_menu = new EditPopup(mwindow, this));
	edit_menu->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	add_subwindow(plugin_menu = new PluginPopup(mwindow, this));
	plugin_menu->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	add_subwindow(transition_menu = new TransitionPopup(mwindow, this));
	transition_menu->create_objects();
//printf("MWindowGUI::create_objects 1\n");

	canvas->activate();
//printf("MWindowGUI::create_objects 1\n");
	return 0;
}

void MWindowGUI::update_title(char *path)
{
	FileSystem fs;
	char filename[BCTEXTLEN], string[BCTEXTLEN];
	fs.extract_name(filename, path);
	sprintf(string, PROGRAM_NAME ": %s", filename);
	set_title(string);
//printf("MWindowGUI::update_title %s\n", string);
	flush();
}

void MWindowGUI::redraw_time_dependancies() 
{
	zoombar->redraw_time_dependancies();
	timebar->update();
	mainclock->update(mwindow->edl->local_session->selectionstart);
}

int MWindowGUI::focus_in_event()
{
	cursor->focus_in_event();
	return 1;
}

int MWindowGUI::focus_out_event()
{
	cursor->focus_out_event();
	return 1;
}


int MWindowGUI::resize_event(int w, int h)
{
	mwindow->session->mwindow_w = w;
	mwindow->session->mwindow_h = h;
	mwindow->theme->get_mwindow_sizes(this, w, h);
	mwindow->theme->draw_mwindow_bg(this);
	flash();
	mainmenu->reposition_window(0, 0, w, mainmenu->get_h());
	mbuttons->resize_event();
	statusbar->resize_event();
	timebar->resize_event();
	patchbay->resize_event();
	zoombar->resize_event();
	get_scrollbars();
	canvas->resize_event();
	return 0;
}


void MWindowGUI::update(int scrollbars,
	int canvas,
	int timebar,
	int zoombar,
	int patchbay, 
	int clock,
	int buttonbar)
{
TRACE("MWindowGUI::update 1");
	mwindow->edl->tracks->update_y_pixels(mwindow->theme);
TRACE("MWindowGUI::update 1");
	if(scrollbars) this->get_scrollbars();
TRACE("MWindowGUI::update 1");
	if(timebar) this->timebar->update();
TRACE("MWindowGUI::update 1");
	if(zoombar) this->zoombar->update();
TRACE("MWindowGUI::update 1");
	if(patchbay) this->patchbay->update();
TRACE("MWindowGUI::update 1");
	if(clock) this->mainclock->update(mwindow->edl->local_session->selectionstart);
TRACE("MWindowGUI::update 1");
	if(canvas)
	{
		this->canvas->draw(canvas == 2);
		this->cursor->show();
		this->canvas->flash();
		this->canvas->activate();
	}
TRACE("MWindowGUI::update 1");
	if(buttonbar) mbuttons->update();
TRACE("MWindowGUI::update 100");
}

int MWindowGUI::visible(int64_t x1, int64_t x2, int64_t view_x1, int64_t view_x2)
{
	return (x1 >= view_x1 && x1 < view_x2) ||
		(x2 > view_x1 && x2 <= view_x2) ||
		(x1 <= view_x1 && x2 >= view_x2);
}


int MWindowGUI::show_message(char *message, int color)
{
//printf("MWindowGUI::show_message %s %d\n", message, color);
	statusbar->status_text->set_color(color);
	statusbar->status_text->update(message);
	return 0;
}

// Drag motion called from other window
int MWindowGUI::drag_motion()
{
	if(get_hidden()) return 0;

	canvas->drag_motion();
	return 0;
}

int MWindowGUI::drag_stop()
{
	if(get_hidden()) return 0;

	int result = canvas->drag_stop();
	return result;
}

void MWindowGUI::default_positions()
{
//printf("MWindowGUI::default_positions 1\n");
	mwindow->vwindow->gui->lock_window("MWindowGUI::default_positions");
	mwindow->cwindow->gui->lock_window("MWindowGUI::default_positions");
	mwindow->awindow->gui->lock_window("MWindowGUI::default_positions");

// printf("MWindowGUI::default_positions 1 %d %d %d %d\n", mwindow->session->vwindow_x, 
// mwindow->session->vwindow_y,
// mwindow->session->vwindow_w, 
// mwindow->session->vwindow_h);
	reposition_window(mwindow->session->mwindow_x, 
		mwindow->session->mwindow_y,
		mwindow->session->mwindow_w, 
		mwindow->session->mwindow_h);
	mwindow->vwindow->gui->reposition_window(mwindow->session->vwindow_x, 
		mwindow->session->vwindow_y,
		mwindow->session->vwindow_w, 
		mwindow->session->vwindow_h);
	mwindow->cwindow->gui->reposition_window(mwindow->session->cwindow_x, 
		mwindow->session->cwindow_y,
		mwindow->session->cwindow_w, 
		mwindow->session->cwindow_h);
	mwindow->awindow->gui->reposition_window(mwindow->session->awindow_x, 
		mwindow->session->awindow_y,
		mwindow->session->awindow_w, 
		mwindow->session->awindow_h);
//printf("MWindowGUI::default_positions 1\n");

	resize_event(mwindow->session->mwindow_w, 
		mwindow->session->mwindow_h);
//printf("MWindowGUI::default_positions 1\n");
	mwindow->vwindow->gui->resize_event(mwindow->session->vwindow_w, 
		mwindow->session->vwindow_h);
//printf("MWindowGUI::default_positions 1\n");
	mwindow->cwindow->gui->resize_event(mwindow->session->cwindow_w, 
		mwindow->session->cwindow_h);
//printf("MWindowGUI::default_positions 1\n");
	mwindow->awindow->gui->resize_event(mwindow->session->awindow_w, 
		mwindow->session->awindow_h);

//printf("MWindowGUI::default_positions 1\n");

	flush();
	mwindow->vwindow->gui->flush();
	mwindow->cwindow->gui->flush();
	mwindow->awindow->gui->flush();

	mwindow->vwindow->gui->unlock_window();
	mwindow->cwindow->gui->unlock_window();
	mwindow->awindow->gui->unlock_window();
//printf("MWindowGUI::default_positions 2\n");
}

















int MWindowGUI::repeat_event(int64_t duration)
{
	return cursor->repeat_event(duration);
}


int MWindowGUI::translation_event()
{
//printf("MWindowGUI::translation_event 1 %d %d\n", get_x(), get_y());
	mwindow->session->mwindow_x = get_x();
	mwindow->session->mwindow_y = get_y();
	return 0;
}


int MWindowGUI::save_defaults(Defaults *defaults)
{
	defaults->update("MWINDOWWIDTH", get_w());
	defaults->update("MWINDOWHEIGHT", get_h());
	mainmenu->save_defaults(defaults);
	BC_WindowBase::save_defaults(defaults);
}

int MWindowGUI::keypress_event()
{
//printf("MWindowGUI::keypress_event 1 %d\n", get_keypress());
	int result = 0;
	result = mbuttons->keypress_event();
	return result;
}


int MWindowGUI::close_event() 
{ 
	mainmenu->quit(); 
}

int MWindowGUI::menu_h()
{
	return mainmenu->get_h();
}
