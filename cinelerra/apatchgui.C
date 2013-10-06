
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
#include "mwindowgui.h"
#include "panauto.h"
#include "panautos.h"
#include "patchbay.h"
#include "theme.h"
#include "trackcanvas.h"


APatchGUI::APatchGUI(MWindow *mwindow, 
	PatchBay *patchbay, 
	ATrack *track, 
	int x, 
	int y)
 : PatchGUI(mwindow, 
	patchbay,
	track, 
	x, 
	y)
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
	if(fade) delete fade;
	if(meter) delete meter;
	if(pan) delete pan;
}

int APatchGUI::reposition(int x, int y)
{
	int x1 = 0;
	int y1 = PatchGUI::reposition(x, y);

	if(fade) fade->reposition_window(fade->get_x(), 
		y1 + y);
	y1 += mwindow->theme->fade_h;

	if(meter) meter->reposition_window(meter->get_x(),
		y1 + y,
		meter->get_w());
	y1 += mwindow->theme->meter_h;

	if(pan) pan->reposition_window(pan->get_x(),
		y1 + y);

	if(nudge) nudge->reposition_window(nudge->get_x(),
		y1 + y);

	y1 += mwindow->theme->pan_h;
	return y1;
}

int APatchGUI::update(int x, int y)
{
	int h = track->vertical_span(mwindow->theme);
	int x1 = 0;
	int y1 = PatchGUI::update(x, y);

	if(fade)
	{
		if(h - y1 < mwindow->theme->fade_h)
		{
			delete fade;
			fade = 0;
		}
		else
		{
			FloatAuto *previous = 0, *next = 0;
			ptstime unit_position = mwindow->edl->local_session->get_selectionstart(1);
			unit_position = mwindow->edl->align_to_frame(unit_position);
			FloatAutos *ptr = (FloatAutos*)atrack->automation->autos[AUTOMATION_FADE];
			float value = ptr->get_value(
				unit_position,
				previous, 
				next);
			fade->update(fade->get_w(),
					value,
					mwindow->edl->local_session->automation_mins[AUTOGROUPTYPE_AUDIO_FADE],
					mwindow->edl->local_session->automation_maxs[AUTOGROUPTYPE_AUDIO_FADE]);
		}
	}
	else
	if(h - y1 >= mwindow->theme->fade_h)
	{
		patchbay->add_subwindow(fade = new AFadePatch(mwindow, 
			this, 
			x1 + x, 
			y1 + y, 
			patchbay->get_w() - 10));
	}
	y1 += mwindow->theme->fade_h;

	if(meter)
	{
		if(h - y1 < mwindow->theme->meter_h)
		{
			delete meter;
			meter = 0;
		}
	}
	else
	if(h - y1 >= mwindow->theme->meter_h)
	{
		patchbay->add_subwindow(meter = new AMeterPatch(mwindow,
			this,
			x1 + x, 
			y1 + y));
	}
	y1 += mwindow->theme->meter_h;
	x1 += 10;

	if(pan)
	{
		
		if(h - y1 < mwindow->theme->pan_h)
		{
			delete pan;
			pan = 0;
			delete nudge;
			nudge = 0;
		}
		else
		{
			if(pan->get_total_values() != mwindow->edl->session->audio_channels)
			{
				pan->change_channels(mwindow->edl->session->audio_channels,
					mwindow->edl->session->achannel_positions);
			}
			else
			{
				int handle_x, handle_y;
				PanAuto *previous = 0, *next = 0;
				ptstime position = mwindow->edl->local_session->get_selectionstart(1);
				position = mwindow->edl->align_to_frame(position);
				PanAutos *ptr = (PanAutos*)atrack->automation->autos[AUTOMATION_PAN];
				ptr->get_handle(handle_x,
					handle_y,
					position, 
					previous,
					next);
				pan->update(handle_x, handle_y);
			}
			nudge->update();
		}
	}
	else
	if(h - y1 >= mwindow->theme->pan_h)
	{
		int handle_x, handle_y;
		PanAuto *previous, *next;
		PanAutos *ptr = (PanAutos*)atrack->automation->autos[AUTOMATION_PAN];
		ptstime position = mwindow->edl->local_session->get_selectionstart(1);
		float *values;

		ptr->get_handle(handle_x, handle_y,
			position, previous, next);

		if(previous)
			values = previous->values;
		else
			values = ptr->default_values;

		patchbay->add_subwindow(pan = new APanPatch(mwindow,
			this,
			x1 + x,
			y1 + y,
			handle_x, handle_y, values));
		x1 += pan->get_w() + 10;
		patchbay->add_subwindow(nudge = new NudgePatch(mwindow,
			this,
			x1 + x,
			y1 + y,
			patchbay->get_w() - x1 - 10));
	}
	y1 += mwindow->theme->pan_h;

	return y1;
}

