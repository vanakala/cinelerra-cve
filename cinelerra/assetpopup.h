// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
	~AssetPopup();

	void update(Asset *asset, EDL *edl);
	void paste_assets();

	Asset *current_asset;
	EDL *current_edl;
private:
	AWindowGUI *gui;

	AssetPopupInfo *info;
	AssetPopupBuildIndex *index;
	AssetPopupView *view;
	AssetListFormat *format;
	AssetMatchRate *matchrate;
	AssetMatchSize *matchsize;
	int show_size;
	int show_rate;
};


class AssetPopupInfo : public BC_MenuItem
{
public:
	AssetPopupInfo(AWindow *awindow, AssetPopup *popup);

	int handle_event();
private:
	AWindow *awindow;
	AssetPopup *popup;
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
	AssetPopupBuildIndex(AssetPopup *popup);

	int handle_event();

private:
	AssetPopup *popup;
};


class AssetPopupView : public BC_MenuItem
{
public:
	AssetPopupView(AssetPopup *popup);

	int handle_event();
private:
	AssetPopup *popup;
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
