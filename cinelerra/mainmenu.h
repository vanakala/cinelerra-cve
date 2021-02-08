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
#include "mwindow.inc"
#include "mwindowgui.inc"
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
	MainMenu(MWindow *mwindow, BC_WindowBase *gui);

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

	MWindow *mwindow;
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
	Undo(MWindow *mwindow);

	int handle_event();
	int update_caption(const char *new_caption = "");

	MWindow *mwindow;
};

class DumpEDL : public BC_MenuItem
{
public:
	DumpEDL(MWindow *mwindow);

	int handle_event();
	MWindow *mwindow;
};

class DumpPlugins : public BC_MenuItem
{
public:
	DumpPlugins(MWindow *mwindow);

	int handle_event();
	MWindow *mwindow;
};

class Redo : public BC_MenuItem
{
public:
	Redo(MWindow *mwindow);

	int handle_event();
	int update_caption(const char *new_caption = "");

	MWindow *mwindow;
};

class Cut : public BC_MenuItem
{
public:
	Cut(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class Copy : public BC_MenuItem
{
public:
	Copy(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class Paste : public BC_MenuItem
{
public:
	Paste(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class Clear : public BC_MenuItem
{
public:
	Clear(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
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
	PasteSilence(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class SelectAll : public BC_MenuItem
{
public:
	SelectAll(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class ClearLabels : public BC_MenuItem
{
public:
	ClearLabels(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class MuteSelection : public BC_MenuItem
{
public:
	MuteSelection(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class TrimSelection : public BC_MenuItem
{
public:
	TrimSelection(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
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
	AddAudioTrack(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class DefaultATransition : public BC_MenuItem
{
public:
	DefaultATransition(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class MapAudio1 : public BC_MenuItem
{
public:
	MapAudio1(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class MapAudio2 : public BC_MenuItem
{
public:
	MapAudio2(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

// ========================================== video

class AddVideoTrack : public BC_MenuItem
{
public:
	AddVideoTrack(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};


class DefaultVTransition : public BC_MenuItem
{
public:
	DefaultVTransition(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

// ========================================== settings

class MoveTracksUp : public BC_MenuItem
{
public:
	MoveTracksUp(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class MoveTracksDown : public BC_MenuItem
{
public:
	MoveTracksDown(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class DeleteTracks : public BC_MenuItem
{
public:
	DeleteTracks(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class ConcatenateTracks : public BC_MenuItem
{
public:
	ConcatenateTracks(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class DeleteTrack : public BC_MenuItem
{
public:
	DeleteTrack(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class LoopPlayback : public BC_MenuItem
{
public:
	LoopPlayback(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class SetBRenderStart : public BC_MenuItem
{
public:
	SetBRenderStart(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class LabelsFollowEdits : public BC_MenuItem
{
public:
	LabelsFollowEdits(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class CursorOnFrames : public BC_MenuItem
{
public:
	CursorOnFrames(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class AutosFollowEdits : public BC_MenuItem
{
public:
	AutosFollowEdits(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class SaveSettingsNow : public BC_MenuItem
{
public:
	SaveSettingsNow(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

// ========================================== window
class ShowVWindow : public BC_MenuItem
{
public:
	ShowVWindow(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class ShowAWindow : public BC_MenuItem
{
public:
	ShowAWindow(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class ShowGWindow : public BC_MenuItem
{
public:
	ShowGWindow(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class ShowCWindow : public BC_MenuItem
{
public:
	ShowCWindow(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class ShowLWindow : public BC_MenuItem
{
public:
	ShowLWindow(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class TileWindows : public BC_MenuItem
{
public:
	TileWindows(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class ShowRuler : public BC_MenuItem
{
public:
	ShowRuler(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

class ShowStatus : public BC_MenuItem
{
public:
	ShowStatus(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};

#endif
