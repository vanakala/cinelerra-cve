
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

#include "asset.h"
#include "assetedit.h"
#include "assetpopup.h"
#include "assets.h"
#include "awindowgui.h"
#include "awindowgui.inc"
#include "awindow.h"
#include "awindowmenu.h"
#include "bcsignals.h"
#include "cache.h"
#include "colormodels.h"
#include "cursors.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "language.h"
#include "labels.h"
#include "labeledit.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "newfolder.h"
#include "preferences.h"
#include "theme.h"
#include "vframe.h"
#include "vwindowgui.h"
#include "vwindow.h"




AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	Asset *asset)
 : BC_ListBoxItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->asset = asset;
	this->id = asset->id;
}

AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	EDL *edl)
 : BC_ListBoxItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->edl = edl;
	this->id = edl->id;
}

AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	char *folder)
 : BC_ListBoxItem(folder, gui->folder_icon)
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
}

AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	PluginServer *plugin)
 : BC_ListBoxItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->plugin = plugin;
	asset = 0;
	icon = 0;
	id = 0;
}

AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	Label *label)
 : BC_ListBoxItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->label = label;
	asset = 0;
	icon = 0;
	id = 0;
}

AssetPicon::~AssetPicon()
{
	if(icon)
	{
		if(icon != gui->file_icon &&
			icon != gui->audio_icon &&
			icon != gui->folder_icon &&
			icon != gui->clip_icon &&
			icon != gui->video_icon) 
		{
			delete icon;
			delete icon_vframe;
		}
	}
}

void AssetPicon::reset()
{
	plugin = 0;
	label = 0;
	asset = 0;
	edl = 0;
	icon = 0;
	icon_vframe = 0;
	in_use = 1;
	id = 0;
	persistent = 0;
}

void AssetPicon::create_objects()
{
	FileSystem fs;
	char name[BCTEXTLEN];
	int pixmap_w, pixmap_h;


	pixmap_h = 50;

	if(asset)
	{
		fs.extract_name(name, asset->path);
		set_text(name);
		if(asset->video_data)
		{
			if(mwindow->preferences->use_thumbnails)
			{
				File *file = mwindow->video_cache->check_out(asset, mwindow->edl);

				if(file)
				{
					pixmap_w = pixmap_h * asset->width / asset->height;

					file->set_layer(0);
					file->set_video_position(0, mwindow->edl->session->frame_rate);

					if(gui->temp_picon && 
						(gui->temp_picon->get_w() != asset->width ||
						gui->temp_picon->get_h() != asset->height))
					{
						delete gui->temp_picon;
						gui->temp_picon = 0;
					}

					if(!gui->temp_picon)
					{
						gui->temp_picon = new VFrame(0, 
							asset->width, 
							asset->height, 
							BC_RGB888);
					}

					file->read_frame(gui->temp_picon);

					icon = new BC_Pixmap(gui, pixmap_w, pixmap_h);
					icon->draw_vframe(gui->temp_picon,
						0, 
						0, 
						pixmap_w, 
						pixmap_h,
						0,
						0);
//printf("%d %d\n", gui->temp_picon->get_w(), gui->temp_picon->get_h());
					icon_vframe = new VFrame(0, 
						pixmap_w, 
						pixmap_h, 
						BC_RGB888);
					cmodel_transfer(icon_vframe->get_rows(), /* Leave NULL if non existent */
						gui->temp_picon->get_rows(),
						0, /* Leave NULL if non existent */
						0,
						0,
						0, /* Leave NULL if non existent */
						0,
						0,
						0,        /* Dimensions to capture from input frame */
						0, 
						gui->temp_picon->get_w(), 
						gui->temp_picon->get_h(),
						0,       /* Dimensions to project on output frame */
						0, 
						pixmap_w, 
						pixmap_h,
						BC_RGB888, 
						BC_RGB888,
						0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
						0,       /* For planar use the luma rowspan */
						0);     /* For planar use the luma rowspan */


					mwindow->video_cache->check_in(asset);
				}
				else
				{
					icon = gui->video_icon;
					icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_FILM];

				}
			}
			else
			{
				icon = gui->video_icon;
				icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_FILM];			
			}
		}
		else
		if(asset->audio_data)
		{
			icon = gui->audio_icon;
			icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_SOUND];
		}
//printf("AssetPicon::create_objects 2\n");

		set_icon(icon);
		set_icon_vframe(icon_vframe);
