// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "cplayback.h"
#include "cwindow.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainundo.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "theme.h"
#include "timebar.h"
#include "zoombar.h"
#include "manualgoto.h"


EditPanel::EditPanel(MWindow *mwindow, 
	BC_WindowBase *subwindow,
	int x, 
	int y, 
	int use_flags,
	MeterPanel *meter_panel)
{
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->use_flags = use_flags;

	this->x = x;
	this->y = y;
	arrow = 0;
	ibeam = 0;
	keyframe = 0;
	fit = 0;
	fit_autos = 0;
	locklabels = 0;
	copy = 0;
	splice = 0;
	overwrite = 0;
	cut = 0;
	paste = 0;
	labelbutton = 0;
	prevlabel = 0;
	nextlabel = 0;
	undo = 0;
	redo = 0;
	meters = 0;

	x1 = x, y1 = y;

	if(use_flags & EDTP_EDITING_MODE)
	{
		subwindow->add_subwindow(arrow = new ArrowButton(mwindow, this, x1, y1));
		x1 += arrow->get_w();
		subwindow->add_subwindow(ibeam = new IBeamButton(mwindow, this, x1, y1));
		x1 += ibeam->get_w();
		x1 += mwindow->theme->toggle_margin;
	}

	if(use_flags & EDTP_KEYFRAME)
	{
		subwindow->add_subwindow(keyframe = new KeyFrameButton(mwindow, x1, y1));
		x1 += keyframe->get_w();
	}

	if(use_flags & EDTP_LOCKLABELS)
	{
		subwindow->add_subwindow(locklabels = new LockLabelsButton(mwindow, 
			x1, 
			y1));
		x1 += locklabels->get_w();
	}

	if(use_flags & (EDTP_KEYFRAME | EDTP_LOCKLABELS))
		x1 += mwindow->theme->toggle_margin;

// Mandatory
	subwindow->add_subwindow(inpoint = new EditInPoint(mwindow, this, x1, y1));
	x1 += inpoint->get_w();
	subwindow->add_subwindow(outpoint = new EditOutPoint(mwindow, this, x1, y1));
	x1 += outpoint->get_w();

	if(use_flags & EDTP_SPLICE)
	{
		subwindow->add_subwindow(splice = new EditSplice(mwindow, this, x1, y1));
		x1 += splice->get_w();
	}

	if(use_flags & EDTP_OVERWRITE)
	{
		subwindow->add_subwindow(overwrite = new EditOverwrite(mwindow, this, x1, y1));
		x1 += overwrite->get_w();
	}

	if(use_flags & EDTP_TOCLIP)
	{
		subwindow->add_subwindow(clip = new EditToClip(mwindow, this, x1, y1));
		x1 += clip->get_w();
	}

	if(use_flags & EDTP_CUT)
	{
		subwindow->add_subwindow(cut = new EditCut(mwindow, this, x1, y1));
		x1 += cut->get_w();
	}

	if(use_flags & EDTP_COPY)
	{
		subwindow->add_subwindow(copy = new EditCopy(mwindow, this, x1, y1));
		x1 += copy->get_w();
	}

	if(use_flags & EDTP_PASTE)
	{
		subwindow->add_subwindow(paste = new EditPaste(mwindow, this, x1, y1));
		x1 += paste->get_w();
	}

	if(meter_panel)
	{
		subwindow->add_subwindow(meters = new MeterShow(mwindow, meter_panel, x1, y1));
		x1 += meters->get_w();
	}

	if(use_flags & EDTP_LABELS)
	{
		subwindow->add_subwindow(labelbutton = new EditLabelbutton(mwindow, 
			this, 
			x1, 
			y1));
		x1 += labelbutton->get_w();
		subwindow->add_subwindow(prevlabel = new EditPrevLabel(mwindow, 
			this, 
			x1, 
			y1,
			use_flags & EDTP_MWINDOW));
		x1 += prevlabel->get_w();
		subwindow->add_subwindow(nextlabel = new EditNextLabel(mwindow, 
			this, 
			x1, 
			y1,
			use_flags & EDTP_MWINDOW));
		x1 += nextlabel->get_w();
	}

	if(use_flags & EDTP_FIT)
	{
		subwindow->add_subwindow(fit = new EditFit(mwindow, this, x1, y1));
		x1 += fit->get_w();
		subwindow->add_subwindow(fit_autos = new EditFitAutos(mwindow, this, x1, y1));
		x1 += fit_autos->get_w();
	}

	if(use_flags & EDTP_UNDO)
	{
		subwindow->add_subwindow(undo = new EditUndo(mwindow, this, x1, y1));
		x1 += undo->get_w();
		subwindow->add_subwindow(redo = new EditRedo(mwindow, this, x1, y1));
		x1 += redo->get_w();
	}

	subwindow->add_subwindow(mangoto = new EditManualGoto(mwindow, this, x1, y1));
	x1 += mangoto->get_w();
}

