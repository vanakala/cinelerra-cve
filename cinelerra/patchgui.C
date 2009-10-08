
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

#include "automation.h"
#include "bcsignals.h"
#include "cplayback.h"
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
#include "mwindowgui.h"
#include "patchbay.h"
#include "patchgui.h"
#include "playbackengine.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "vframe.h"



PatchGUI::PatchGUI(MWindow *mwindow, 
		PatchBay *patchbay, 
		Track *track, 
		int x, 
		int y)
{
	this->mwindow = mwindow;
	this->patchbay = patchbay;
	this->track = track;
	this->x = x;
	this->y = y;
	title = 0;
	record = 0;
	play = 0;
//	automate = 0;
	gang = 0;
	draw = 0;
	mute = 0;
	expand = 0;
	nudge = 0;
	change_source = 0;
	track_id = -1;
	if(track) track_id = track->get_id();
}

PatchGUI::~PatchGUI()
{
	if(title) delete title;
	if(record) delete record;
	if(play) delete play;
//	if(automate) delete automate;
	if(gang) delete gang;
	if(draw) delete draw;
	if(mute) delete mute;
	if(expand) delete expand;
	if(nudge) delete nudge;
}

int PatchGUI::create_objects()
{
	return update(x, y);
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
TRACE("PatchGUI::reposition 1\n");
			title->reposition_window(x1, y1 + y);
TRACE("PatchGUI::reposition 2\n");
		}
		y1 += mwindow->theme->title_h;

		if(play)
		{
TRACE("PatchGUI::reposition 3\n");
			play->reposition_window(x1, y1 + y);
			x1 += play->get_w();
TRACE("PatchGUI::reposition 4\n");
			record->reposition_window(x1, y1 + y);
			x1 += record->get_w();
TRACE("PatchGUI::reposition 5\n");
//			automate->reposition_window(x1, y1 + y);
//			x1 += automate->get_w();
			gang->reposition_window(x1, y1 + y);
			x1 += gang->get_w();
TRACE("PatchGUI::reposition 6\n");
			draw->reposition_window(x1, y1 + y);
			x1 += draw->get_w();
TRACE("PatchGUI::reposition 7\n");
			mute->reposition_window(x1, y1 + y);
			x1 += mute->get_w();
TRACE("PatchGUI::reposition 8\n");

			if(expand)
			{
TRACE("PatchGUI::reposition 9\n");
				VFrame **expandpatch_data = mwindow->theme->get_image_set("expandpatch_data");
				expand->reposition_window(
					patchbay->get_w() - 10 - expandpatch_data[0]->get_w(), 
					y1 + y);
TRACE("PatchGUI::reposition 10\n");
				x1 += expand->get_w();
TRACE("PatchGUI::reposition 11\n");
			}
		}
		y1 += mwindow->theme->play_h;
	}
	else
	{
		y1 += mwindow->theme->title_h;
		y1 += mwindow->theme->play_h;
	}

	return y1;
}

int PatchGUI::update(int x, int y)
{
//TRACE("PatchGUI::update 1");
	reposition(x, y);
//TRACE("PatchGUI::update 10");

	int h = track->vertical_span(mwindow->theme);
	int y1 = 0;
	int x1 = 0;
//printf("PatchGUI::update 10\n");

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
		patchbay->add_subwindow(title = new TitlePatch(mwindow, this, x1 + x, y1 + y));
	}
	y1 += mwindow->theme->title_h;

	if(play)
	{
		if(h - y1 < mwindow->theme->play_h)
		{
			delete play;
			delete record;
			delete gang;
			delete draw;
			delete mute;
			delete expand;
			play = 0;
			record = 0;
			draw = 0;
			mute = 0;
			expand = 0;
		}
		else
		{
			play->update(track->play);
			record->update(track->record);
			gang->update(track->gang);
			draw->update(track->draw);
			mute->update(mute->get_keyframe(mwindow, this)->value);
			expand->update(track->expand_view);
		}
	}
	else
	if(h - y1 >= mwindow->theme->play_h)
	{
		patchbay->add_subwindow(play = new PlayPatch(mwindow, this, x1 + x, y1 + y));
//printf("PatchGUI::update 1 %p %p\n", play, &play->status);
		x1 += play->get_w();
		patchbay->add_subwindow(record = new RecordPatch(mwindow, this, x1 + x, y1 + y));
		x1 += record->get_w();
		patchbay->add_subwindow(gang = new GangPatch(mwindow, this, x1 + x, y1 + y));
		x1 += gang->get_w();
		patchbay->add_subwindow(draw = new DrawPatch(mwindow, this, x1 + x, y1 + y));
		x1 += draw->get_w();
		patchbay->add_subwindow(mute = new MutePatch(mwindow, this, x1 + x, y1 + y));
		x1 += mute->get_w();

		VFrame **expandpatch_data = mwindow->theme->get_image_set("expandpatch_data");
		patchbay->add_subwindow(expand = new ExpandPatch(mwindow, 
			this, 
			patchbay->get_w() - 10 - expandpatch_data[0]->get_w(), 
			y1 + y));
		x1 += expand->get_w();
	}
	y1 += mwindow->theme->play_h;

