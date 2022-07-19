// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINMENU_H
#define MAINMENU_H

class AEffectMenu;
class LabelsFollowEdits;
class CursorOnFrames;
class LoopPlayback;

class Redo;
class ShowVWindow;
class ShowAWindow;
class ShowGWindow;
class ShowCWindow;
class ShowLWindow;
class ShowRuler;
class Undo;

#include "arraylist.h"
#include "bchash.inc"
#include "bcmenubar.h"
#include "bcmenuitem.h"
#include "bcrecentlist.inc"
#include "loadfile.inc"
#include "menuaeffects.inc"
#include "menuveffects.inc"
#include "new.inc"
#include "plugindialog.inc"
#include "quit.inc"
#include "render.inc"
#include "viewmenu.inc"

#define TOTAL_LOADS 10      // number of files to cache
#define TOTAL_EFFECTS 10     // number of effects to cache

class MainMenu : public BC_MenuBar
{
public:
	MainMenu(BC_WindowBase *gui);

	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);

// most recent loads
	void add_load(const char *path);
	void init_loads(BC_Hash *defaults);

// most recent effects
	void init_aeffects(BC_Hash *defaults);
	void save_aeffects(BC_Hash *defaults);
	void add_aeffect(const char *title);
	void init_veffects(BC_Hash *defaults);
	void save_veffects(BC_Hash *defaults);
	void add_veffect(const char *title);

	void quit();
// show only one of these at a time
	int set_show_autos();
	void update_toggles();

	MenuAEffects *aeffects;
	MenuVEffects *veffects;

	Load *load_file;
	BC_RecentList *recent_load;
	LoadPrevious *load[TOTAL_LOADS];

	RenderItem *render;
	New *new_project;
	MenuAEffectItem *aeffect[TOTAL_EFFECTS];
	MenuVEffectItem *veffect[TOTAL_EFFECTS];
	Quit *quit_program;              // affected by save
	Undo *undo;
	Redo *redo;
	int total_aeffects;
	int total_veffects;
	BC_Menu *filemenu, *audiomenu, *videomenu;      // needed by most recents

	LabelsFollowEdits *labels_follow_edits;
	CursorOnFrames *cursor_on_frames;
	LoopPlayback *loop_playback;
	ShowAssets *show_assets;
	ShowTitles *show_titles;
	ShowTransitions *show_transitions;
	ShowAutomation *fade_automation;
	ShowAutomation *mute_automation;
	ShowAutomation *pan_automation;
	ShowAutomation *camera_x;
	ShowAutomation *camera_y;
	ShowAutomation *camera_z;
	ShowAutomation *project_x;
	ShowAutomation *project_y;
	ShowAutomation *project_z;
	PluginAutomation *plugin_automation;
	ShowAutomation *mask_automation;
	ShowAutomation *mode_automation;
	ShowVWindow *show_vwindow;
	ShowAWindow *show_awindow;
	ShowCWindow *show_cwindow;
	ShowGWindow *show_gwindow;
	ShowLWindow *show_lwindow;
	ShowRuler *show_ruler;
};

// ========================================= edit

class Undo : public BC_MenuItem
{
public:
	Undo();

	int handle_event();
	void update_caption(const char *new_caption = "");
};

class DumpEDL : public BC_MenuItem
{
public:
	DumpEDL();

	int handle_event();
};

class DumpPlugins : public BC_MenuItem
{
public:
	DumpPlugins();

	int handle_event();
};

class Redo : public BC_MenuItem
{
public:
	Redo();

	int handle_event();
	void update_caption(const char *new_caption = "");
};

class Cut : public BC_MenuItem
{
public:
	Cut();

	int handle_event();
};

class Copy : public BC_MenuItem
{
public:
	Copy();

	int handle_event();
};

class Paste : public BC_MenuItem
{
public:
	Paste();

	int handle_event();
};

class Clear : public BC_MenuItem
{
public:
	Clear();

	int handle_event();
};

class CutEffects : public BC_MenuItem
{
public:
	CutEffects();