void EditPanel::update()
{
	int new_editing_mode = edlsession->editing_mode;
	if(arrow) arrow->update(new_editing_mode == EDITING_ARROW);
	if(ibeam) ibeam->update(new_editing_mode == EDITING_IBEAM);
	if(keyframe) keyframe->update(edlsession->auto_keyframes);
	if(locklabels) locklabels->set_value(edlsession->labels_follow_edits);
	subwindow->flush();
}

void EditPanel::toggle_label()
{
	mwindow->toggle_label();
}

void EditPanel::prev_label()
{
	int shift_down = subwindow->shift_down();

	mwindow->stop_composer();

	mwindow->prev_label(shift_down);
}

void EditPanel::next_label()
{
	int shift_down = subwindow->shift_down();

	mwindow->stop_composer();

	mwindow->next_label(shift_down);
}

void EditPanel::reposition_buttons(int x, int y)
{
	this->x = x; 
	this->y = y;
	x1 = x, y1 = y;

	if(use_flags & EDTP_EDITING_MODE)
	{
		arrow->reposition_window(x1, y1);
		x1 += arrow->get_w();
		ibeam->reposition_window(x1, y1);
		x1 += ibeam->get_w();
		x1 += mwindow->theme->toggle_margin;
	}

	if(use_flags & EDTP_KEYFRAME)
	{
		keyframe->reposition_window(x1, y1);
		x1 += keyframe->get_w();
	}

	if(use_flags & EDTP_LOCKLABELS)
	{
		locklabels->reposition_window(x1,y1);
		x1 += locklabels->get_w();
	}

	if(use_flags & (EDTP_KEYFRAME | EDTP_LOCKLABELS))
		x1 += mwindow->theme->toggle_margin;

	inpoint->reposition_window(x1, y1);
	x1 += inpoint->get_w();
	outpoint->reposition_window(x1, y1);
	x1 += outpoint->get_w();

	if(use_flags & EDTP_SPLICE)
	{
		splice->reposition_window(x1, y1);
		x1 += splice->get_w();
	}

	if(use_flags & EDTP_OVERWRITE)
	{
		overwrite->reposition_window(x1, y1);
		x1 += overwrite->get_w();
	}

	if(use_flags & EDTP_TOCLIP)
	{
		clip->reposition_window(x1, y1);
		x1 += clip->get_w();
	}

	if(use_flags & EDTP_CUT)
	{
		cut->reposition_window(x1, y1);
		x1 += cut->get_w();
	}

	if(use_flags & EDTP_COPY)
	{
		copy->reposition_window(x1, y1);
		x1 += copy->get_w();
	}

	if(use_flags & EDTP_PASTE)
	{
		paste->reposition_window(x1, y1);
		x1 += paste->get_w();
	}

	if(meters)
	{
		meters->reposition_window(x1, y1);
		x1 += meters->get_w();
	}

	if(use_flags & EDTP_LABELS)
	{
		labelbutton->reposition_window(x1, y1);
		x1 += labelbutton->get_w();
		prevlabel->reposition_window(x1, y1);
		x1 += prevlabel->get_w();
		nextlabel->reposition_window(x1, y1);
		x1 += nextlabel->get_w();
	}

	if(use_flags & EDTP_FIT)
	{
		fit->reposition_window(x1, y1);
		x1 += fit->get_w();
		fit_autos->reposition_window(x1, y1);
		x1 += fit_autos->get_w();
	}

	if(use_flags & EDTP_UNDO)
	{
		undo->reposition_window(x1, y1);
		x1 += undo->get_w();
		redo->reposition_window(x1, y1);
		x1 += redo->get_w();
	}

	mangoto->reposition_window(x1, y1);
	x1 += mangoto->get_w();
}