//UNTRACE
	return y1;
}


void PatchGUI::toggle_behavior(int type, 
		int value,
		BC_Toggle *toggle,
		int *output)
{
	if(toggle->shift_down())
	{
		int total_selected = mwindow->edl->tracks->total_of(type);

// nothing previously selected
		if(total_selected == 0)
		{
			mwindow->edl->tracks->select_all(type,
				1);
		}
		else
		if(total_selected == 1)
		{
// this patch was previously the only one on
			if(*output)
			{
				mwindow->edl->tracks->select_all(type,
					1);
			}
// another patch was previously the only one on
			else
			{
				mwindow->edl->tracks->select_all(type,
					0);
				*output = 1;
			}
		}
		else
		if(total_selected > 1)
		{
			mwindow->edl->tracks->select_all(type,
				0);
			*output = 1;
		}
		toggle->set_value(*output);
		patchbay->update();
		patchbay->drag_operation = type;
		patchbay->new_status = 1;
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
			mwindow->gui->unlock_window();
			mwindow->restart_brender();
			mwindow->sync_parameters(CHANGE_EDL);
			mwindow->gui->lock_window("PatchGUI::toggle_behavior 1");
			mwindow->undo->update_undo(_("play patch"), LOAD_PATCHES);
			break;

		case Tracks::MUTE:
			mwindow->gui->unlock_window();
			mwindow->restart_brender();
			mwindow->sync_parameters(CHANGE_PARAMS);
			mwindow->gui->lock_window("PatchGUI::toggle_behavior 2");
			mwindow->undo->update_undo(_("mute patch"), LOAD_PATCHES);
			break;

// Update affected tracks in cwindow
		case Tracks::RECORD:
			mwindow->cwindow->update(0, 1, 1);
			mwindow->undo->update_undo(_("record patch"), LOAD_PATCHES);
			break;

		case Tracks::GANG:
			mwindow->undo->update_undo(_("gang patch"), LOAD_PATCHES);
			break;

		case Tracks::DRAW:
			mwindow->undo->update_undo(_("draw patch"), LOAD_PATCHES);
			mwindow->gui->update(0, 1, 0, 0, 0, 0, 0);
			break;

		case Tracks::EXPAND:
			mwindow->undo->update_undo(_("expand patch"), LOAD_PATCHES);
			break;
	}
}


char* PatchGUI::calculate_nudge_text(int *changed)
{
	if(changed) *changed = 0;
	if(track->edl->session->nudge_seconds)
	{
		sprintf(string_return, "%.4f", track->from_units(track->nudge));
		if(changed && nudge && atof(nudge->get_text()) - atof(string_return) != 0)
			*changed = 1;
	}
	else
	{
		sprintf(string_return, "%d", track->nudge);
		if(changed && nudge && atoi(nudge->get_text()) - atoi(string_return) != 0)
			*changed = 1;
	}
	return string_return;
}


int64_t PatchGUI::calculate_nudge(char *string)
{
	if(mwindow->edl->session->nudge_seconds)
	{
		float result;
		sscanf(string, "%f", &result);
		return track->to_units(result, 0);
	}
	else
	{
		int64_t temp;
		sscanf(string, "%lld", &temp);
		return temp;
	}
}












PlayPatch::PlayPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->get_image_set("playpatch_data"),
		patch->track->play, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Play track"));
	set_select_drag(1);
}

int PlayPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::PLAY,
			get_value(),
			this,
			&patch->track->play);
		return 1;
	}
	return 0;
}

int PlayPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}











RecordPatch::RecordPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->get_image_set("recordpatch_data"),
		patch->track->record, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Arm track"));
	set_select_drag(1);
}

int RecordPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::RECORD,
			get_value(),
			this,
			&patch->track->record);
		return 1;
	}
	return 0;
}

int RecordPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}











GangPatch::GangPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, 
		mwindow->theme->get_image_set("gangpatch_data"),
		patch->track->gang, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Gang faders"));
	set_select_drag(1);
}

int GangPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::GANG,
			get_value(),
			this,
			&patch->track->gang);
		return 1;
	}
	return 0;
}

int GangPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}











DrawPatch::DrawPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, 
		mwindow->theme->get_image_set("drawpatch_data"),
		patch->track->draw, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Draw media"));
	set_select_drag(1);
}

int DrawPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::DRAW,
			get_value(),
			this,
			&patch->track->draw);
		return 1;
	}
	return 0;
}

int DrawPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}










MutePatch::MutePatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, 
		mwindow->theme->get_image_set("mutepatch_data"),
		get_keyframe(mwindow, patch)->value, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Don't send to output"));
	set_select_drag(1);
}

int MutePatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		IntAuto *current;
		double position = mwindow->edl->local_session->get_selectionstart(1);
		Autos *mute_autos = patch->track->automation->autos[AUTOMATION_MUTE];


		current = (IntAuto*)mute_autos->get_auto_for_editing(position);
		current->value = get_value();

		patch->toggle_behavior(Tracks::MUTE,
			get_value(),
			this,
			&current->value);


		mwindow->undo->update_undo(_("keyframe"), LOAD_AUTOMATION);

		if(mwindow->edl->session->auto_conf->autos[AUTOMATION_MUTE])
		{
			mwindow->gui->canvas->draw_overlays();
			mwindow->gui->canvas->flash();
		}
		return 1;
	}
	return 0;
}

int MutePatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}

IntAuto* MutePatch::get_keyframe(MWindow *mwindow, PatchGUI *patch)
{
	Auto *current = 0;
	double unit_position = mwindow->edl->local_session->get_selectionstart(1);
	unit_position = mwindow->edl->align_to_frame(unit_position, 0);
	unit_position = patch->track->to_units(unit_position, 0);
	return (IntAuto*)patch->track->automation->autos[AUTOMATION_MUTE]->get_prev_auto(
		(int64_t)unit_position, 
		PLAY_FORWARD,
		current);
}












ExpandPatch::ExpandPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->get_image_set("expandpatch_data"),
		patch->track->expand_view, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_select_drag(1);
}

int ExpandPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::EXPAND,
			get_value(),
			this,
			&patch->track->expand_view);
		mwindow->trackmovement(mwindow->edl->local_session->track_start);
		return 1;
	}
	return 0;
}

int ExpandPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}





TitlePatch::TitlePatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_TextBox(x, 
 		y, 
		patch->patchbay->get_w() - 10, 
		1,
		patch->track->title)
{
	this->mwindow = mwindow;
	this->patch = patch;
}

int TitlePatch::handle_event()
{
	strcpy(patch->track->title, get_text());
	mwindow->update_plugin_titles();
	mwindow->gui->canvas->draw_overlays();
	mwindow->gui->canvas->flash();
	mwindow->undo->update_undo(_("track title"), LOAD_PATCHES);
	return 1;
}









NudgePatch::NudgePatch(MWindow *mwindow, 
	PatchGUI *patch, 
	int x, 
	int y, 
	int w)
 : BC_TextBox(x,
 	y,
	w,
	1,
	patch->calculate_nudge_text(0))
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Nudge"));
}

int NudgePatch::handle_event()
{
	set_value(patch->calculate_nudge(get_text()));
	return 1;
}

void NudgePatch::set_value(int64_t value)
{
	patch->track->nudge = value;

	if(patch->track->gang)
		patch->patchbay->synchronize_nudge(patch->track->nudge, patch->track);

	mwindow->undo->update_undo("nudge", LOAD_AUTOMATION, this);

	mwindow->gui->unlock_window();
	if(patch->track->data_type == TRACK_VIDEO)
		mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->gui->lock_window("NudgePatch::handle_event 2");

	mwindow->session->changes_made = 1;
}


int NudgePatch::button_press_event()
{
	int result = 0;

	if(is_event_win() && cursor_inside())
	{
		if(get_buttonpress() == 4)
		{
			int value = patch->calculate_nudge(get_text());
			value += calculate_increment();
			set_value(value);
			update();
			result = 1;
		}
		else
		if(get_buttonpress() == 5)
		{
			int value = patch->calculate_nudge(get_text());
			value -= calculate_increment();
			set_value(value);
			update();
			result = 1;
		}
		else
		if(get_buttonpress() == 3)
		{
			patch->patchbay->nudge_popup->activate_menu(patch);
			result = 1;
		}
	}

	if(!result)
		return BC_TextBox::button_press_event();
	else
		return result;
}

int64_t NudgePatch::calculate_increment()
{
	if(patch->track->data_type == TRACK_AUDIO)
	{
		return (int64_t)ceil(patch->track->edl->session->sample_rate / 10);
	}
	else
	{
		return (int64_t)ceil(1.0 / patch->track->edl->session->frame_rate);
	}
}

void NudgePatch::update()
{
	int changed;
	char *string = patch->calculate_nudge_text(&changed);
	if(changed)
		BC_TextBox::update(string);
}




