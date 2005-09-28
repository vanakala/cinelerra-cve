#include "gwindow.h"
#include "gwindowgui.h"
#include "mwindow.h"
#include "mwindowgui.h"

GWindow::GWindow(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
}

void GWindow::run()
{
	gui->run_window();
}

void GWindow::create_objects()
{
	int w, h;


	GWindowGUI::calculate_extents(mwindow->gui, &w, &h);
	gui = new GWindowGUI(mwindow, w, h);
	gui->create_objects();
}