//printf("AssetPicon::create_objects 4\n");
	}
	else
	if(edl)
	{
//printf("AssetPicon::create_objects 4 %s\n", edl->local_session->clip_title);
		strcpy(name, edl->local_session->clip_title);
		set_text(name);
		set_icon(gui->clip_icon);
		set_icon_vframe(mwindow->theme->get_image("clip_icon"));
	}
	else
	if(plugin)
	{
		strcpy(name, _(plugin->title));
		set_text(name);
		if(plugin->picon)
		{
			if(plugin->picon->get_color_model() == BC_RGB888)
			{
				icon = new BC_Pixmap(gui, 
					plugin->picon->get_w(), 
					plugin->picon->get_h());
				icon->draw_vframe(plugin->picon,
					0,
					0,
					plugin->picon->get_w(), 
					plugin->picon->get_h(),
					0,
					0);
			}
			else
			{
				icon = new BC_Pixmap(gui, 
					plugin->picon, 
					PIXMAP_ALPHA);
			}
			icon_vframe = new VFrame (*plugin->picon);
		}
		else
		{
			icon = gui->file_icon;
			icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_UNKNOWN];
		}
		set_icon(icon);
		set_icon_vframe(icon_vframe);
	}
	else
	if(label)
	{
		Units::totext(name, 
			      label->position,
			      mwindow->edl->session->time_format,
			      mwindow->edl->session->sample_rate,
			      mwindow->edl->session->frame_rate,
			      mwindow->edl->session->frames_per_foot);
		set_text(name);
		icon = gui->file_icon;
		icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_UNKNOWN];
		set_icon(icon);
		set_icon_vframe(icon_vframe);
	}

}






AWindowGUI::AWindowGUI(MWindow *mwindow, AWindow *awindow)
 : BC_Window(PROGRAM_NAME ": Resources",
 	mwindow->session->awindow_x, 
    mwindow->session->awindow_y, 
    mwindow->session->awindow_w, 
    mwindow->session->awindow_h,
    100,
    100,
    1,
    1,
    1)
{
// printf("AWindowGUI::AWindowGUI %d %d %d %d\n",
// mwindow->session->awindow_x, 
// mwindow->session->awindow_y, 
// mwindow->session->awindow_w, 
// mwindow->session->awindow_h);
	this->mwindow = mwindow;
	this->awindow = awindow;
	temp_picon = 0;
	allow_iconlisting = 1;
}

AWindowGUI::~AWindowGUI()
{
	assets.remove_all_objects();
	folders.remove_all_objects();
	aeffects.remove_all_objects();
	veffects.remove_all_objects();
	atransitions.remove_all_objects();
	vtransitions.remove_all_objects();
	labellist.remove_all_objects();
	displayed_assets[1].remove_all_objects();
	delete file_icon;
	delete audio_icon;
	delete folder_icon;
	delete clip_icon;
	delete newfolder_thread;
	delete asset_menu;
	delete label_menu;
	delete assetlist_menu;
	delete folderlist_menu;
	if(temp_picon) delete temp_picon;
}

int AWindowGUI::create_objects()
{
	int x, y;
	AssetPicon *picon;

SET_TRACE
//printf("AWindowGUI::create_objects 1\n");
	asset_titles[0] = _("Title");
	asset_titles[1] = _("Comments");

SET_TRACE

	set_icon(mwindow->theme->get_image("awindow_icon"));
	file_icon = new BC_Pixmap(this, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_UNKNOWN],
		PIXMAP_ALPHA);

	folder_icon = new BC_Pixmap(this, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_FOLDER],
		PIXMAP_ALPHA);

	audio_icon = new BC_Pixmap(this, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_SOUND],
		PIXMAP_ALPHA);

	video_icon = new BC_Pixmap(this, 
		BC_WindowBase::get_resources()->type_to_icon[ICON_FILM],
		PIXMAP_ALPHA);

SET_TRACE

	clip_icon = new BC_Pixmap(this, 
		mwindow->theme->get_image("clip_icon"),
		PIXMAP_ALPHA);

SET_TRACE

// Mandatory folders
	folders.append(picon = new AssetPicon(mwindow,
		this,
		AEFFECT_FOLDER));
	picon->persistent = 1;
	folders.append(picon = new AssetPicon(mwindow,
		this,
		VEFFECT_FOLDER));
	picon->persistent = 1;
	folders.append(picon = new AssetPicon(mwindow,
		this,
		ATRANSITION_FOLDER));
	picon->persistent = 1;
	folders.append(picon = new AssetPicon(mwindow,
		this,
		VTRANSITION_FOLDER));
	picon->persistent = 1;
	folders.append(picon = new AssetPicon(mwindow,
		this,
		LABEL_FOLDER));
	picon->persistent = 1;

	create_label_folder();

SET_TRACE

	create_persistent_folder(&aeffects, 1, 0, 1, 0);
	create_persistent_folder(&veffects, 0, 1, 1, 0);
	create_persistent_folder(&atransitions, 1, 0, 0, 1);
	create_persistent_folder(&vtransitions, 0, 1, 0, 1);

SET_TRACE

	mwindow->theme->get_awindow_sizes(this);

SET_TRACE
	add_subwindow(asset_list = new AWindowAssets(mwindow,
		this,
 		mwindow->theme->alist_x, 
    	mwindow->theme->alist_y, 
    	mwindow->theme->alist_w, 
    	mwindow->theme->alist_h));

