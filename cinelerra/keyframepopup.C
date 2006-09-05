#include "cwindow.h"
#include "edl.h"
#include "keyframe.h"
#include "keyframepopup.h"
#include "language.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "localsession.h"
#include "cwindowgui.h" 
#include "cpanel.h"
#include "patchbay.h"
#include "patchgui.h" 
#include "apatchgui.h"
#include "vpatchgui.h"
#include "track.h"
#include "maincursor.h"
#include "bcwindowbase.h"
#include "filexml.h"
#include "edlsession.h"
#include "autoconf.h"

KeyframePopup::KeyframePopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	key_show = 0;
	key_delete = 0;
	key_copy = 0;
}

KeyframePopup::~KeyframePopup()
{
}

void KeyframePopup::create_objects()
{
//	add_item(key_show = new KeyframePopupShow(mwindow, this));
	add_item(key_delete = new KeyframePopupDelete(mwindow, this));
	add_item(key_copy = new KeyframePopupCopy(mwindow, this));
}

int KeyframePopup::update(Plugin *plugin, KeyFrame *keyframe)
{
	this->keyframe_plugin = plugin;
	this->keyframe_auto = keyframe;
	this->keyframe_autos = 0;
	this->keyframe_automation = 0;
	return 0;
}

int KeyframePopup::update(Automation *automation, Autos *autos, Auto *auto_keyframe)
{
	this->keyframe_plugin = 0;
	this->keyframe_automation = automation;
	this->keyframe_autos = autos;
	this->keyframe_auto = auto_keyframe;

	/* snap to cursor */
	double current_position = mwindow->edl->local_session->get_selectionstart(1);
	double new_position = keyframe_automation->track->from_units(keyframe_auto->position);
	mwindow->edl->local_session->set_selectionstart(new_position);
	mwindow->edl->local_session->set_selectionend(new_position);
	if (current_position != new_position)
	{
		mwindow->edl->local_session->set_selectionstart(new_position);
		mwindow->edl->local_session->set_selectionend(new_position);
		mwindow->gui->lock_window();
		mwindow->gui->update(1, 1, 1, 1, 1, 1, 0);	
		mwindow->gui->unlock_window();
	}
	return 0;
}

KeyframePopupDelete::KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Delete keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupDelete::~KeyframePopupDelete()
{
}

int KeyframePopupDelete::handle_event()
{
	delete popup->keyframe_auto;
	mwindow->save_backup();
	mwindow->undo->update_undo(_("delete keyframe"), LOAD_ALL);

	mwindow->gui->update(0,
	        1,      // 1 for incremental drawing.  2 for full refresh
	        0,
	        0,
	        0,
            0,   
            0);
	mwindow->update_plugin_guis();
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);

	return 1;
}

