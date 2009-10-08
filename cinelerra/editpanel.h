
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef EDITPANEL_H
#define EDITPANEL_H

#include "guicast.h"
#include "meterpanel.inc"
#include "mwindow.inc"
#include "manualgoto.inc"

class EditPanel;


class EditInPoint : public BC_Button
{
public:
	EditInPoint(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditInPoint();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditOutPoint : public BC_Button
{
public:
	EditOutPoint(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditOutPoint();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditDelInPoint : public BC_Button
{
public:
	EditDelInPoint(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditDelInPoint();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditDelOutPoint : public BC_Button
{
public:
	EditDelOutPoint(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditDelOutPoint();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditSplice : public BC_Button
{
public:
	EditSplice(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditSplice();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditOverwrite : public BC_Button
{
public:
	EditOverwrite(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditOverwrite();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditLift : public BC_Button
{
public:
	EditLift(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditLift();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditExtract : public BC_Button
{
public:
	EditExtract(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditExtract();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditToClip : public BC_Button
{
public:
	EditToClip(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditToClip();
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
	~EditCut();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditCopy : public BC_Button
{
public:
	EditCopy(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditCopy();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditAppend : public BC_Button
{
public:
	EditAppend(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditAppend();

	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditInsert : public BC_Button
{
public:
	EditInsert(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditInsert();

	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditPaste : public BC_Button
{
public:
	EditPaste(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditPaste();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditTransition : public BC_Button
{
public:
	EditTransition(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditTransition();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditPresentation : public BC_Button
{
public:
	EditPresentation(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditPresentation();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditUndo : public BC_Button
{
public:
	EditUndo(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditUndo();
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditRedo : public BC_Button
{
public:
	EditRedo(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditRedo();
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditLabelbutton : public BC_Button
{
public:
	EditLabelbutton(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditLabelbutton();
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditFit : public BC_Button
{
public:
	EditFit(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditFit();
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditFitAutos : public BC_Button
{
public:
	EditFitAutos(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditFitAutos();
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
	~EditPrevLabel();

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
	~EditNextLabel();

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
		int editing_mode,   // From edl.inc
		int use_editing_mode,
		int use_keyframe, 
		int use_splice,   // Extra buttons
		int use_overwrite,
		int use_lift,
		int use_extract,
		int use_copy,  // Use copy when in EDITING_ARROW
		int use_paste, 
		int use_undo,
		int use_fit, 
		int use_locklabels,
		int use_labels,
		int use_toclip,
		int use_meters,
		int is_mwindow,
		int use_cut);
	~EditPanel();

	void set_meters(MeterPanel *meter_panel);
	void update();
	void delete_buttons();
	void create_buttons();
	void reposition_buttons(int x, int y);
	int create_objects();
	int get_w();
	virtual void copy_selection();
	virtual void splice_selection();
	virtual void overwrite_selection();
	virtual void set_inpoint();
	virtual void set_outpoint();
	virtual void clear_inpoint();
	virtual void clear_outpoint();
	virtual void to_clip();
	virtual void toggle_label();
	virtual void prev_label();
	virtual void next_label();

	MWindow *mwindow;
	BC_WindowBase *subwindow;
	MeterPanel *meter_panel;

	int use_editing_mode;
	int use_keyframe;
	int editing_mode;
	int use_splice;
	int use_overwrite;
	int use_lift;
	int use_extract;
	int use_paste;
	int use_undo;
	int use_fit;
	int use_copy;
	int use_locklabels;
	int use_labels;
	int use_toclip;
	int use_meters;
	int x, y, x1, y1;
	int is_mwindow;
	int use_cut;

	EditFit *fit;
	EditFitAutos *fit_autos;
	EditInPoint *inpoint;
	EditOutPoint *outpoint;
//	EditDelInPoint *delinpoint;
//	EditDelOutPoint *deloutpoint;
	EditSplice *splice;
	EditOverwrite *overwrite;
	EditLift *lift;
	EditExtract *extract;
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
	KeyFrameButton *keyframe;
	LockLabelsButton *locklabels;
};

#endif
