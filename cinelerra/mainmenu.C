#include "assets.h"
#include "cache.h"
#include "cplayback.h"
#include "cropvideo.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "featheredits.h"
#include "filesystem.h"
#include "filexml.h"
#include "keys.h"
#include "levelwindow.h"
#include "loadfile.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "menuaeffects.h"
#include "menuveffects.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "new.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "quit.h"
#include "record.h"
#include "render.h"
#include "savefile.h"
#include "setaudio.h"
#include "setformat.h"
#include "setvideo.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
//#include "videowindowgui.h"
//#include "videowindow.h"
#include "viewmenu.h"
#include "zoombar.h"

#include <string.h>

MainMenu::MainMenu(MWindow *mwindow, MWindowGUI *gui)
 : BC_MenuBar(0, 0, gui->get_w())
{
	this->gui = gui;
	this->mwindow = mwindow; 
}

MainMenu::~MainMenu()
{
}

int MainMenu::create_objects()
{
	BC_Menu *viewmenu, *windowmenu, *settingsmenu, *trackmenu;
	PreferencesMenuitem *preferences;
	Load *append_file;
	total_loads = 0; 

	add_menu(filemenu = new BC_Menu("File"));
	filemenu->add_item(new_project = new New(mwindow));
	new_project->create_objects();

// file loaders
	filemenu->add_item(load_file = new Load(mwindow, this));
	load_file->create_objects();

// new and load can be undone so no need to prompt save
	Save *save;                   //  affected by saveas
	filemenu->add_item(save = new Save(mwindow));
	SaveAs *saveas;
	filemenu->add_item(saveas = new SaveAs(mwindow));
	save->create_objects(saveas);
	saveas->set_mainmenu(this);
	filemenu->add_item(record = new RecordMenuItem(mwindow));

	filemenu->add_item(render = new RenderItem(mwindow));
	filemenu->add_item(new BC_MenuItem("-"));
	filemenu->add_item(quit_program = new Quit(mwindow));
	quit_program->create_objects(save);
	filemenu->add_item(new DumpEDL(mwindow));
	filemenu->add_item(new DumpPlugins(mwindow));
	filemenu->add_item(new LoadBackup(mwindow));
	filemenu->add_item(new SaveBackup(mwindow));

	BC_Menu *editmenu;
	add_menu(editmenu = new BC_Menu("Edit"));
	editmenu->add_item(undo = new Undo(mwindow));
	editmenu->add_item(redo = new Redo(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new Cut(mwindow));
	editmenu->add_item(new Copy(mwindow));
	editmenu->add_item(new Paste(mwindow));
	editmenu->add_item(new Clear(mwindow));
	editmenu->add_item(new PasteSilence(mwindow));
	editmenu->add_item(new MuteSelection(mwindow));
	editmenu->add_item(new TrimSelection(mwindow));
	editmenu->add_item(new SelectAll(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new ClearLabels(mwindow));

	BC_Menu *keyframemenu;
	add_menu(keyframemenu = new BC_Menu("Keyframes"));
	keyframemenu->add_item(new CutKeyframes(mwindow));
	keyframemenu->add_item(new CopyKeyframes(mwindow));
	keyframemenu->add_item(new PasteKeyframes(mwindow));
	keyframemenu->add_item(new ClearKeyframes(mwindow));
	keyframemenu->add_item(new BC_MenuItem("-"));
	keyframemenu->add_item(new CopyDefaultKeyframe(mwindow));
	keyframemenu->add_item(new PasteDefaultKeyframe(mwindow));




	add_menu(audiomenu = new BC_Menu("Audio"));
	audiomenu->add_item(new AddAudioTrack(mwindow));
	audiomenu->add_item(new DefaultATransition(mwindow));
	audiomenu->add_item(aeffects = new MenuAEffects(mwindow));

	add_menu(videomenu = new BC_Menu("Video"));
	videomenu->add_item(new AddVideoTrack(mwindow));
	videomenu->add_item(new DefaultVTransition(mwindow));
	videomenu->add_item(veffects = new MenuVEffects(mwindow));

	add_menu(trackmenu = new BC_Menu("Tracks"));
	trackmenu->add_item(new MoveTracksUp(mwindow));
	trackmenu->add_item(new MoveTracksDown(mwindow));
	trackmenu->add_item(new DeleteTracks(mwindow));
	trackmenu->add_item(new DeleteTrack(mwindow));
	trackmenu->add_item(new ConcatenateTracks(mwindow));

	add_menu(settingsmenu = new BC_Menu("Settings"));

	settingsmenu->add_item(new SetFormat(mwindow));
	settingsmenu->add_item(preferences = new PreferencesMenuitem(mwindow));
	mwindow->preferences_thread = preferences->thread;
	settingsmenu->add_item(labels_follow_edits = new LabelsFollowEdits(mwindow));
	settingsmenu->add_item(plugins_follow_edits = new PluginsFollowEdits(mwindow));
	settingsmenu->add_item(cursor_on_frames = new CursorOnFrames(mwindow));
	settingsmenu->add_item(new SaveSettingsNow(mwindow));
	settingsmenu->add_item(loop_playback = new LoopPlayback(mwindow));
	settingsmenu->add_item(new SetBRenderStart(mwindow));
// set scrubbing speed
//	ScrubSpeed *scrub_speed;
//	settingsmenu->add_item(scrub_speed = new ScrubSpeed(mwindow));
//	if(mwindow->edl->session->scrub_speed == .5) 
//		scrub_speed->set_text("Fast Shuttle");






	add_menu(viewmenu = new BC_Menu("View"));
	viewmenu->add_item(show_titles = new ShowTitles(mwindow, "1"));
	viewmenu->add_item(show_transitions = new ShowTransitions(mwindow, "2"));
	viewmenu->add_item(fade_automation = new FadeAutomation(mwindow, "3"));
//	viewmenu->add_item(play_automation = new PlayAutomation(mwindow, "4"));
	viewmenu->add_item(mute_automation = new MuteAutomation(mwindow, "4"));
	viewmenu->add_item(mode_automation = new ModeAutomation(mwindow, "5"));
	viewmenu->add_item(pan_automation = new PanAutomation(mwindow, "6"));
	viewmenu->add_item(camera_automation = new CameraAutomation(mwindow, "7"));
	viewmenu->add_item(project_automation = new ProjectAutomation(mwindow, "8"));
	viewmenu->add_item(plugin_automation = new PluginAutomation(mwindow, "9"));
	viewmenu->add_item(mask_automation = new MaskAutomation(mwindow, "0"));
	viewmenu->add_item(czoom_automation = new CZoomAutomation(mwindow, "-"));
	viewmenu->add_item(pzoom_automation = new PZoomAutomation(mwindow, "="));


	add_menu(windowmenu = new BC_Menu("Window"));
	windowmenu->add_item(show_vwindow = new ShowVWindow(mwindow));
	windowmenu->add_item(show_awindow = new ShowAWindow(mwindow));
	windowmenu->add_item(show_cwindow = new ShowCWindow(mwindow));
	windowmenu->add_item(show_lwindow = new ShowLWindow(mwindow));
	windowmenu->add_item(new TileWindows(mwindow));

	return 0;
}

int MainMenu::load_defaults(Defaults *defaults)
{
	init_loads(defaults);
	init_aeffects(defaults);
	init_veffects(defaults);
	return 0;
}

void MainMenu::update_toggles()
{
	labels_follow_edits->set_checked(mwindow->edl->session->labels_follow_edits);
	plugins_follow_edits->set_checked(mwindow->edl->session->plugins_follow_edits);
	cursor_on_frames->set_checked(mwindow->edl->session->cursor_on_frames);
	loop_playback->set_checked(mwindow->edl->local_session->loop_playback);
	show_titles->set_checked(mwindow->edl->session->show_titles);
	show_transitions->set_checked(mwindow->edl->session->auto_conf->transitions);
	fade_automation->set_checked(mwindow->edl->session->auto_conf->fade);
//	play_automation->set_checked(mwindow->edl->session->auto_conf->play);
	mute_automation->set_checked(mwindow->edl->session->auto_conf->mute);
	pan_automation->set_checked(mwindow->edl->session->auto_conf->pan);
	camera_automation->set_checked(mwindow->edl->session->auto_conf->camera);
	project_automation->set_checked(mwindow->edl->session->auto_conf->projector);
	plugin_automation->set_checked(mwindow->edl->session->auto_conf->plugins);
	mode_automation->set_checked(mwindow->edl->session->auto_conf->mode);
	mask_automation->set_checked(mwindow->edl->session->auto_conf->mask);
	czoom_automation->set_checked(mwindow->edl->session->auto_conf->czoom);
	pzoom_automation->set_checked(mwindow->edl->session->auto_conf->pzoom);
}

int MainMenu::save_defaults(Defaults *defaults)
{
	save_loads(defaults);
	save_aeffects(defaults);
	save_veffects(defaults);
	return 0;
}





int MainMenu::quit()
{
	quit_program->handle_event();
	return 0;
}





// ================================== load most recent

int MainMenu::init_aeffects(Defaults *defaults)
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
	return 0;
}

int MainMenu::init_veffects(Defaults *defaults)
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
	return 0;
}

