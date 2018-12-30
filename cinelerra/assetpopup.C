
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
	add_item(new AssetPopupProjectRemove(mwindow, this));
	add_item(matchsize = new AssetMatchSize(mwindow, this));
	add_item(matchrate = new AssetMatchRate(this));
}

void AssetPopup::paste_assets()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->paste_assets(master_edl->local_session->get_selectionstart(1),
		master_edl->first_track(),
		0);   // do not overwrite
}

void AssetPopup::match_size()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->asset_to_size();
}

void AssetPopup::match_rate()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->asset_to_rate();
}

void AssetPopup::update(int options)
{
	if(options & ASSETPOP_MATCHSIZE)
	{
		if(!matchsize)
			add_item(matchsize = new AssetMatchSize(mwindow, this));
	}
	else if(matchsize)
	{
		remove_item(matchsize);
		matchsize = 0;
	}
	if(options & ASSETPOP_MATCHRATE)
	{
		if(!matchrate)
			add_item(matchrate = new AssetMatchRate(this));
	}
	else if(matchrate)
	{
		remove_item(matchrate);
		matchrate = 0;
	}
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
	if(mainsession->drag_assets->total)
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
				mainsession->drag_assets->values[0]);
		}
	}
	else
	if(mainsession->drag_clips->total)
	{
		popup->gui->awindow->clip_edit->edit_clip(
			mainsession->drag_clips->values[0]);
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
	if(mainsession->drag_assets->total)
		mwindow->vwindow->change_source(
			mainsession->drag_assets->values[0]);
	else
	if(mainsession->drag_clips->total)
		mwindow->vwindow->change_source(
			mainsession->drag_clips->values[0]);
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


AssetMatchRate::AssetMatchRate(AssetPopup *popup)
 : BC_MenuItem(_("Match frame rate"))
{
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

int AssetPopupProjectRemove::handle_event()
{
	mwindow->remove_assets_from_project(1);
	return 1;
}

