#ifndef AWINDOWMENU_H
#define AWINDOWMENU_H

#include "awindowgui.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"

class AssetListFormat;

class AssetListMenu : public BC_PopupMenu
{
public:
	AssetListMenu(MWindow *mwindow, AWindowGUI *gui);
	~AssetListMenu();
	
	void create_objects();
	void update_titles();

	AssetListFormat *format;
	
	MWindow *mwindow;
	AWindowGUI *gui;
};


class AssetListFormat : public BC_MenuItem
{
public:
	AssetListFormat(MWindow *mwindow);

	void update();
	int handle_event();
	MWindow *mwindow;
};


class AssetListSort : public BC_MenuItem
{
public:
	AssetListSort(MWindow *mwindow);

	void update();
	int handle_event();
	MWindow *mwindow;
};





class FolderListFormat;



class FolderListMenu : public BC_PopupMenu
{
public:
	FolderListMenu(MWindow *mwindow, AWindowGUI *gui);
	~FolderListMenu();
	
	void create_objects();
	void update_titles();

	FolderListFormat *format;
	
	MWindow *mwindow;
	AWindowGUI *gui;
};


class FolderListFormat : public BC_MenuItem
{
public:
	FolderListFormat(MWindow *mwindow, FolderListMenu *menu);

	int handle_event();
	MWindow *mwindow;
	FolderListMenu *menu;
};



#endif
