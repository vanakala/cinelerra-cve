#ifndef CROPVIDEO_H
#define CROPVIDEO_H

#include "guicast.h"
#include "mwindow.inc"
#include "thread.h"
#include "cropvideo.inc"

class CropVideo : public BC_MenuItem, public Thread
{
public:
	CropVideo(MWindow *mwindow);
	~CropVideo();

	int handle_event();
	void run();
	int load_defaults();
	int save_defaults();
	int fix_aspect_ratio();

	MWindow *mwindow;
};

class CropVideoWindow : public BC_Window
{
public:
	CropVideoWindow(MWindow *mwindow, CropVideo *thread);
	~CropVideoWindow();

	int create_objects();

	CropVideo *thread;
	MWindow *mwindow;
};


#endif
