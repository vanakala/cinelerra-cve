#ifndef PLUGINPOPUP_H
#define PLUGINPOPUP_H

class PluginPopupAttach;
class PluginPopupDetach;
class PluginPopupIn;
class PluginPopupOut;
class PluginPopupOn;
class PluginPopupShow;
class KeyframePopupDelete;
class KeyframePopupShow;
class KeyframePopupCopy;

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "plugindialog.inc"
#include "keyframe.inc"


class PluginPopup : public BC_PopupMenu
{
public:
	PluginPopup(MWindow *mwindow, MWindowGUI *gui);
	~PluginPopup();

	void create_objects();
	int update(Plugin *plugin);

	MWindow *mwindow;
	MWindowGUI *gui;
// Acquired through the update command as the plugin currently being operated on
	Plugin *plugin;







	PluginPopupAttach *attach;
	PluginPopupDetach *detach;
//	PluginPopupIn *in;
//	PluginPopupOut *out;
	PluginPopupShow *show;
	PluginPopupOn *on;
};

class PluginPopupAttach : public BC_MenuItem
{
public:
	PluginPopupAttach(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupAttach();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
	PluginDialogThread *dialog_thread;
};

class PluginPopupDetach : public BC_MenuItem
{
public:
	PluginPopupDetach(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupDetach();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
};


class PluginPopupIn : public BC_MenuItem
{
public:
	PluginPopupIn(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupIn();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupOut : public BC_MenuItem
{
public:
	PluginPopupOut(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupOut();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupShow : public BC_MenuItem
{
public:
	PluginPopupShow(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupShow();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupOn : public BC_MenuItem
{
public:
	PluginPopupOn(MWindow *mwindow, PluginPopup *popup);
	~PluginPopupOn();

	int handle_event();

	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupUp : public BC_MenuItem
{
public:
	PluginPopupUp(MWindow *mwindow, PluginPopup *popup);
	int handle_event();
	MWindow *mwindow;
	PluginPopup *popup;
};

class PluginPopupDown : public BC_MenuItem
{
public:
	PluginPopupDown(MWindow *mwindow, PluginPopup *popup);
	int handle_event();
	MWindow *mwindow;
	PluginPopup *popup;
};


class KeyframePopup : public BC_PopupMenu
{
public:
	KeyframePopup(MWindow *mwindow, MWindowGUI *gui);
	~KeyframePopup();

	void create_objects();
	int update(Plugin *plugin, KeyFrame *keyframe);

	MWindow *mwindow;
	MWindowGUI *gui;
// Acquired through the update command as the plugin currently being operated on
	
	Plugin *plugin;
	KeyFrame *keyframe;
	
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
