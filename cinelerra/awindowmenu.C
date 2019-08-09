
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

#include "awindowgui.h"
#include "awindowmenu.h"
#include "edlsession.h"
#include "language.h"


AssetListMenu::AssetListMenu(AWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->gui = gui;
	add_item(format = new AssetListFormat(gui));
	add_item(new AssetListSort(gui));
	update_titles();
}

void AssetListMenu::update_titles()
{
	format->update();
}


AssetListFormat::AssetListFormat(AWindowGUI *gui)
 : BC_MenuItem("")
{
	this->gui = gui;
}

void AssetListFormat::update()
{
	set_text(edlsession->assetlist_format == ASSETS_TEXT ?
		_("Display icons") : _("Display text"));
}

int AssetListFormat::handle_event()
{
	switch(edlsession->assetlist_format)
	{
	case ASSETS_TEXT:
		edlsession->assetlist_format = ASSETS_ICONS;
		break;
	case ASSETS_ICONS:
		edlsession->assetlist_format = ASSETS_TEXT;
		break;
	}

	gui->asset_list->update_format(
		edlsession->assetlist_format == ASSETS_ICONS ?
			(LISTBOX_ICONS | LISTBOX_SMALLFONT) : 0,
		1);

	return 1;
}


AssetListSort::AssetListSort(AWindowGUI *gui)
 : BC_MenuItem(_("Sort items"))
{
	this->gui = gui;
}

int AssetListSort::handle_event()
{
	gui->sort_assets();
	return 1;
}


FolderListMenu::FolderListMenu(AWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	add_item(format = new FolderListFormat(gui, this));
	update_titles();
}

void FolderListMenu::update_titles()
{
	format->set_text(edlsession->folderlist_format == ASSETS_TEXT ?
		_("Display icons") : _("Display text"));
}


FolderListFormat::FolderListFormat(AWindowGUI *gui, FolderListMenu *menu)
 : BC_MenuItem("")
{
	this->gui = gui;
	this->menu = menu;
}

int FolderListFormat::handle_event()
{
	switch(edlsession->folderlist_format)
	{
	case ASSETS_TEXT:
		edlsession->folderlist_format = ASSETS_ICONS;
		break;
	case ASSETS_ICONS:
		edlsession->folderlist_format = ASSETS_TEXT;
		break;
	}

	gui->folder_list->update_format(
		edlsession->folderlist_format == ASSETS_ICONS ?
			(LISTBOX_ICONS | LISTBOX_SMALLFONT) : 0, 1);
	menu->update_titles();

	return 1;
}
