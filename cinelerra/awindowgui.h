// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AWINDOWGUI_H
#define AWINDOWGUI_H

#include "arraylist.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "bcmenuitem.h"
#include "bcpixmap.inc"
#include "bcpopupmenu.h"
#include "bcwindow.h"
#include "bcwindow.h"
#include "assetpopup.inc"
#include "asset.inc"
#include "awindow.inc"
#include "awindowmenu.inc"
#include "edl.inc"
#include "labels.h"
#include "pluginserver.inc"

#include <X11/Xatom.h>

class AWindowAssets;
class AWindowFolders;
class AWindowDivider;

class LabelPopup;
class LabelPopupEdit;

class AWindowGUI;

class AssetPicon : public BC_ListBoxItem
{
public:
	AssetPicon(AWindowGUI *gui, Asset *asset);
	AssetPicon(AWindowGUI *gui, EDL *edl);
	AssetPicon(AWindowGUI *gui, PluginServer *plugin);
	AssetPicon(AWindowGUI *gui, Label *plugin);
	AssetPicon(AWindowGUI *gui, int folder);
	virtual ~AssetPicon();

	AWindowGUI *gui;
	BC_Pixmap *icon;
	VFrame *icon_vframe;
	int foldernum;
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
private:
	void reset();
	void init_object();
};


class AWindowGUI : public BC_Window
{
public:
	AWindowGUI(AWindow *awindow);
	~AWindowGUI();

	void resize_event(int w, int h);
	void translation_event();
	void close_event();
	int keypress_event();
	void async_update_assets();     // Sends update asset event
	void sort_assets();
	void reposition_objects();
	static int folder_number(const char *name);

// Collect items into the drag vectors of MainSession
	void collect_assets();

	void copy_picons(ArrayList<BC_ListBoxItem*> *dst, 
		ArrayList<BC_ListBoxItem*> *src, 
		int folder);
	void sort_picons(ArrayList<BC_ListBoxItem*> *src);

// Return the selected asset in asset_list
	Asset* selected_asset();
	PluginServer* selected_plugin();
	AssetPicon* selected_folder();

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

	const char *asset_titles[ASSET_COLUMNS];

// Persistent icons
	BC_Pixmap *folder_icon;
	BC_Pixmap *file_icon;
	BC_Pixmap *audio_icon;
	BC_Pixmap *video_icon;
	BC_Pixmap *clip_icon;

// Popup menus
	AssetPopup *asset_menu;
	LabelPopup *label_menu;
	AssetListMenu *assetlist_menu;
	FolderListMenu *folderlist_menu;

// Function to overload to recieve customly defined atoms
	virtual void recieve_custom_xatoms(XClientMessageEvent *event);
	static const char *folder_names[];

private:
	void update_asset_list();
	void filter_displayed_assets();
	Atom UpdateAssetsXAtom;
	void update_assets();
	void create_persistent_folder(ArrayList<BC_ListBoxItem*> *output,
		int strdesc,
		int is_realtime,
		int is_transition);
	void create_label_folder();
// Create custom atoms to be used for async messages between windows
	void create_custom_xatoms();
};

class AWindowAssets : public BC_ListBox
{
public:
	AWindowAssets(AWindowGUI *gui, int x, int y, int w, int h);

	int handle_event();
	void selection_changed();
	void draw_background();
	int drag_start_event();
	void drag_motion_event();
	void drag_stop_event();
	int button_press_event();
	int column_resize_event();

	AWindowGUI *gui;
};

class AWindowDivider : public BC_SubWindow
{
public:
	AWindowDivider(AWindowGUI *gui, int x, int y, int w, int h);

	int button_press_event();
	int cursor_motion_event();
	int button_release_event();

	AWindowGUI *gui;
};

class AWindowFolders : public BC_ListBox
{
public:
	AWindowFolders(AWindowGUI *gui, int x, int y, int w, int h);

	void selection_changed();
	int button_press_event();

	AWindowGUI *gui;
};

class LabelPopup : public BC_PopupMenu
{
public:
	LabelPopup(AWindowGUI *gui);

// Set mainsession with the current selections
	int update();

	AWindowGUI *gui;

	LabelPopupEdit *editlabel;
};

class LabelPopupEdit : public BC_MenuItem
{
public:
	LabelPopupEdit(LabelPopup *popup);

	int handle_event();

	LabelPopup *popup;
};

#endif