void APatchGUI::synchronize_fade(float value_change)
{
	if(fade && !change_source) 
	{
		fade->update(fade->get_value() + value_change);
		fade->update_edl();
	}
}


AFadePatch::AFadePatch(MWindow *mwindow, APatchGUI *patch, int x, int y, int w)
 : BC_FSlider(x, 
	y,
	0,
	w,
	w,
	mwindow->edl->local_session->automation_mins[AUTOGROUPTYPE_AUDIO_FADE],
	mwindow->edl->local_session->automation_maxs[AUTOGROUPTYPE_AUDIO_FADE],
	get_keyframe_value(mwindow, patch))
{
	this->mwindow = mwindow;
	this->patch = patch;
}

float AFadePatch::update_edl()
{
	FloatAuto *current;
	ptstime position = mwindow->edl->local_session->get_selectionstart(1);
	Autos *fade_autos = patch->atrack->automation->autos[AUTOMATION_FADE];
	int need_undo = !fade_autos->auto_exists_for_editing(position);

	current = (FloatAuto*)fade_autos->get_auto_for_editing(position);

	float result = get_value() - current->get_value();
	current->set_value(get_value());

	mwindow->undo->update_undo(_("fade"), 
		LOAD_AUTOMATION, 
		need_undo ? 0 : this);

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
	float change = update_edl();
	if(patch->track->gang) 
		patch->patchbay->synchronize_faders(change, TRACK_AUDIO, patch->track);
	patch->change_source = 0;

	mwindow->sync_parameters(CHANGE_PARAMS);

	if(mwindow->edl->session->auto_conf->autos[AUTOMATION_FADE])
	{
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
	}
	return 1;
}

float AFadePatch::get_keyframe_value(MWindow *mwindow, APatchGUI *patch)
{
	FloatAuto *prev = 0;
	FloatAuto *next = 0;
	ptstime unit_position = mwindow->edl->local_session->get_selectionstart(1);
	unit_position = mwindow->edl->align_to_frame(unit_position);

	FloatAutos *ptr = (FloatAutos*)patch->atrack->automation->autos[AUTOMATION_FADE];
	return ptr->get_value(unit_position, prev, next);
}


APanPatch::APanPatch(MWindow *mwindow, APatchGUI *patch, int x, int y, 
    int handle_x, int handle_y, float *values)
 : BC_Pan(x,
		y, 
		PAN_RADIUS, 
		MAX_PAN, 
		mwindow->edl->session->audio_channels, 
		mwindow->edl->session->achannel_positions, 
		handle_x,
		handle_y,
		values)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip("Pan");
}

int APanPatch::handle_event()
{
	PanAuto *current;
	ptstime position = mwindow->edl->local_session->get_selectionstart(1);
	Autos *pan_autos = patch->atrack->automation->autos[AUTOMATION_PAN];
	int need_undo = !pan_autos->auto_exists_for_editing(position);

	current = (PanAuto*)pan_autos->get_auto_for_editing(position);

	current->handle_x = get_stick_x();
	current->handle_y = get_stick_y();
	memcpy(current->values, get_values(), sizeof(float) * mwindow->edl->session->audio_channels);

	mwindow->undo->update_undo(_("pan"), LOAD_AUTOMATION, need_undo ? 0 : this);

	mwindow->sync_parameters(CHANGE_PARAMS);

	if(need_undo && mwindow->edl->session->auto_conf->autos[AUTOMATION_PAN])
	{
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
	}
	return 1;
}


AMeterPatch::AMeterPatch(MWindow *mwindow, APatchGUI *patch, int x, int y)
 : BC_Meter(x, 
		y,
		METER_HORIZ,
		patch->patchbay->get_w() - 10,
		mwindow->edl->session->min_meter_db,
		mwindow->edl->session->max_meter_db,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
}

int AMeterPatch::button_press_event()
{
	if(cursor_inside() && is_event_win() && get_buttonpress() == 1)
	{
		mwindow->reset_meters();
		return 1;
	}

	return 0;
}
