// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>


#include "automation.h"
#include "bcsignals.h"
#include "cinelerra.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "intauto.h"
#include "intautos.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "patchbay.h"
#include "patchgui.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
#include "vframe.h"

#include <string.h>
#include <stdlib.h>

#define PATCH_INCREMENT 0.01

PatchGUI::PatchGUI(PatchBay *patchbay,
	Track *track,
	int x,
	int y)
{
	this->patchbay = patchbay;
	this->track = track;
	this->x = x;
	this->y = y;
	title = 0;
	record = 0;
	play = 0;
	gang = 0;
	draw = 0;
	mute = 0;
	expand = 0;
	nudge = 0;
	master = 0;
	change_source = 0;
	track_id = -1;
	if(track) track_id = track->get_id();
	patchgui_lock = new Mutex("PatchGui");
}

PatchGUI::~PatchGUI()
{
	delete title;
	delete record;
	delete play;
	delete gang;
	delete draw;
	delete mute;
	delete expand;
	delete nudge;
	delete master;
	delete patchgui_lock;
}

int PatchGUI::reposition(int x, int y)
{
	int x1 = 0;
	int y1 = 0;

	if(x != this->x || y != this->y)
	{
		this->x = x;
		this->y = y;

		if(title)
		{
			title->reposition_window(x1, y1 + y);
		}
		y1 += theme_global->title_h;

		if(play)
		{
			master->reposition_window(x1, y1 + y);
			x1 += master->get_w();
			play->reposition_window(x1, y1 + y);
			x1 += play->get_w();
			record->reposition_window(x1, y1 + y);
			x1 += record->get_w();
			gang->reposition_window(x1, y1 + y);
			x1 += gang->get_w();
			draw->reposition_window(x1, y1 + y);
			x1 += draw->get_w();
			mute->reposition_window(x1, y1 + y);
			x1 += mute->get_w();

			if(expand)
			{
				VFrame **expandpatch_data = theme_global->get_image_set("expandpatch_data");
				expand->reposition_window(
					patchbay->get_w() - 10 - expandpatch_data[0]->get_w(), 
					y1 + y);
				x1 += expand->get_w();
			}
		}
		y1 += theme_global->play_h;
	}
	else
	{
		y1 += theme_global->title_h;
		y1 += theme_global->play_h;
	}

	return y1;
}

int PatchGUI::update(int x, int y)
{
	reposition(x, y);

	int h = track->vertical_span(theme_global);
	int y1 = 0;
	int x1 = 0;

	if(title)
	{
		if(h - y1 < 0)
		{
			delete title;
			title = 0;
		}
		else
		{
			title->update(track->title);
		}
	}
	else
	if(h - y1 >= 0)
	{
		patchbay->add_subwindow(title = new TitlePatch(this, x1 + x, y1 + y));
	}
	y1 += theme_global->title_h;

	if(play)
	{
		if(h - y1 < theme_global->play_h)
		{
			delete play;
			delete record;
			delete gang;
			delete draw;
			delete mute;
			delete expand;
			delete master;
			play = 0;
			record = 0;
			gang = 0;
			draw = 0;
			mute = 0;
			expand = 0;
			master = 0;
		}
		else
		{
			play->update(track->play);
			record->update(track->record);
			gang->update(track->gang);
			draw->update(track->draw);
			mute->update(mute->get_keyframe_value(this));
			expand->update(track->expand_view);
			master->update(track->master);
		}
	}
	else
	if(h - y1 >= theme_global->play_h)
	{
		patchbay->add_subwindow(master = new MasterTrackPatch(this, 0, y1 + y));
		x1 += master->get_w();
		patchbay->add_subwindow(play = new PlayPatch(this, x1 + x, y1 + y));
		x1 += play->get_w();
		patchbay->add_subwindow(record = new RecordPatch(this, x1 + x, y1 + y));
		x1 += record->get_w();
		patchbay->add_subwindow(gang = new GangPatch(this, x1 + x, y1 + y));
		x1 += gang->get_w();
		patchbay->add_subwindow(draw = new DrawPatch(this, x1 + x, y1 + y));
		x1 += draw->get_w();
		patchbay->add_subwindow(mute = new MutePatch(this, x1 + x, y1 + y));
		x1 += mute->get_w();

		VFrame **expandpatch_data = theme_global->get_image_set("expandpatch_data");
		patchbay->add_subwindow(expand = new ExpandPatch(this,
			patchbay->get_w() - 10 - expandpatch_data[0]->get_w(),
			y1 + y));
		x1 += expand->get_w();
	}
	y1 += theme_global->play_h;

	return y1;
}


