#ifndef ASSETREMOVE_H
#define ASSETREMOVE_H

#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"


class AssetRemoveWindow : public BC_Window
{
public:
	AssetRemoveWindow(MWindow *mwindow);
	void create_objects();
	MWindow *mwindow;
};
class AssetRemoveThread : public Thread
{
public:
	AssetRemoveThread(MWindow *mwindow);
	void run();
	MWindow *mwindow;
};


#endif
