#include "assetedit.h"
#include "assetremove.h"
#include "awindow.h"
#include "awindowgui.h"
#include "clipedit.h"

AWindow::AWindow(MWindow *mwindow) : Thread()
{
	this->mwindow = mwindow;
	current_folder[0] = 0;
}


AWindow::~AWindow()
{
	delete asset_edit;
}

int AWindow::create_objects()
{
//printf("AWindow::create_objects 1\n");
	gui = new AWindowGUI(mwindow, this);
//printf("AWindow::create_objects 1\n");
	gui->create_objects();
//printf("AWindow::create_objects 1\n");
	gui->update_assets();
	asset_remove = new AssetRemoveThread(mwindow);
//printf("AWindow::create_objects 1\n");
	asset_edit = new AssetEdit(mwindow);
//printf("AWindow::create_objects 2\n");
	clip_edit = new ClipEdit(mwindow, this, 0);
//printf("AWindow::create_objects 3\n");
	return 0;
}


void AWindow::run()
{
	gui->run_window();
}
