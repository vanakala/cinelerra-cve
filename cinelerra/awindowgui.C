// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "assetlist.h"
#include "assetedit.h"
#include "assetpopup.h"
#include "awindowgui.h"
#include "awindowgui.inc"
#include "awindow.h"
#include "awindowmenu.h"
#include "bcdragwindow.h"
#include "bcsignals.h"
#include "bcpixmap.h"
#include "bcresources.h"
#include "cache.h"
#include "cliplist.h"
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
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindb.h"
#include "preferences.h"
#include "theme.h"
#include "vframe.h"
#include "vwindowgui.h"
#include "vwindow.h"

#include <ctype.h>

const char *AWindowGUI::folder_names[] =
{
	N_("Audio Effects"),
	N_("Video Effects"),
	N_("Audio Transitions"),
	N_("Video Transitions"),
	N_("Labels"),
	N_("Clips"),
	N_("Media")
};


AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	Asset *asset)
 : BC_ListBoxItem()
{
	reset();
	this->mwindow = mwindow;
	this->gui = gui;
	this->asset = asset;
	id = asset->id;
	init_object();
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
	id = edl->id;
	init_object();
}

AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	int folder)
 : BC_ListBoxItem(_(AWindowGUI::folder_names[folder]), gui->folder_icon)
{
	reset();
	foldernum = folder;
	this->mwindow = mwindow;
	this->gui = gui;
	persistent = 1;
	init_object();
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
	init_object();
}

AssetPicon::AssetPicon(MWindow *mwindow, 
	AWindowGUI *gui, 
	Label *label)
 : BC_ListBoxItem()
{
	reset();
	foldernum = -1;
	this->mwindow = mwindow;
	this->gui = gui;
	this->label = label;
	init_object();
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
	foldernum = AW_NO_FOLDER;
}

void AssetPicon::init_object()
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
				File file;

				if(!file.open_file(asset, FILE_OPEN_READ | FILE_OPEN_VIDEO))
				{
					pixmap_w = pixmap_h * asset->width / asset->height;
					icon_vframe = new VFrame(0, 
						pixmap_w, 
						pixmap_h, 
						BC_RGB888);
					icon_vframe->clear_pts();
					file.get_frame(icon_vframe);
					icon = new BC_Pixmap(gui, pixmap_w, pixmap_h);
					icon->draw_vframe(icon_vframe,
						0, 
						0, 
						pixmap_w, 
						pixmap_h,
						0,
						0);
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

		set_icon(icon);
		set_icon_vframe(icon_vframe);
	}
	else
	if(edl)
	{
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
		edlsession->ptstotext(name, label->position);
		set_text(name);
		icon = gui->file_icon;
		icon_vframe = BC_WindowBase::get_resources()->type_to_icon[ICON_UNKNOWN];
		set_icon(icon);
		set_icon_vframe(icon_vframe);
	}
}


