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
#include "mwindowgui.h"
#include "overlayframe.inc"
#include "patchbay.h"
#include "theme.h"
#include "trackcanvas.h"
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
}

VPatchGUI::~VPatchGUI()
{
	if(fade) delete fade;
	if(mode) delete mode;
}

int VPatchGUI::create_objects()
{
	return update(x, y);
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
			double unit_position = mwindow->edl->local_session->selectionstart;
			unit_position = mwindow->edl->align_to_frame(unit_position, 0);
			unit_position = vtrack->to_units(unit_position, 0);
			int value = (int)((FloatAutos*)vtrack->automation->fade_autos)->get_value(
				(int64_t)unit_position,
				PLAY_FORWARD, 
				previous, 
				next);
			fade->update(value);
//			fade->update((int)fade->get_keyframe(mwindow, this)->value);
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
			mode->update(mode->get_keyframe(mwindow, this)->value);
			nudge->update();
		}
	}
	else
	if(h - y1 >= mwindow->theme->mode_h)
	{
		patchbay->add_subwindow(mode = new VModePatch(mwindow, 
			this, 
			x1 + x, 
			y1 + y));
		mode->create_objects();
		x1 += mode->get_w() + 10;
		patchbay->add_subwindow(nudge = new NudgePatch(mwindow,
			this,
			x1 + x,
			y1 + y,
			patchbay->get_w() - x1 - 10));
	}





	y1 += mwindow->theme->mode_h;
	
	return y1;
}



void VPatchGUI::synchronize_fade(float value_change)
{
	if(fade && !change_source) 
	{
		fade->update(Units::to_int64(fade->get_value() + value_change));
		fade->update_edl();
	}
}


VFadePatch::VFadePatch(MWindow *mwindow, VPatchGUI *patch, int x, int y, int w)
 : BC_ISlider(x, 
			y,
			0,
			w, 
			w, 
			0, 
			MAX_VIDEO_FADE, 
			(int64_t)get_keyframe(mwindow, patch)->value)
{
	this->mwindow = mwindow;
	this->patch = patch;
}

float VFadePatch::update_edl()
{
	FloatAuto *current;
	double position = mwindow->edl->local_session->selectionstart;
	Autos *fade_autos = patch->vtrack->automation->fade_autos;
	int update_undo = !fade_autos->auto_exists_for_editing(position);

	if(update_undo)
		mwindow->undo->update_undo_before(_("fade"), LOAD_AUTOMATION);

	current = (FloatAuto*)fade_autos->get_auto_for_editing(position);

	float result = get_value() - current->value;
	current->value = get_value();

	if(update_undo)
		mwindow->undo->update_undo_after();

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


	mwindow->gui->unlock_window();
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->gui->lock_window("VFadePatch::handle_event");
	if(mwindow->edl->session->auto_conf->fade)
	{
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
	}
	return 1;
}

FloatAuto* VFadePatch::get_keyframe(MWindow *mwindow, VPatchGUI *patch)
{
	double unit_position = mwindow->edl->local_session->selectionstart;
	unit_position = mwindow->edl->align_to_frame(unit_position, 0);
	unit_position = patch->vtrack->to_units(unit_position, 0);
	Auto *current = 0;

	return (FloatAuto*)patch->vtrack->automation->fade_autos->get_prev_auto(
		(int64_t)unit_position, 
		PLAY_FORWARD,
		current);
}




VModePatch::VModePatch(MWindow *mwindow, VPatchGUI *patch, int x, int y)
 : BC_PopupMenu(x, 
 	y,
	patch->patchbay->mode_icons[0]->get_w() + 40,
	"",
	1)
{
	this->mwindow = mwindow;
	this->patch = patch;
	this->mode = get_keyframe(mwindow, patch)->value;
	set_icon(patch->patchbay->mode_to_icon(this->mode));
	set_tooltip("Overlay mode");
}

int VModePatch::handle_event()
{
// Set menu items
	for(int i = 0; i < total_items(); i++)
	{
		VModePatchItem *item = (VModePatchItem*)get_item(i);
		if(item->mode == mode) 
			item->set_checked(1);
		else
			item->set_checked(0);
	}

// Set keyframe
	IntAuto *current;
	double position = mwindow->edl->local_session->selectionstart;
	Autos *mode_autos = patch->vtrack->automation->mode_autos;
	int update_undo = !mode_autos->auto_exists_for_editing(position);

	if(update_undo)
		mwindow->undo->update_undo_before(_("mode"), LOAD_AUTOMATION);

	current = (IntAuto*)mode_autos->get_auto_for_editing(position);
	current->value = mode;

	if(update_undo)
		mwindow->undo->update_undo_after();

	mwindow->sync_parameters(CHANGE_PARAMS);

	if(mwindow->edl->session->auto_conf->mode)
	{
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
	}
	mwindow->session->changes_made = 1;
	return 1;
}

IntAuto* VModePatch::get_keyframe(MWindow *mwindow, VPatchGUI *patch)
{
	Auto *current = 0;
	double unit_position = mwindow->edl->local_session->selectionstart;
	unit_position = mwindow->edl->align_to_frame(unit_position, 0);
	unit_position = patch->vtrack->to_units(unit_position, 0);

	return (IntAuto*)patch->vtrack->automation->mode_autos->get_prev_auto(
		(int64_t)unit_position, 
		PLAY_FORWARD,
		current);
}


int VModePatch::create_objects()
{
	add_item(new VModePatchItem(this, mode_to_text(TRANSFER_NORMAL), TRANSFER_NORMAL));
	add_item(new VModePatchItem(this, mode_to_text(TRANSFER_ADDITION), TRANSFER_ADDITION));
	add_item(new VModePatchItem(this, mode_to_text(TRANSFER_SUBTRACT), TRANSFER_SUBTRACT));
	add_item(new VModePatchItem(this, mode_to_text(TRANSFER_MULTIPLY), TRANSFER_MULTIPLY));
	add_item(new VModePatchItem(this, mode_to_text(TRANSFER_DIVIDE), TRANSFER_DIVIDE));
	add_item(new VModePatchItem(this, mode_to_text(TRANSFER_REPLACE), TRANSFER_REPLACE));
	return 0;
}

void VModePatch::update(int mode)
{
	set_icon(patch->patchbay->mode_to_icon(mode));
}


char* VModePatch::mode_to_text(int mode)
{
	switch(mode)
	{
		case TRANSFER_NORMAL:
			return _("Normal");
			break;

		case TRANSFER_REPLACE:
			return _("Replace");
			break;

		case TRANSFER_ADDITION:
			return _("Addition");
			break;

		case TRANSFER_SUBTRACT:
			return _("Subtract");
			break;

		case TRANSFER_MULTIPLY:
			return _("Multiply");
			break;

		case TRANSFER_DIVIDE:
			return _("Divide");
			break;

		default:
			return _("Normal");
			break;
	}
	return "";
}






VModePatchItem::VModePatchItem(VModePatch *popup, char *text, int mode)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->mode = mode;
	if(this->mode == popup->mode) set_checked(1);
}

int VModePatchItem::handle_event()
{
	popup->mode = mode;
	popup->set_icon(popup->patch->patchbay->mode_to_icon(mode));
	popup->handle_event();
	return 1;
}
