#include "apatchgui.h"
#include "apatchgui.inc"
#include "atrack.h"
#include "autoconf.h"
#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "localsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "panauto.h"
#include "panautos.h"
#include "patchbay.h"
#include "theme.h"
#include "trackcanvas.h"


APatchGUI::APatchGUI(MWindow *mwindow, PatchBay *patchbay, ATrack *track, int x, int y)
 : PatchGUI(mwindow, patchbay, track, x, y)
{
	data_type = TRACK_AUDIO;
	this->atrack = track;
	meter = 0;
	pan = 0;
	fade = 0;
}
APatchGUI::~APatchGUI()
{
	if(fade) delete fade;
	if(meter) delete meter;
	if(pan) delete pan;
}

int APatchGUI::create_objects()
{
	return update(x, y);
}

int APatchGUI::reposition(int x, int y)
{
	int x1 = 0;
	int y1 = PatchGUI::reposition(x, y);

	if(fade) fade->reposition_window(x1 + x, 
		y1 + y);
	y1 += mwindow->theme->fade_h;

	if(meter) meter->reposition_window(x1 + x,
		y1 + y,
		meter->get_w());
	y1 += mwindow->theme->meter_h;

	if(pan) pan->reposition_window(x1 + x + mwindow->theme->pan_x,
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
//printf("APatchGUI::update %f\n", fade->get_keyframe(mwindow, this)->value);
			FloatAuto *previous = 0, *next = 0;
			double unit_position = mwindow->edl->local_session->selectionstart;
			unit_position = mwindow->edl->align_to_frame(unit_position, 0);
			unit_position = atrack->to_units(unit_position, 0);
			float value = atrack->automation->fade_autos->get_value(
				(long)unit_position,
				PLAY_FORWARD, 
				previous, 
				next);
			fade->update(value);
//			fade->update(fade->get_keyframe(mwindow, this)->value);
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

	if(pan)
	{
		if(h - y1 < mwindow->theme->pan_h)
		{
			delete pan;
			pan = 0;
		}
		else
		if(pan->get_total_values() != mwindow->edl->session->audio_channels)
		{
			pan->change_channels(mwindow->edl->session->audio_channels,
				mwindow->edl->session->achannel_positions);
		}
		else
		{
			int handle_x, handle_y;
			PanAuto *previous = 0, *next = 0;
			double unit_position = mwindow->edl->local_session->selectionstart;
			unit_position = mwindow->edl->align_to_frame(unit_position, 0);
			unit_position = atrack->to_units(unit_position, 0);
			atrack->automation->pan_autos->get_handle(handle_x,
				handle_y,
				(long)unit_position, 
				PLAY_FORWARD,
				previous,
				next);
			pan->update(handle_x, handle_y);
		}
	}
	else
	if(h - y1 >= mwindow->theme->pan_h)
	{
		patchbay->add_subwindow(pan = new APanPatch(mwindow,
			this,
			x1 + x + mwindow->theme->pan_x, 
			y1 + y));
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
			(float)INFINITYGAIN, 
			(float)MAX_AUDIO_FADE, 
			get_keyframe(mwindow, patch)->value)
{
//printf("AFadePatch::AFadePatch 1 %p %f\n", patch->track, get_keyframe(mwindow, patch)->value);
//printf("AFadePatch::AFadePatch 2 %f\n", ((FloatAuto*)patch->track->automation->fade_autos->first)->value);
	this->mwindow = mwindow;
	this->patch = patch;
}

float AFadePatch::update_edl()
{
	FloatAuto *current;
	double position = mwindow->edl->local_session->selectionstart;
	Autos *fade_autos = patch->atrack->automation->fade_autos;
	int update_undo = !fade_autos->auto_exists_for_editing(position);

//printf("AFadePatch::update_edl 1 %d\n", update_undo);
	if(update_undo)
		mwindow->undo->update_undo_before("fade", LOAD_AUTOMATION);

	current = (FloatAuto*)fade_autos->get_auto_for_editing(position);

	float result = get_value() - current->value;
	current->value = get_value();

	if(update_undo)
		mwindow->undo->update_undo_after();

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

	if(mwindow->edl->session->auto_conf->fade)
	{
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
	}
	return 1;
}

FloatAuto* AFadePatch::get_keyframe(MWindow *mwindow, APatchGUI *patch)
{
	Auto *current = 0;
	double unit_position = mwindow->edl->local_session->selectionstart;
	unit_position = mwindow->edl->align_to_frame(unit_position, 0);
	unit_position = patch->atrack->to_units(unit_position, 0);

	return (FloatAuto*)patch->atrack->automation->fade_autos->get_prev_auto(
		(long)unit_position, 
		PLAY_FORWARD,
		current);
}


APanPatch::APanPatch(MWindow *mwindow, APatchGUI *patch, int x, int y)
 : BC_Pan(x, 
		y, 
		PAN_RADIUS, 
		1, 
		mwindow->edl->session->audio_channels, 
		mwindow->edl->session->achannel_positions, 
		get_keyframe(mwindow, patch)->handle_x, 
		get_keyframe(mwindow, patch)->handle_y,
 		get_keyframe(mwindow, patch)->values)
{
	this->mwindow = mwindow;
	this->patch = patch;
//printf("APanPatch::APanPatch %d %d\n", get_keyframe(mwindow, patch)->handle_x, get_keyframe(mwindow, patch)->handle_y);
}

int APanPatch::handle_event()
{
	PanAuto *current;
	double position = mwindow->edl->local_session->selectionstart;
	Autos *pan_autos = patch->atrack->automation->pan_autos;
	int update_undo = !pan_autos->auto_exists_for_editing(position);

	if(update_undo)
		mwindow->undo->update_undo_before("pan", LOAD_AUTOMATION);

	current = (PanAuto*)pan_autos->get_auto_for_editing(position);

	current->handle_x = get_stick_x();
	current->handle_y = get_stick_y();
	memcpy(current->values, get_values(), sizeof(float) * mwindow->edl->session->audio_channels);

	if(update_undo)
		mwindow->undo->update_undo_after();

	mwindow->sync_parameters(CHANGE_PARAMS);

	if(update_undo && mwindow->edl->session->auto_conf->pan)
	{
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
	}
	return 1;
}

PanAuto* APanPatch::get_keyframe(MWindow *mwindow, APatchGUI *patch)
{
	Auto *current = 0;
	double unit_position = mwindow->edl->local_session->selectionstart;
	unit_position = mwindow->edl->align_to_frame(unit_position, 0);
	unit_position = patch->atrack->to_units(unit_position, 0);

	return (PanAuto*)patch->atrack->automation->pan_autos->get_prev_auto(
		(long)unit_position, 
		PLAY_FORWARD,
		current);
}




AMeterPatch::AMeterPatch(MWindow *mwindow, APatchGUI *patch, int x, int y)
 : BC_Meter(x, 
			y, 
			METER_HORIZ, 
			patch->patchbay->get_w() - 10, 
			mwindow->edl->session->min_meter_db, 
			mwindow->edl->session->meter_format, 
			0,
			mwindow->edl->session->record_speed * 10,
			mwindow->edl->session->record_speed)
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