AWindowGUI::AWindowGUI(MWindow *mwindow, AWindow *awindow)
 : BC_Window(MWindow::create_title(N_("Resources")),
	mainsession->awindow_x,
	mainsession->awindow_y,
	mainsession->awindow_w,
	mainsession->awindow_h,
	250,
	100,
	1,
	1,
	1)
{
	int x, y;
	AssetPicon *picon;

	this->mwindow = mwindow;
	this->awindow = awindow;

	asset_titles[0] = _("Title");
	asset_titles[1] = _("Comments");

	set_icon(awindow->get_window_icon());
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

	clip_icon = new BC_Pixmap(this, 
		mwindow->theme->get_image("clip_icon"),
		PIXMAP_ALPHA);

// Mandatory folders
	folders.append(new AssetPicon(mwindow,
		this,
		AW_AEFFECT_FOLDER));
	folders.append(new AssetPicon(mwindow,
		this,
		AW_VEFFECT_FOLDER));
	folders.append(new AssetPicon(mwindow,
		this,
		AW_ATRANSITION_FOLDER));
	folders.append(new AssetPicon(mwindow,
		this,
		AW_VTRANSITION_FOLDER));
	folders.append(new AssetPicon(mwindow,
		this,
		AW_LABEL_FOLDER));
	folders.append(new AssetPicon(mwindow,
		this,
		AW_CLIP_FOLDER));
	folders.append(new AssetPicon(mwindow,
		this,
		AW_MEDIA_FOLDER));

	create_label_folder();

	create_persistent_folder(&aeffects, 1, 0, 1, 0);
	create_persistent_folder(&veffects, 0, 1, 1, 0);
	create_persistent_folder(&atransitions, 1, 0, 0, 1);
	create_persistent_folder(&vtransitions, 0, 1, 0, 1);

	mwindow->theme->get_awindow_sizes(this);

	add_subwindow(asset_list = new AWindowAssets(mwindow,
		this,
		mwindow->theme->alist_x,
		mwindow->theme->alist_y,
		mwindow->theme->alist_w,
		mwindow->theme->alist_h));

	add_subwindow(divider = new AWindowDivider(mwindow,
		this,
		mwindow->theme->adivider_x,
		mwindow->theme->adivider_y,
		mwindow->theme->adivider_w,
		mwindow->theme->adivider_h));

	divider->set_cursor(HSEPARATE_CURSOR);

	add_subwindow(folder_list = new AWindowFolders(mwindow,
		this,
		mwindow->theme->afolders_x, 
		mwindow->theme->afolders_y,
		mwindow->theme->afolders_w,
		mwindow->theme->afolders_h));

	x = mwindow->theme->abuttons_x;
	y = mwindow->theme->abuttons_y;

	add_subwindow(asset_menu = new AssetPopup(awindow, this));

	add_subwindow(label_menu = new LabelPopup(mwindow, this));

	add_subwindow(assetlist_menu = new AssetListMenu(this));

	add_subwindow(folderlist_menu = new FolderListMenu(this));

	create_custom_xatoms();
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
	delete asset_menu;
	delete label_menu;
	delete assetlist_menu;
	delete folderlist_menu;
}

void AWindowGUI::resize_event(int w, int h)
{
	mainsession->awindow_x = get_x();
	mainsession->awindow_y = get_y();
	mainsession->awindow_w = w;
	mainsession->awindow_h = h;

	mwindow->theme->get_awindow_sizes(this);
	mwindow->theme->draw_awindow_bg(this);

	reposition_objects();

	int x = mwindow->theme->abuttons_x;
	int y = mwindow->theme->abuttons_y;

	BC_WindowBase::resize_event(w, h);
}

void AWindowGUI::translation_event()
{
	mainsession->awindow_x = get_x();
	mainsession->awindow_y = get_y();
}

void AWindowGUI::reposition_objects()
{
	int wmax = mainsession->awindow_w-mwindow->theme->adivider_w;
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

void AWindowGUI::close_event()
{
	hide_window();
	mwindow->mark_awindow_hidden();
	mwindow->save_defaults();
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

void AWindowGUI::create_custom_xatoms()
{
	UpdateAssetsXAtom = create_xatom("CWINDOWGUI_UPDATE_ASSETS");
}

void AWindowGUI::recieve_custom_xatoms(XClientMessageEvent *event)
{
	if(event->message_type == UpdateAssetsXAtom)
		update_assets();
}

void AWindowGUI::async_update_assets()
{
	XClientMessageEvent event;
	event.message_type = UpdateAssetsXAtom;
	event.format = 32;
	send_custom_xatom(&event);
}

void AWindowGUI::create_persistent_folder(ArrayList<BC_ListBoxItem*> *output, 
	int do_audio, 
	int do_video, 
	int is_realtime, 
	int is_transition)
{
	ArrayList<PluginServer*> plugins;

// Get pointers to plugindb entries
	plugindb.fill_plugindb(do_audio,
			do_video, 
			is_realtime, 
			-1,
			is_transition,
			0,
			plugins);

	for(int i = 0; i < plugins.total; i++)
	{
		PluginServer *server = plugins.values[i];
		int exists = 0;

// Create new listitem
		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, this, server);
			output->append(picon);
		}
	}
}

