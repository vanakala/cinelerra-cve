#ifndef AWINDOWGUI_H
#define AWINDOWGUI_H

#include "arraylist.h"
#include "assetpopup.inc"
#include "asset.inc"
#include "assets.inc"
#include "awindow.inc"
#include "awindowmenu.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "newfolder.inc"
#include "pluginserver.inc"

class AWindowAssets;
class AWindowFolders;
class AWindowNewFolder;
class AWindowDeleteFolder;
class AWindowRenameFolder;
class AWindowDeleteDisk;
class AWindowDeleteProject;
class AWindowDivider;
class AWindowInfo;
class AWindowRedrawIndex;
class AWindowPaste;
class AWindowAppend;
class AWindowView;


class AWindowGUI;

class AssetPicon : public BC_ListBoxItem
{
public:
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, Asset *asset);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, EDL *edl);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, PluginServer *plugin);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, char *folder);
	virtual ~AssetPicon();

	void create_objects();
	void reset();

	MWindow *mwindow;
	AWindowGUI *gui;
	BC_Pixmap *icon;

// ID of thing pointed to
	int id;

// Check ID first.  Update these next before dereferencing
// Asset if asset
	Asset *asset;
// EDL if clip
	EDL *edl;

	int in_use;


	int persistent;
	PluginServer *plugin;
};


class AWindowGUI : public BC_Window
{
public:
	AWindowGUI(MWindow *mwindow, AWindow *awindow);
	~AWindowGUI();

	int create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	int keypress_event();
	void update_assets();
	void sort_assets();
	void reposition_objects();
	int current_folder_number();
// Call back for MWindow entry point
	int drag_motion();
	int drag_stop();
// Collect items into the drag vectors of MainSession
	void collect_assets();
	void create_persistent_folder(ArrayList<BC_ListBoxItem*> *output, 
		int do_audio, 
		int do_video, 
		int is_realtime, 
		int is_transition);
	void copy_picons(ArrayList<BC_ListBoxItem*> *dst, 
		ArrayList<BC_ListBoxItem*> *src, 
		char *folder);
	void sort_picons(ArrayList<BC_ListBoxItem*> *src, 
		char *folder);
// Return the selected asset in asset_list
	Asset* selected_asset();
	PluginServer* selected_plugin();
	AssetPicon* selected_folder();

	MWindow *mwindow;
	AWindow *awindow;

	AWindowAssets *asset_list;
	AWindowFolders *folder_list;
	AWindowDivider *divider;

// Store data to speed up responses
// Persistant data for listboxes
// All assets in current EDL
	ArrayList<BC_ListBoxItem*> assets;
	ArrayList<BC_ListBoxItem*> folders;
	ArrayList<BC_ListBoxItem*> aeffects;
	ArrayList<BC_ListBoxItem*> veffects;
	ArrayList<BC_ListBoxItem*> atransitions;
	ArrayList<BC_ListBoxItem*> vtransitions;

// Currently displayed data for listboxes
// Currently displayed assets + comments
	ArrayList<BC_ListBoxItem*> displayed_assets[2];

	char *asset_titles[ASSET_COLUMNS];

// Persistent icons
	BC_Pixmap *folder_icon;
	BC_Pixmap *file_icon;
	BC_Pixmap *audio_icon;
	BC_Pixmap *video_icon;
	BC_Pixmap *clip_icon;
	NewFolderThread *newfolder_thread;

// Popup menus
	AssetPopup *asset_menu;
	AssetListMenu *assetlist_menu;
	FolderListMenu *folderlist_menu;

private:
	void update_folder_list();
	void update_asset_list();
	void filter_displayed_assets();
};

class AWindowAssets : public BC_ListBox
{
public:
	AWindowAssets(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h);
	~AWindowAssets();
	
	int handle_event();
	int selection_changed();
	void draw_background();
	int drag_start_event();
	int drag_motion_event();
	int drag_stop_event();
	int button_press_event();
	int column_resize_event();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class AWindowDivider : public BC_SubWindow
{
public:
	AWindowDivider(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h);
	~AWindowDivider();

	int button_press_event();
	int cursor_motion_event();
	int button_release_event();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class AWindowFolders : public BC_ListBox
{
public:
	AWindowFolders(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h);
	~AWindowFolders();
	
	int selection_changed();
	int button_press_event();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class AWindowNewFolder : public BC_Button
{
public:
	AWindowNewFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowDeleteFolder : public BC_Button
{
public:
	AWindowDeleteFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowRenameFolder : public BC_Button
{
public:
	AWindowRenameFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowDeleteDisk : public BC_Button
{
public:
	AWindowDeleteDisk(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowDeleteProject : public BC_Button
{
public:
	AWindowDeleteProject(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowInfo : public BC_Button
{
public:
	AWindowInfo(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowRedrawIndex : public BC_Button
{
public:
	AWindowRedrawIndex(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowPaste : public BC_Button
{
public:
	AWindowPaste(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowAppend : public BC_Button
{
public:
	AWindowAppend(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowView : public BC_Button
{
public:
	AWindowView(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

#endif