int MainMenu::init_loads(Defaults *defaults)
{
//printf("MainMenu::init_loads 1\n");
	total_loads = defaults->get("TOTAL_LOADS", 0);
//printf("MainMenu::init_loads 1\n");
	char string[1024], path[1024], filename[1024];
//printf("MainMenu::init_loads 1\n");
	FileSystem dir;
//printf("MainMenu::init_loads 2\n");
	if(total_loads > 0) filemenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_loads; i++)
	{
		sprintf(string, "LOADPREVIOUS%d", i);
//printf("MainMenu::init_loads 3\n");
		defaults->get(string, path);
//printf("MainMenu::init_loads 4\n");

		filemenu->add_item(load[i] = new LoadPrevious(mwindow, load_file));
//printf("MainMenu::init_loads 5\n");
		dir.extract_name(filename, path, 0);
//printf("MainMenu::init_loads 6\n");
		load[i]->set_text(filename);
//printf("MainMenu::init_loads 7\n");
		load[i]->set_path(path);
//printf("MainMenu::init_loads 8\n");
	}
//printf("MainMenu::init_loads 9\n");
	return 0;
}

// ============================ save most recent

int MainMenu::save_aeffects(Defaults *defaults)
{
	defaults->update("TOTAL_AEFFECTS", total_aeffects);
	char string[1024];
	for(int i = 0; i < total_aeffects; i++)
	{
		sprintf(string, "AEFFECTRECENT%d", i);
		defaults->update(string, aeffect[i]->get_text());
	}
	return 0;
}