SET_TRACE
	add_subwindow(divider = new AWindowDivider(mwindow,
		this,
		mwindow->theme->adivider_x,
		mwindow->theme->adivider_y,
		mwindow->theme->adivider_w,
		mwindow->theme->adivider_h));

SET_TRACE
	divider->set_cursor(HSEPARATE_CURSOR);

SET_TRACE
	add_subwindow(folder_list = new AWindowFolders(mwindow,
		this,
 		mwindow->theme->afolders_x, 
    	mwindow->theme->afolders_y, 
    	mwindow->theme->afolders_w, 
    	mwindow->theme->afolders_h));
	
SET_TRACE

	x = mwindow->theme->abuttons_x;
	y = mwindow->theme->abuttons_y;

SET_TRACE

	newfolder_thread = new NewFolderThread(mwindow, this);

	add_subwindow(asset_menu = new AssetPopup(mwindow, this));
	asset_menu->create_objects();

	add_subwindow(label_menu = new LabelPopup(mwindow, this));
	label_menu->create_objects();

SET_TRACE

	add_subwindow(assetlist_menu = new AssetListMenu(mwindow, this));

SET_TRACE
	assetlist_menu->create_objects();

SET_TRACE

	add_subwindow(folderlist_menu = new FolderListMenu(mwindow, this));
	folderlist_menu->create_objects();
//printf("AWindowGUI::create_objects 2\n");

SET_TRACE
	create_custom_xatoms();

	return 0;
}

int AWindowGUI::resize_event(int w, int h)
{
	mwindow->session->awindow_x = get_x();
	mwindow->session->awindow_y = get_y();
	mwindow->session->awindow_w = w;
	mwindow->session->awindow_h = h;

	mwindow->theme->get_awindow_sizes(this);
	mwindow->theme->draw_awindow_bg(this);

	reposition_objects();

	int x = mwindow->theme->abuttons_x;
	int y = mwindow->theme->abuttons_y;

// 	new_bin->reposition_window(x, y);
// 	x += new_bin->get_w();
// 	delete_bin->reposition_window(x, y);
// 	x += delete_bin->get_w();
// 	rename_bin->reposition_window(x, y);
// 	x += rename_bin->get_w();
// 	delete_disk->reposition_window(x, y);
// 	x += delete_disk->get_w();
// 	delete_project->reposition_window(x, y);
// 	x += delete_project->get_w();
// 	info->reposition_window(x, y);
// 	x += info->get_w();
// 	redraw_index->reposition_window(x, y);
// 	x += redraw_index->get_w();
// 	paste->reposition_window(x, y);
// 	x += paste->get_w();
// 	append->reposition_window(x, y);
// 	x += append->get_w();
// 	view->reposition_window(x, y);

	BC_WindowBase::resize_event(w, h);
	return 1;
}

int AWindowGUI::translation_event()
{
	mwindow->session->awindow_x = get_x();
	mwindow->session->awindow_y = get_y();
	return 0;
}

void AWindowGUI::reposition_objects()
{
	int wmax = mwindow->session->awindow_w-mwindow->theme->adivider_w;
	int x = mwindow->theme->afolders_x;
	int w = mwindow->theme->afolders_w;
	if (w > wmax)
		w = wmax;
	if (w <= 0)
		w = 1;
	folder_list->reposition_window(x, mwindow->theme->afolders_y,
		w, mwindow->theme->afolders_h);
	x = mwindow->theme->adivider_x;
	if (x > wmax)
		x = wmax;
	if (x < 0)
		x = 0;
	divider->reposition_window(x,
		mwindow->theme->adivider_y,
		mwindow->theme->adivider_w,
		mwindow->theme->adivider_h);
	int x2 = mwindow->theme->alist_x;
	if (x2 < x+mwindow->theme->adivider_w)
		x2 = x+mwindow->theme->adivider_w;
	w = mwindow->theme->alist_w;
	if (w > wmax)
		w = wmax;
	if (w <= 0)
		w = 1;
	asset_list->reposition_window(x2, mwindow->theme->alist_y,
		w, mwindow->theme->alist_h);
	flush();
}

int AWindowGUI::close_event()
{
	hide_window();
	mwindow->session->show_awindow = 0;
	unlock_window();

	mwindow->gui->lock_window("AWindowGUI::close_event");
	mwindow->gui->mainmenu->show_awindow->set_checked(0);
	mwindow->gui->unlock_window();

	lock_window("AWindowGUI::close_event");
	mwindow->save_defaults();
	return 1;
}


int AWindowGUI::keypress_event()
{
	switch(get_keypress())
	{
		case 'w':
		case 'W':
			if(ctrl_down())
			{
				close_event();
				return 1;
			}
			break;
	}
	return 0;
}



int AWindowGUI::create_custom_xatoms()
{
	UpdateAssetsXAtom = create_xatom("CWINDOWGUI_UPDATE_ASSETS");
	return 0;
}
int AWindowGUI::recieve_custom_xatoms(xatom_event *event)
{
	if (event->message_type == UpdateAssetsXAtom)
	{
		update_assets();
		return 1;
	} else
	return 0;
}

