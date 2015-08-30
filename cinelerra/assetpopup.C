
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
#include "awindow.h"
#include "awindowgui.h"
#include "awindowmenu.h"
#include "clipedit.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "mainerror.h"
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
	add_item(format = new AssetListFormat(mwindow));
	add_item(info = new AssetPopupInfo(mwindow, this));
	add_item(new AssetPopupSort(mwindow, this));
	add_item(index = new AssetPopupBuildIndex(mwindow, this));
	add_item(view = new AssetPopupView(mwindow, this));
	add_item(new AssetPopupPaste(mwindow, this));
	add_item(new AssetMatchSize(mwindow, this));
	add_item(new AssetPopupProjectRemove(mwindow, this));
	add_item(new AssetPopupDiskRemove(mwindow, this));
}

void AssetPopup::paste_assets()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->paste_assets(mwindow->edl->local_session->get_selectionstart(1), 
		mwindow->edl->tracks->first,
		0);   // do not overwrite
}

void AssetPopup::match_size()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->asset_to_size();
}

void AssetPopup::update()
{
	format->update();
	gui->collect_assets();
}


AssetPopupInfo::AssetPopupInfo(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Info..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
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

int AssetPopupBuildIndex::handle_event()
{
	mwindow->rebuild_indices();
	return 1;
}


AssetPopupSort::AssetPopupSort(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Sort items"))
{
	this->mwindow = mwindow;
	this->popup = popup;
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

int AssetPopupView::handle_event()
{
	if(mwindow->session->drag_assets->total)
		mwindow->vwindow->change_source(
			mwindow->session->drag_assets->values[0]);
	else
	if(mwindow->session->drag_clips->total)
		mwindow->vwindow->change_source(
			mwindow->session->drag_clips->values[0]);
	return 1;
}


AssetPopupPaste::AssetPopupPaste(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Paste"))
{
	this->mwindow = mwindow;
	this->popup = popup;
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


AssetPopupProjectRemove::AssetPopupProjectRemove(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Remove from project"))
{
	this->mwindow = mwindow;
	this->popup = popup;
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

int AssetPopupDiskRemove::handle_event()
{
	if(!confirmbox(_("Remove this asset from disk?")))
		mwindow->remove_assets_from_disk();
	return 1;
}