void PatchGUI::toggle_behavior(int type, 
		int value,
		BC_Toggle *toggle,
		int *output)
{
	if(toggle->shift_down())
	{
		int total_selected = master_edl->total_toggled(type);

		if(type == Tracks::MUTE)
		{
			total_selected = master_edl->tracks->total() - total_selected;
			value = 0;
		}
		else
			value = 1;
// nothing previously selected
		if(total_selected == 0)
		{
			master_edl->set_all_toggles(type, value);
		}
		else
		if(total_selected == 1)
		{
// this patch was previously the only one on
			if((*output && value) || (!*output && !value))
			{
				master_edl->set_all_toggles(type, value);
			}
// another patch was previously the only one on
			else
			{
				master_edl->set_all_toggles(type, !value);
				*output = value;
			}
		}
		else
		if(total_selected > 1)
		{
			master_edl->set_all_toggles(type, !value);
			*output = value;
		}
		toggle->set_value(value);
		patchbay->update();
		patchbay->drag_operation = type;
		patchbay->new_status = value;
		patchbay->button_down = 1;
	}
	else
	{
		*output = value;
// Select + drag behavior
		patchbay->drag_operation = type;
		patchbay->new_status = value;
		patchbay->button_down = 1;
	}

	switch(type)
	{
	case Tracks::PLAY:
		mwindow_global->sync_parameters();
		mwindow_global->undo->update_undo(_("play patch"), LOAD_PATCHES);
		break;

	case Tracks::MUTE:
		mwindow_global->sync_parameters();
		mwindow_global->undo->update_undo(_("mute patch"), LOAD_PATCHES);
		break;

// Update affected tracks in cwindow
	case Tracks::RECORD:
		mwindow_global->cwindow->update(WUPD_OVERLAYS | WUPD_TOOLWIN);
		mwindow_global->undo->update_undo(_("record patch"), LOAD_PATCHES);
		break;

	case Tracks::GANG:
		mwindow_global->undo->update_undo(_("gang patch"), LOAD_PATCHES);
		break;

	case Tracks::DRAW:
		mwindow_global->undo->update_undo(_("draw patch"), LOAD_PATCHES);
		mwindow_global->update_gui(WUPD_CANVINCR);
		break;

	case Tracks::EXPAND:
		mwindow_global->undo->update_undo(_("expand patch"), LOAD_PATCHES);
		break;
	}
}

void PatchGUI::toggle_master(int value,
	MasterTrackPatch *toggle,
	int *output)
{
	if(!*output)
	{
		master_edl->set_all_toggles(Tracks::MASTER, 0);
		*output = 1;
		patchbay->update();
	}
	else
		toggle->update(*output);
}


PlayPatch::PlayPatch(PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("playpatch_data"),
	patch->track->play, 0, 0, 0, 0)
{
	this->patch = patch;
	set_tooltip(_("Play track"));
	set_select_drag(1);
}

int PlayPatch::handle_event()
{
	patch->toggle_behavior(Tracks::PLAY,
		get_value(),
		this,
		&patch->track->play);
	return 1;
}

int PlayPatch::button_release_event()
{
	patch->patchbay->drag_operation = Tracks::NONE;
	return BC_Toggle::button_release_event();;
}


RecordPatch::RecordPatch(PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("recordpatch_data"),
	patch->track->record, 0, 0, 0, 0)
{
	this->patch = patch;
	set_tooltip(_("Arm track"));
	set_select_drag(1);
}

int RecordPatch::handle_event()
{
	patch->toggle_behavior(Tracks::RECORD,
		get_value(),
		this,
		&patch->track->record);
	return 1;
}

int RecordPatch::button_release_event()
{
	patch->patchbay->drag_operation = Tracks::NONE;
	return BC_Toggle::button_release_event();
}


GangPatch::GangPatch(PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("gangpatch_data"),
	patch->track->gang, 0, 0, 0, 0)
{
	this->patch = patch;
	set_tooltip(_("Gang faders"));
	set_select_drag(1);
}

int GangPatch::handle_event()
{
	patch->toggle_behavior(Tracks::GANG,
		get_value(),
		this,
		&patch->track->gang);
	return 1;
}

int GangPatch::button_release_event()
{
	patch->patchbay->drag_operation = Tracks::NONE;
	return BC_Toggle::button_release_event();
}


DrawPatch::DrawPatch(PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("drawpatch_data"),
	patch->track->draw, 0, 0, 0, 0)
{
	this->patch = patch;
	set_tooltip(_("Draw media"));
	set_select_drag(1);
}

