// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "autoconf.h"
#include "automation.inc"
#include "batchrender.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "bchash.h"
#include "bclistboxitem.h"
#include "bcmenu.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "keys.h"
#include "language.h"
#include "loadfile.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "menuaeffects.h"
#include "menuveffects.h"
#include "mwindow.h"
#include "new.h"
#include "plugindb.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "quit.h"
#include "render.h"
#include "savefile.h"
#include "setformat.h"
#include "viewmenu.h"
#include "exportedl.h"

#include <string.h>


MainMenu::MainMenu(BC_WindowBase *gui)
 : BC_MenuBar(0, 0, gui->get_w())
{
	BC_Menu *viewmenu, *windowmenu, *settingsmenu, *trackmenu;
	PreferencesMenuitem *preferences;

	recent_load = new BC_RecentList("PATH", mwindow_global->defaults);
	recent_load->set_options(RECENT_OPT_BASEUNQ);

	add_menu(filemenu = new BC_Menu(_("File")));
	filemenu->add_item(new_project = new New);

// file loaders
	filemenu->add_item(load_file = new Load(this));

// new and load can be undone so no need to prompt save
	Save *save;                   //  affected by saveas
	filemenu->add_item(save = new Save());
	SaveAs *saveas;
	filemenu->add_item(saveas = new SaveAs());
	save->set_saveas(saveas);
	saveas->set_mainmenu(this);

	filemenu->add_item(render = new RenderItem());
	filemenu->add_item(new ExportEDLItem());
	filemenu->add_item(new BatchRenderMenuItem());
	filemenu->add_item(new BC_MenuItem("-"));
	filemenu->add_item(quit_program = new Quit(save));
	filemenu->add_item(new DumpEDL());
	filemenu->add_item(new DumpPlugins());
	filemenu->add_item(new ShowStatus());
	filemenu->add_item(new LoadBackup());
	filemenu->add_item(new SaveBackup());

	BC_Menu *editmenu;
	add_menu(editmenu = new BC_Menu(_("Edit")));
	editmenu->add_item(undo = new Undo());
	editmenu->add_item(redo = new Redo());
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new Cut());
	editmenu->add_item(new Copy());
	editmenu->add_item(new Paste());
	editmenu->add_item(new Clear());
	editmenu->add_item(new PasteSilence());
	editmenu->add_item(new MuteSelection());
	editmenu->add_item(new TrimSelection());
	editmenu->add_item(new SelectAll());
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new ClearLabels());
	editmenu->add_item(new EditNotes());

	BC_Menu *menu;
	add_menu(menu = new BC_Menu(_("Effects")));
	menu->add_item(new CutEffects());
	menu->add_item(new CopyEffects());
	menu->add_item(new PasteEffects());
	menu->add_item(new PasteAutos());
	menu->add_item(new PastePlugins());
	menu->add_item(new ClearKeyframes());
	menu->add_item(new StraightenKeyframes());

	add_menu(audiomenu = new BC_Menu(_("Audio")));
	audiomenu->add_item(new AddAudioTrack());
	audiomenu->add_item(new DefaultATransition());
	audiomenu->add_item(new MapAudio1());
	audiomenu->add_item(new MapAudio2());
	audiomenu->add_item(aeffects = new MenuAEffects());
	add_menu(videomenu = new BC_Menu(_("Video")));
	videomenu->add_item(new AddVideoTrack());
	videomenu->add_item(new DefaultVTransition());
	videomenu->add_item(veffects = new MenuVEffects());

	add_menu(trackmenu = new BC_Menu(_("Tracks")));
	trackmenu->add_item(new MoveTracksUp());
	trackmenu->add_item(new MoveTracksDown());
	trackmenu->add_item(new DeleteTracks());
	trackmenu->add_item(new DeleteTrack());
	trackmenu->add_item(new ConcatenateTracks());

	add_menu(settingsmenu = new BC_Menu(_("Settings")));

	settingsmenu->add_item(new SetFormat());
	settingsmenu->add_item(preferences = new PreferencesMenuitem());
	mwindow_global->preferences_thread = preferences->thread;
	settingsmenu->add_item(labels_follow_edits = new LabelsFollowEdits());
	settingsmenu->add_item(cursor_on_frames = new CursorOnFrames());
	settingsmenu->add_item(new SaveSettingsNow());
	settingsmenu->add_item(loop_playback = new LoopPlayback());
	settingsmenu->add_item(new SetBRenderStart());

	add_menu(viewmenu = new BC_Menu(_("View")));
	viewmenu->add_item(show_assets = new ShowAssets("0"));
	viewmenu->add_item(show_titles = new ShowTitles("1"));
	viewmenu->add_item(show_transitions = new ShowTransitions("2"));
	viewmenu->add_item(fade_automation = new ShowAutomation(_("Fade"), "3", AUTOMATION_AFADE));
	viewmenu->add_item(mute_automation = new ShowAutomation(_("Mute"), "4", AUTOMATION_MUTE));
	viewmenu->add_item(mode_automation = new ShowAutomation(_("Overlay mode"), "5", AUTOMATION_MODE));
	viewmenu->add_item(pan_automation = new ShowAutomation(_("Pan"), "6", AUTOMATION_PAN));
	viewmenu->add_item(plugin_automation = new PluginAutomation("7"));
	viewmenu->add_item(mask_automation = new ShowAutomation(_("Mask"), "8", AUTOMATION_MASK));
	viewmenu->add_item(mask_automation = new ShowAutomation(_("Crop"), "9", AUTOMATION_CROP));
	viewmenu->add_item(camera_x = new ShowAutomation(_("Camera X"), "", AUTOMATION_CAMERA_X));
	viewmenu->add_item(camera_y = new ShowAutomation(_("Camera Y"), "", AUTOMATION_CAMERA_Y));
	viewmenu->add_item(camera_z = new ShowAutomation(_("Camera Z"), "", AUTOMATION_CAMERA_Z));
	viewmenu->add_item(project_x = new ShowAutomation(_("Projector X"), "", AUTOMATION_PROJECTOR_X));
	viewmenu->add_item(project_y = new ShowAutomation(_("Projector Y"), "", AUTOMATION_PROJECTOR_Y));
	viewmenu->add_item(project_z = new ShowAutomation(_("Projector Z"), "", AUTOMATION_PROJECTOR_Z));

	add_menu(windowmenu = new BC_Menu(_("Window")));
	windowmenu->add_item(show_vwindow = new ShowVWindow());
	windowmenu->add_item(show_awindow = new ShowAWindow());
	windowmenu->add_item(show_cwindow = new ShowCWindow());
	windowmenu->add_item(show_gwindow = new ShowGWindow());
	windowmenu->add_item(show_lwindow = new ShowLWindow());
	windowmenu->add_item(show_ruler = new ShowRuler());
}

