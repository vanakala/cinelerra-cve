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
	gui = new AWindowGUI(mwindow, this);
	gui->create_objects();
	gui->update_assets();
	asset_remove = new AssetRemoveThread(mwindow);
	asset_edit = new AssetEdit(mwindow);
	clip_edit = new ClipEdit(mwindow, this, 0);
	return 0;
}


void AWindow::run()
{
	gui->run_window();
}