int EditPanel::get_w()
{
	return x1 - x;
}

void EditPanel::copy_selection()
{
	mwindow->copy();
}

void EditPanel::set_inpoint()
{
	mwindow->set_inpoint();
}

void EditPanel::set_outpoint()
{
	mwindow->set_outpoint();
}

void EditPanel::to_clip()
{
	mwindow->to_clip();
}


EditInPoint::EditInPoint(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("inbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("In point ( [ )"));
}

int EditInPoint::handle_event()
{
	panel->set_inpoint();
	return 1;
}

int EditInPoint::keypress_event()
{
	if(get_keypress() == '[') 
	{
		panel->set_inpoint();
		return 1;
	}
	return 0;
}


EditOutPoint::EditOutPoint(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("outbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Out point ( ] )"));
}

int EditOutPoint::handle_event()
{
	panel->set_outpoint();
	return 1;
}

int EditOutPoint::keypress_event()
{
	if(get_keypress() == ']') 
	{
		panel->set_outpoint();
		return 1;
	}
	return 0;
}


EditNextLabel::EditNextLabel(MWindow *mwindow, 
	EditPanel *panel, 
	int x, 
	int y,
	int is_mwindow)
 : BC_Button(x, y, mwindow->theme->get_image_set("nextlabel"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	this->is_mwindow = is_mwindow;
	set_tooltip(_("Next label ( ctrl -> )"));
}

int EditNextLabel::keypress_event()
{
	if(get_keypress() == RIGHT && ctrl_down())
		return handle_event();
	return 0;
}
int EditNextLabel::handle_event()
{
	panel->next_label();
	return 1;
}

EditPrevLabel::EditPrevLabel(MWindow *mwindow, 
	EditPanel *panel, 
	int x, 
	int y,
	int is_mwindow)
 : BC_Button(x, y, mwindow->theme->get_image_set("prevlabel"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	this->is_mwindow = is_mwindow;
	set_tooltip(_("Previous label ( ctrl <- )"));
}

int EditPrevLabel::keypress_event()
{
	if(get_keypress() == LEFT && ctrl_down())
		return handle_event();
	return 0;
}

int EditPrevLabel::handle_event()
{
	panel->prev_label();
	return 1;
}


EditOverwrite::EditOverwrite(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->overwrite_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Overwrite ( b )"));
}

int EditOverwrite::handle_event()
{
	panel->overwrite_selection();
	return 1;
}

int EditOverwrite::keypress_event()
{
	if(get_keypress() == 'b')
	{
		handle_event();
		return 1;
	}
	return 0;
}


EditToClip::EditToClip(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("toclip"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("To clip ( i )"));
}

int EditToClip::handle_event()
{
	panel->to_clip();
	return 1;
}

int EditToClip::keypress_event()
{
	if(get_keypress() == 'i')
	{
		handle_event();
		return 1;
	}
	return 0;
}


EditManualGoto::EditManualGoto(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("goto"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	mangoto = new ManualGoto(mwindow, panel->subwindow);
	set_tooltip(_("Manual goto ( g )"));
}

EditManualGoto::~EditManualGoto()
{
	delete mangoto;
}

int EditManualGoto::handle_event()
{
	mangoto->open_window();
	return 1;
}

int EditManualGoto::keypress_event()
{
	if(get_keypress() == 'g')
	{
		handle_event();
		return 1;
	}
	return 0;
}


EditSplice::EditSplice(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->splice_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Splice ( v )"));
}

int EditSplice::handle_event()
{
	panel->splice_selection();
	return 1;
}

int EditSplice::keypress_event()
{
	if(get_keypress() == 'v')
	{
		handle_event();
		return 1;
	}
	return 0;
}


EditCut::EditCut(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("cut"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Cut ( x )"));
}

int EditCut::keypress_event()
{
	if(get_keypress() == 'x')
		return handle_event();
	return 0;
}

int EditCut::handle_event()
{
	mwindow->cut();
	return 1;
}


EditCopy::EditCopy(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("copy"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Copy ( c )"));
}

int EditCopy::keypress_event()
{
	if(get_keypress() == 'c')
		return handle_event();
	return 0;
}

int EditCopy::handle_event()
{
	panel->copy_selection();
	return 1;
}

EditPaste::EditPaste(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("paste"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Paste ( v )"));
}

int EditPaste::keypress_event()
{
	if(get_keypress() == 'v')
		return handle_event();
	return 0;
}

int EditPaste::handle_event()
{
	mwindow->paste();
	return 1;
}

EditUndo::EditUndo(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("undo"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Undo ( z )"));
}

int EditUndo::keypress_event()
{
	if(get_keypress() == 'z')
		return handle_event();
	return 0;
}

int EditUndo::handle_event()
{
	mwindow->undo_entry();
	return 1;
}


EditRedo::EditRedo(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("redo"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Redo ( shift Z )"));
}

int EditRedo::keypress_event()
{
	if(get_keypress() == 'Z')
		return handle_event();
	return 0;
}

int EditRedo::handle_event()
{
	mwindow->redo_entry();
	return 1;
};


EditLabelbutton::EditLabelbutton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("labelbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Toggle label at current position ( l )"));
}

int EditLabelbutton::keypress_event()
{
	if(get_keypress() == 'l')
		return handle_event();
	return 0;
}

int EditLabelbutton::handle_event()
{
	panel->toggle_label();
	return 1;
}


EditFit::EditFit(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Fit selection to display ( f )"));
}

int EditFit::keypress_event()
{
	if(!alt_down() && get_keypress() == 'f') 
	{
		handle_event();
		return 1;
	}
	return 0;
}

int EditFit::handle_event()
{
	mwindow->fit_selection();
	return 1;
}


EditFitAutos::EditFitAutos(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fitautos"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Fit all autos to display ( Alt + f )"));
}

int EditFitAutos::keypress_event()
{
	if(!ctrl_down() && alt_down() && get_keypress() == 'f') 
	{
		mwindow->fit_autos(1);
		return 1;
	}
	if(ctrl_down() && alt_down() && get_keypress() == 'f') 
	{
		mwindow->fit_autos(0);
		return 1;
	}
	return 0;
}

int EditFitAutos::handle_event()
{
	mwindow->fit_autos(1);
	return 1;
}


ArrowButton::ArrowButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, 
	y, 
	mwindow->theme->get_image_set("arrow"),
	edlsession->editing_mode == EDITING_ARROW,
	"",
	0,
	0,
	0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Drag and drop editing mode"));
}

int ArrowButton::handle_event()
{
	update(1);
	panel->ibeam->update(0);
	mwindow->set_editing_mode(EDITING_ARROW);
// Nothing after this
	return 1;
}


IBeamButton::IBeamButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, 
	y, 
	mwindow->theme->get_image_set("ibeam"),
	edlsession->editing_mode == EDITING_IBEAM,
	"",
	0,
	0,
	0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Cut and paste editing mode"));
}

int IBeamButton::handle_event()
{
	update(1);
	panel->arrow->update(0);
	mwindow->set_editing_mode(EDITING_IBEAM);
// Nothing after this
	return 1;
}


KeyFrameButton::KeyFrameButton(MWindow *mwindow, int x, int y)
 : BC_Toggle(x, 
	y,
	mwindow->theme->get_image_set("autokeyframe"),
	edlsession->auto_keyframes,
	"",
	0,
	0,
	0)
{
	this->mwindow = mwindow;
	set_tooltip(_("Generate keyframes while tweaking"));
}

int KeyFrameButton::handle_event()
{
	mwindow->set_auto_keyframes(get_value());
	return 1;
}


LockLabelsButton::LockLabelsButton(MWindow *mwindow, int x, int y)
 : BC_Toggle(x, 
	y,
	mwindow->theme->get_image_set("locklabels"),
	edlsession->labels_follow_edits,
	"",
	0,
	0,
	0)
{
	this->mwindow = mwindow;
	set_tooltip(_("Lock labels from moving"));
}

int LockLabelsButton::handle_event()
{
	mwindow->set_labels_follow_edits(get_value());
	return 1;
}
