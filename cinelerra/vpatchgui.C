// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "autoconf.h"
#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "intauto.h"
#include "intautos.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "patchbay.h"
#include "theme.h"
#include "vpatchgui.h"
#include "vtrack.h"

#include <string.h>


VPatchGUI::VPatchGUI(MWindow *mwindow, PatchBay *patchbay, VTrack *track, int x, int y)
 : PatchGUI(mwindow, patchbay, track, x, y)
{
	data_type = TRACK_VIDEO;
	this->vtrack = track;
	mode = 0;
	fade = 0;
	update(x, y);
}

VPatchGUI::~VPatchGUI()
{
	if(fade) delete fade;
	if(mode) delete mode;
}

int VPatchGUI::reposition(int x, int y)
{
	int x1 = 0;
	int y1 = PatchGUI::reposition(x, y);

	if(fade) fade->reposition_window(fade->get_x(), 
		y1 + y);

	y1 += mwindow->theme->fade_h;

	if(mode) mode->reposition_window(mode->get_x(),
		y1 + y);

	if(nudge) nudge->reposition_window(nudge->get_x(), 
		y1 + y);

	y1 += mwindow->theme->mode_h;

	return y1;
}

int VPatchGUI::update(int x, int y)
{
	int h, x1, y1;

	patchgui_lock->lock("VPatchGUI::update");

	h = track->vertical_span(mwindow->theme);
	x1 = 0;
	y1 = PatchGUI::update(x, y);

	if(fade)
	{
		if(h - y1 < mwindow->theme->fade_h)
		{
			delete fade;
			fade = 0;
		}
		else
		{
			ptstime unit_position = master_edl->local_session->get_selectionstart(1);
			unit_position = master_edl->align_to_frame(unit_position);

			int value = (int)((FloatAutos*)vtrack->automation->autos[AUTOMATION_FADE])->get_value(
				unit_position);
			fade->update(fade->get_w(),
					value, 
					master_edl->local_session->automation_mins[AUTOGROUPTYPE_VIDEO_FADE],
					master_edl->local_session->automation_maxs[AUTOGROUPTYPE_VIDEO_FADE]);
		}
	}
	else
	if(h - y1 >= mwindow->theme->fade_h)
	{
		patchbay->add_subwindow(fade = new VFadePatch(mwindow, 
			this, 
			x1 + x, 
			y1 + y, 
			patchbay->get_w() - 10));
	}
	y1 += mwindow->theme->fade_h;

	if(mode)
	{
		if(h - y1 < mwindow->theme->mode_h)
		{
			delete mode;
			mode = 0;
			delete nudge;
			nudge = 0;
		}
		else
		{
			mode->update(mode->get_keyframe_value(mwindow, this));
			nudge->update(vtrack->nudge);
		}
	}
	else
	if(h - y1 >= mwindow->theme->mode_h)
	{
		patchbay->add_subwindow(mode = new VModePatch(mwindow, 
			this, 
			x1 + x, 
			y1 + y));
		x1 += mode->get_w() + 10;
		patchbay->add_subwindow(nudge = new NudgePatch(mwindow,
			this,
			x1 + x,
			y1 + y,
			patchbay->get_w() - x1 - 10));
	}

	y1 += mwindow->theme->mode_h;

	patchgui_lock->unlock();
	return y1;
}

void VPatchGUI::synchronize_fade(float value_change)
{
	if(fade && !change_source) 
	{
		fade->update(round(fade->get_value() + value_change));
		fade->update_edl();
	}
}


VFadePatch::VFadePatch(MWindow *mwindow, VPatchGUI *patch, int x, int y, int w)
 : BC_ISlider(x, 
		y,
		0,
		w, 
		w, 
		master_edl->local_session->automation_mins[AUTOGROUPTYPE_VIDEO_FADE],
		master_edl->local_session->automation_maxs[AUTOGROUPTYPE_VIDEO_FADE],
		(int64_t)get_keyframe_value(mwindow, patch))
{
	this->mwindow = mwindow;
	this->patch = patch;
}

float VFadePatch::update_edl()
{
	FloatAuto *current;
	ptstime position = master_edl->local_session->get_selectionstart(1);
	Autos *fade_autos = patch->vtrack->automation->autos[AUTOMATION_FADE];
	int need_undo = !fade_autos->auto_exists_for_editing(position);

	current = (FloatAuto*)fade_autos->get_auto_for_editing(position);

	float result = get_value() - current->get_value();
	current->set_value(get_value());

	mwindow->undo->update_undo(_("fade"), LOAD_AUTOMATION, need_undo ? 0 : this);

	return result;
}