void AWindowGUI::async_update_assets()
{
	xatom_event event;
	event.message_type = UpdateAssetsXAtom;
	send_custom_xatom(&event);
}







void AWindowGUI::update_folder_list()
{
//printf("AWindowGUI::update_folder_list 1\n");
	for(int i = 0; i < folders.total; i++)
	{
		AssetPicon *picon = (AssetPicon*)folders.values[i];
		picon->in_use--;
	}
//printf("AWindowGUI::update_folder_list 1\n");

// Search assets for folders
	for(int i = 0; i < mwindow->edl->folders.total; i++)
	{
		char *folder = mwindow->edl->folders.values[i];
		int exists = 0;
//printf("AWindowGUI::update_folder_list 1.1\n");

		for(int j = 0; j < folders.total; j++)
		{
			AssetPicon *picon = (AssetPicon*)folders.values[j];
			if(!strcasecmp(picon->get_text(), folder))
			{
				exists = 1;
				picon->in_use = 1;
				break;
			}
		}

		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, this, folder);
			picon->create_objects();
			folders.append(picon);
		}
//printf("AWindowGUI::update_folder_list 1.3\n");
	}
//printf("AWindowGUI::update_folder_list 1\n");
//for(int i = 0; i < folders.total; i++)
//	printf("AWindowGUI::update_folder_list %s\n", folders.values[i]->get_text());

// Delete excess
	for(int i = folders.total - 1; i >= 0; i--)
	{
		AssetPicon *picon = (AssetPicon*)folders.values[i];
		if(!picon->in_use && !picon->persistent)
		{
			delete picon;
			folders.remove_number(i);
		}
	}
//for(int i = 0; i < folders.total; i++)
//	printf("AWindowGUI::update_folder_list %s\n", folders.values[i]->get_text());
//printf("AWindowGUI::update_folder_list 2\n");
}

void AWindowGUI::create_persistent_folder(ArrayList<BC_ListBoxItem*> *output, 
	int do_audio, 
	int do_video, 
	int is_realtime, 
	int is_transition)
{
	ArrayList<PluginServer*> plugindb;

// Get pointers to plugindb entries
	mwindow->create_plugindb(do_audio, 
			do_video, 
			is_realtime, 
			is_transition,
			0,
			plugindb);

	for(int i = 0; i < plugindb.total; i++)
	{
		PluginServer *server = plugindb.values[i];
		int exists = 0;

// Create new listitem
		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, this, server);
			picon->create_objects();
			output->append(picon);
		}
	}
}

void AWindowGUI::create_label_folder()
{
	Label *current;
	for(current = mwindow->edl->labels->first; current; current = NEXT) {
		AssetPicon *picon = new AssetPicon(mwindow, this, current);
		picon->create_objects();
		labellist.append(picon);
	}
}


void AWindowGUI::update_asset_list()
{
//printf("AWindowGUI::update_asset_list 1\n");
	for(int i = 0; i < assets.total; i++)
	{
		AssetPicon *picon = (AssetPicon*)assets.values[i];
		picon->in_use--;
	}





//printf("AWindowGUI::update_asset_list 2\n");


// Synchronize EDL clips
	for(int i = 0; i < mwindow->edl->clips.total; i++)
	{
		int exists = 0;
		
// Look for clip in existing listitems
		for(int j = 0; j < assets.total && !exists; j++)
		{
			AssetPicon *picon = (AssetPicon*)assets.values[j];
			
			if(picon->id == mwindow->edl->clips.values[i]->id)
			{
				picon->edl = mwindow->edl->clips.values[i];
				picon->set_text(mwindow->edl->clips.values[i]->local_session->clip_title);
				exists = 1;
				picon->in_use = 1;
			}
		}

// Create new listitem
		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, 
				this, 
				mwindow->edl->clips.values[i]);
			picon->create_objects();
			assets.append(picon);
		}
	}







//printf("AWindowGUI::update_asset_list 3 %d\n", assets.total);
// Synchronize EDL assets
	for(Asset *current = mwindow->edl->assets->first; 
		current; 
		current = NEXT)
	{
		int exists = 0;

//printf("AWindowGUI::update_asset_list 3 %s\n", current->path);
// Look for asset in existing listitems
		for(int j = 0; j < assets.total && !exists; j++)
		{
			AssetPicon *picon = (AssetPicon*)assets.values[j];

			if(picon->id == current->id)
			{
//printf("AWindowGUI::update_asset_list 4 %p %d %d\n", picon->asset, picon->get_icon_x(), picon->get_icon_y());
				picon->asset = current;
				exists = 1;
				picon->in_use = 1;
				break;
			}
		}

//printf("AWindowGUI::update_asset_list 5\n");
// Create new listitem
		if(!exists)
		{
//printf("AWindowGUI::update_asset_list 4.1 %s\n", current->path);
			AssetPicon *picon = new AssetPicon(mwindow, this, current);
//printf("AWindowGUI::update_asset_list 4.2 %s\n", current->path);
			picon->create_objects();
//printf("AWindowGUI::update_asset_list 4.3 %s\n", current->path);
			assets.append(picon);
//printf("AWindowGUI::update_asset_list 4.4 %s\n", current->path);
		}
	}