int MainMenu::save_veffects(Defaults *defaults)
{
	defaults->update("TOTAL_VEFFECTS", total_veffects);
	char string[1024];
	for(int i = 0; i < total_veffects; i++)
	{
		sprintf(string, "VEFFECTRECENT%d", i);
		defaults->update(string, veffect[i]->get_text());
	}
	return 0;
}

int MainMenu::save_loads(Defaults *defaults)
{
	defaults->update("TOTAL_LOADS", total_loads);
	char string[1024];
	for(int i = 0; i < total_loads; i++)
	{
		sprintf(string, "LOADPREVIOUS%d", i);
		defaults->update(string, load[i]->path);
	}
	return 0;
}

// =================================== add most recent

int MainMenu::add_aeffect(char *title)
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
			return 1;
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
	return 0;
}

int MainMenu::add_veffect(char *title)
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
			return 1;
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
	return 0;
}

int MainMenu::add_load(char *path)
{
	if(total_loads == 0)
	{
		filemenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy
	FileSystem fs;
	char text[1024], new_path[1024];      // get text and path
	fs.extract_name(text, path);
	strcpy(new_path, path);

	for(int i = 0; i < total_loads; i++)
	{
		if(!strcmp(load[i]->get_text(), text))     // already exists
		{                                // swap for top load
			for(int j = i; j > 0; j--)   // move preceeding loads down
			{
				load[j]->set_text(load[j - 1]->get_text());
				load[j]->set_path(load[j - 1]->path);
			}
			load[0]->set_text(text);
			load[0]->set_path(new_path);
			
			return 1;
		}
	}
	
// add another load
	if(total_loads < TOTAL_LOADS)
	{
		filemenu->add_item(load[total_loads] = new LoadPrevious(mwindow, load_file));
		total_loads++;
	}
	
// cycle loads down
	for(int i = total_loads - 1; i > 0; i--)
	{
	// set menu item text
		load[i]->set_text(load[i - 1]->get_text());
	// set filename
		load[i]->set_path(load[i - 1]->path);
	}

// set up the new load
	load[0]->set_text(text);
	load[0]->set_path(new_path);
	return 0;
}








// ================================== menu items


DumpCICache::DumpCICache(MWindow *mwindow)
 : BC_MenuItem("Dump CICache")
{ this->mwindow = mwindow; }

int DumpCICache::handle_event()
{
//	mwindow->cache->dump();
}

DumpEDL::DumpEDL(MWindow *mwindow)
 : BC_MenuItem("Dump EDL")
{ 
	this->mwindow = mwindow;
}

int DumpEDL::handle_event()
{
//printf("DumpEDL::handle_event 1\n");
	mwindow->edl->dump();
//printf("DumpEDL::handle_event 2\n");
	return 1;
}

DumpPlugins::DumpPlugins(MWindow *mwindow)
 : BC_MenuItem("Dump Plugins")
{ 
	this->mwindow = mwindow;
}

int DumpPlugins::handle_event()
{
//printf("DumpEDL::handle_event 1\n");
	mwindow->dump_plugins();
//printf("DumpEDL::handle_event 2\n");
	return 1;
}


DumpAssets::DumpAssets(MWindow *mwindow)
 : BC_MenuItem("Dump Assets")
{ this->mwindow = mwindow; }

int DumpAssets::handle_event()
{
	mwindow->assets->dump();
}

// ================================================= edit

Undo::Undo(MWindow *mwindow) : BC_MenuItem("Undo", "z", 'z') 
{ 
	this->mwindow = mwindow; 
}
int Undo::handle_event()
{ 
	mwindow->undo_entry(1);
	return 1;
}
int Undo::update_caption(char *new_caption)
{
	char string[1024];
	sprintf(string, "Undo %s", new_caption);
	set_text(string);
}


Redo::Redo(MWindow *mwindow) : BC_MenuItem("Redo", "Shift+Z", 'Z') 
{ 
	set_shift(1); 
	this->mwindow = mwindow; 
}

int Redo::handle_event()
{ 
	mwindow->redo_entry(1);

	return 1;
}
int Redo::update_caption(char *new_caption)
{
	char string[1024];
	sprintf(string, "Redo %s", new_caption);
	set_text(string);
}

CutKeyframes::CutKeyframes(MWindow *mwindow)
 : BC_MenuItem("Cut keyframes", "Shift-X", 'X')
{ 
	set_shift(); 
	this->mwindow = mwindow; 
}

int CutKeyframes::handle_event()
{
	mwindow->cut_automation(); 
}

CopyKeyframes::CopyKeyframes(MWindow *mwindow)
 : BC_MenuItem("Copy keyframes", "Shift-C", 'C')
{ 
	set_shift(); 
	this->mwindow = mwindow; 
}

int CopyKeyframes::handle_event()
{
	mwindow->copy_automation();
	return 1;
}

PasteKeyframes::PasteKeyframes(MWindow *mwindow)
 : BC_MenuItem("Paste keyframes", "Shift-V", 'V')
{
	set_shift(); 
	this->mwindow = mwindow; 
}

int PasteKeyframes::handle_event()
{
	mwindow->paste_automation(); 
}

ClearKeyframes::ClearKeyframes(MWindow *mwindow)
 : BC_MenuItem("Clear keyframes", "Shift-Del", BACKSPACE)
{
	set_shift(); 
	this->mwindow = mwindow; 
}

int ClearKeyframes::handle_event()
{
	mwindow->clear_automation();
	return 1;
}





CutDefaultKeyframe::CutDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem("Cut default keyframe", "Alt-X", 'X')
{ 
	set_alt(); 
	this->mwindow = mwindow; 
}

int CutDefaultKeyframe::handle_event()
{
	mwindow->cut_default_keyframe(); 
	return 1;
}

CopyDefaultKeyframe::CopyDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem("Copy default keyframe", "Alt-c", 'c')
{ 
	set_alt(); 
	this->mwindow = mwindow; 
}

int CopyDefaultKeyframe::handle_event()
{
	mwindow->copy_default_keyframe();
	return 1;
}

PasteDefaultKeyframe::PasteDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem("Paste default keyframe", "Alt-v", 'v')
{
	set_alt(); 
	this->mwindow = mwindow; 
}

int PasteDefaultKeyframe::handle_event()
{
	mwindow->paste_default_keyframe(); 
	return 1;
}

ClearDefaultKeyframe::ClearDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem("Clear default keyframe", "Alt-Del", BACKSPACE)
{
	set_alt(); 
	this->mwindow = mwindow; 
}

int ClearDefaultKeyframe::handle_event()
{
	mwindow->clear_default_keyframe();
	return 1;
}

Cut::Cut(MWindow *mwindow)
 : BC_MenuItem("Cut", "x", 'x') 
{
	this->mwindow = mwindow; 
}

int Cut::handle_event()
{
	mwindow->cut();
	return 1;
}

Copy::Copy(MWindow *mwindow)
 : BC_MenuItem("Copy", "c", 'c') 
{
	this->mwindow = mwindow; 
}

int Copy::handle_event()
{
	mwindow->copy();
	return 1;
}

Paste::Paste(MWindow *mwindow)
 : BC_MenuItem("Paste", "v", 'v') 
{
	this->mwindow = mwindow; 
}

int Paste::handle_event()
{
	mwindow->paste();
	return 1;
}

Clear::Clear(MWindow *mwindow)
 : BC_MenuItem("Clear", "Del", BACKSPACE) 
{
	this->mwindow = mwindow; 
}

int Clear::handle_event()
{
	mwindow->cwindow->gui->lock_window();
	mwindow->clear();
	mwindow->cwindow->gui->unlock_window();
	return 1;
}

PasteSilence::PasteSilence(MWindow *mwindow)
 : BC_MenuItem("Paste silence", "Shift+Space", ' ')
{ 
	this->mwindow = mwindow; 
	set_shift(); 
}

int PasteSilence::handle_event()
{ 
	mwindow->paste_silence();
	return 1;
}

SelectAll::SelectAll(MWindow *mwindow)
 : BC_MenuItem("Select All", "a", 'a')
{ 
	this->mwindow = mwindow; 
}

int SelectAll::handle_event()
{
	mwindow->select_all();
	return 1;
}

ClearLabels::ClearLabels(MWindow *mwindow) : BC_MenuItem("Clear labels") 
{ 
	this->mwindow = mwindow; 
}

int ClearLabels::handle_event()
{
	mwindow->clear_labels();
	return 1;
}

MuteSelection::MuteSelection(MWindow *mwindow)
 : BC_MenuItem("Mute Region", "m", 'm')
{
	this->mwindow = mwindow;
}

int MuteSelection::handle_event()
{
	mwindow->mute_selection();
	return 1;
}


TrimSelection::TrimSelection(MWindow *mwindow)
 : BC_MenuItem("Trim Selection")
{
	this->mwindow = mwindow;
}

int TrimSelection::handle_event()
{
	mwindow->trim_selection();
	return 1;
}












// ============================================= audio

AddAudioTrack::AddAudioTrack(MWindow *mwindow)
 : BC_MenuItem("Add track", "t", 't')
{
	this->mwindow = mwindow;
}

int AddAudioTrack::handle_event()
{
	mwindow->add_audio_track_entry();
	return 1;
}

DeleteAudioTrack::DeleteAudioTrack(MWindow *mwindow)
 : BC_MenuItem("Delete track")
{
	this->mwindow = mwindow;
}

int DeleteAudioTrack::handle_event()
{
	return 1;
}

DefaultATransition::DefaultATransition(MWindow *mwindow)
 : BC_MenuItem("Default Transition", "u", 'u')
{
	this->mwindow = mwindow;
}

int DefaultATransition::handle_event()
{
	mwindow->paste_audio_transition();
	return 1;
}




// ============================================= video


AddVideoTrack::AddVideoTrack(MWindow *mwindow)
 : BC_MenuItem("Add track", "Shift-T", 'T')
{
	set_shift();
	this->mwindow = mwindow;
}

int AddVideoTrack::handle_event()
{
	mwindow->add_video_track_entry();
	return 1;
}


DeleteVideoTrack::DeleteVideoTrack(MWindow *mwindow)
 : BC_MenuItem("Delete track")
{
	this->mwindow = mwindow;
}

int DeleteVideoTrack::handle_event()
{
	return 1;
}



ResetTranslation::ResetTranslation(MWindow *mwindow)
 : BC_MenuItem("Reset Translation")
{
	this->mwindow = mwindow;
}

int ResetTranslation::handle_event()
{
	return 1;
}



DefaultVTransition::DefaultVTransition(MWindow *mwindow)
 : BC_MenuItem("Default Transition", "Shift-U", 'U')
{
	set_shift();
	this->mwindow = mwindow;
}

int DefaultVTransition::handle_event()
{
	mwindow->paste_video_transition();
	return 1;
}














// ============================================ settings

DeleteTracks::DeleteTracks(MWindow *mwindow)
 : BC_MenuItem("Delete tracks")
{
	this->mwindow = mwindow;
}

int DeleteTracks::handle_event()
{
	mwindow->delete_tracks();
	return 1;
}

DeleteTrack::DeleteTrack(MWindow *mwindow)
 : BC_MenuItem("Delete last track", "d", 'd')
{
	this->mwindow = mwindow;
}

int DeleteTrack::handle_event()
{
	mwindow->delete_track();
	return 1;
}

MoveTracksUp::MoveTracksUp(MWindow *mwindow)
 : BC_MenuItem("Move tracks up")
{
	set_shift(); this->mwindow = mwindow;
}

int MoveTracksUp::handle_event()
{
	mwindow->move_tracks_up();
	return 1;
}

MoveTracksDown::MoveTracksDown(MWindow *mwindow)
 : BC_MenuItem("Move tracks down")
{
	set_shift(); this->mwindow = mwindow;
}

int MoveTracksDown::handle_event()
{
	mwindow->move_tracks_down();
	return 1;
}




ConcatenateTracks::ConcatenateTracks(MWindow *mwindow)
 : BC_MenuItem("Concatenate tracks")
{
	set_shift(); 
	this->mwindow = mwindow;
}

int ConcatenateTracks::handle_event()
{
	mwindow->concatenate_tracks();
	return 1;
}





LoopPlayback::LoopPlayback(MWindow *mwindow)
 : BC_MenuItem("Loop Playback", "Shift+L", 'L')
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->local_session->loop_playback);
	set_shift();
}