void MainMenu::load_defaults(BC_Hash *defaults)
{
	init_loads(defaults);
	init_aeffects(defaults);
	init_veffects(defaults);
}

void MainMenu::update_toggles()
{
	labels_follow_edits->set_checked(edlsession->labels_follow_edits);
	cursor_on_frames->set_checked(edlsession->cursor_on_frames);
	loop_playback->set_checked(master_edl->local_session->loop_playback);
	show_titles->set_checked(edlsession->show_titles);
	show_transitions->set_checked(edlsession->auto_conf->transitions_visible);
	fade_automation->update_toggle();
	mute_automation->update_toggle();
	pan_automation->update_toggle();
	camera_x->update_toggle();
	camera_y->update_toggle();
	camera_z->update_toggle();
	project_x->update_toggle();
	project_y->update_toggle();
	project_z->update_toggle();
	plugin_automation->set_checked(edlsession->keyframes_visible);
	mode_automation->update_toggle();
	mask_automation->update_toggle();
}

void MainMenu::save_defaults(BC_Hash *defaults)
{
	save_aeffects(defaults);
	save_veffects(defaults);
}

void MainMenu::quit()
{
	quit_program->handle_event();
}

// ================================== load most recent

void MainMenu::init_aeffects(BC_Hash *defaults)
{
	total_aeffects = defaults->get("TOTAL_AEFFECTS", 0);

	char string[1024], title[1024];
	if(total_aeffects) audiomenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_aeffects; i++)
	{
		sprintf(string, "AEFFECTRECENT%d", i);
		defaults->get(string, title);
		audiomenu->add_item(aeffect[i] = new MenuAEffectItem(aeffects, title));
	}
}