//printf("AWindowGUI::update_asset_list 6\n");
	for(int i = assets.total - 1; i >= 0; i--)
	{
		AssetPicon *picon = (AssetPicon*)assets.values[i];
//printf("AWindowGUI::update_asset_list %s %d\n", picon->asset->path, picon->in_use);
		if(!picon->in_use)
		{
			delete picon;
			assets.remove_number(i);
		}
	}
//printf("AWindowGUI::update_asset_list 7 %d\n", assets.total);
}





void AWindowGUI::sort_assets()
{
//printf("AWindowGUI::sort_assets 1 %s\n", mwindow->edl->session->current_folder);
	if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER))
		sort_picons(&aeffects, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER))
		sort_picons(&veffects, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER))
		sort_picons(&atransitions, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		sort_picons(&vtransitions, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, LABEL_FOLDER))
		;// Labels should ALWAYS be sorted by time.
	else
		sort_picons(&assets, 
			mwindow->edl->session->current_folder);

	update_assets();
}











void AWindowGUI::collect_assets()
{
	int i = 0;
	mwindow->session->drag_assets->remove_all();
	mwindow->session->drag_clips->remove_all();
	while(1)
	{
		AssetPicon *result = (AssetPicon*)asset_list->get_selection(0, i++);
		if(!result) break;

		if(result->asset) mwindow->session->drag_assets->append(result->asset);
		if(result->edl) mwindow->session->drag_clips->append(result->edl);
	}
}

void AWindowGUI::copy_picons(ArrayList<BC_ListBoxItem*> *dst, 
	ArrayList<BC_ListBoxItem*> *src, 
	char *folder)
{
// Remove current pointers
	dst[0].remove_all();
	dst[1].remove_all_objects();

// Create new pointers
//if(folder) printf("AWindowGUI::copy_picons 1 %s\n", folder);
	for(int i = 0; i < src->total; i++)
	{
		AssetPicon *picon = (AssetPicon*)src->values[i];
//printf("AWindowGUI::copy_picons 2 %s\n", picon->asset->folder);
		if(!folder ||
			(folder && picon->asset && !strcasecmp(picon->asset->folder, folder)) ||
			(folder && picon->edl && !strcasecmp(picon->edl->local_session->folder, folder)))
		{
			BC_ListBoxItem *item2, *item1;
			dst[0].append(item1 = picon);
			if(picon->edl)
				dst[1].append(item2 = new BC_ListBoxItem(picon->edl->local_session->clip_notes));
			else
			if(picon->label && picon->label->textstr)
				dst[1].append(item2 = new BC_ListBoxItem(picon->label->textstr));
			else
				dst[1].append(item2 = new BC_ListBoxItem(""));
			item1->set_autoplace_text(1);
			item2->set_autoplace_text(1);
//printf("AWindowGUI::copy_picons 3 %s\n", picon->get_text());
		}
	}
}

void AWindowGUI::sort_picons(ArrayList<BC_ListBoxItem*> *src, 
		char *folder)
{
//printf("AWindowGUI::sort_picons 1\n")
	int done = 0;
	while(!done)
	{
		done = 1;
		for(int i = 0; i < src->total - 1; i++)
		{
			BC_ListBoxItem *item1 = src->values[i];
			BC_ListBoxItem *item2 = src->values[i + 1];
			item1->set_autoplace_icon(1);
			item2->set_autoplace_icon(1);
			item1->set_autoplace_text(1);
			item2->set_autoplace_text(1);
			if(strcmp(item1->get_text(), item2->get_text()) > 0)
			{
				src->values[i + 1] = item1;
				src->values[i] = item2;
				done = 0;
			}
		}
	}
}


void AWindowGUI::filter_displayed_assets()
{
	allow_iconlisting = 1;
	asset_titles[0] = _("Title");
	asset_titles[1] = _("Comments");
	if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER))
		copy_picons(displayed_assets, 
			&aeffects, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER))
		copy_picons(displayed_assets, 
			&veffects, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER))
		copy_picons(displayed_assets, 
			&atransitions, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		copy_picons(displayed_assets, 
			&vtransitions, 
			0);
	else
	if(!strcasecmp(mwindow->edl->session->current_folder, LABEL_FOLDER)) {
		copy_picons(displayed_assets, 
			    &labellist, 
			    0);
		asset_titles[0] = _("Time Stamps");
		asset_titles[1] = _("Title");
		allow_iconlisting = 0;
	}
	else
		copy_picons(displayed_assets, 
			&assets, 
			mwindow->edl->session->current_folder);
	// Ensure the current folder icon is highlighted
	for(int i = 0; i < folders.total; i++)
	{
		if(!strcasecmp(mwindow->edl->session->current_folder, folders.values[i]->get_text()))
			folders.values[i]->set_selected(1);
		else
			folders.values[i]->set_selected(0);
	}
}


