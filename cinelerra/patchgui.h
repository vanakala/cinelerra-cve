#ifndef PATCHGUI_H
#define PATCHGUI_H

#include "guicast.h"
#include "mwindow.inc"
#include "patchbay.inc"
#include "intauto.inc"
#include "track.inc"




class TitlePatch;
class PlayPatch;
class RecordPatch;
class AutoPatch;
class GangPatch;
class DrawPatch;
class MutePatch;
class ExpandPatch;


class PatchGUI
{
public:
	PatchGUI(MWindow *mwindow, 
		PatchBay *patchbay, 
		Track *track, 
		int x, 
		int y);
	virtual ~PatchGUI();

	virtual int create_objects();
	virtual int reposition(int x, int y);
	void toggle_behavior(int type, 
		int value,
		BC_Toggle *toggle,
		int *output);
	virtual int update(int x, int y);
	virtual void synchronize_fade(float change) {};
	void synchronize_faders(float change, int audio, int video);

	MWindow *mwindow;
	PatchBay *patchbay;
	Track *track;
	int data_type;
	int x, y;
// Don't synchronize the fader if this is true.
	int change_source;

	TitlePatch *title;
	RecordPatch *record;
	PlayPatch *play;
//	AutoPatch *automate;
	GangPatch *gang;
	DrawPatch *draw;
	MutePatch *mute;
	ExpandPatch *expand;
};



class PlayPatch : public BC_Toggle
{
public:
	PlayPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class RecordPatch : public BC_Toggle
{
public:
	RecordPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class TitlePatch : public BC_TextBox
{
public:
	TitlePatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int handle_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class AutoPatch : public BC_Toggle
{
public:
	AutoPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class GangPatch : public BC_Toggle
{
public:
	GangPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class DrawPatch : public BC_Toggle
{
public:
	DrawPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class MutePatch : public BC_Toggle
{
public:
	MutePatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	static IntAuto* get_keyframe(MWindow *mwindow, PatchGUI *patch);
	MWindow *mwindow;
	PatchGUI *patch;
};

class ExpandPatch : public BC_Toggle
{
public:
	ExpandPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};



#endif