int VFadePatch::handle_event()
{
	if(shift_down())
	{
		update(100);
		set_tooltip(get_caption());
	}

	patch->change_source = 1;

	float change = update_edl();

	if(patch->track->gang) 
		patch->patchbay->synchronize_faders(change, TRACK_VIDEO, patch->track);

	patch->change_source = 0;

	mwindow->sync_parameters();

	if(edlsession->auto_conf->auto_visible[AUTOMATION_FADE])
		mwindow->draw_canvas_overlays();

	return 1;
}

float VFadePatch::get_keyframe_value(MWindow *mwindow, VPatchGUI *patch)
{
	ptstime unit_position = master_edl->local_session->get_selectionstart(1);
	unit_position = master_edl->align_to_frame(unit_position);

	return ((FloatAutos*)patch->vtrack->automation->autos[AUTOMATION_FADE])->get_value(
		unit_position);
}


VModePatch::VModePatch(MWindow *mwindow, VPatchGUI *patch, int x, int y)
 : BC_PopupMenu(x, 
	y,
	patch->patchbay->mode_icons[0]->get_w() + 20,
	"",
	1,
	mwindow->theme->get_image_set("mode_popup", 0),
	10)
{
	this->mwindow = mwindow;
	this->patch = patch;
	this->mode = get_keyframe_value(mwindow, patch);
	set_icon(patch->patchbay->mode_to_icon(this->mode));
	set_tooltip(_("Overlay mode"));
	add_item(new VModePatchItem(this, _(OverlayFrame::transfer_name(TRANSFER_NORMAL)), TRANSFER_NORMAL));
	add_item(new VModePatchItem(this, _(OverlayFrame::transfer_name(TRANSFER_ADDITION)), TRANSFER_ADDITION));
	add_item(new VModePatchItem(this, _(OverlayFrame::transfer_name(TRANSFER_SUBTRACT)), TRANSFER_SUBTRACT));
	add_item(new VModePatchItem(this, _(OverlayFrame::transfer_name(TRANSFER_MULTIPLY)), TRANSFER_MULTIPLY));
	add_item(new VModePatchItem(this, _(OverlayFrame::transfer_name(TRANSFER_DIVIDE)), TRANSFER_DIVIDE));
	add_item(new VModePatchItem(this, _(OverlayFrame::transfer_name(TRANSFER_REPLACE)), TRANSFER_REPLACE));
	add_item(new VModePatchItem(this, _(OverlayFrame::transfer_name(TRANSFER_MAX)), TRANSFER_MAX));
}

int VModePatch::handle_event()
{
	update(mode);

// Set keyframe
	IntAuto *current;
	ptstime position = master_edl->local_session->get_selectionstart(1);
	Autos *mode_autos = patch->vtrack->automation->autos[AUTOMATION_MODE];
	int need_undo = !mode_autos->auto_exists_for_editing(position);

	current = (IntAuto*)mode_autos->get_auto_for_editing(position);
	current->value = mode;

	mwindow->undo->update_undo(_("mode"), LOAD_AUTOMATION, need_undo ? 0 : this);

	mwindow->sync_parameters();

	if(edlsession->auto_conf->auto_visible[AUTOMATION_MODE])
		mwindow->draw_canvas_overlays();
	mainsession->changes_made = 1;
	return 1;
}

int VModePatch::get_keyframe_value(MWindow *mwindow, VPatchGUI *patch)
{
	Auto *current = 0;
	ptstime unit_position = master_edl->local_session->get_selectionstart(1);
	unit_position = master_edl->align_to_frame(unit_position);

	return ((IntAutos*)patch->vtrack->automation->autos[AUTOMATION_MODE])->get_value(
		unit_position);
}

void VModePatch::update(int mode)
{
	set_icon(patch->patchbay->mode_to_icon(mode));
	for(int i = 0; i < total_items(); i++)
	{
		VModePatchItem *item = (VModePatchItem*)get_item(i);
		item->set_checked(item->mode == mode);
	}
}

VModePatchItem::VModePatchItem(VModePatch *popup, const char *text, int mode)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->mode = mode;
	if(this->mode == popup->mode) set_checked(1);
}

int VModePatchItem::handle_event()
{
	popup->mode = mode;
	popup->handle_event();
	return 1;
}
