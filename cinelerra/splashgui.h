#ifndef SPLASHGUI_H
#define SPLASHGUI_H

#include "guicast.h"


class SplashGUI : public BC_Window
{
public:
	SplashGUI(VFrame *bg, int x, int y);
	~SplashGUI();
	void create_objects();
	BC_Title *operation;
	VFrame *bg;
	BC_ProgressBar *progress;
};


#endif
