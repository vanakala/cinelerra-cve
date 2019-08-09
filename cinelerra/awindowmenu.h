
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