void AWindowGUI::create_label_folder()
{
	Label *current;
	for(current = master_edl->labels->first; current; current = NEXT)
	{
		AssetPicon *picon = new AssetPicon(mwindow, this, current);
		labellist.append(picon);
	}
}

void AWindowGUI::update_asset_list()
{
	for(int i = 0; i < assets.total; i++)
	{
		AssetPicon *picon = (AssetPicon*)assets.values[i];
		picon->in_use--;
	}

// Synchronize EDL clips
	for(int i = 0; i < cliplist_global.total; i++)
	{
		int exists = 0;
		
// Look for clip in existing listitems
		for(int j = 0; j < assets.total && !exists; j++)
		{
			AssetPicon *picon = (AssetPicon*)assets.values[j];

			if(picon->id == cliplist_global.values[i]->id)
			{
				picon->edl = cliplist_global.values[i];
				picon->set_text(cliplist_global.values[i]->local_session->clip_title);
				exists = 1;
				picon->in_use = 1;
			}
		}

// Create new listitem
		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, 
				this, 
				cliplist_global.values[i]);
			assets.append(picon);
		}
	}

// Synchronize global assets
	for(Asset *current = assetlist_global.first;
		current; 
		current = NEXT)
	{
		int exists = 0;

// Look for asset in existing listitems
		for(int j = 0; j < assets.total && !exists; j++)
		{
			AssetPicon *picon = (AssetPicon*)assets.values[j];

			if(picon->id == current->id)
			{
				picon->asset = current;
				exists = 1;
				picon->in_use = 1;
				break;
			}
		}

// Create new listitem
		if(!exists)
		{
			AssetPicon *picon = new AssetPicon(mwindow, this, current);
			assets.append(picon);
		}
	}

	for(int i = assets.total - 1; i >= 0; i--)
	{
		AssetPicon *picon = (AssetPicon*)assets.values[i];
		if(!picon->in_use)
		{
			delete picon;
			assets.remove_number(i);
		}
	}
}

void AWindowGUI::sort_assets()
{
	switch(edlsession->awindow_folder)
	{
	case AW_AEFFECT_FOLDER:
		sort_picons(&aeffects);
		break;
	case AW_VEFFECT_FOLDER:
		sort_picons(&veffects);
		break;
	case AW_ATRANSITION_FOLDER:
		sort_picons(&atransitions);
		break;
	case AW_VTRANSITION_FOLDER:
		sort_picons(&vtransitions);
		break;
	case AW_LABEL_FOLDER:
		// Labels should ALWAYS be sorted by time
		break;
	default:
		sort_picons(&assets);
	}

	update_assets();
}

void AWindowGUI::collect_assets()
{
	int i = 0;
	mainsession->drag_assets->remove_all();
	mainsession->drag_clips->remove_all();
	while(1)
	{
		AssetPicon *result = (AssetPicon*)asset_list->get_selection(0, i++);
		if(!result) break;

		if(result->asset) mainsession->drag_assets->append(result->asset);
		if(result->edl) mainsession->drag_clips->append(result->edl);
	}
}