KeyframePopupShow::KeyframePopupShow(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Show keyframe settings"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupShow::~KeyframePopupShow()
{
}

int KeyframePopupShow::handle_event()
{
	if (popup->keyframe_plugin)
	{
		mwindow->update_plugin_guis();
		mwindow->show_plugin(popup->keyframe_plugin);
	} else
	if (popup->keyframe_automation)
	{
/*

		mwindow->cwindow->gui->lock_window();
		int show_window = 1;
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->projector_autos ||
		   popup->keyframe_autos == (Autos *)popup->keyframe_automation->pzoom_autos)
		   
		{
			mwindow->cwindow->gui->set_operation(CWINDOW_PROJECTOR);	
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->camera_autos ||
		   popup->keyframe_autos == (Autos *)popup->keyframe_automation->czoom_autos)
		   
		{
			mwindow->cwindow->gui->set_operation(CWINDOW_CAMERA);	
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mode_autos)
		   
		{
			// no window to be shown
			show_window = 0;
			// first find the appropriate patchgui
			PatchBay *patchbay = mwindow->gui->patchbay;
			PatchGUI *patchgui = 0;
			for (int i = 0; i < patchbay->patches.total; i++)
				if (patchbay->patches.values[i]->track == popup->keyframe_automation->track)
					patchgui = patchbay->patches.values[i];		
			if (patchgui != 0)
			{
// FIXME: repositioning of the listbox needs support in guicast
//				int cursor_x = popup->get_relative_cursor_x();
//				int cursor_y = popup->get_relative_cursor_y();
//				vpatchgui->mode->reposition_window(cursor_x, cursor_y);


// Open the popup menu
				VPatchGUI *vpatchgui = (VPatchGUI *)patchgui;
				vpatchgui->mode->activate_menu();
			}
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mask_autos)
		   
		{
			mwindow->cwindow->gui->set_operation(CWINDOW_MASK);	
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->pan_autos)
		   
		{
			// no window to be shown
			show_window = 0;
			// first find the appropriate patchgui
			PatchBay *patchbay = mwindow->gui->patchbay;
			PatchGUI *patchgui = 0;
			for (int i = 0; i < patchbay->patches.total; i++)
				if (patchbay->patches.values[i]->track == popup->keyframe_automation->track)
					patchgui = patchbay->patches.values[i];		
			if (patchgui != 0)
			{
// Open the popup menu at current mouse position
				APatchGUI *apatchgui = (APatchGUI *)patchgui;
				int cursor_x = popup->get_relative_cursor_x();
				int cursor_y = popup->get_relative_cursor_y();
				apatchgui->pan->activate(cursor_x, cursor_y);
			}
			

		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->fade_autos)
		   
		{
			// no window to be shown, so do nothing
			// IDEA: open window for fading
			show_window = 0;
		} else
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mute_autos)
		   
		{
			// no window to be shown, so do nothing
			// IDEA: directly switch
			show_window = 0;
		} else;
		

// ensure bringing to front
		if (show_window)
		{
			((CPanelToolWindow *)(mwindow->cwindow->gui->composite_panel->operation[CWINDOW_TOOL_WINDOW]))->set_shown(0);
			((CPanelToolWindow *)(mwindow->cwindow->gui->composite_panel->operation[CWINDOW_TOOL_WINDOW]))->set_shown(1);
		}
		mwindow->cwindow->gui->unlock_window();


*/
	}
	return 1;
}



KeyframePopupCopy::KeyframePopupCopy(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Copy keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupCopy::~KeyframePopupCopy()
{
}

int KeyframePopupCopy::handle_event()
{
/*
	FIXME:
	we want to copy just keyframe under cursor, NOT all keyframes at this frame
	- very hard to do, so this is good approximation for now...
*/
	
#if 0
	if (popup->keyframe_automation)
	{
		FileXML file;
		EDL *edl = mwindow->edl;
		Track *track = popup->keyframe_automation->track;
		int64_t position = popup->keyframe_auto->position;
		AutoConf autoconf;
// first find out type of our auto
		autoconf.set_all(0);
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->projector_autos)
			autoconf.projector = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->pzoom_autos)
			autoconf.pzoom = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->camera_autos)
			autoconf.camera = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->czoom_autos)
			autoconf.czoom = 1;		
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mode_autos)
		   	autoconf.mode = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mask_autos)
			autoconf.mask = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->pan_autos)
			autoconf.pan = 1;		   
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->fade_autos)
			autoconf.fade = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mute_autos)
			autoconf.mute = 1;		


// now create a clipboard
		file.tag.set_title("AUTO_CLIPBOARD");
		file.tag.set_property("LENGTH", 0);
		file.tag.set_property("FRAMERATE", edl->session->frame_rate);
		file.tag.set_property("SAMPLERATE", edl->session->sample_rate);
		file.append_tag();
		file.append_newline();
		file.append_newline();

/*		track->copy_automation(position, 
			position, 
			&file,
			0,
			0);
			*/
		file.tag.set_title("TRACK");
// Video or audio
		track->save_header(&file);
		file.append_tag();
		file.append_newline();

		track->automation->copy(position, 
			position, 
			&file,
			0,
			0,
			&autoconf);
		
		
		
		file.tag.set_title("/TRACK");
		file.append_tag();
		file.append_newline();
		file.append_newline();
		file.append_newline();
		file.append_newline();



		file.tag.set_title("/AUTO_CLIPBOARD");
		file.append_tag();
		file.append_newline();
		file.terminate_string();

		mwindow->gui->lock_window();
		mwindow->gui->get_clipboard()->to_clipboard(file.string, 
			strlen(file.string), 
			SECONDARY_SELECTION);
		mwindow->gui->unlock_window();

	} else
#endif
		mwindow->copy_automation();
	return 1;
}