void AWindowGUI::update_assets()
{
//printf("AWindowGUI::update_assets 1\n");
	update_folder_list();
//printf("AWindowGUI::update_assets 2\n");
	update_asset_list();
	labellist.remove_all_objects();
	create_label_folder();
//printf("AWindowGUI::update_assets 3\n");
	filter_displayed_assets();

//for(int i = 0; i < folders.total; i++)
//printf("AWindowGUI::update_assets 4\n");
//	printf("AWindowGUI::update_assets %s\n", folders.values[i]->get_text());
	if(mwindow->edl->session->folderlist_format != folder_list->get_format())
		folder_list->update_format(mwindow->edl->session->folderlist_format, 0);
	folder_list->update(&folders,
		0,
		0,
		1,
		folder_list->get_xposition(),
		folder_list->get_yposition(),
		-1);
//printf("AWindowGUI::update_assets 5\n");

	if(mwindow->edl->session->assetlist_format != asset_list->get_format())
		asset_list->update_format(mwindow->edl->session->assetlist_format, 0);


//printf("AWindowGUI::update_assets 6 %d\n", displayed_assets[0].total);
	asset_list->update(displayed_assets,
		asset_titles,
		mwindow->edl->session->asset_columns,
		ASSET_COLUMNS, 
		asset_list->get_xposition(),
		asset_list->get_yposition(),
		-1,
		0);
//printf("AWindowGUI::update_assets 7\n");

	flush();
//printf("AWindowGUI::update_assets 8\n");
	return;
}

int AWindowGUI::current_folder_number()
{
	int result = -1;
	for(int i = 0; i < folders.total; i++)
	{
		if(!strcasecmp(folders.values[i]->get_text(), mwindow->edl->session->current_folder))
		{
			result = i;
			break;
		}
	}
	return result;
}

int AWindowGUI::drag_motion()
{
	if(get_hidden()) return 0;

	int result = 0;
	return result;
}

int AWindowGUI::drag_stop()
{
	if(get_hidden()) return 0;

	return 0;
}

Asset* AWindowGUI::selected_asset()
{
	AssetPicon *picon = (AssetPicon*)asset_list->get_selection(0, 0);
	if(picon) return picon->asset;
}

PluginServer* AWindowGUI::selected_plugin()
{
	AssetPicon *picon = (AssetPicon*)asset_list->get_selection(0, 0);
	if(picon) return picon->plugin;
}

AssetPicon* AWindowGUI::selected_folder()
{
	AssetPicon *picon = (AssetPicon*)folder_list->get_selection(0, 0);
    return picon;
}










AWindowDivider::AWindowDivider(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
AWindowDivider::~AWindowDivider()
{
}

int AWindowDivider::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		mwindow->session->current_operation = DRAG_PARTITION;
		return 1;
	}
	return 0;
}

int AWindowDivider::cursor_motion_event()
{
	if(mwindow->session->current_operation == DRAG_PARTITION)
	{
		mwindow->session->afolders_w = gui->get_relative_cursor_x();
		mwindow->theme->get_awindow_sizes(gui);
		gui->reposition_objects();
	}
	return 0;
}

int AWindowDivider::button_release_event()
{
	if(mwindow->session->current_operation == DRAG_PARTITION)
	{
		mwindow->session->current_operation = NO_OPERATION;
		return 1;
	}
	return 0;
}






AWindowFolders::AWindowFolders(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_ListBox(x, 
 		y, 
		w, 
		h,
		mwindow->edl->session->folderlist_format == FOLDERS_ICONS ? 
			LISTBOX_ICONS : LISTBOX_TEXT, 
		&gui->folders, // Each column has an ArrayList of BC_ListBoxItems.
		0,             // Titles for columns.  Set to 0 for no titles
		0,                // width of each column
		1,                      // Total columns.
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_SINGLE,  // Select one item or multiple items
		ICON_TOP,        // Position of icon relative to text of each item
		1)               // Allow drags
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_drag_scroll(0);
}

AWindowFolders::~AWindowFolders()
{
}
	
int AWindowFolders::selection_changed()
{
	AssetPicon *picon = (AssetPicon*)get_selection(0, 0);
	if(picon)
	{
		strcpy(mwindow->edl->session->current_folder, picon->get_text());
//printf("AWindowFolders::selection_changed 1\n");
		gui->asset_list->draw_background();
		gui->async_update_assets();
	}
	return 1;
}