	int handle_event();
};

class CopyEffects : public BC_MenuItem
{
public:
	CopyEffects();

	int handle_event();
};

class PasteEffects : public BC_MenuItem
{
public:
	PasteEffects();

	int handle_event();
};

class PasteAutos : public BC_MenuItem
{
public:
	PasteAutos();

	int handle_event();
};

class PastePlugins : public BC_MenuItem
{
public:
	PastePlugins();

	int handle_event();
};

class ClearKeyframes : public BC_MenuItem
{
public:
	ClearKeyframes();

	int handle_event();
};

class StraightenKeyframes : public BC_MenuItem
{
public:
	StraightenKeyframes();

	int handle_event();
};

class PasteSilence : public BC_MenuItem
{
public:
	PasteSilence();

	int handle_event();
};

class SelectAll : public BC_MenuItem
{
public:
	SelectAll();

	int handle_event();
};

class ClearLabels : public BC_MenuItem
{
public:
	ClearLabels();

	int handle_event();
};

class MuteSelection : public BC_MenuItem
{
public:
	MuteSelection();

	int handle_event();
};

class TrimSelection : public BC_MenuItem
{
public:
	TrimSelection();

	int handle_event();
};

class EditNotes : public BC_MenuItem
{
public:
	EditNotes();

	int handle_event();
};

// ======================================== audio

class AddAudioTrack : public BC_MenuItem
{
public:
	AddAudioTrack();

	int handle_event();
};

class DefaultATransition : public BC_MenuItem
{
public:
	DefaultATransition();

	int handle_event();
};

class MapAudio1 : public BC_MenuItem
{
public:
	MapAudio1();

	int handle_event();
};

class MapAudio2 : public BC_MenuItem
{
public:
	MapAudio2();

	int handle_event();
};

// ========================================== video

class AddVideoTrack : public BC_MenuItem
{
public:
	AddVideoTrack();

	int handle_event();
};


class DefaultVTransition : public BC_MenuItem
{
public:
	DefaultVTransition();

	int handle_event();
};

// ========================================== settings

class MoveTracksUp : public BC_MenuItem
{
public:
	MoveTracksUp();

	int handle_event();
};

class MoveTracksDown : public BC_MenuItem
{
public:
	MoveTracksDown();

	int handle_event();
};

class DeleteTracks : public BC_MenuItem
{
public:
	DeleteTracks();

	int handle_event();
};

class ConcatenateTracks : public BC_MenuItem
{
public:
	ConcatenateTracks();

	int handle_event();
};

class DeleteTrack : public BC_MenuItem
{
public:
	DeleteTrack();

	int handle_event();
};

class LoopPlayback : public BC_MenuItem
{
public:
	LoopPlayback();

	int handle_event();
};

class SetBRenderStart : public BC_MenuItem
{
public:
	SetBRenderStart();

	int handle_event();
};

class LabelsFollowEdits : public BC_MenuItem
{
public:
	LabelsFollowEdits();

	int handle_event();
};

class CursorOnFrames : public BC_MenuItem
{
public:
	CursorOnFrames();

	int handle_event();
};

class SaveSettingsNow : public BC_MenuItem
{
public:
	SaveSettingsNow();

	int handle_event();
};

// ========================================== window
class ShowVWindow : public BC_MenuItem
{
public:
	ShowVWindow();

	int handle_event();
};

class ShowAWindow : public BC_MenuItem
{
public:
	ShowAWindow();

	int handle_event();
};

class ShowGWindow : public BC_MenuItem
{
public:
	ShowGWindow();

	int handle_event();
};

class ShowCWindow : public BC_MenuItem
{
public:
	ShowCWindow();

	int handle_event();
};

class ShowLWindow : public BC_MenuItem
{
public:
	ShowLWindow();

	int handle_event();
};

class ShowRuler : public BC_MenuItem
{
public:
	ShowRuler();

	int handle_event();
};

class ShowStatus : public BC_MenuItem
{
public:
	ShowStatus();

	int handle_event();
};

#endif
