#include "autoconf.h"
#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "intauto.h"
#include "intautos.h"
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

	if(fade) fade->reposition_window(x1 + x, 
		y1 + y);
	y1 += mwindow->theme->fade_h;
	if(mode) mode->reposition_window(x1 + x,
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
				(long)unit_position,
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
		}
		else
		{
			mode->update(mode->get_keyframe(mwindow, this)->value);
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
	}
	y1 += mwindow->theme->mode_h;
	
	return y1;
}


void VPatchGUI::create_mode()
{
}


void VPatchGUI::synchronize_fade(float value_change)
{
	if(fade && !change_source) 
	{
		fade->update(Units::to_long(fade->get_value() + value_change));
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
			(long)get_keyframe(mwindow, patch)->value)
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
		mwindow->undo->update_undo_before("fade", LOAD_AUTOMATION);

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
	mwindow->gui->lock_window();
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
		(long)unit_position, 
		PLAY_FORWARD,
		current);
}




VModePatch::VModePatch(MWindow *mwindow, VPatchGUI *patch, int x, int y)
 : BC_PopupMenu(x, 
 	y,
	patch->patchbay->get_w() - x - 10,
	mode_to_text(get_keyframe(mwindow, patch)->value),
	1)
{
	this->mwindow = mwindow;
	this->patch = patch;
}

int VModePatch::handle_event()
{
	IntAuto *current;
	double position = mwindow->edl->local_session->selectionstart;
	Autos *mode_autos = patch->vtrack->automation->mode_autos;
	int update_undo = !mode_autos->auto_exists_for_editing(position);

	if(update_undo)
		mwindow->undo->update_undo_before("mode", LOAD_AUTOMATION);

	current = (IntAuto*)mode_autos->get_auto_for_editing(position);
	current->value = text_to_mode(get_text());

	if(update_undo)
		mwindow->undo->update_undo_after();

//printf("VModePatch::handle_event %d\n", text_to_mode(get_text()));
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
		(long)unit_position, 
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
	set_text(mode_to_text(mode));
}

int VModePatch::text_to_mode(char *text)
{
//printf("%s %s\n", mode_to_text(TRANSFER_MULTIPLY), text);
	if(!strcasecmp(mode_to_text(TRANSFER_NORMAL), text)) return TRANSFER_NORMAL;
	if(!strcasecmp(mode_to_text(TRANSFER_ADDITION), text)) return TRANSFER_ADDITION;
	if(!strcasecmp(mode_to_text(TRANSFER_SUBTRACT), text)) return TRANSFER_SUBTRACT;
	if(!strcasecmp(mode_to_text(TRANSFER_MULTIPLY), text)) return TRANSFER_MULTIPLY;
	if(!strcasecmp(mode_to_text(TRANSFER_DIVIDE), text)) return TRANSFER_DIVIDE;
	if(!strcasecmp(mode_to_text(TRANSFER_REPLACE), text)) return TRANSFER_REPLACE;
	return TRANSFER_NORMAL;
}

char* VModePatch::mode_to_text(int mode)
{
	switch(mode)
	{
		case TRANSFER_NORMAL:
			return "Normal";
			break;
		
		case TRANSFER_REPLACE:
			return "Replace";
			break;
		
		case TRANSFER_ADDITION:
			return "Addition";
			break;
		
		case TRANSFER_SUBTRACT:
			return "Subtract";
			break;
		
		case TRANSFER_MULTIPLY:
			return "Multiply";
			break;
		
		case TRANSFER_DIVIDE:
			return "Divide";
			break;
		
		default:
			return "Normal";
			break;
	}
	return "";
}





VModePatchItem::VModePatchItem(VModePatch *popup, char *text, int mode)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->mode = mode;
}

int VModePatchItem::handle_event()
{
	popup->set_text(get_text());
	popup->handle_event();
	return 1;
}
