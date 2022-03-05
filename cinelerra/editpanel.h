// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef EDITPANEL_H
#define EDITPANEL_H

#include "bcbutton.h"
#include "bctextbox.h"
#include "editpanel.inc"
#include "meterpanel.inc"
#include "manualgoto.inc"


class EditInPoint : public BC_Button
{
public:
	EditInPoint(EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	EditPanel *panel;
};

class EditOutPoint : public BC_Button
{
public:
	EditOutPoint(EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	EditPanel *panel;
};

class EditSplice : public BC_Button
{
public:
	EditSplice(EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	EditPanel *panel;
};

class EditOverwrite : public BC_Button
{
public:
	EditOverwrite(EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	EditPanel *panel;
};

class EditToClip : public BC_Button
{
public:
	EditToClip(EditPanel *panel, int x, int y);

	int handle_event();
	int keypress_event();

	EditPanel *panel;
};

class EditManualGoto : public BC_Button
{
public:
	EditManualGoto(EditPanel *panel, int x, int y);
	~EditManualGoto();

	int handle_event();
	int keypress_event();

	EditPanel *panel;
	ManualGoto *mangoto;
};

class EditCut : public BC_Button
{
public:
	EditCut(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class EditCopy : public BC_Button
{
public:
	EditCopy(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class EditPaste : public BC_Button
{
public:
	EditPaste(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class EditUndo : public BC_Button
{
public:
	EditUndo(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class EditRedo : public BC_Button
{
public:
	EditRedo(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class EditLabelbutton : public BC_Button
{
public:
	EditLabelbutton(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class EditFit : public BC_Button
{
public:
	EditFit(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class EditFitAutos : public BC_Button
{
public:
	EditFitAutos(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class EditPrevLabel : public BC_Button
{
public:
	EditPrevLabel(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class EditNextLabel : public BC_Button
{
public:
	EditNextLabel(EditPanel *panel, int x, int y);

	int keypress_event();
	int handle_event();

	EditPanel *panel;
};

class ArrowButton : public BC_Toggle
{
public:
	ArrowButton(EditPanel *panel, int x, int y);

	int handle_event();

	EditPanel *panel;
};

class IBeamButton : public BC_Toggle
{
public:
	IBeamButton(EditPanel *panel, int x, int y);

	int handle_event();

	EditPanel *panel;
};

class KeyFrameButton : public BC_Toggle
{
public:
	KeyFrameButton(int x, int y);

	int handle_event();
};

class LockLabelsButton : public BC_Toggle
{
public:
	LockLabelsButton(int x, int y);

	int handle_event();
};


class EditPanel
{
public:
	EditPanel(BC_WindowBase *subwindow,
		int x, int y, int use_flags,
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
