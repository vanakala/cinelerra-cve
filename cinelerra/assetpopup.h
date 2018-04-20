
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
#include "assets.inc"


class AssetPopup : public BC_PopupMenu
{
public:
	AssetPopup(MWindow *mwindow, AWindowGUI *gui);

// Set mainsession with the current selections
	void update(int options);
	void paste_assets();
	void match_size();
	void match_rate();

	MWindow *mwindow;
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
	AssetPopupInfo(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};


class AssetPopupSort : public BC_MenuItem
{
public:
	AssetPopupSort(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};


class AssetPopupBuildIndex : public BC_MenuItem
{
public:
	AssetPopupBuildIndex(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};


class AssetPopupView : public BC_MenuItem
{
public:
	AssetPopupView(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};


class AssetPopupPaste : public BC_MenuItem
{
public:
	AssetPopupPaste(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};


class AssetMatchSize : public BC_MenuItem
{
public:
	AssetMatchSize(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetMatchRate : public BC_MenuItem
{
public:
	AssetMatchRate(AssetPopup *popup);

	int handle_event();

	AssetPopup *popup;
};


class AssetPopupProjectRemove : public BC_MenuItem
{
public:
	AssetPopupProjectRemove(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

#endif
