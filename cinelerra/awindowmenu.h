// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AWINDOWMENU_H
#define AWINDOWMENU_H

#include "awindowgui.inc"

class AssetListFormat;

class AssetListMenu : public BC_PopupMenu
{
public:
	AssetListMenu(AWindowGUI *gui);

	void update_titles();
private:
	AssetListFormat *format;
	AWindowGUI *gui;
};


class AssetListFormat : public BC_MenuItem
{
public:
	AssetListFormat(AWindowGUI *gui);

	void update();
	int handle_event();
private:
	AWindowGUI *gui;
};


class AssetListSort : public BC_MenuItem
{
public:
	AssetListSort(AWindowGUI *gui);

	void update();
	int handle_event();
private:
	AWindowGUI *gui;
};


class FolderListFormat : public BC_MenuItem
{
public:
	FolderListFormat(AWindowGUI *gui, FolderListMenu *menu);

	int handle_event();
private:
	AWindowGUI *gui;
	FolderListMenu *menu;
};


class FolderListMenu : public BC_PopupMenu
{
public:
	FolderListMenu(AWindowGUI *gui);

	void update_titles();
private:
	FolderListFormat *format;
};

#endif
