#ifndef MBUTTONS_H
#define MBUTTONS_H

class KeyFrameButton;
class ExpandX;
class ZoomX;
class ExpandY;
class ZoomY;
class ExpandTrack;
class ZoomTrack;
class ExpandVideo;
class MainEditing;
class ZoomVideo;
class LabelButton;
class Cut;
class Copy;
class Paste;

#include "editpanel.h"
#include "guicast.h"
#include "labelnavigate.inc"
#include "mbuttons.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "playtransport.h"
#include "record.inc"




class MButtons : public BC_SubWindow
{
public:
	MButtons(MWindow *mwindow, MWindowGUI *gui);
	~MButtons();

	int create_objects();
	int resize_event();
	int keypress_event();
	int start_playback(BC_Button *button, int reverse, float speed);
	int resume_playback();
	int pause_transport(); // After frame advance
	void update();

	MWindowGUI *gui;
	MWindow *mwindow;
	PlayTransport *transport;
	LabelNavigate *labelnavigate;
	MainEditing *edit_panel;


	ExpandX *expand_x_button;
	ZoomX *zoom_x_button;

	BC_Title *y_title;
	ExpandY *expand_y_button;
	ZoomY *zoom_y_button;

	BC_Title *t_title;
	ExpandTrack *expand_t_button;
	ZoomTrack *zoom_t_button;

	BC_Title *v_title;
	ExpandVideo *expand_v_button;
	ZoomVideo *zoom_v_button;

	LabelButton *label_button;
	Cut *cut_button;
	Copy *copy_button;
	Paste *paste_button;
};

class MainTransport : public PlayTransport
{
public:
	MainTransport(MWindow *mwindow, MButtons *mbuttons, int x, int y);
	void goto_start();
	void goto_end();
};

class MainEditing : public EditPanel
{
public:
	MainEditing(MWindow *mwindow, MButtons *mbuttons, int x, int y);

	
	MWindow *mwindow;
	MButtons *mbuttons;
};

class RecButton : public BC_Button
{
public:
	RecButton(int x, int y, MWindow *mwindow);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

class ExpandX : public BC_Button
{
public:
	ExpandX(int x, int y, MWindow *mwindow);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

class ZoomX : public BC_Button
{
public:
	ZoomX(int x, int y, MWindow *mwindow);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

class ExpandY : public BC_Button
{
public:
	ExpandY(int x, int y, MWindow *mwindow);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

class ZoomY : public BC_Button
{
public:
	ZoomY(int x, int y, MWindow *mwindow);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

class ExpandTrack : public BC_Button
{
public:
	ExpandTrack(int x, int y, MWindow *mwindow);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

class ZoomTrack : public BC_Button
{
public:
	ZoomTrack(int x, int y, MWindow *mwindow);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

class ExpandVideo : public BC_Button
{
public:
	ExpandVideo(int x, int y, MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class ZoomVideo : public BC_Button
{
public:
	ZoomVideo(int x, int y, MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class LabelButton : public BC_Button
{
public:
	LabelButton(int x, int y, MWindow *mwindow);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
};

#endif