void MainMenu::init_veffects(BC_Hash *defaults)
{
	total_veffects = defaults->get("TOTAL_VEFFECTS", 0);

	char string[1024], title[1024];
	if(total_veffects) videomenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_veffects; i++)
	{
		sprintf(string, "VEFFECTRECENT%d", i);
		defaults->get(string, title);
		videomenu->add_item(veffect[i] = new MenuVEffectItem(veffects, title));
	}
}

void MainMenu::init_loads(BC_Hash *defaults)
{
	char string[BCTEXTLEN], path[BCTEXTLEN], filename[BCTEXTLEN];
	FileSystem dir;

	recent_load->load_items();

	int total_loads = recent_load->items.total;
	if(total_loads > 0) filemenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_loads; i++)
	{
		char *path = recent_load->items.values[i]->get_text();

		filemenu->add_item(load[i] = new LoadPrevious());
		dir.extract_name(filename, path, 0);
		load[i]->set_text(filename);
		load[i]->set_path(path);
	}
}

// ============================ save most recent

void MainMenu::save_aeffects(BC_Hash *defaults)
{
	defaults->update("TOTAL_AEFFECTS", total_aeffects);
	char string[1024];
	for(int i = 0; i < total_aeffects; i++)
	{
		sprintf(string, "AEFFECTRECENT%d", i);
		defaults->update(string, aeffect[i]->get_text());
	}
}

void MainMenu::save_veffects(BC_Hash *defaults)
{
	defaults->update("TOTAL_VEFFECTS", total_veffects);
	char string[1024];
	for(int i = 0; i < total_veffects; i++)
	{
		sprintf(string, "VEFFECTRECENT%d", i);
		defaults->update(string, veffect[i]->get_text());
	}
}

// =================================== add most recent

