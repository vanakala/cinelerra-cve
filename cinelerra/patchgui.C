#include "automation.h"
#include "cplayback.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "intauto.h"
#include "intautos.h"
#include "localsession.h"
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
	change_source = 0;
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
			title->reposition_window(x1, y1 + y);
		}
		y1 += mwindow->theme->title_h;

		if(play)
		{
			play->reposition_window(x1, y1 + y);
			x1 += play->get_w();
			record->reposition_window(x1, y1 + y);
			x1 += record->get_w();
//			automate->reposition_window(x1, y1 + y);
//			x1 += automate->get_w();
			gang->reposition_window(x1, y1 + y);
			x1 += gang->get_w();
			draw->reposition_window(x1, y1 + y);
			x1 += draw->get_w();
			mute->reposition_window(x1, y1 + y);
			x1 += mute->get_w();

			expand->reposition_window(
				patchbay->get_w() - 10 - mwindow->theme->expandpatch_data[0]->get_w(), 
				y1 + y);
			x1 += expand->get_w();
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
	reposition(x, y);

	int h = track->vertical_span(mwindow->theme);
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
		x1 += play->get_w();
		patchbay->add_subwindow(record = new RecordPatch(mwindow, this, x1 + x, y1 + y));
		x1 += record->get_w();
		patchbay->add_subwindow(gang = new GangPatch(mwindow, this, x1 + x, y1 + y));
		x1 += gang->get_w();
		patchbay->add_subwindow(draw = new DrawPatch(mwindow, this, x1 + x, y1 + y));
		x1 += draw->get_w();
		patchbay->add_subwindow(mute = new MutePatch(mwindow, this, x1 + x, y1 + y));
		x1 += mute->get_w();


		patchbay->add_subwindow(expand = new ExpandPatch(mwindow, 
			this, 
			patchbay->get_w() - 10 - mwindow->theme->expandpatch_data[0]->get_w(), 
			y1 + y));
		x1 += expand->get_w();
	}
	y1 += mwindow->theme->play_h;

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
	}
	else
	{
		*output = value;
// Select + drag behavior
		patchbay->drag_operation = type;
		patchbay->new_status = value;
		patchbay->button_down = 1;
	}

	if(type == Tracks::PLAY)
	{
		mwindow->gui->unlock_window();
		mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_EDL);
		mwindow->gui->lock_window();
	}
	else
	if(type == Tracks::MUTE)
	{
		mwindow->gui->unlock_window();
		mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_PARAMS);
		mwindow->gui->lock_window();
	}
	
// Update affected tracks in cwindow
	if(type == Tracks::RECORD)
		mwindow->cwindow->update(0, 1, 1);

}











PlayPatch::PlayPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->playpatch_data,
		patch->track->play, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip("Play track");
	set_select_drag(1);
}

int PlayPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
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
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
		return 1;
	}
	return 0;
}











RecordPatch::RecordPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->recordpatch_data,
		patch->track->record, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip("Arm track");
	set_select_drag(1);
}

int RecordPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
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
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
		return 1;
	}
	return 0;
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
	return 1;
}











GangPatch::GangPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, 
		mwindow->theme->gangpatch_data,
		patch->track->gang, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip("Gang faders");
	set_select_drag(1);
}

int GangPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
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
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
		return 1;
	}
	return 0;
}











DrawPatch::DrawPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, 
		mwindow->theme->drawpatch_data,
		patch->track->draw, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip("Draw media");
	set_select_drag(1);
}

int DrawPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
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
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
		return 1;
	}
	return 0;
}










MutePatch::MutePatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, 
		mwindow->theme->mutepatch_data,
		get_keyframe(mwindow, patch)->value, 
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip("Don't send to output");
	set_select_drag(1);
}

int MutePatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		update(!get_value());
		IntAuto *current;
		double position = mwindow->edl->local_session->selectionstart;
		Autos *mute_autos = patch->track->automation->mute_autos;

		mwindow->undo->update_undo_before("keyframe", LOAD_AUTOMATION);

		current = (IntAuto*)mute_autos->get_auto_for_editing(position);
		current->value = get_value();

		patch->toggle_behavior(Tracks::MUTE,
			get_value(),
			this,
			&current->value);


		mwindow->undo->update_undo_after();

		if(mwindow->edl->session->auto_conf->mute)
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
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
		return 1;
	}
	return 0;
}

IntAuto* MutePatch::get_keyframe(MWindow *mwindow, PatchGUI *patch)
{
	Auto *current = 0;
	double unit_position = mwindow->edl->local_session->selectionstart;
	unit_position = mwindow->edl->align_to_frame(unit_position, 0);
	unit_position = patch->track->to_units(unit_position, 0);
	return (IntAuto*)patch->track->automation->mute_autos->get_prev_auto(
		(long)unit_position, 
		PLAY_FORWARD,
		current);
}












ExpandPatch::ExpandPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, 
 		y, 
		mwindow->theme->expandpatch_data,
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
	if(patch->patchbay->drag_operation != Tracks::NONE)
	{
		patch->patchbay->drag_operation = Tracks::NONE;
		return 1;
	}
	return 0;
}




