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


EditPanel::EditPanel(BC_WindowBase *subwindow,
	int x, int y, int use_flags,
	MeterPanel *meter_panel)
{
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
		subwindow->add_subwindow(arrow = new ArrowButton(this, x1, y1));
		x1 += arrow->get_w();
		subwindow->add_subwindow(ibeam = new IBeamButton(this, x1, y1));
		x1 += ibeam->get_w();
		x1 += theme_global->toggle_margin;
	}

	if(use_flags & EDTP_KEYFRAME)
	{
		subwindow->add_subwindow(keyframe = new KeyFrameButton(x1, y1));
		x1 += keyframe->get_w();
	}

	if(use_flags & EDTP_LOCKLABELS)
	{
		subwindow->add_subwindow(locklabels = new LockLabelsButton(x1, y1));
		x1 += locklabels->get_w();
	}

	if(use_flags & (EDTP_KEYFRAME | EDTP_LOCKLABELS))
		x1 += theme_global->toggle_margin;

// Mandatory
	subwindow->add_subwindow(inpoint = new EditInPoint(this, x1, y1));
	x1 += inpoint->get_w();
	subwindow->add_subwindow(outpoint = new EditOutPoint(this, x1, y1));
	x1 += outpoint->get_w();

	if(use_flags & EDTP_SPLICE)
	{
		subwindow->add_subwindow(splice = new EditSplice(this, x1, y1));
		x1 += splice->get_w();
	}

	if(use_flags & EDTP_OVERWRITE)
	{
		subwindow->add_subwindow(overwrite = new EditOverwrite(this, x1, y1));
		x1 += overwrite->get_w();
	}

	if(use_flags & EDTP_TOCLIP)
	{
		subwindow->add_subwindow(clip = new EditToClip(this, x1, y1));
		x1 += clip->get_w();
	}

	if(use_flags & EDTP_CUT)
	{
		subwindow->add_subwindow(cut = new EditCut(this, x1, y1));
		x1 += cut->get_w();
	}

	if(use_flags & EDTP_COPY)
	{
		subwindow->add_subwindow(copy = new EditCopy(this, x1, y1));
		x1 += copy->get_w();
	}

	if(use_flags & EDTP_PASTE)
	{
		subwindow->add_subwindow(paste = new EditPaste(this, x1, y1));
		x1 += paste->get_w();
	}

	if(meter_panel)
	{
		subwindow->add_subwindow(meters = new MeterShow(meter_panel, x1, y1));
		x1 += meters->get_w();
	}

	if(use_flags & EDTP_LABELS)
	{
		subwindow->add_subwindow(labelbutton = new EditLabelbutton(this,
			x1, y1));
		x1 += labelbutton->get_w();
		subwindow->add_subwindow(prevlabel = new EditPrevLabel(this,
			x1, y1));
		x1 += prevlabel->get_w();
		subwindow->add_subwindow(nextlabel = new EditNextLabel(this,
			x1, y1));
		x1 += nextlabel->get_w();
	}

	if(use_flags & EDTP_FIT)
	{
		subwindow->add_subwindow(fit = new EditFit(this, x1, y1));
		x1 += fit->get_w();
		subwindow->add_subwindow(fit_autos = new EditFitAutos(this, x1, y1));
		x1 += fit_autos->get_w();
	}

	if(use_flags & EDTP_UNDO)
	{
		subwindow->add_subwindow(undo = new EditUndo(this, x1, y1));
		x1 += undo->get_w();
		subwindow->add_subwindow(redo = new EditRedo(this, x1, y1));
		x1 += redo->get_w();
	}

	subwindow->add_subwindow(mangoto = new EditManualGoto(this, x1, y1));
	x1 += mangoto->get_w();
}

void EditPanel::update()
{
	int new_editing_mode = edlsession->editing_mode;

	if(arrow)
		arrow->update(new_editing_mode == EDITING_ARROW);
	if(ibeam)
		ibeam->update(new_editing_mode == EDITING_IBEAM);
	if(keyframe)
		keyframe->update(edlsession->auto_keyframes);
	if(locklabels)
		locklabels->set_value(edlsession->labels_follow_edits);
}

void EditPanel::toggle_label()
{
	mwindow_global->toggle_label();
}

void EditPanel::prev_label()
{
	int shift_down = subwindow->shift_down();

	mwindow_global->stop_composer();

	mwindow_global->prev_label(shift_down);
}