int LoopPlayback::handle_event()
{
	mwindow->toggle_loop_playback();
	set_checked(mwindow->edl->local_session->loop_playback);
	return 1;
}





SetBRenderStart::SetBRenderStart(MWindow *mwindow)
 : BC_MenuItem("Set background render")
{
	this->mwindow = mwindow;
}

int SetBRenderStart::handle_event()
{
	mwindow->set_brender_start();
	return 1;
}







LabelsFollowEdits::LabelsFollowEdits(MWindow *mwindow)
 : BC_MenuItem("Edit labels") 
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->labels_follow_edits);
}

int LabelsFollowEdits::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->labels_follow_edits = get_checked(); 
}




PluginsFollowEdits::PluginsFollowEdits(MWindow *mwindow)
 : BC_MenuItem("Edit effects") 
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->plugins_follow_edits);
}

int PluginsFollowEdits::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->plugins_follow_edits = get_checked(); 
}




AutosFollowEdits::AutosFollowEdits(MWindow *mwindow)
 : BC_MenuItem("Autos follow edits") 
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->autos_follow_edits);
}

int AutosFollowEdits::handle_event()
{ 
	mwindow->edl->session->autos_follow_edits ^= 1; 
	set_checked(!get_checked());
}


CursorOnFrames::CursorOnFrames(MWindow *mwindow)
 : BC_MenuItem("Align cursor on frames") 
{ 
	this->mwindow = mwindow; 
	set_checked(mwindow->edl->session->cursor_on_frames);
}

