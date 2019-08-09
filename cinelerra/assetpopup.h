
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

#ifndef ASSETPOPUP_H
#define ASSETPOPUP_H

class AssetPopupInfo;
class AssetPopupBuildIndex;
class AssetPopupView;
class AssetMatchRate;
class AssetMatchSize;

#include "assetedit.inc"
#include "assetpopup.inc"
#include "awindowgui.inc"
#include "awindowmenu.inc"
#include "edl.inc"
#include "mwindow.inc"


class AssetPopup : public BC_PopupMenu
{
public:
	AssetPopup(AWindow *window, AWindowGUI *gui);

// Set mainsession with the current selections
	void update(int options);
	void paste_assets();
	void match_size();
	void match_rate();
private:
	AWindowGUI *gui;

	AssetPopupInfo *info;
	AssetPopupBuildIndex *index;
	AssetPopupView *view;
	AssetListFormat *format;
	AssetMatchRate *matchrate;
	AssetMatchSize *matchsize;
};


class AssetPopupInfo : public BC_MenuItem
{
public:
	AssetPopupInfo(AWindow *awindow);

	int handle_event();
private:
	AWindow *awindow;
};


class AssetPopupSort : public BC_MenuItem
{
public:
	AssetPopupSort(AWindowGUI *gui);

	int handle_event();
private:
	AWindowGUI *gui;
};


class AssetPopupBuildIndex : public BC_MenuItem
{
public:
	AssetPopupBuildIndex();

	int handle_event();
};


class AssetPopupView : public BC_MenuItem
{
public:
	AssetPopupView();

	int handle_event();
};


class AssetPopupPaste : public BC_MenuItem
{
public:
	AssetPopupPaste(AssetPopup *popup);

	int handle_event();
private:
	AssetPopup *popup;
};


class AssetMatchSize : public BC_MenuItem
{
public:
	AssetMatchSize(AssetPopup *popup);

	int handle_event();
private:
	AssetPopup *popup;
};


class AssetMatchRate : public BC_MenuItem
{
public:
	AssetMatchRate(AssetPopup *popup);

	int handle_event();
private:
	AssetPopup *popup;
};


class AssetPopupProjectRemove : public BC_MenuItem
{
public:
	AssetPopupProjectRemove();

	int handle_event();
};

#endif