void AWindowGUI::copy_picons(ArrayList<BC_ListBoxItem*> *dst, 
	ArrayList<BC_ListBoxItem*> *src, 
	int folder)
{
// Remove current pointers
	dst[0].remove_all();
	dst[1].remove_all_objects();

// Create new pointers
	for(int i = 0; i < src->total; i++)
	{
		AssetPicon *picon = (AssetPicon*)src->values[i];
		if(folder < 0 ||
			(picon->asset && AW_MEDIA_FOLDER == folder) ||
			(picon->edl && picon->edl->local_session->awindow_folder == folder))
		{
			BC_ListBoxItem *item2, *item1;
			dst[0].append(item1 = picon);
			if(picon->edl)
				dst[1].append(item2 = new BC_ListBoxItem(picon->edl->local_session->clip_notes));
			else
			if(picon->label && picon->label->textstr)
			{
				char dststr[BCTEXTLEN];
				int whitespace = 0;
				char *p = dststr;
				for(char *s = picon->label->textstr; *s; s++)
				{
					if(isspace(*s))
					{
						if(!whitespace)
						{
							*p++ = ' ';
							whitespace = 1;
						}
					}
					else
					{
						*p++ = *s;
						whitespace = 0;
					}
				}
				*p = 0;
				dst[1].append(item2 = new BC_ListBoxItem(dststr));
			}
			else
				dst[1].append(item2 = new BC_ListBoxItem(""));
			item1->set_autoplace_text(1);
			item2->set_autoplace_text(1);
		}
	}
}

void AWindowGUI::sort_picons(ArrayList<BC_ListBoxItem*> *src)
{
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
	asset_titles[0] = _("Title");
	asset_titles[1] = _("Comments");
	switch(edlsession->awindow_folder)
	{
	case AW_AEFFECT_FOLDER:
		copy_picons(displayed_assets, &aeffects, AW_NO_FOLDER);
		break;
	case AW_VEFFECT_FOLDER:
		copy_picons(displayed_assets, &veffects, AW_NO_FOLDER);
		break;
	case AW_ATRANSITION_FOLDER:
		copy_picons(displayed_assets, &atransitions, AW_NO_FOLDER);
		break;
	case AW_VTRANSITION_FOLDER:
		copy_picons(displayed_assets, &vtransitions, AW_NO_FOLDER);
		break;
	case AW_LABEL_FOLDER:
		copy_picons(displayed_assets, &labellist, AW_NO_FOLDER);
		asset_titles[0] = _("Time Stamps");
		asset_titles[1] = _("Title");
		break;
	default:
		copy_picons(displayed_assets, &assets, edlsession->awindow_folder);
		break;
	}
	// Ensure the current folder icon is highlighted
	for(int i = 0; i < folders.total; i++)
		folders.values[i]->set_selected(0);
	folders.values[edlsession->awindow_folder]->set_selected(1);
}

void AWindowGUI::update_assets()
{
	int current_format;

	update_asset_list();
	labellist.remove_all_objects();
	create_label_folder();
	filter_displayed_assets();

	current_format =  (folder_list->get_format() & LISTBOX_ICONS) ? ASSETS_ICONS : ASSETS_TEXT;
	if(edlsession->folderlist_format != current_format)
		folder_list->update_format(edlsession->folderlist_format ?
				LISTBOX_SMALLFONT | LISTBOX_ICONS : 0, 0);
	folder_list->update(&folders,
		0,
		0,
		1,
		folder_list->get_xposition(),
		folder_list->get_yposition(),
		-1);

	current_format = (asset_list->get_format() & LISTBOX_ICONS) ? ASSETS_ICONS : ASSETS_TEXT;
	if(edlsession->assetlist_format != current_format)
		asset_list->update_format(edlsession->assetlist_format ?
			LISTBOX_SMALLFONT | LISTBOX_ICONS : 0, 0);

	asset_list->update(displayed_assets,
		asset_titles,
		edlsession->asset_columns,
		ASSET_COLUMNS, 
		asset_list->get_xposition(),
		asset_list->get_yposition(),
		-1,
		0);

	flush();
}

int AWindowGUI::folder_number(const char *name)
{
	for(int i = 0; i < AWINDOW_FOLDERS; i++)
	{
		if(strcasecmp(name, folder_names[i]) == 0)
			return i;
	}
	return AW_MEDIA_FOLDER;
}

