#ifndef AWINDOW_H
#define AWINDOW_H

#include "assetedit.inc"
#include "assetremove.inc"
#include "awindowgui.inc"
#include "bcwindowbase.inc"
#include "clipedit.inc"
#include "mwindow.inc"
#include "thread.h"

class AWindow : public Thread
{
public:
	AWindow(MWindow *mwindow);
	~AWindow();

	void run();
	int create_objects();

	char current_folder[BCTEXTLEN];

	AWindowGUI *gui;
	MWindow *mwindow;
	AssetEdit *asset_edit;
	AssetRemoveThread *asset_remove;
	ClipEdit *clip_edit;
};

#endif
