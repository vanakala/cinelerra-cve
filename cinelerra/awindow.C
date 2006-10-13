#include "assetedit.h"
#include "assetremove.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "labeledit.h"

AWindow::AWindow(MWindow *mwindow) : Thread()
{
	this->mwindow = mwindow;
	current_folder[0] = 0;
}


AWindow::~AWindow()
{
	delete asset_edit;
	if (label_edit) delete label_edit;
}

int AWindow::create_objects()
{
	gui = new AWindowGUI(mwindow, this);
	gui->create_objects();
	gui->async_update_assets();
	asset_remove = new AssetRemoveThread(mwindow);
	asset_edit = new AssetEdit(mwindow);
	clip_edit = new ClipEdit(mwindow, this, 0);
	label_edit = new LabelEdit(mwindow, this, 0);
	return 0;
}


void AWindow::run()
{
	gui->run_window();
}