int CursorOnFrames::handle_event()
{
	mwindow->edl->session->cursor_on_frames = !mwindow->edl->session->cursor_on_frames; 
	set_checked(mwindow->edl->session->cursor_on_frames);
}


ScrubSpeed::ScrubSpeed(MWindow *mwindow) : BC_MenuItem("Slow Shuttle")
{
	this->mwindow = mwindow;
}

int ScrubSpeed::handle_event()
{
	if(mwindow->edl->session->scrub_speed == .5)
	{
		mwindow->edl->session->scrub_speed = 2;
		set_text("Slow Shuttle");
	}
	else
	{
		mwindow->edl->session->scrub_speed = .5;
		set_text("Fast Shuttle");
	}
}

SaveSettingsNow::SaveSettingsNow(MWindow *mwindow) : BC_MenuItem("Save settings now") 
{ 
	this->mwindow = mwindow; 
}

int SaveSettingsNow::handle_event()
{
	mwindow->save_defaults();
	mwindow->save_backup();
	return 1;
}



// ============================================ window





ShowVWindow::ShowVWindow(MWindow *mwindow)
 : BC_MenuItem("Show Viewer")
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_vwindow);
}
int ShowVWindow::handle_event()
{
	mwindow->show_vwindow();
	return 1;
}

ShowAWindow::ShowAWindow(MWindow *mwindow)
 : BC_MenuItem("Show Resources")
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_awindow);
}
int ShowAWindow::handle_event()
{
	mwindow->show_awindow();
	return 1;
}

