#include "awindowgui.h"
#include "awindow.h"
#include "bcsignals.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "bchash.h"
#include "editpopup.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "keyframepopup.h"
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
	delete edit_menu;
	delete plugin_menu;
	delete keyframe_menu;
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
	view_h = mwindow->theme->mcanvas_h;
	view_w = mwindow->theme->mcanvas_w;

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
				mwindow->theme->mhscroll_x, 
				mwindow->theme->mhscroll_y, 
				mwindow->theme->mhscroll_w));
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
				mwindow->theme->mvscroll_x,
				mwindow->theme->mvscroll_y,
				mwindow->theme->mvscroll_h));
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
SET_TRACE
	set_icon(mwindow->theme->get_image("mwindow_icon"));
	
SET_TRACE

	cursor = 0;
	add_subwindow(mainmenu = new MainMenu(mwindow, this));
SET_TRACE

	mwindow->theme->get_mwindow_sizes(this, get_w(), get_h());
SET_TRACE
	mwindow->theme->draw_mwindow_bg(this);
SET_TRACE
	mainmenu->create_objects();
SET_TRACE

	add_subwindow(mbuttons = new MButtons(mwindow, this));
	mbuttons->create_objects();
SET_TRACE

	add_subwindow(timebar = new MTimeBar(mwindow, 
		this,
		mwindow->theme->mtimebar_x,
 		mwindow->theme->mtimebar_y,
		mwindow->theme->mtimebar_w,
		mwindow->theme->mtimebar_h));
	timebar->create_objects();
SET_TRACE

	add_subwindow(patchbay = new PatchBay(mwindow, this));
	patchbay->create_objects();
SET_TRACE

	get_scrollbars();

SET_TRACE
	mwindow->gui->add_subwindow(canvas = new TrackCanvas(mwindow, this));
	canvas->create_objects();
SET_TRACE

	add_subwindow(zoombar = new ZoomBar(mwindow, this));
	zoombar->create_objects();
SET_TRACE

	add_subwindow(statusbar = new StatusBar(mwindow, this));
	statusbar->create_objects();
SET_TRACE

	add_subwindow(mainclock = new MainClock(mwindow, 
		mwindow->theme->mclock_x,
 		mwindow->theme->mclock_y,
		mwindow->theme->mclock_w));
	mainclock->set_frame_offset( (double) 
		(mwindow->edl->session->get_frame_offset() /
		mwindow->edl->session->frame_rate));
	mainclock->update(0);

SET_TRACE

	cursor = new MainCursor(mwindow, this);
	cursor->create_objects();
SET_TRACE

	add_subwindow(edit_menu = new EditPopup(mwindow, this));
	edit_menu->create_objects();
SET_TRACE

	add_subwindow(plugin_menu = new PluginPopup(mwindow, this));
	plugin_menu->create_objects();
SET_TRACE

	add_subwindow(keyframe_menu = new KeyframePopup(mwindow, this));
	keyframe_menu->create_objects();

	add_subwindow(transition_menu = new TransitionPopup(mwindow, this));
	transition_menu->create_objects();
SET_TRACE

	canvas->activate();
SET_TRACE
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
	mainclock->update(mwindow->edl->local_session->get_selectionstart(1));
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
	mwindow->edl->tracks->update_y_pixels(mwindow->theme);
	if(scrollbars) this->get_scrollbars();
	if(timebar) this->timebar->update();
	if(zoombar) this->zoombar->update();
	if(patchbay) this->patchbay->update();
	if(clock) this->mainclock->update(
		mwindow->edl->local_session->get_selectionstart(1));
	if(canvas)
	{
		this->canvas->draw(canvas);
		this->cursor->show();
		this->canvas->flash();
// Activate causes the menubar to deactivate.  Don't want this for
// picon thread.
		if(canvas != 3) this->canvas->activate();
	}
	if(buttonbar) mbuttons->update();

// Can't age if the cache called this to draw missing picons
	if(canvas != 2 && canvas != 3)
		mwindow->age_caches();
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
	if(color < 0) color = mwindow->theme->message_normal;
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
// if(duration == 100)
// mwindow->sync_parameters(CHANGE_ALL);
	return cursor->repeat_event(duration);
}


int MWindowGUI::translation_event()
{
//printf("MWindowGUI::translation_event 1 %d %d\n", get_x(), get_y());
	mwindow->session->mwindow_x = get_x();
	mwindow->session->mwindow_y = get_y();
	return 0;
}