int AWindowFolders::button_press_event()
{
	int result = 0;

	result = BC_ListBox::button_press_event();

	if(!result)
	{
		if(get_buttonpress() == 3 && is_event_win() && cursor_inside())
		{
			gui->folderlist_menu->update_titles();
			gui->folderlist_menu->activate_menu();
			result = 1;
		}
	}


	return result;
}







AWindowAssets::AWindowAssets(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_ListBox(x, 
 		y, 
		w, 
		h,
		(mwindow->edl->session->assetlist_format == ASSETS_ICONS && gui->allow_iconlisting ) ? 
			LISTBOX_ICONS : LISTBOX_TEXT,
		&gui->assets,  	  // Each column has an ArrayList of BC_ListBoxItems.
		gui->asset_titles,             // Titles for columns.  Set to 0 for no titles
		mwindow->edl->session->asset_columns,                // width of each column
		1,                      // Total columns.
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_MULTIPLE,  // Select one item or multiple items
		ICON_TOP,        // Position of icon relative to text of each item
		1)               // Allow drag
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_drag_scroll(0);
}

AWindowAssets::~AWindowAssets()
{
}

int AWindowAssets::button_press_event()
{
	int result = 0;

	result = BC_ListBox::button_press_event();

	if(!result && get_buttonpress() == 3 && is_event_win() && cursor_inside())
	{
		BC_ListBox::deactivate_selection();
		gui->assetlist_menu->update_titles();
		gui->assetlist_menu->activate_menu();
		result = 1;
	}


	return result;
}


int AWindowAssets::handle_event()
{
//printf("AWindowAssets::handle_event 1 %d %d\n", get_buttonpress(), get_selection(0, 0));
	if(get_selection(0, 0))
	{
		if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER))
		{
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER))
		{
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER))
		{
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		{
		}
		else
		{
//printf("AWindowAssets::handle_event 2 %d %d\n", get_buttonpress(), get_selection(0, 0));
			mwindow->vwindow->gui->lock_window("AWindowAssets::handle_event");
			
			if(((AssetPicon*)get_selection(0, 0))->asset)
				mwindow->vwindow->change_source(((AssetPicon*)get_selection(0, 0))->asset);
			else
			if(((AssetPicon*)get_selection(0, 0))->edl)
				mwindow->vwindow->change_source(((AssetPicon*)get_selection(0, 0))->edl);

			mwindow->vwindow->gui->unlock_window();
		}
		return 1;
	}

	return 0;
}

int AWindowAssets::selection_changed()
{
// Show popup window
	if(get_button_down() && get_buttonpress() == 3 && get_selection(0, 0))
	{
		if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER) || 
			!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER) ||
			!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER) ||
			!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		{
			gui->assetlist_menu->update_titles();
			gui->assetlist_menu->activate_menu();
		}
		else
                if (!strcasecmp(mwindow->edl->session->current_folder, LABEL_FOLDER)) 
		{
			if(((AssetPicon*)get_selection(0, 0))->label)
				gui->label_menu->activate_menu();
		}
		else
		{
			if(((AssetPicon*)get_selection(0, 0))->asset)
				gui->asset_menu->update();
			else
			if(((AssetPicon*)get_selection(0, 0))->edl)
				gui->asset_menu->update();



			gui->asset_menu->activate_menu();
		}

		BC_ListBox::deactivate_selection();
		return 1;
	}
	return 0;
}

void AWindowAssets::draw_background()
{
	BC_ListBox::draw_background();
	set_color(BC_WindowBase::get_resources()->audiovideo_color);
	set_font(LARGEFONT);
	draw_text(get_w() - 
			get_text_width(LARGEFONT, mwindow->edl->session->current_folder) - 4, 
		30, 
		mwindow->edl->session->current_folder, 
		-1, 
		get_bg_surface());
}

int AWindowAssets::drag_start_event()
{
	int collect_pluginservers = 0;
	int collect_assets = 0;

	if(BC_ListBox::drag_start_event())
	{
		if(!strcasecmp(mwindow->edl->session->current_folder, AEFFECT_FOLDER))
		{
			mwindow->session->current_operation = DRAG_AEFFECT;
			collect_pluginservers = 1;
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, VEFFECT_FOLDER))
		{
			mwindow->session->current_operation = DRAG_VEFFECT;
			collect_pluginservers = 1;
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, ATRANSITION_FOLDER))
		{
			mwindow->session->current_operation = DRAG_ATRANSITION;
			collect_pluginservers = 1;
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, VTRANSITION_FOLDER))
		{
			mwindow->session->current_operation = DRAG_VTRANSITION;
			collect_pluginservers = 1;
		}
		else
		if(!strcasecmp(mwindow->edl->session->current_folder, LABEL_FOLDER))
		{
			// do nothing!
		}
		else
		{
			mwindow->session->current_operation = DRAG_ASSET;
			collect_assets = 1;
		}
		
		
		if(collect_pluginservers)
		{
			int i = 0;
			mwindow->session->drag_pluginservers->remove_all();
			while(1)
			{
				AssetPicon *result = (AssetPicon*)get_selection(0, i++);
				if(!result) break;
				
				mwindow->session->drag_pluginservers->append(result->plugin);
			}
		}
		
		if(collect_assets)
		{
			gui->collect_assets();
		}

		return 1;
	}
	return 0;
}

