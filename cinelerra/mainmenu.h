
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef MAINMENU_H
#define MAINMENU_H

class AEffectMenu;
class LabelsFollowEdits;
class PluginsFollowEdits;
class CursorOnFrames;
class LoopPlayback;

class Redo;
class ShowVWindow;
class ShowAWindow;
class ShowGWindow;
class ShowCWindow;
class ShowLWindow;
class Undo;

#include "arraylist.h"
#include "guicast.h"
#include "bchash.inc"
#include "loadfile.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "maxchannels.h"
#include "menuaeffects.inc"
#include "menuveffects.inc"
#include "module.inc"
#include "new.inc"
#include "plugindialog.inc"
#include "quit.inc"
#include "record.inc"
#include "render.inc"
#include "threadloader.inc"
#include "viewmenu.inc"

#define TOTAL_LOADS 10      // number of files to cache
#define TOTAL_EFFECTS 10     // number of effects to cache

class MainMenu : public BC_MenuBar
{
public:
	MainMenu(MWindow *mwindow, MWindowGUI *gui);
	~MainMenu();
	int create_objects();
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);

// most recent loads
	int add_load(char *path);
	int init_loads(BC_Hash *defaults);

// most recent effects
	int init_aeffects(BC_Hash *defaults);
	int save_aeffects(BC_Hash *defaults);
	int add_aeffect(char *title);
	int init_veffects(BC_Hash *defaults);
	int save_veffects(BC_Hash *defaults);
	int add_veffect(char *title);

	int quit();
// show only one of these at a time
	int set_show_autos();
	void update_toggles(int use_lock);

	MWindowGUI *gui;
	MWindow *mwindow;
	ThreadLoader *threadloader;
	MenuAEffects *aeffects;
	MenuVEffects *veffects;

	Load *load_file;
	BC_RecentList *recent_load;
	LoadPrevious *load[TOTAL_LOADS];


	RecordMenuItem *record;
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
	PluginsFollowEdits *plugins_follow_edits;
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
};

// ========================================= edit

class Undo : public BC_MenuItem
{
public:
	Undo(MWindow *mwindow);
	int handle_event();
	int update_caption(char *new_caption = "");
	MWindow *mwindow;
};



class DumpCICache : public BC_MenuItem
{
public:
	DumpCICache(MWindow *mwindow);
	int handle_event();
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

class DumpAssets : public BC_MenuItem
{
public:
	DumpAssets(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class Redo : public BC_MenuItem
{
public:
	Redo(MWindow *mwindow);
	int handle_event();
	int update_caption(char *new_caption = "");
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

class CutKeyframes : public BC_MenuItem
{
public:
	CutKeyframes(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class CopyKeyframes : public BC_MenuItem
{
public:
	CopyKeyframes(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class PasteKeyframes : public BC_MenuItem
{
public:
	PasteKeyframes(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class ClearKeyframes : public BC_MenuItem
{
public:
	ClearKeyframes(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class StraightenKeyframes : public BC_MenuItem
{
public:
	StraightenKeyframes(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class CutDefaultKeyframe : public BC_MenuItem
{
public:
	CutDefaultKeyframe(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class CopyDefaultKeyframe : public BC_MenuItem
{
public:
	CopyDefaultKeyframe(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class PasteDefaultKeyframe : public BC_MenuItem
{
public:
	PasteDefaultKeyframe(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class ClearDefaultKeyframe : public BC_MenuItem
{
public:
	ClearDefaultKeyframe(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
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

// ======================================== audio

class AddAudioTrack : public BC_MenuItem
{
public:
	AddAudioTrack(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class DeleteAudioTrack : public BC_MenuItem
{
public:
	DeleteAudioTrack(MWindow *mwindow);
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


class DeleteVideoTrack : public BC_MenuItem
{
public:
	DeleteVideoTrack(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class ResetTranslation : public BC_MenuItem
{
public:
	ResetTranslation(MWindow *mwindow);
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

class PluginsFollowEdits : public BC_MenuItem
{
public:
	PluginsFollowEdits(MWindow *mwindow);
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

class ScrubSpeed : public BC_MenuItem
{
public:
	ScrubSpeed(MWindow *mwindow);
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

#endif
