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


VPatchGUI::VPatchGUI(PatchBay *patchbay, VTrack *track, int x, int y)
 : PatchGUI(patchbay, track, x, y)
{
	data_type = TRACK_VIDEO;
	this->vtrack = track;
	mode = 0;
	fade = 0;
	update(x, y);
}

VPatchGUI::~VPatchGUI()
{
	delete fade;
	delete mode;
}

int VPatchGUI::reposition(int x, int y)
{
	int x1 = 0;
	int y1 = PatchGUI::reposition(x, y);

	if(fade)
		fade->reposition_window(fade->get_x(), y1 + y);

	y1 += theme_global->fade_h;

	if(mode)
		mode->reposition_window(mode->get_x(), y1 + y);

	if(nudge)
		nudge->reposition_window(nudge->get_x(), y1 + y);

	y1 += theme_global->mode_h;

	return y1;
}

int VPatchGUI::update(int x, int y)
{
	int h, x1, y1;

	patchgui_lock->lock("VPatchGUI::update");

	h = track->vertical_span(theme_global);
	x1 = 0;
	y1 = PatchGUI::update(x, y);

	if(fade)
	{
		if(h - y1 < theme_global->fade_h)
		{
			delete fade;
			fade = 0;
		}
		else
		{
			ptstime unit_position = master_edl->local_session->get_selectionstart(1);
			unit_position = master_edl->align_to_frame(unit_position);

			int value = round(vtrack->automation->get_floatvalue(
				unit_position, AUTOMATION_FADE));
			fade->update(fade->get_w(),value,
				master_edl->local_session->automation_mins[AUTOGROUPTYPE_VIDEO_FADE],
				master_edl->local_session->automation_maxs[AUTOGROUPTYPE_VIDEO_FADE]);
		}
	}
	else
	if(h - y1 >= theme_global->fade_h)
	{
		patchbay->add_subwindow(fade = new VFadePatch(this,
			x1 + x, y1 + y,
			patchbay->get_w() - 10));
	}
	y1 += theme_global->fade_h;

	if(mode)
	{
		if(h - y1 < theme_global->mode_h)
		{
			delete mode;
			mode = 0;
			delete nudge;
			nudge = 0;
		}
		else
		{
			mode->update(mode->get_keyframe_value(this));
			nudge->update(vtrack->nudge);
		}
	}
	else
	if(h - y1 >= theme_global->mode_h)
	{
		patchbay->add_subwindow(mode = new VModePatch(this,
			x1 + x, y1 + y));
		x1 += mode->get_w() + 10;
		patchbay->add_subwindow(nudge = new NudgePatch(this,
			x1 + x, y1 + y,
			patchbay->get_w() - x1 - 10));
	}

	y1 += theme_global->mode_h;

	patchgui_lock->unlock();
	return y1;
}

void VPatchGUI::synchronize_fade(double value_change)
{
	if(fade && !change_source) 
	{
		fade->update(round(fade->get_value() + value_change));
		fade->update_edl();
	}
}


VFadePatch::VFadePatch(VPatchGUI *patch, int x, int y, int w)
 : BC_ISlider(x, y, 0, w, w,
	master_edl->local_session->automation_mins[AUTOGROUPTYPE_VIDEO_FADE],
	master_edl->local_session->automation_maxs[AUTOGROUPTYPE_VIDEO_FADE],
	round(get_keyframe_value(patch)))
{
	this->patch = patch;
}

double VFadePatch::update_edl()
{
	FloatAuto *current;
	ptstime position = master_edl->local_session->get_selectionstart(1);
	int need_undo = !patch->vtrack->automation->auto_exists_for_editing(position, AUTOMATION_FADE);

	current = (FloatAuto*)patch->vtrack->automation->get_auto_for_editing(position, AUTOMATION_FADE);

	double result = get_value() - current->get_value();
	current->set_value(get_value());

	mwindow_global->undo->update_undo(_("fade"), LOAD_AUTOMATION, need_undo ? 0 : this);

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

	double change = update_edl();

	if(patch->track->gang) 
		patch->patchbay->synchronize_faders(change, TRACK_VIDEO, patch->track);

	patch->change_source = 0;

	mwindow_global->sync_parameters();

	if(edlsession->auto_conf->auto_visible[AUTOMATION_FADE])
		mwindow_global->draw_canvas_overlays();

	return 1;
}

double VFadePatch::get_keyframe_value(VPatchGUI *patch)
{
	ptstime unit_position = master_edl->local_session->get_selectionstart(1);
	unit_position = master_edl->align_to_frame(unit_position);

	return patch->vtrack->automation->get_floatvalue(unit_position, AUTOMATION_FADE);
}


VModePatch::VModePatch(VPatchGUI *patch, int x, int y)
 : BC_PopupMenu(x, y,
	patch->patchbay->mode_icons[0]->get_w() + 20,
	0, 1, theme_global->get_image_set("mode_popup", 0), 10)
{
	this->patch = patch;
	this->mode = get_keyframe_value(patch);
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
	ptstime position = master_edl->local_session->get_selectionstart(1);
	int need_undo = !patch->vtrack->automation->auto_exists_for_editing(position, AUTOMATION_MODE);
	IntAuto *current = (IntAuto*)patch->vtrack->automation->get_auto_for_editing(position, AUTOMATION_MODE);

	current->value = mode;

	mwindow_global->undo->update_undo(_("mode"), LOAD_AUTOMATION, need_undo ? 0 : this);
	mwindow_global->sync_parameters();

	if(edlsession->auto_conf->auto_visible[AUTOMATION_MODE])
		mwindow_global->draw_canvas_overlays();
	mainsession->changes_made = 1;
	return 1;
}

int VModePatch::get_keyframe_value(VPatchGUI *patch)
{
	ptstime unit_position = master_edl->local_session->get_selectionstart(1);
	unit_position = master_edl->align_to_frame(unit_position);

	return patch->vtrack->automation->get_intvalue(unit_position, AUTOMATION_MODE);
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
