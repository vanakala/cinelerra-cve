#ifndef KEYFRAMEPOPUP_H
#define KEYFRAMEPOPUP_H

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "plugindialog.inc"
#include "keyframe.inc"
#include "automation.h" 


class KeyframePopupDelete;
class KeyframePopupShow;
class KeyframePopupCopy;
 
 
class KeyframePopup : public BC_PopupMenu
{
public:
	KeyframePopup(MWindow *mwindow, MWindowGUI *gui);
	~KeyframePopup();

	void create_objects();
	int update(Plugin *plugin, KeyFrame *keyframe);
	int update(Automation *automation, Autos *autos, Auto *auto_keyframe);

	MWindow *mwindow;
	MWindowGUI *gui;
// Acquired through the update command as the plugin currently being operated on
	
	Plugin *keyframe_plugin;
	Autos *keyframe_autos;
	Automation *keyframe_automation;
	Auto *keyframe_auto;
	

	
	KeyframePopupDelete *key_delete;
	KeyframePopupShow *key_show;
	KeyframePopupCopy *key_copy;
};

class KeyframePopupDelete : public BC_MenuItem
{
public:
	KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup);
	~KeyframePopupDelete();
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupShow : public BC_MenuItem
{
public:
	KeyframePopupShow(MWindow *mwindow, KeyframePopup *popup);
	~KeyframePopupShow();
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupCopy : public BC_MenuItem
{
public:
	KeyframePopupCopy(MWindow *mwindow, KeyframePopup *popup);
	~KeyframePopupCopy();
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};



 #endif
