#include "cplayback.h"
#include "cwindow.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keys.h"
#include "labelnavigate.h"
#include "localsession.h"
#include "mbuttons.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "record.h"
#include "mainsession.h"
#include "theme.h"
#include "tracks.h"

MButtons::MButtons(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mbuttons_x, 
 	mwindow->theme->mbuttons_y, 
	mwindow->theme->mbuttons_w, 
	mwindow->theme->mbuttons_h)
{
	this->gui = gui;
	this->mwindow = mwindow; 
}

MButtons::~MButtons()
{
	delete transport;
	delete edit_panel;
}

int MButtons::create_objects()
{
	int x = 3, y = 0;
	BC_SubWindow *button;

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	transport = new MainTransport(mwindow, this, x, y);
	transport->create_objects();
	transport->set_engine(mwindow->cwindow->playback_engine);
	x += transport->get_w();

	edit_panel = new MainEditing(mwindow, this, x, y);

	edit_panel->create_objects();
	
	x += edit_panel->get_w();
	flash();
	return 0;
}

int MButtons::resize_event()
{
	reposition_window(mwindow->theme->mbuttons_x, 
 		mwindow->theme->mbuttons_y, 
		mwindow->theme->mbuttons_w, 
		mwindow->theme->mbuttons_h);
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	flash();
}

int MButtons::keypress_event()
{
	int result = 0;
	result = transport->keypress_event();
	if(!result)
	{
	}

//printf("MButtons::keypress_event %d\n", result);
	return result;
}

void MButtons::update()
{
	edit_panel->update();
}

MainTransport::MainTransport(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : PlayTransport(mwindow, mbuttons, x, y)
{
}

void MainTransport::goto_start()
{
	mwindow->gui->unlock_window();
	handle_transport(REWIND, 1);
	mwindow->gui->lock_window();
	mwindow->goto_start();
}


void MainTransport::goto_end()
{
	mwindow->gui->unlock_window();
	handle_transport(GOTO_END, 1);
	mwindow->gui->lock_window();
	mwindow->goto_end();
}

MainEditing::MainEditing(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : EditPanel(mwindow, 
		mbuttons, 
		x, 
		y,
		mwindow->edl->session->editing_mode,
		1,
		1,
		0, 
		0, 
		1, 
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		0,
		1,
		1)
{
	this->mwindow = mwindow;
	this->mbuttons = mbuttons;
}









RecButton::RecButton(int x, int y, MWindow *mwindow)
 : BC_Button(x, y, mwindow->theme->rec_data) 
{ this->mwindow = mwindow; }
int RecButton::handle_event()
{
//printf("Rec pressed\n");
}
int RecButton::keypress_event()
{
	if(get_keypress() == 18) { handle_event(); return 1; }
	return 0;
}


ExpandX::ExpandX(int x, int y, MWindow *mwindow)
 : BC_Button(x, y, mwindow->theme->rec_data) 
{ this->mwindow = mwindow; }
int ExpandX::handle_event()
{
	mwindow->expand_sample(); 
}
int ExpandX::keypress_event()
{
	if(get_keypress() == UP && !ctrl_down()) { handle_event(); return 1; }
	return 0;
}


ZoomX::ZoomX(int x, int y, MWindow *mwindow)
 : BC_Button(x, y, mwindow->theme->rec_data)
{ this->mwindow = mwindow; }
int ZoomX::handle_event()
{
	mwindow->zoom_in_sample(); 
}
int ZoomX::keypress_event()
{
	if(get_keypress() == DOWN && !ctrl_down()) { handle_event(); return 1; }
	return 0;
}


ExpandY::ExpandY(int x, int y, MWindow *mwindow)
 : BC_Button(x, y, mwindow->theme->rec_data) 
{ this->mwindow = mwindow; }
int ExpandY::handle_event()
{
	mwindow->expand_y();
}
int ExpandY::keypress_event()
{
	if(get_keypress() == UP && ctrl_down()) { handle_event(); return 1; }
	return 0;
}


ZoomY::ZoomY(int x, int y, MWindow *mwindow)
 : BC_Button(x, y, mwindow->theme->rec_data) 
{ this->mwindow = mwindow; }
int ZoomY::handle_event()
{
	mwindow->zoom_in_y();
}
int ZoomY::keypress_event()
{ 
	if(get_keypress() == DOWN && ctrl_down()) { handle_event(); return 1; }
	return 0;
}


ExpandTrack::ExpandTrack(int x, int y, MWindow *mwindow)
 : BC_Button(x, y, mwindow->theme->rec_data) 
{ this->mwindow = mwindow; }
int ExpandTrack::handle_event()
{	
	mwindow->expand_t();
}
int ExpandTrack::keypress_event()
{ 
	if(get_keypress() == PGUP && ctrl_down()) { handle_event(); return 1; }
	return 0;
}


ZoomTrack::ZoomTrack(int x, int y, MWindow *mwindow)
 : BC_Button(x, y, mwindow->theme->rec_data) 
{ 
	this->mwindow = mwindow; 
}
int ZoomTrack::handle_event()
{
	mwindow->zoom_in_t();
}
int ZoomTrack::keypress_event()
{ 
	if(get_keypress() == PGDN && ctrl_down()) { handle_event(); return 1; }
	return 0;
}
