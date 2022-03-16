// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "autos.h"
#include "bcsignals.h"
#include "cinelerra.h"
#include "cwindow.h"
#include "edl.h"
#include "floatauto.h"
#include "keyframe.h"
#include "keyframes.h"
#include "keyframepopup.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "localsession.h"
#include "plugin.h"
#include "track.h"
#include "trackplugin.h"

KeyframePopup::KeyframePopup()
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	key_delete = 0;
	delete_active = 0;
	key_copy = 0;
	tan_smooth = 0;
	tan_linear = 0;
	tan_free = 0;
	tangent_mode_displayed = false;

	add_item(key_copy = new KeyframePopupCopy(this));
	add_item(key_delete = new KeyframePopupDelete(this));
	delete_active = 1;

	hline = new BC_MenuItem("-");
	tan_smooth = new KeyframePopupTangentMode(this, TGNT_SMOOTH);
	tan_linear = new KeyframePopupTangentMode(this, TGNT_LINEAR);
	tan_free_t = new KeyframePopupTangentMode(this, TGNT_TFREE);
	tan_free = new KeyframePopupTangentMode(this, TGNT_FREE);
}

KeyframePopup::~KeyframePopup()
{
	if(!tangent_mode_displayed)
	{
		delete tan_smooth;
		delete tan_linear;
		delete tan_free_t;
		delete tan_free;
		delete hline;
	}
	if(!delete_active)
		delete key_delete;
} // if they are currently displayed, the menu class will delete them automatically

void KeyframePopup::update(Plugin *plugin, KeyFrame *keyframe)
{
	this->keyframe_plugin = plugin;
	this->keyframe_auto = keyframe;
	this->keyframe_autos = 0;
	this->keyframe_automation = 0;
	handle_tangent_mode(0, 0);

	if(plugin->keyframes->first == keyframe)
	{
		if(delete_active)
		{
			remove_item(key_delete);
			delete_active = 0;
		}
	}
	else
	{
		if(!delete_active)
		{
			add_item(key_delete);
			delete_active = 1;
		}
	}
}

void KeyframePopup::update(Automation *automation, Autos *autos, Auto *auto_keyframe)
{
	this->keyframe_plugin = 0;
	this->keyframe_automation = automation;
	this->keyframe_autos = autos;
	this->keyframe_auto = auto_keyframe;

	/* FIXPOS snap to cursor */
	ptstime current_position = master_edl->local_session->get_selectionstart(1);
	ptstime new_position = keyframe_auto->pos_time;
	if(autos->first == auto_keyframe)
	{
		if(delete_active)
		{
			remove_item(key_delete);
			delete_active = 0;
		}
	}
	else
	{
		if(!delete_active)
		{
			if(tangent_mode_displayed)
				handle_tangent_mode(0, 0);

			add_item(key_delete);
			delete_active = 1;
		}
	}

	handle_tangent_mode(autos, auto_keyframe);

	master_edl->local_session->set_selection(new_position);

	if(!PTSEQU(current_position, new_position))
	{
		mwindow_global->update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR |
			WUPD_TIMEBAR | WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);
	}
}

void KeyframePopup::handle_tangent_mode(Autos *autos, Auto *auto_keyframe)
// determines the type of automation node. if floatauto, adds
// menu entries showing the tangent mode of the node
{
	if(!tangent_mode_displayed && autos && autos->get_type() == AUTOMATION_TYPE_FLOAT)
	{ // append additional menu entries showing the tangent-mode
		add_item(hline);
		add_item(tan_smooth);
		add_item(tan_linear);
		add_item(tan_free_t);
		add_item(tan_free);
		tangent_mode_displayed = true;
	}
	if(tangent_mode_displayed && (!autos || autos->get_type() != AUTOMATION_TYPE_FLOAT))
	{ // remove additional menu entries
		remove_item(tan_free);
		remove_item(tan_free_t);
		remove_item(tan_linear);
		remove_item(tan_smooth);
		remove_item(hline);
		tangent_mode_displayed = false;
	}
	if(tangent_mode_displayed && auto_keyframe)
	{ // set checkmarks to display current mode
		tan_smooth->toggle_mode((FloatAuto*)auto_keyframe);
		tan_linear->toggle_mode((FloatAuto*)auto_keyframe);
		tan_free_t->toggle_mode((FloatAuto*)auto_keyframe);
		tan_free->toggle_mode((FloatAuto*)auto_keyframe);
	}
}


KeyframePopupDelete::KeyframePopupDelete(KeyframePopup *popup)
 : BC_MenuItem(_("Delete keyframe"))
{
	this->popup = popup;
}

int KeyframePopupDelete::handle_event()
{
	delete popup->keyframe_auto;
	mwindow_global->save_backup();
	mwindow_global->undo->update_undo(_("delete keyframe"), LOAD_ALL);

	if(popup->keyframe_plugin)
	{
		if(popup->keyframe_plugin->trackplugin)
			popup->keyframe_plugin->trackplugin->update();
	}
	else
		mwindow_global->update_gui(WUPD_CANVINCR);
	mwindow_global->update_plugin_guis();
	mwindow_global->sync_parameters();
	return 1;
}


KeyframePopupCopy::KeyframePopupCopy(KeyframePopup *popup)
 : BC_MenuItem(_("Copy keyframe"))
{
	this->popup = popup;
}

int KeyframePopupCopy::handle_event()
{
	mwindow_global->copy_keyframes(popup->keyframe_autos,
		popup->keyframe_auto, popup->keyframe_plugin);
	return 1;
}


KeyframePopupTangentMode::KeyframePopupTangentMode(KeyframePopup *popup,
	tgnt_mode tangent_mode)
 : BC_MenuItem(get_labeltext(tangent_mode))
{
	this->tangent_mode = tangent_mode;
	this->popup = popup;
}

const char* KeyframePopupTangentMode::get_labeltext(tgnt_mode mode)
{
	switch(mode)
	{
	case TGNT_SMOOTH:
		return _("Smooth curve");
	case TGNT_LINEAR:
		return _("Linear segments");
	case TGNT_TFREE:
		return _("Tangent edit");
	case TGNT_FREE:
		return _("Disjoint edit");
	}
	return "misconfigured";
}

void KeyframePopupTangentMode::toggle_mode(FloatAuto *keyframe)
{
	set_checked(tangent_mode == keyframe->tangent_mode);
}

int KeyframePopupTangentMode::handle_event()
{
	if(popup->keyframe_autos &&
		popup->keyframe_autos->get_type() == AUTOMATION_TYPE_FLOAT)
	{
		((FloatAuto*)popup->keyframe_auto)->
			change_tangent_mode(tangent_mode);

		// if we switched to some "auto" mode, this may imply a
		// real change to parameters, so this needs to be undoable...
		mwindow_global->save_backup();
		mwindow_global->undo->update_undo(_("change keyframe tangent mode"), LOAD_ALL);

		mwindow_global->update_gui(WUPD_CANVINCR);
		mwindow_global->cwindow->update(WUPD_TOOLWIN);
		mwindow_global->update_plugin_guis();
		mwindow_global->sync_parameters();
	}
	return 1;
}