void EditPanel::next_label()
{
	int shift_down = subwindow->shift_down();

	mwindow_global->stop_composer();

	mwindow_global->next_label(shift_down);
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
		x1 += theme_global->toggle_margin;
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
		x1 += theme_global->toggle_margin;

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
	mwindow_global->copy();
}

void EditPanel::set_inpoint()
{
	mwindow_global->set_inpoint();
}

void EditPanel::set_outpoint()
{
	mwindow_global->set_outpoint();
}

void EditPanel::to_clip()
{
	mwindow_global->to_clip();
}


EditInPoint::EditInPoint(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("inbutton"))
{
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


EditOutPoint::EditOutPoint(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("outbutton"))
{
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


EditNextLabel::EditNextLabel(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("nextlabel"))
{
	this->panel = panel;
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


EditPrevLabel::EditPrevLabel(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("prevlabel"))
{
	this->panel = panel;
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


EditOverwrite::EditOverwrite(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->overwrite_data)
{
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


EditToClip::EditToClip(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("toclip"))
{
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


EditManualGoto::EditManualGoto(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("goto"))
{
	this->panel = panel;
	mangoto = new ManualGoto(panel->subwindow);
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


EditSplice::EditSplice(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->splice_data)
{
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


EditCut::EditCut(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("cut"))
{
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
	mwindow_global->cut();
	return 1;
}


EditCopy::EditCopy(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("copy"))
{
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

EditPaste::EditPaste(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("paste"))
{
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
	mwindow_global->paste();
	return 1;
}

EditUndo::EditUndo(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("undo"))
{
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
	mwindow_global->undo_entry();
	return 1;
}


EditRedo::EditRedo(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("redo"))
{
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
	mwindow_global->redo_entry();
	return 1;
};


EditLabelbutton::EditLabelbutton(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("labelbutton"))
{
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


EditFit::EditFit(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("fit"))
{
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
	mwindow_global->fit_selection();
	return 1;
}


EditFitAutos::EditFitAutos(EditPanel *panel, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("fitautos"))
{
	this->panel = panel;
	set_tooltip(_("Fit all autos to display ( Alt + f )"));
}

int EditFitAutos::keypress_event()
{
	if(!ctrl_down() && alt_down() && get_keypress() == 'f') 
	{
		mwindow_global->fit_autos(1);
		return 1;
	}
	if(ctrl_down() && alt_down() && get_keypress() == 'f') 
	{
		mwindow_global->fit_autos(0);
		return 1;
	}
	return 0;
}

int EditFitAutos::handle_event()
{
	mwindow_global->fit_autos(1);
	return 1;
}


ArrowButton::ArrowButton(EditPanel *panel, int x, int y)
 : BC_Toggle(x, 
	y, 
	theme_global->get_image_set("arrow"),
	edlsession->editing_mode == EDITING_ARROW,
	"",
	0,
	0,
	0)
{
	this->panel = panel;
	set_tooltip(_("Drag and drop editing mode"));
}

int ArrowButton::handle_event()
{
	update(1);
	panel->ibeam->update(0);
	mwindow_global->set_editing_mode(EDITING_ARROW);
// Nothing after this
	return 1;
}


IBeamButton::IBeamButton(EditPanel *panel, int x, int y)
 : BC_Toggle(x, 
	y, 
	theme_global->get_image_set("ibeam"),
	edlsession->editing_mode == EDITING_IBEAM,
	"",
	0,
	0,
	0)
{
	this->panel = panel;
	set_tooltip(_("Cut and paste editing mode"));
}

int IBeamButton::handle_event()
{
	update(1);
	panel->arrow->update(0);
	mwindow_global->set_editing_mode(EDITING_IBEAM);
// Nothing after this
	return 1;
}


KeyFrameButton::KeyFrameButton(int x, int y)
 : BC_Toggle(x, 
	y,
	theme_global->get_image_set("autokeyframe"),
	edlsession->auto_keyframes,
	"",
	0,
	0,
	0)
{
	set_tooltip(_("Generate keyframes while tweaking"));
}

int KeyFrameButton::handle_event()
{
	mwindow_global->set_auto_keyframes(get_value());
	return 1;
}


LockLabelsButton::LockLabelsButton(int x, int y)
 : BC_Toggle(x, 
	y,
	theme_global->get_image_set("locklabels"),
	edlsession->labels_follow_edits,
	"",
	0,
	0,
	0)
{
	set_tooltip(_("Lock labels from moving"));
}

int LockLabelsButton::handle_event()
{
	mwindow_global->set_labels_follow_edits(get_value());
	return 1;
}
