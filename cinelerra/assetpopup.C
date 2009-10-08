
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

#include "assetedit.h"
#include "assetpopup.h"
#include "assetremove.h"
#include "awindow.h"
#include "awindowgui.h"
#include "awindowmenu.h"
#include "clipedit.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "language.h"
#include "localsession.h"
#include "mainindexes.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "tracks.h"
#include "vwindow.h"
#include "vwindowgui.h"



AssetPopup::AssetPopup(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

AssetPopup::~AssetPopup()
{
}

void AssetPopup::create_objects()
{
	add_item(format = new AssetListFormat(mwindow));
	add_item(info = new AssetPopupInfo(mwindow, this));
	add_item(new AssetPopupSort(mwindow, this));
	add_item(index = new AssetPopupBuildIndex(mwindow, this));
	add_item(view = new AssetPopupView(mwindow, this));
	add_item(new AssetPopupPaste(mwindow, this));
	add_item(new AssetMatchSize(mwindow, this));
	add_item(new AssetMatchRate(mwindow, this));
	add_item(new AssetPopupProjectRemove(mwindow, this));
	add_item(new AssetPopupDiskRemove(mwindow, this));
}

void AssetPopup::paste_assets()
{
// Collect items into the drag vectors for temporary storage
	gui->lock_window("AssetPopup::paste_assets");
	mwindow->gui->lock_window("AssetPopup::paste_assets");
	mwindow->cwindow->gui->lock_window("AssetPopup::paste_assets");

	gui->collect_assets();
	mwindow->paste_assets(mwindow->edl->local_session->get_selectionstart(1), 
		mwindow->edl->tracks->first,
		0);   // do not overwrite

	gui->unlock_window();
	mwindow->gui->unlock_window();
	mwindow->cwindow->gui->unlock_window();
}

void AssetPopup::match_size()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("AssetPopup::match_size");
	mwindow->asset_to_size();
	mwindow->gui->unlock_window();
}

void AssetPopup::match_rate()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("AssetPopup::match_rate");
	mwindow->asset_to_rate();
	mwindow->gui->unlock_window();
}

int AssetPopup::update()
{
	format->update();
	gui->collect_assets();
	return 0;
}









AssetPopupInfo::AssetPopupInfo(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Info..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupInfo::~AssetPopupInfo()
{
}

int AssetPopupInfo::handle_event()
{
	if(mwindow->session->drag_assets->total)
	{
		if(mwindow->awindow->asset_edit->running() && 
			mwindow->awindow->asset_edit->window)
		{
			mwindow->awindow->asset_edit->window->raise_window();
			mwindow->awindow->asset_edit->window->flush();
		}
		else
		{
			mwindow->awindow->asset_edit->edit_asset(
				mwindow->session->drag_assets->values[0]);
		}
	}
	else
	if(mwindow->session->drag_clips->total)
	{
		popup->gui->awindow->clip_edit->edit_clip(
			mwindow->session->drag_clips->values[0]);
	}
	return 1;
}







AssetPopupBuildIndex::AssetPopupBuildIndex(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Rebuild index"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupBuildIndex::~AssetPopupBuildIndex()
{
}

int AssetPopupBuildIndex::handle_event()
{
//printf("AssetPopupBuildIndex::handle_event 1\n");
	mwindow->rebuild_indices();
	return 1;
}







AssetPopupSort::AssetPopupSort(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Sort items"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupSort::~AssetPopupSort()
{
}

int AssetPopupSort::handle_event()
{
	mwindow->awindow->gui->sort_assets();
	return 1;
}







AssetPopupView::AssetPopupView(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("View"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupView::~AssetPopupView()
{
}

int AssetPopupView::handle_event()
{
	mwindow->vwindow->gui->lock_window("AssetPopupView::handle_event");

	if(mwindow->session->drag_assets->total)
		mwindow->vwindow->change_source(
			mwindow->session->drag_assets->values[0]);
	else
	if(mwindow->session->drag_clips->total)
		mwindow->vwindow->change_source(
			mwindow->session->drag_clips->values[0]);

	mwindow->vwindow->gui->unlock_window();
	return 1;
}







AssetPopupPaste::AssetPopupPaste(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Paste"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupPaste::~AssetPopupPaste()
{
}

int AssetPopupPaste::handle_event()
{
	popup->paste_assets();
	return 1;
}








AssetMatchSize::AssetMatchSize(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Match project size"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int AssetMatchSize::handle_event()
{
	popup->match_size();
	return 1;
}








AssetMatchRate::AssetMatchRate(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Match frame rate"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int AssetMatchRate::handle_event()
{
	popup->match_rate();
	return 1;
}














AssetPopupProjectRemove::AssetPopupProjectRemove(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Remove from project"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}



AssetPopupProjectRemove::~AssetPopupProjectRemove()
{
}

int AssetPopupProjectRemove::handle_event()
{
	mwindow->remove_assets_from_project(1);
	return 1;
}




AssetPopupDiskRemove::AssetPopupDiskRemove(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Remove from disk"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}


AssetPopupDiskRemove::~AssetPopupDiskRemove()
{
}


int AssetPopupDiskRemove::handle_event()
{
	mwindow->awindow->asset_remove->start();
	return 1;
}