Asset* AWindowGUI::selected_asset()
{
	AssetPicon *picon = (AssetPicon*)asset_list->get_selection(0, 0);
	if(picon) return picon->asset;
	return 0;
}

PluginServer* AWindowGUI::selected_plugin()
{
	AssetPicon *picon = (AssetPicon*)asset_list->get_selection(0, 0);
	if(picon) return picon->plugin;
	return 0;
}

AssetPicon* AWindowGUI::selected_folder()
{
	return (AssetPicon*)folder_list->get_selection(0, 0);
}


AWindowDivider::AWindowDivider(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int AWindowDivider::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		mainsession->current_operation = DRAG_PARTITION;
		return 1;
	}
	return 0;
}

int AWindowDivider::cursor_motion_event()
{
	if(mainsession->current_operation == DRAG_PARTITION)
	{
		int cy;

		gui->get_relative_cursor_pos(&mainsession->afolders_w, &cy);
		mwindow->theme->get_awindow_sizes(gui);
		gui->reposition_objects();
		return 1;
	}
	return 0;
}

int AWindowDivider::button_release_event()
{
	if(mainsession->current_operation == DRAG_PARTITION)
	{
		mainsession->current_operation = NO_OPERATION;
		return 1;
	}
	return 0;
}


AWindowFolders::AWindowFolders(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h)
 : BC_ListBox(x, 
		y,
		w, 
		h,
		&gui->folders, // Each column has an ArrayList of BC_ListBoxItems.
		(edlsession->folderlist_format == ASSETS_ICONS ?
			LISTBOX_ICONS | LISTBOX_SMALLFONT : 0) | LISTBOX_ICON_TOP | LISTBOX_SROW)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_drag_scroll(0);
}

void AWindowFolders::selection_changed()
{
	AssetPicon *picon = (AssetPicon*)get_selection(0, 0);
	if(picon)
	{
		edlsession->awindow_folder =  picon->foldernum;
		gui->asset_list->draw_background();
		gui->async_update_assets();
	}
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
		&gui->assets,    // Each column has an ArrayList of BC_ListBoxItems.
		(edlsession->assetlist_format == ASSETS_ICONS ?
			(LISTBOX_ICONS | LISTBOX_SMALLFONT) : 0) | LISTBOX_MULTIPLE | LISTBOX_ICON_TOP | LISTBOX_DRAG,
		gui->asset_titles,             // Titles for columns.  Set to 0 for no titles
		edlsession->asset_columns)                // width of each column
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_drag_scroll(0);
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
	if(get_selection(0, 0))
	{
		switch(edlsession->awindow_folder)
		{
		case AW_AEFFECT_FOLDER:
		case AW_VEFFECT_FOLDER:
		case AW_ATRANSITION_FOLDER:
		case AW_VTRANSITION_FOLDER:
			break;
		default:
			if(((AssetPicon*)get_selection(0, 0))->asset)
				mwindow->vwindow->change_source(((AssetPicon*)get_selection(0, 0))->asset);
			else
			if(((AssetPicon*)get_selection(0, 0))->edl)
				mwindow->vwindow->change_source(((AssetPicon*)get_selection(0, 0))->edl);
		}
		return 1;
	}

	return 0;
}

void AWindowAssets::selection_changed()
{
// Show popup window
	if(get_button_down() && get_buttonpress() == 3 && get_selection(0, 0))
	{
		if(edlsession->awindow_folder == AW_AEFFECT_FOLDER ||
			edlsession->awindow_folder == AW_AEFFECT_FOLDER ||
			edlsession->awindow_folder == AW_VEFFECT_FOLDER ||
			edlsession->awindow_folder == AW_ATRANSITION_FOLDER ||
			edlsession->awindow_folder == AW_VTRANSITION_FOLDER)
		{
			gui->assetlist_menu->update_titles();
			gui->assetlist_menu->activate_menu();
		}
		else
		if(edlsession->awindow_folder == AW_LABEL_FOLDER)
		{
			if(((AssetPicon*)get_selection(0, 0))->label)
				gui->label_menu->activate_menu();
		}
		else
		{
			Asset *asset;
			EDL *edl;

			if(asset = ((AssetPicon*)get_selection(0, 0))->asset)
				gui->asset_menu->update(asset, 0);
			else
			if(((AssetPicon*)get_selection(0, 0))->edl)
				gui->asset_menu->update(0, edl);
			gui->asset_menu->activate_menu();
		}

		BC_ListBox::deactivate_selection();
	}
}