ShowCWindow::ShowCWindow(MWindow *mwindow)
 : BC_MenuItem("Show Compositor")
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_cwindow);
}
int ShowCWindow::handle_event()
{
	mwindow->show_cwindow();
	return 1;
}


ShowLWindow::ShowLWindow(MWindow *mwindow)
 : BC_MenuItem("Show Levels")
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_lwindow);
}
int ShowLWindow::handle_event()
{
	mwindow->show_lwindow();
	return 1;
}

TileWindows::TileWindows(MWindow *mwindow)
 : BC_MenuItem("Default positions")
{
	this->mwindow = mwindow;
}
int TileWindows::handle_event()
{
	mwindow->tile_windows();
	return 1;
}


PanMenu::PanMenu() : BC_SubMenu() {}

PanItem::PanItem(MWindow *mwindow, char *text, int number)
 : BC_MenuItem(text) 
{
	this->mwindow = mwindow; 
	this->number = number; 
	set_checked(0);
}
int PanItem::handle_event()
{
//	mwindow->tracks->toggle_auto_pan(number);
	set_checked(get_checked() ^ 1);
}

PluginMenu::PluginMenu() : BC_SubMenu() {}

PluginItem::PluginItem(MWindow *mwindow, char *text, int number)
 : BC_MenuItem(text) 
{
	this->mwindow = mwindow; 
	this->number = number; 
	set_checked(0);
}
int PluginItem::handle_event()
{
//	mwindow->tracks->toggle_auto_plugin(number);
	set_checked(get_checked() ^ 1);
}
