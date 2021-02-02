// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef KEYFRAMEPOPUP_H
#define KEYFRAMEPOPUP_H

#include "bcpopupmenu.h"
#include "bcmenuitem.h"
#include "floatauto.inc"
#include "mwindow.inc"
#include "plugin.inc"
#include "keyframe.inc"
#include "automation.h" 


class KeyframePopupDelete;
class KeyframePopupCopy;
class KeyframePopupTangentMode;


class KeyframePopup : public BC_PopupMenu
{
public:
	KeyframePopup();
	~KeyframePopup();

	void update(Plugin *plugin, KeyFrame *keyframe);
	void update(Automation *automation, Autos *autos, Auto *auto_keyframe);

// Acquired through the update command as the plugin currently being operated on
	Plugin *keyframe_plugin;

	Autos *keyframe_autos;
	Automation *keyframe_automation;
	Auto *keyframe_auto;
private:
	void handle_tangent_mode(Autos *autos, Auto *auto_keyframe);

	KeyframePopupDelete *key_delete;
	KeyframePopupCopy *key_copy;
	KeyframePopupTangentMode *tan_smooth;
	KeyframePopupTangentMode *tan_linear;
	KeyframePopupTangentMode *tan_free_t;
	KeyframePopupTangentMode *tan_free;
	BC_MenuItem *hline;
	int delete_active;
	bool tangent_mode_displayed;
};

class KeyframePopupDelete : public BC_MenuItem
{
public:
	KeyframePopupDelete(KeyframePopup *popup);

	int handle_event();
private:
	KeyframePopup *popup;
};

class KeyframePopupCopy : public BC_MenuItem
{
public:
	KeyframePopupCopy(KeyframePopup *popup);

	int handle_event();
private:
	KeyframePopup *popup;
};

class KeyframePopupTangentMode : public BC_MenuItem
{
public:
	KeyframePopupTangentMode(KeyframePopup *popup, tgnt_mode tangent_mode);

	friend class KeyframePopup;

	int handle_event();

private:
	KeyframePopup *popup;
	tgnt_mode tangent_mode;
	const char* get_labeltext(tgnt_mode);
	void toggle_mode(FloatAuto*);

};

#endif