void AWindowAssets::draw_background()
{
	BC_ListBox::draw_background();
	set_color(BC_WindowBase::get_resources()->audiovideo_color);
	set_font(LARGEFONT);
	draw_text(get_w() - 
		get_text_width(LARGEFONT, _(AWindowGUI::folder_names[edlsession->awindow_folder])) - 24,
		30, 
		_(AWindowGUI::folder_names[edlsession->awindow_folder]),
		-1, 
		get_bg_surface());
}

int AWindowAssets::drag_start_event()
{
	int collect_pluginservers = 0;
	int collect_assets = 0;

	if(BC_ListBox::drag_start_event())
	{
		switch(edlsession->awindow_folder)
		{
		case AW_AEFFECT_FOLDER:
			mainsession->current_operation = DRAG_AEFFECT;
			collect_pluginservers = 1;
			break;
		case AW_VEFFECT_FOLDER:
			mainsession->current_operation = DRAG_VEFFECT;
			collect_pluginservers = 1;
			break;
		case AW_ATRANSITION_FOLDER:
			mainsession->current_operation = DRAG_ATRANSITION;
			collect_pluginservers = 1;
			break;
		case AW_VTRANSITION_FOLDER:
			mainsession->current_operation = DRAG_VTRANSITION;
			collect_pluginservers = 1;
			break;
		case AW_LABEL_FOLDER:
			// do nothing!
			break;
		default:
			mainsession->current_operation = DRAG_ASSET;
			collect_assets = 1;
			break;
		}

		if(collect_pluginservers)
		{
			int i = 0;
			mainsession->drag_pluginservers->remove_all();
			while(1)
			{
				AssetPicon *result = (AssetPicon*)get_selection(0, i++);
				if(!result) break;
				
				mainsession->drag_pluginservers->append(result->plugin);
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

void AWindowAssets::drag_motion_event()
{
	BC_ListBox::drag_motion_event();

	mwindow->gui->drag_motion();
	mwindow->vwindow->gui->drag_motion();
	mwindow->cwindow->gui->drag_motion();
}

void AWindowAssets::drag_stop_event()
{
	int result = 0;

	if(!result)
		result = mwindow->gui->drag_stop();

	if(!result)
		result = mwindow->vwindow->gui->drag_stop();

	if(!result)
		result = mwindow->cwindow->gui->drag_stop();

	if(result) get_drag_popup()->set_animation(0);

	BC_ListBox::drag_stop_event();
	mainsession->current_operation = ::NO_OPERATION; // since NO_OPERATION is also defined in listbox, we have to reach for global scope...
}

int AWindowAssets::column_resize_event()
{
	edlsession->asset_columns[0] = get_column_width(0);
	edlsession->asset_columns[1] = get_column_width(1);
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
	add_item(editlabel = new LabelPopupEdit(mwindow, this));
}


LabelPopupEdit::LabelPopupEdit(MWindow *mwindow, LabelPopup *popup)
 : BC_MenuItem(_("Edit..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int LabelPopupEdit::handle_event()
{
	int i = 0;
	while(1)
	{
		AssetPicon *result = (AssetPicon*)mwindow->awindow->gui->asset_list->get_selection(0, i++);
		if(!result) break;

		if(result->label)
		{
			mwindow->awindow->gui->awindow->label_edit->edit_label(result->label);
			break;
		}
	}

	return 1;
}
