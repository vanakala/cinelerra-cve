// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "apatchgui.h"
#include "bcsignals.h"
#include "apatchgui.inc"
#include "atrack.h"
#include "autoconf.h"
#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "language.h"
#include "localsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "panauto.h"
#include "panautos.h"
#include "patchbay.h"
#include "theme.h"


APatchGUI::APatchGUI(PatchBay *patchbay,
	ATrack *track,
	int x, int y)
 : PatchGUI(patchbay, track, x, y)
{
	data_type = TRACK_AUDIO;
	this->atrack = track;
	meter = 0;
	pan = 0;
	fade = 0;
	update(x, y);
}

APatchGUI::~APatchGUI()
{
	delete fade;
	delete meter;
	delete pan;
}

int APatchGUI::reposition(int x, int y)
{
	int x1 = 0;
	int y1 = PatchGUI::reposition(x, y);

	if(fade)
		fade->reposition_window(fade->get_x(), y1 + y);
	y1 += theme_global->fade_h;

	if(meter)
		meter->reposition_window(meter->get_x(), y1 + y,
			meter->get_w());
	y1 += theme_global->meter_h;

	if(pan)
		pan->reposition_window(pan->get_x(), y1 + y);

	if(nudge)
		nudge->reposition_window(nudge->get_x(), y1 + y);

	y1 += theme_global->pan_h;
	return y1;
}

int APatchGUI::update(int x, int y)
{
	int h, x1, y1;

	patchgui_lock->lock("APatchGUI::update");

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
			double value = atrack->automation->get_floatvalue(unit_position, AUTOMATION_AFADE);
			fade->update(fade->get_w(),
				value,
				master_edl->local_session->automation_mins[AUTOGROUPTYPE_AUDIO_FADE],
				master_edl->local_session->automation_maxs[AUTOGROUPTYPE_AUDIO_FADE]);
		}
	}
	else
	if(h - y1 >= theme_global->fade_h)
	{
		patchbay->add_subwindow(fade = new AFadePatch(this,
			x1 + x, y1 + y,
			patchbay->get_w() - 10));
	}
	y1 += theme_global->fade_h;

	if(meter)
	{
		if(h - y1 < theme_global->meter_h)
		{
			delete meter;
			meter = 0;
		}
	}
	else
	if(h - y1 >= theme_global->meter_h)
	{
		patchbay->add_subwindow(meter = new AMeterPatch(this,
			x1 + x, y1 + y));
	}
	y1 += theme_global->meter_h;
	x1 += 10;

	if(pan)
	{
		if(h - y1 < theme_global->pan_h)
		{
			delete pan;
			pan = 0;
			delete nudge;
			nudge = 0;
		}
		else
		{
			if(pan->get_total_values() != edlsession->audio_channels)
			{
				pan->change_channels(edlsession->audio_channels,
					edlsession->achannel_positions);
			}
			else
			{
				int handle_x, handle_y;
				PanAuto *previous = 0, *next = 0;
				ptstime position = master_edl->local_session->get_selectionstart(1);
				position = master_edl->align_to_frame(position);
				PanAutos *ptr = (PanAutos*)atrack->automation->get_autos(AUTOMATION_PAN);

				ptr->get_handle(handle_x,
					handle_y,
					position, 
					previous,
					next);
				pan->update(handle_x, handle_y);
			}
			nudge->update(atrack->nudge);
		}
	}
	else
	if(h - y1 >= theme_global->pan_h)
	{
		int handle_x, handle_y;
		PanAuto *previous = 0, *next = 0;
		PanAutos *ptr = (PanAutos*)atrack->automation->get_autos(AUTOMATION_PAN);
		ptstime position = master_edl->local_session->get_selectionstart(1);
		double *values;

		ptr->get_handle(handle_x, handle_y,
			position, previous, next);

		if(previous)
			values = previous->values;
		else
			values = ptr->default_values;

		patchbay->add_subwindow(pan = new APanPatch(this,
			x1 + x, y1 + y,
			handle_x, handle_y, values));
		x1 += pan->get_w() + 10;
		patchbay->add_subwindow(nudge = new NudgePatch(this,
			x1 + x, y1 + y,
			patchbay->get_w() - x1 - 10));
	}
	y1 += theme_global->pan_h;
	patchgui_lock->unlock();
	return y1;
}