void MainMenu::add_aeffect(const char *title)
{
// add bar for first effect
	if(total_aeffects == 0)
	{
		audiomenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy of effect
	for(int i = 0; i < total_aeffects; i++)
	{
		if(!strcmp(aeffect[i]->get_text(), title))     // already exists
		{                                // swap for top effect
			for(int j = i; j > 0; j--)   // move preceeding effects down
			{
				aeffect[j]->set_text(aeffect[j - 1]->get_text());
			}
			aeffect[0]->set_text(title);
			return;
		}
	}

// add another blank effect
	if(total_aeffects < TOTAL_EFFECTS)
	{
		audiomenu->add_item(aeffect[total_aeffects] = new MenuAEffectItem(aeffects, ""));
		total_aeffects++;
	}

// cycle effect down
	for(int i = total_aeffects - 1; i > 0; i--)
	{
	// set menu item text
		aeffect[i]->set_text(aeffect[i - 1]->get_text());
	}

// set up the new effect
	aeffect[0]->set_text(title);
}

void MainMenu::add_veffect(const char *title)
{
// add bar for first effect
	if(total_veffects == 0)
	{
		videomenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy of effect
	for(int i = 0; i < total_veffects; i++)
	{
		if(!strcmp(veffect[i]->get_text(), title))     // already exists
		{                                // swap for top effect
			for(int j = i; j > 0; j--)   // move preceeding effects down
			{
				veffect[j]->set_text(veffect[j - 1]->get_text());
			}
			veffect[0]->set_text(title);
			return;
		}
	}

// add another blank effect
	if(total_veffects < TOTAL_EFFECTS)
	{
		videomenu->add_item(veffect[total_veffects] = new MenuVEffectItem(veffects, ""));
		total_veffects++;
	}

// cycle effect down
	for(int i = total_veffects - 1; i > 0; i--)
	{
// set menu item text
		veffect[i]->set_text(veffect[i - 1]->get_text());
	}

// set up the new effect
	veffect[0]->set_text(title);
}

void MainMenu::add_load(const char *new_path)
{
	char filename[BCTEXTLEN];
	FileSystem dir;

	int total_loads = recent_load->items.total;

	if(total_loads == 0)
	{
		filemenu->add_item(new BC_MenuItem("-"));
	}

	int new_total = recent_load->add_item(NULL, new_path);

	if (new_total > total_loads) {
		// just create a new item if there is room for it
		int i = new_total - 1;
		load[i] = new LoadPrevious();
		dir.extract_name(filename, new_path, 0);
		load[i]->set_text(filename);
		load[i]->set_path(new_path);
		filemenu->add_item(load[i]);
	}

	// reassign the paths to adjust for the shift down
	for(int i = 0; i < new_total; i++) {
		char *path = recent_load->items.values[i]->get_text();
		dir.extract_name(filename, path, 0);
		load[i]->set_text(filename);
		load[i]->set_path(path);
	}
}


// ================================== menu items

DumpEDL::DumpEDL()
 : BC_MenuItem(_("Dump EDL"))
{
}

int DumpEDL::handle_event()
{
	master_edl->dump();
	return 1;
}

DumpPlugins::DumpPlugins()
 : BC_MenuItem(_("Dump Plugins"))
{
}

int DumpPlugins::handle_event()
{
	plugindb.dump(4);
	return 1;
}

ShowStatus::ShowStatus()
 : BC_MenuItem(_("Show Status"))
{
}

int ShowStatus::handle_event()
{
	mwindow_global->show_program_status();
	return 1;
}

// ================================================= edit

Undo::Undo()
 : BC_MenuItem(_("Undo"), "z", 'z')
{
}

int Undo::handle_event()
{
	mwindow_global->undo_entry();
	return 1;
}

void Undo::update_caption(const char *new_caption)
{
	char string[BCTEXTLEN];

	if(new_caption)
	{
		sprintf(string, _("Undo %s"), new_caption);
		set_text(string);
	}
	else
		set_text(_("Undo"));
}


Redo::Redo()
 : BC_MenuItem(_("Redo"), "Shift+Z", 'Z')
{
	set_shift(1);
}

int Redo::handle_event()
{
	mwindow_global->redo_entry();
	return 1;
}

void Redo::update_caption(const char *new_caption)
{
	char string[BCTEXTLEN];

	if(new_caption)
	{
		sprintf(string, _("Redo %s"), new_caption);
		set_text(string);
	}
	else
		set_text(_("Redo"));
}

CutEffects::CutEffects()
 : BC_MenuItem(_("Cut effects"), "Shift-X", 'X')
{ 
	set_shift();
}

int CutEffects::handle_event()
{
	mwindow_global->cut_effects();
	return 1;
}

CopyEffects::CopyEffects()
 : BC_MenuItem(_("Copy effects"), "Shift-C", 'C')
{
	set_shift();
}

int CopyEffects::handle_event()
{
	mwindow_global->copy_effects();
	return 1;
}

PasteEffects::PasteEffects()
 : BC_MenuItem(_("Paste effects"), "Shift-V", 'V')
{
	set_shift();
}

int PasteEffects::handle_event()
{
	mwindow_global->paste_effects(PASTE_ALL);
	return 1;
}

PasteAutos::PasteAutos()
 : BC_MenuItem(_("Paste keyframes"))
{
}

int PasteAutos::handle_event()
{
	mwindow_global->paste_effects(PASTE_AUTOS);
	return 1;
}

PastePlugins::PastePlugins()
 : BC_MenuItem(_("Paste plugins"))
{
}

int PastePlugins::handle_event()
{
	mwindow_global->paste_effects(PASTE_PLUGINS);
	return 1;
}

ClearKeyframes::ClearKeyframes()
 : BC_MenuItem(_("Clear keyframes"), "Shift-Del", DELETE)
{
	set_shift();
}

int ClearKeyframes::handle_event()
{
	mwindow_global->clear_automation();
	return 1;
}


StraightenKeyframes::StraightenKeyframes()
 : BC_MenuItem(_("Straighten curves"))
{
}

int StraightenKeyframes::handle_event()
{
	mwindow_global->straighten_automation();
	return 1;
}


Cut::Cut()
 : BC_MenuItem(_("Cut"), "x", 'x') 
{
}

int Cut::handle_event()
{
	mwindow_global->cut();
	return 1;
}

Copy::Copy()
 : BC_MenuItem(_("Copy"), "c", 'c')
{
}

int Copy::handle_event()
{
	mwindow_global->copy();
	return 1;
}

Paste::Paste()
 : BC_MenuItem(_("Paste"), "v", 'v') 
{
}

int Paste::handle_event()
{
	mwindow_global->paste();
	return 1;
}

Clear::Clear()
 : BC_MenuItem(_("Clear"), "Del", DELETE)
{
}

int Clear::handle_event()
{
	mwindow_global->clear_entry();
	return 1;
}

PasteSilence::PasteSilence()
 : BC_MenuItem(_("Paste silence"), "Shift+Space", ' ')
{
	set_shift();
}

int PasteSilence::handle_event()
{
	mwindow_global->paste_silence();
	return 1;
}

SelectAll::SelectAll()
 : BC_MenuItem(_("Select All"), "a", 'a')
{
}

int SelectAll::handle_event()
{
	mwindow_global->select_all();
	return 1;
}

ClearLabels::ClearLabels()
 : BC_MenuItem(_("Clear labels"))
{
}

int ClearLabels::handle_event()
{
	mwindow_global->clear_labels();
	return 1;
}

MuteSelection::MuteSelection()
 : BC_MenuItem(_("Mute Region"), "m", 'm')
{
}

int MuteSelection::handle_event()
{
	mwindow_global->mute_selection();
	return 1;
}


TrimSelection::TrimSelection()
 : BC_MenuItem(_("Trim Selection"))
{
}

int TrimSelection::handle_event()
{
	mwindow_global->trim_selection();
	return 1;
}


EditNotes::EditNotes()
 : BC_MenuItem(_("Edit EDL info"))
{
}

int EditNotes::handle_event()
{
	mwindow_global->clip_edit->edit_clip(master_edl);
	return 1;
}

// ============================================= audio

AddAudioTrack::AddAudioTrack()
 : BC_MenuItem(_("Add track"), "t", 't')
{
}

int AddAudioTrack::handle_event()
{
	mwindow_global->add_track(TRACK_AUDIO);
	return 1;
}


DefaultATransition::DefaultATransition()
 : BC_MenuItem(_("Default Transition"), "u", 'u')
{
}

int DefaultATransition::handle_event()
{
	mwindow_global->paste_transition(STRDSC_AUDIO);
	return 1;
}


MapAudio1::MapAudio1()
 : BC_MenuItem(_("Map 1:1"))
{
}

int MapAudio1::handle_event()
{
	mwindow_global->map_audio(MWindow::AUDIO_1_TO_1);
	return 1;
}

MapAudio2::MapAudio2()
 : BC_MenuItem(_("Map 5.1:2"))
{
}

int MapAudio2::handle_event()
{
	mwindow_global->map_audio(MWindow::AUDIO_5_1_TO_2);
	return 1;
}

// ============================================= video

AddVideoTrack::AddVideoTrack()
 : BC_MenuItem(_("Add track"), "Shift-T", 'T')
{
	set_shift();
}

int AddVideoTrack::handle_event()
{
	mwindow_global->add_track(TRACK_VIDEO, 1);
	return 1;
}

DefaultVTransition::DefaultVTransition()
 : BC_MenuItem(_("Default Transition"), "Shift-U", 'U')
{
	set_shift();
}

int DefaultVTransition::handle_event()
{
	mwindow_global->paste_transition(STRDSC_VIDEO);
	return 1;
}

// ============================================ settings

DeleteTracks::DeleteTracks()
 : BC_MenuItem(_("Delete tracks"))
{
}

int DeleteTracks::handle_event()
{
	mwindow_global->delete_tracks();
	return 1;
}

DeleteTrack::DeleteTrack()
 : BC_MenuItem(_("Delete last track"), "d", 'd')
{
}

int DeleteTrack::handle_event()
{
	mwindow_global->delete_track();
	return 1;
}

MoveTracksUp::MoveTracksUp()
 : BC_MenuItem(_("Move tracks up"), "Shift+Up", UP)
{
	set_shift();
}

int MoveTracksUp::handle_event()
{
	mwindow_global->move_tracks_up();
	return 1;
}

MoveTracksDown::MoveTracksDown()
 : BC_MenuItem(_("Move tracks down"), "Shift+Down", DOWN)
{
	set_shift();
}

int MoveTracksDown::handle_event()
{
	mwindow_global->move_tracks_down();
	return 1;
}

ConcatenateTracks::ConcatenateTracks()
 : BC_MenuItem(_("Concatenate tracks"))
{
	set_shift();
}

int ConcatenateTracks::handle_event()
{
	mwindow_global->concatenate_tracks();
	return 1;
}

LoopPlayback::LoopPlayback()
 : BC_MenuItem(_("Loop Playback"), "Shift+L", 'L')
{
	set_checked(master_edl->local_session->loop_playback);
	set_shift();
}

int LoopPlayback::handle_event()
{
	mwindow_global->toggle_loop_playback();
	set_checked(master_edl->local_session->loop_playback);
	return 1;
}


SetBRenderStart::SetBRenderStart()
 : BC_MenuItem(_("Set background render"))
{
}

int SetBRenderStart::handle_event()
{
	mwindow_global->set_brender_start();
	return 1;
}


LabelsFollowEdits::LabelsFollowEdits()
 : BC_MenuItem(_("Edit labels"))
{ 
	set_checked(edlsession->labels_follow_edits);
}

int LabelsFollowEdits::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow_global->set_labels_follow_edits(get_checked());
	return 1;
}


CursorOnFrames::CursorOnFrames()
 : BC_MenuItem(_("Align cursor on frames"))
{ 
	set_checked(edlsession->cursor_on_frames);
}

int CursorOnFrames::handle_event()
{
	edlsession->cursor_on_frames = !edlsession->cursor_on_frames;
	set_checked(edlsession->cursor_on_frames);
	return 1;
}

SaveSettingsNow::SaveSettingsNow()
 : BC_MenuItem(_("Save settings now"))
{
}

int SaveSettingsNow::handle_event()
{
	mwindow_global->save_defaults();
	mwindow_global->save_backup();
	mwindow_global->show_message(_("Saved settings."));
	return 1;
}

// ============================================ window

ShowVWindow::ShowVWindow()
 : BC_MenuItem(_("Show Viewer"))
{
	set_checked(mainsession->show_vwindow);
}
int ShowVWindow::handle_event()
{
	mwindow_global->show_vwindow();
	return 1;
}

ShowAWindow::ShowAWindow()
 : BC_MenuItem(_("Show Resources"))
{
	set_checked(mainsession->show_awindow);
}

int ShowAWindow::handle_event()
{
	mwindow_global->show_awindow();
	return 1;
}

ShowCWindow::ShowCWindow()
 : BC_MenuItem(_("Show Compositor"))
{
	set_checked(mainsession->show_cwindow);
}

int ShowCWindow::handle_event()
{
	mwindow_global->show_cwindow();
	return 1;
}

ShowGWindow::ShowGWindow()
 : BC_MenuItem(_("Show Overlays"))
{
	set_checked(mainsession->show_gwindow);
}

int ShowGWindow::handle_event()
{
	mwindow_global->show_gwindow();
	return 1;
}

ShowLWindow::ShowLWindow()
 : BC_MenuItem(_("Show Levels"))
{
	set_checked(mainsession->show_lwindow);
}

int ShowLWindow::handle_event()
{
	mwindow_global->show_lwindow();
	return 1;
}

ShowRuler::ShowRuler()
 : BC_MenuItem(_("Show Ruler"))
{
	set_checked(mainsession->show_ruler);
}

int ShowRuler::handle_event()
{
	mwindow_global->show_ruler();
	return 1;
}
