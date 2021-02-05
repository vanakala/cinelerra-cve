// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef EDITPANEL_H
#define EDITPANEL_H

#include "bcbutton.h"
#include "bctextbox.h"
#include "editpanel.inc"
#include "meterpanel.inc"
#include "mwindow.inc"
#include "manualgoto.inc"


class EditInPoint : public BC_Button
{
public:
	EditInPoint(MWindow *mwindow, EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditOutPoint : public BC_Button
{
public:
	EditOutPoint(MWindow *mwindow, EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditSplice : public BC_Button
{
public:
	EditSplice(MWindow *mwindow, EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditOverwrite : public BC_Button
{
public:
	EditOverwrite(MWindow *mwindow, EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditToClip : public BC_Button
{
public:
	EditToClip(MWindow *mwindow, EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditManualGoto : public BC_Button
{
public:
	EditManualGoto(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditManualGoto();

	int handle_event();
	int keypress_event();

	MWindow *mwindow;
	EditPanel *panel;
	ManualGoto *mangoto;
};

class EditCut : public BC_Button
{
public:
	EditCut(MWindow *mwindow, EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditCopy : public BC_Button
{
public:
	EditCopy(MWindow *mwindow, EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditPaste : public BC_Button
{
public:
	EditPaste(MWindow *mwindow, EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditUndo : public BC_Button
{
public:
	EditUndo(MWindow *mwindow, EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditRedo : public BC_Button
{
public:
	EditRedo(MWindow *mwindow, EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditLabelbutton : public BC_Button
{
public:
	EditLabelbutton(MWindow *mwindow, EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditFit : public BC_Button
{
public:
	EditFit(MWindow *mwindow, EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditFitAutos : public BC_Button
{
public:
	EditFitAutos(MWindow *mwindow, EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditPrevLabel : public BC_Button
{
public:
	EditPrevLabel(MWindow *mwindow, 
		EditPanel *panel, 
		int x, 
		int y,
		int is_mwindow);

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
	int is_mwindow;
};

class EditNextLabel : public BC_Button
{
public:
	EditNextLabel(MWindow *mwindow, 
		EditPanel *panel, 
		int x, 
		int y,
		int is_mwindow);

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
	int is_mwindow;
};

class ArrowButton : public BC_Toggle
{
public:
	ArrowButton(MWindow *mwindow, EditPanel *panel, int x, int y);

	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class IBeamButton : public BC_Toggle
{
public:
	IBeamButton(MWindow *mwindow, EditPanel *panel, int x, int y);

	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class KeyFrameButton : public BC_Toggle
{
public:
	KeyFrameButton(MWindow *mwindow, int x, int y);

	int handle_event();

	MWindow *mwindow;
};

class LockLabelsButton : public BC_Toggle
{
public:
	LockLabelsButton(MWindow *mwindow, int x, int y);

	int handle_event();

	MWindow *mwindow;
};


class EditPanel
{
public:
	EditPanel(MWindow *mwindow, 
		BC_WindowBase *subwindow,
		int x, 
		int y, 
		int use_flags,
		MeterPanel *meter_panel);

	friend class IBeamButton;
	friend class ArrowButton;

	void update();
	void reposition_buttons(int x, int y);
	int get_w();
	virtual void copy_selection();
	virtual void splice_selection() {};
	virtual void overwrite_selection() {};
	virtual void set_inpoint();
	virtual void set_outpoint();
	virtual void to_clip();
	virtual void toggle_label();
	virtual void prev_label();
	virtual void next_label();

	BC_WindowBase *subwindow;
	KeyFrameButton *keyframe;
	LockLabelsButton *locklabels;

private:
	MWindow *mwindow;
	int use_flags;

	int x, y, x1, y1;

	EditFit *fit;
	EditFitAutos *fit_autos;
	EditInPoint *inpoint;
	EditOutPoint *outpoint;
	EditSplice *splice;
	EditOverwrite *overwrite;
	EditToClip *clip;
	EditManualGoto *mangoto;
	EditCut *cut;
	EditCopy *copy;
	EditPaste *paste;
	EditLabelbutton *labelbutton;
	EditPrevLabel *prevlabel;
	EditNextLabel *nextlabel;
	EditUndo *undo;
	EditRedo *redo;
	MeterShow *meters;
	ArrowButton *arrow;
	IBeamButton *ibeam;
};

#endif
