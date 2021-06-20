// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "assetedit.h"
#include "assetpopup.h"
#include "awindow.h"
#include "awindowgui.h"
#include "awindowmenu.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "edl.h"
#include "mainerror.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "vwindow.h"


AssetPopup::AssetPopup(AWindow *awindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0)
{
	this->gui = gui;
	add_item(format = new AssetListFormat(gui));
	add_item(info = new AssetPopupInfo(awindow, this));
	add_item(new AssetPopupSort(gui));
	add_item(index = new AssetPopupBuildIndex());
	add_item(view = new AssetPopupView(this));
	add_item(new AssetPopupPaste(this));
	add_item(new AssetPopupProjectRemove());
	matchsize = new AssetMatchSize(this);
	matchrate = new AssetMatchRate(this);
	show_size = 0;
	show_rate = 0;
}

AssetPopup::~AssetPopup()
{
	if(!show_size)
		delete matchsize;
	if(!show_rate)
		delete matchrate;
}

void AssetPopup::paste_assets()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow_global->paste_assets(master_edl->local_session->get_selectionstart(1),
		master_edl->first_track(),
		0);   // do not overwrite
}

void AssetPopup::update(Asset *asset, EDL *edl)
{
	current_asset = asset;
	current_edl = edl;

	if(asset && asset->video_data &&
		asset->width >= MIN_FRAME_WIDTH &&
		asset->height >= MIN_FRAME_HEIGHT &&
		asset->width <= MAX_FRAME_WIDTH &&
		asset->height <= MAX_FRAME_HEIGHT)
	{
		if(!show_size)
		{
			add_item(matchsize);
			show_size = 1;
		}
	}
	else
	{
		remove_item(matchsize);
		show_size = 0;
	}

	if(asset && asset->video_data &&
		asset->frame_rate >= MIN_FRAME_RATE &&
		asset->frame_rate <= MAX_FRAME_RATE)
	{
		if(!show_rate)
		{
			add_item(matchrate);
			show_rate = 1;
		}
	}
	else
	{
		remove_item(matchrate);
		show_rate = 0;
	}

	format->update();
}


AssetPopupInfo::AssetPopupInfo(AWindow *awindow, AssetPopup *popup)
 : BC_MenuItem(_("Info..."))
{
	this->awindow = awindow;
	this->popup = popup;
}

int AssetPopupInfo::handle_event()
{
	if(popup->current_asset)
	{
		if(awindow->asset_edit->running() &&
			awindow->asset_edit->window)
		{
			awindow->asset_edit->window->raise_window();
			awindow->asset_edit->window->flush();
		}
		else
		{
			awindow->asset_edit->edit_asset(
				popup->current_asset);
		}
		return 1;
	}
	else if(popup->current_edl)
	{
		mwindow_global->clip_edit->edit_clip(
			popup->current_edl);
		return 1;
	}
	return 0;
}


AssetPopupBuildIndex::AssetPopupBuildIndex()
 : BC_MenuItem(_("Rebuild index"))
{
}

int AssetPopupBuildIndex::handle_event()
{
	mwindow_global->rebuild_indices();
	return 1;
}


AssetPopupSort::AssetPopupSort(AWindowGUI *gui)
 : BC_MenuItem(_("Sort items"))
{
	this->gui = gui;
}

int AssetPopupSort::handle_event()
{
	gui->sort_assets();
	return 1;
}


AssetPopupView::AssetPopupView(AssetPopup *popup)
 : BC_MenuItem(_("View"))
{
	this->popup = popup;
}

int AssetPopupView::handle_event()
{
	if(popup->current_asset)
		mwindow_global->vwindow->change_source(
			popup->current_asset);
	else
	if(popup->current_edl)
		mwindow_global->vwindow->change_source(
			popup->current_edl);
	return 1;
}


AssetPopupPaste::AssetPopupPaste(AssetPopup *popup)
 : BC_MenuItem(_("Paste"))
{
	this->popup = popup;
}

int AssetPopupPaste::handle_event()
{
	popup->paste_assets();
	return 1;
}


AssetMatchSize::AssetMatchSize(AssetPopup *popup)
 : BC_MenuItem(_("Match project size"))
{
	this->popup = popup;
}

int AssetMatchSize::handle_event()
{
	mwindow_global->asset_to_size(popup->current_asset);
	return 1;
}


AssetMatchRate::AssetMatchRate(AssetPopup *popup)
 : BC_MenuItem(_("Match frame rate"))
{
	this->popup = popup;
}

int AssetMatchRate::handle_event()
{
	mwindow_global->asset_to_rate(popup->current_asset);
	return 1;
}


AssetPopupProjectRemove::AssetPopupProjectRemove()
 : BC_MenuItem(_("Remove from project"))
{
}

int AssetPopupProjectRemove::handle_event()
{
	mwindow_global->remove_assets_from_project(1);
	return 1;
}