void APatchGUI::synchronize_fade(double value_change)
{
	if(fade && !change_source) 
	{
		fade->update(fade->get_value() + value_change);
		fade->update_edl();
	}
}


AFadePatch::AFadePatch(APatchGUI *patch, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w,
	master_edl->local_session->automation_mins[AUTOGROUPTYPE_AUDIO_FADE],
	master_edl->local_session->automation_maxs[AUTOGROUPTYPE_AUDIO_FADE],
	get_keyframe_value(patch))
{
	this->patch = patch;
}

double AFadePatch::update_edl()
{
	FloatAuto *current;
	ptstime position = master_edl->local_session->get_selectionstart(1);
	int need_undo = !patch->atrack->automation->auto_exists_for_editing(position, AUTOMATION_AFADE);

	current = (FloatAuto*)patch->atrack->automation->get_auto_for_editing(position, AUTOMATION_AFADE);

	double result = get_value() - current->get_value();
	current->set_value(get_value());

	mwindow_global->undo->update_undo(_("fade"),
		LOAD_AUTOMATION, need_undo ? 0 : this);

	return result;
}

int AFadePatch::handle_event()
{
	if(shift_down()) 
	{
		update(0.0);
		set_tooltip(get_caption());
	}

	patch->change_source = 1;
	double change = update_edl();
	if(patch->track->gang)
		patch->patchbay->synchronize_faders(change,
			TRACK_AUDIO, patch->track);
	patch->change_source = 0;

	mwindow_global->sync_parameters(0);

	if(edlsession->auto_conf->auto_visible[AUTOMATION_AFADE])
		mwindow_global->draw_canvas_overlays();
	return 1;
}

double AFadePatch::get_keyframe_value(APatchGUI *patch)
{
	ptstime unit_position = master_edl->local_session->get_selectionstart(1);
	unit_position = master_edl->align_to_frame(unit_position);

	return patch->atrack->automation->get_floatvalue(unit_position, AUTOMATION_AFADE);
}


APanPatch::APanPatch(APatchGUI *patch, int x, int y,
	int handle_x, int handle_y, double *values)
 : BC_Pan(x, y, PAN_RADIUS, MAX_PAN,
	edlsession->audio_channels,
	edlsession->achannel_positions,
	handle_x,
	handle_y,
	values)
{
	this->patch = patch;
	set_tooltip(_("Pan"));
}

int APanPatch::handle_event()
{
	PanAuto *current;
	ptstime position = master_edl->local_session->get_selectionstart(1);
	Autos *pan_autos = patch->atrack->automation->get_autos(AUTOMATION_PAN);
	int need_undo = !pan_autos->auto_exists_for_editing(position);

	current = (PanAuto*)pan_autos->get_auto_for_editing(position);

	current->handle_x = get_stick_x();
	current->handle_y = get_stick_y();
	memcpy(current->values, get_values(), sizeof(double) * edlsession->audio_channels);

	mwindow_global->undo->update_undo(_("pan"), LOAD_AUTOMATION, need_undo ? 0 : this);

	mwindow_global->sync_parameters(0);

	if(need_undo && edlsession->auto_conf->auto_visible[AUTOMATION_PAN])
		mwindow_global->draw_canvas_overlays();
	return 1;
}


AMeterPatch::AMeterPatch(APatchGUI *patch, int x, int y)
 : BC_Meter(x, y,METER_HORIZ, patch->patchbay->get_w() - 10,
	edlsession->min_meter_db,
	edlsession->max_meter_db, 0)
{
	this->patch = patch;
}

int AMeterPatch::button_press_event()
{
	if(cursor_inside() && is_event_win() && get_buttonpress() == 1)
	{
		mwindow_global->reset_meters();
		return 1;
	}
	return 0;
}
