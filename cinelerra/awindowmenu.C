#include "awindow.h"
#include "awindowgui.h"
#include "awindowmenu.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"






AssetListMenu::AssetListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

AssetListMenu::~AssetListMenu()
{
}

void AssetListMenu::create_objects()
{
	add_item(format = new AssetListFormat(mwindow));
	add_item(new AssetListSort(mwindow));
	update_titles();
}

void AssetListMenu::update_titles()
{
	format->update();
}








AssetListFormat::AssetListFormat(MWindow *mwindow)
 : BC_MenuItem("")
{
	this->mwindow = mwindow;
}

void AssetListFormat::update()
{
	set_text(mwindow->edl->session->assetlist_format == ASSETS_TEXT ?
		(char*)"Display icons" : (char*)"Display text");
}

int AssetListFormat::handle_event()
{
	switch(mwindow->edl->session->assetlist_format)
	{
		case ASSETS_TEXT:
			mwindow->edl->session->assetlist_format = ASSETS_ICONS;
			break;
		case ASSETS_ICONS:
			mwindow->edl->session->assetlist_format = ASSETS_TEXT;
			break;
	}

	mwindow->awindow->gui->asset_list->update_format(
		mwindow->edl->session->assetlist_format, 
		1);

	return 1;
}




AssetListSort::AssetListSort(MWindow *mwindow)
 : BC_MenuItem("Sort items")
{
	this->mwindow = mwindow;
}

int AssetListSort::handle_event()
{
	mwindow->awindow->gui->sort_assets();
	return 1;
}




FolderListMenu::FolderListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 
		0, 
		0, 
		"", 
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

FolderListMenu::~FolderListMenu()
{
}
	
void FolderListMenu::create_objects()
{
	add_item(format = new FolderListFormat(mwindow, this));
	update_titles();
}



void FolderListMenu::update_titles()
{
	format->set_text(mwindow->edl->session->folderlist_format == FOLDERS_TEXT ?
		(char*)"Display icons" : (char*)"Display text");
}







FolderListFormat::FolderListFormat(MWindow *mwindow, FolderListMenu *menu)
 : BC_MenuItem("")
{
	this->mwindow = mwindow;
	this->menu = menu;
}
int FolderListFormat::handle_event()
{
	switch(mwindow->edl->session->folderlist_format)
	{
		case ASSETS_TEXT:
			mwindow->edl->session->folderlist_format = ASSETS_ICONS;
			break;
		case ASSETS_ICONS:
			mwindow->edl->session->folderlist_format = ASSETS_TEXT;
			break;
	}

	mwindow->awindow->gui->folder_list->update_format(mwindow->edl->session->folderlist_format, 1);
	menu->update_titles();

	return 1;
}