int AWindowAssets::drag_motion_event()
{
	BC_ListBox::drag_motion_event();

	mwindow->gui->lock_window("AWindowAssets::drag_motion_event");
	mwindow->gui->drag_motion();
	mwindow->gui->unlock_window();

	mwindow->vwindow->gui->lock_window("AWindowAssets::drag_motion_event");
	mwindow->vwindow->gui->drag_motion();
	mwindow->vwindow->gui->unlock_window();

	mwindow->cwindow->gui->lock_window("AWindowAssets::drag_motion_event");
	mwindow->cwindow->gui->drag_motion();
	mwindow->cwindow->gui->unlock_window();
	return 0;
}

int AWindowAssets::drag_stop_event()
{
	int result = 0;

	result = gui->drag_stop();


	if(!result)
	{
		mwindow->gui->lock_window("AWindowAssets::drag_stop_event");
		result = mwindow->gui->drag_stop();
		mwindow->gui->unlock_window();
	}

	if(!result) 
	{
		mwindow->vwindow->gui->lock_window("AWindowAssets::drag_stop_event");
		result = mwindow->vwindow->gui->drag_stop();
		mwindow->vwindow->gui->unlock_window();
	}

	if(!result) 
	{
		mwindow->cwindow->gui->lock_window("AWindowAssets::drag_stop_event");
		result = mwindow->cwindow->gui->drag_stop();
		mwindow->cwindow->gui->unlock_window();
	}



	if(result) get_drag_popup()->set_animation(0);

	BC_ListBox::drag_stop_event();
	mwindow->session->current_operation = ::NO_OPERATION; // since NO_OPERATION is also defined in listbox, we have to reach for global scope...
	return 0;
}

int AWindowAssets::column_resize_event()
{
	mwindow->edl->session->asset_columns[0] = get_column_width(0);
	mwindow->edl->session->asset_columns[1] = get_column_width(1);
	return 1;
}












AWindowNewFolder::AWindowNewFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->newbin_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("New bin"));
}

int AWindowNewFolder::handle_event()
{
	gui->newfolder_thread->start_new_folder();
	return 1;
}

AWindowDeleteFolder::AWindowDeleteFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->deletebin_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete bin"));
}

int AWindowDeleteFolder::handle_event()
{
	if(gui->folder_list->get_selection(0, 0))
	{
		BC_ListBoxItem *folder = gui->folder_list->get_selection(0, 0);
		mwindow->delete_folder(folder->get_text());
	}
	return 1;
}

AWindowRenameFolder::AWindowRenameFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->renamebin_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Rename bin"));
}

int AWindowRenameFolder::handle_event()
{
	return 1;
}

AWindowDeleteDisk::AWindowDeleteDisk(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->deletedisk_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete asset from disk"));
}

int AWindowDeleteDisk::handle_event()
{
	return 1;
}

AWindowDeleteProject::AWindowDeleteProject(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->deleteproject_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete asset from project"));
}

int AWindowDeleteProject::handle_event()
{
	return 1;
}

AWindowInfo::AWindowInfo(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->infoasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Edit information on asset"));
}

int AWindowInfo::handle_event()
{
	gui->awindow->asset_edit->edit_asset(gui->selected_asset());
	return 1;
}

AWindowRedrawIndex::AWindowRedrawIndex(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->redrawindex_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Redraw index"));
}

int AWindowRedrawIndex::handle_event()
{
	return 1;
}

AWindowPaste::AWindowPaste(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->pasteasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Paste asset on recordable tracks"));
}

int AWindowPaste::handle_event()
{
	return 1;
}

AWindowAppend::AWindowAppend(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->appendasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Append asset in new tracks"));
}

int AWindowAppend::handle_event()
{
	return 1;
}

AWindowView::AWindowView(MWindow *mwindow, AWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->viewasset_data)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("View asset"));
}

int AWindowView::handle_event()
{
	return 1;
}



LabelPopup::LabelPopup(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

LabelPopup::~LabelPopup()
{
}

void LabelPopup::create_objects()
{
	add_item(editlabel = new LabelPopupEdit(mwindow, this));
}




LabelPopupEdit::LabelPopupEdit(MWindow *mwindow, LabelPopup *popup)
 : BC_MenuItem(_("Edit..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

LabelPopupEdit::~LabelPopupEdit()
{
}

int LabelPopupEdit::handle_event()
{
	int i = 0;
	while(1)
	{
		AssetPicon *result = (AssetPicon*)mwindow->awindow->gui->asset_list->get_selection(0, i++);
		if(!result) break;

		if(result->label) {
			mwindow->awindow->gui->awindow->label_edit->edit_label(result->label);
			break;
		}
	}

	return 1;
}