int MWindowGUI::save_defaults(BC_Hash *defaults)
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

	if(!result)
	{
		switch(get_keypress())
		{
			case 'e':
				mwindow->toggle_editing_mode();
				result = 1;
				break;
			case LEFT:
				if(!ctrl_down()) 
				{ 
					if (alt_down())
					{
						unlock_window();
						mbuttons->transport->handle_transport(STOP, 1, 0, 0);
						lock_window("MWindowGUI::keypress_event 1");
						mwindow->prev_edit_handle(shift_down());
					}
					else
						mwindow->move_left(); 
 					result = 1; 
				}
				break;
			case RIGHT:
				if(!ctrl_down()) 
				{ 
					if (alt_down())
					{
						unlock_window();
						mbuttons->transport->handle_transport(STOP, 1, 0, 0);
						lock_window("MWindowGUI::keypress_event 2");
						mwindow->next_edit_handle(shift_down());
					}
					else
						mwindow->move_right(); 
					result = 1; 
				}
				break;

			case UP:
				if(ctrl_down() && !alt_down())
				{
					mwindow->expand_y();
					result = 1;
				}
				else
				if(!ctrl_down() && alt_down())
				{
					mwindow->expand_autos(0,1,1);
					result = 1;
				}
				else
				if(ctrl_down() && alt_down())
				{
					mwindow->expand_autos(1,1,1);
					result = 1;
				}
				else
				{
					mwindow->expand_sample();
					result = 1;
				}
				break;

			case DOWN:
				if(ctrl_down() && !alt_down())
				{
					mwindow->zoom_in_y();
					result = 1;
				}
				else
				if(!ctrl_down() && alt_down())
				{
					mwindow->shrink_autos(0,1,1);
					result = 1;
				}
				else
				if(ctrl_down() && alt_down())
				{
					mwindow->shrink_autos(1,1,1);
					result = 1;
				}
				else
				{
					mwindow->zoom_in_sample();
					result = 1;
				}
				break;

			case PGUP:
				if(!ctrl_down())
				{
					mwindow->move_up();
					result = 1;
				}
				else
				{
					mwindow->expand_t();
					result = 1;
				}
				break;

			case PGDN:
				if(!ctrl_down())
				{
					mwindow->move_down();
					result = 1;
				}
				else
				{
					mwindow->zoom_in_t();
					result = 1;
				}
				break;

			case TAB:
			case LEFTTAB:
				int cursor_x = canvas->get_relative_cursor_x();
				int cursor_y = canvas->get_relative_cursor_y();

				if(get_keypress() == TAB)
				{
// Switch the record button
					for(Track *track = mwindow->edl->tracks->first; track; track = track->next)
					{
						int64_t track_x, track_y, track_w, track_h;
						canvas->track_dimensions(track, track_x, track_y, track_w, track_h);

						if(cursor_y >= track_y && 
							cursor_y < track_y + track_h)
						{
							if (track->record)
								track->record = 0;
							else
								track->record = 1;
							result = 1; 
							break;
						}
					}
				} 
				else 
				{
					Track *this_track = 0;
					for(Track *track = mwindow->edl->tracks->first; track; track = track->next)
					{
						int64_t track_x, track_y, track_w, track_h;
						canvas->track_dimensions(track, track_x, track_y, track_w, track_h);

						if(cursor_y >= track_y && 
							cursor_y < track_y + track_h)
						{
							// This is our track
							this_track = track;
							break;
						}
					}

					int total_selected = mwindow->edl->tracks->total_of(Tracks::RECORD);

	// nothing previously selected
					if(total_selected == 0)
					{
						mwindow->edl->tracks->select_all(Tracks::RECORD,
							1);
					}
					else
					if(total_selected == 1)
					{
	// this patch was previously the only one on
						if(this_track && this_track->record)
						{
							mwindow->edl->tracks->select_all(Tracks::RECORD,
								1);
						}
	// another patch was previously the only one on
						else
						{
							mwindow->edl->tracks->select_all(Tracks::RECORD,
								0);
							if (this_track) 
								this_track->record = 1;

						}
					}
					else
					if(total_selected > 1)
					{
						mwindow->edl->tracks->select_all(Tracks::RECORD,
							0);
						if (this_track) 
							this_track->record = 1;
					}

				}

				update (0,
						1,
						0,
						0,
						1,
						0,
						1);
				unlock_window();
				mwindow->cwindow->update(0, 1, 1);
				lock_window("TrackCanvas::keypress_event 3");

				result = 1;
				break;
		}

// since things under cursor have changed...
		if(result) 
			cursor_motion_event(); 
	}

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
