
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
#include "labels.h"
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

class LabelPopup;
class LabelPopupEdit;

class AWindowGUI;

class AssetPicon : public BC_ListBoxItem
{
public:
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, Asset *asset);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, EDL *edl);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, PluginServer *plugin);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, Label *plugin);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, char *folder);
	virtual ~AssetPicon();

	void create_objects();
	void reset();

	MWindow *mwindow;
	AWindowGUI *gui;
	BC_Pixmap *icon;
	VFrame *icon_vframe;
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
	Label *label;
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
	void async_update_assets();     // Sends update asset event
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
	void create_label_folder();
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
	ArrayList<BC_ListBoxItem*> labellist;

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
	LabelPopup *label_menu;
	AssetListMenu *assetlist_menu;
	FolderListMenu *folderlist_menu;
// Temporary for reading picons from files
	VFrame *temp_picon;

	int allow_iconlisting;
	
// Create custom atoms to be used for async messages between windows
	int create_custom_xatoms();
// Function to overload to recieve customly defined atoms
	virtual int recieve_custom_xatoms(xatom_event *event); 
	
	
	
private:
	void update_folder_list();
	void update_asset_list();
	void filter_displayed_assets();
	Atom UpdateAssetsXAtom;
	void update_assets();

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

class LabelPopup : public BC_PopupMenu
{
public:
	LabelPopup(MWindow *mwindow, AWindowGUI *gui);
	~LabelPopup();

	void create_objects();
// Set mainsession with the current selections
	int update();

	MWindow *mwindow;
	AWindowGUI *gui;

	LabelPopupEdit *editlabel;
};

class LabelPopupEdit : public BC_MenuItem
{
public:
	LabelPopupEdit(MWindow *mwindow, LabelPopup *popup);
	~LabelPopupEdit();

	int handle_event();

	MWindow *mwindow;
	LabelPopup *popup;
};

#endif