int DrawPatch::handle_event()
{
	patch->toggle_behavior(Tracks::DRAW,
		get_value(),
		this,
		&patch->track->draw);
	return 1;
}

int DrawPatch::button_release_event()
{
	patch->patchbay->drag_operation = Tracks::NONE;
	return BC_Toggle::button_release_event();
}


MutePatch::MutePatch(PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("mutepatch_data"),
	get_keyframe_value(patch), 0, 0, 0, 0)
{
	this->patch = patch;
	set_tooltip(_("Don't send to output"));
	set_select_drag(1);
}

int MutePatch::handle_event()
{
	IntAuto *current;
	ptstime position = master_edl->local_session->get_selectionstart(1);
	Autos *mute_autos = patch->track->automation->autos[AUTOMATION_MUTE];

	current = (IntAuto*)mute_autos->get_auto_for_editing(position);

	patch->toggle_behavior(Tracks::MUTE,
		get_value(),
		this,
		&current->value);

	mwindow_global->undo->update_undo(_("keyframe"), LOAD_AUTOMATION);

	if(edlsession->auto_conf->auto_visible[AUTOMATION_MUTE])
		mwindow_global->draw_canvas_overlays();
	return 1;
}

int MutePatch::button_release_event()
{
	patch->patchbay->drag_operation = Tracks::NONE;
	return BC_Toggle::button_release_event();
}

int MutePatch::get_keyframe_value(PatchGUI *patch)
{
	ptstime unit_position = master_edl->local_session->get_selectionstart(1);
	unit_position = master_edl->align_to_frame(unit_position);

	return ((IntAutos*)patch->track->automation->autos[AUTOMATION_MUTE])->get_value(
		unit_position);
}

MasterTrackPatch::MasterTrackPatch(PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y,
	theme_global->get_image_set("mastertrack_data"),
	patch->track->master, 0, 0, 0, 0)
{
	this->patch = patch;
	set_tooltip(_("Set master track"));
}

int MasterTrackPatch::handle_event()
{
	patch->toggle_master(get_value(),
		this, &patch->track->master);
	return 1;
}

ExpandPatch::ExpandPatch(PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("expandpatch_data"),
	patch->track->expand_view, 0, 0, 0, 0)
{
	this->patch = patch;
	set_select_drag(1);
}

int ExpandPatch::handle_event()
{
	patch->toggle_behavior(Tracks::EXPAND,
		get_value(),
		this,
		&patch->track->expand_view);
	mwindow_global->trackmovement(master_edl->local_session->track_start);
	return 1;
}

int ExpandPatch::button_release_event()
{
	patch->patchbay->drag_operation = Tracks::NONE;
	return BC_Toggle::button_release_event();
}


TitlePatch::TitlePatch(PatchGUI *patch, int x, int y)
 : BC_TextBox(x, y, patch->patchbay->get_w() - 10,
	1, patch->track->title,
	1, MEDIUMFONT, 1)
{
	this->patch = patch;
}

int TitlePatch::handle_event()
{
	strcpy(patch->track->title, get_utf8text());
	mwindow_global->update_plugin_titles();
	mwindow_global->draw_canvas_overlays();
	mwindow_global->undo->update_undo(_("track title"), LOAD_PATCHES);
	return 1;
}


NudgePatch::NudgePatch(PatchGUI *patch, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, patch->track->nudge)
{
	this->patch = patch;
	set_tooltip(_("Nudge"));
}

int NudgePatch::handle_event()
{
	set_value(atof(get_text()));
	return 1;
}

void NudgePatch::set_value(ptstime value)
{
	patch->track->nudge = value;

	patch->patchbay->synchronize_nudge(patch->track->nudge, patch->track);

	mwindow_global->undo->update_undo(_("nudge"), LOAD_AUTOMATION, this);
	mwindow_global->sync_parameters(patch->track->data_type == TRACK_VIDEO);
	mainsession->changes_made = 1;
}

int NudgePatch::button_press_event()
{
	int result = 0;
	ptstime value;

	if(is_event_win() && cursor_inside())
	{
		if(get_buttonpress() == 4)
		{
			value = atof(get_text()) + PATCH_INCREMENT;
			set_value(value);
			update(value);
			result = 1;
		}
		else
		if(get_buttonpress() == 5)
		{
			value = atof(get_text()) - PATCH_INCREMENT;
			set_value(value);
			update(value);
			result = 1;
		}
	}

	if(!result)
		return BC_TextBox::button_press_event();
	else
		return result;
}
