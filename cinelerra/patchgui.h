// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PATCHGUI_H
#define PATCHGUI_H

#include "bctoggle.h"
#include "bctextbox.h"
#include "datatype.h"
#include "mutex.h"
#include "mwindow.inc"
#include "patchbay.inc"
#include "intauto.inc"
#include "track.inc"


class TitlePatch;
class PlayPatch;
class RecordPatch;
class GangPatch;
class DrawPatch;
class MutePatch;
class ExpandPatch;
class NudgePatch;
class MasterTrackPatch;

class PatchGUI
{
public:
	PatchGUI(PatchBay *patchbay,
		Track *track,
		int x,
		int y);
	virtual ~PatchGUI();

	virtual int reposition(int x, int y);
	void toggle_behavior(int type, 
		int value,
		BC_Toggle *toggle,
		int *output);
	void toggle_master(int value, MasterTrackPatch *toggle, int *output);
	virtual int update(int x, int y);
	void synchronize_faders(float change, int audio, int video);

	PatchBay *patchbay;
	Track *track;
// Used by update routines so non-existent track doesn't need to be dereferenced
// to know it doesn't match the current EDL.
	int track_id;
	int data_type;
	int x, y;
// Don't synchronize the fader if this is true.
	int change_source;

	TitlePatch *title;
	MasterTrackPatch *master;
	RecordPatch *record;
	PlayPatch *play;
	GangPatch *gang;
	DrawPatch *draw;
	MutePatch *mute;
	ExpandPatch *expand;
	NudgePatch *nudge;
	char string_return[BCTEXTLEN];
protected:
	Mutex *patchgui_lock;
};


class PlayPatch : public BC_Toggle
{
public:
	PlayPatch(PatchGUI *patch, int x, int y);

	int handle_event();
	int button_release_event();

	PatchGUI *patch;
};

class RecordPatch : public BC_Toggle
{
public:
	RecordPatch(PatchGUI *patch, int x, int y);

	int handle_event();
	int button_release_event();

	PatchGUI *patch;
};

class TitlePatch : public BC_TextBox
{
public:
	TitlePatch(PatchGUI *patch, int x, int y);

	int handle_event();

	PatchGUI *patch;
};

class GangPatch : public BC_Toggle
{
public:
	GangPatch(PatchGUI *patch, int x, int y);

	int handle_event();
	int button_release_event();

	PatchGUI *patch;
};

class DrawPatch : public BC_Toggle
{
public:
	DrawPatch(PatchGUI *patch, int x, int y);

	int handle_event();
	int button_release_event();

	PatchGUI *patch;
};

class MutePatch : public BC_Toggle
{
public:
	MutePatch(PatchGUI *patch, int x, int y);

	int handle_event();
	int button_release_event();
	static int get_keyframe_value(PatchGUI *patch);

	PatchGUI *patch;
};

class MasterTrackPatch : public BC_Toggle
{
public:
	MasterTrackPatch(PatchGUI *patch, int x, int y);

	int handle_event();

	PatchGUI *patch;
};

class ExpandPatch : public BC_Toggle
{
public:
	ExpandPatch(PatchGUI *patch, int x, int y);

	int handle_event();
	int button_release_event();

	PatchGUI *patch;
};

class NudgePatch : public BC_TextBox
{
public:
	NudgePatch(PatchGUI *patch, int x, int y, int w);

	int handle_event();
	int button_press_event();
	void set_value(ptstime value);

	PatchGUI *patch;
};


#endif
