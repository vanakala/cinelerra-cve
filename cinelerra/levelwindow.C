#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainmenu.h"
#include "mwindow.h"

LevelWindow::LevelWindow(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	Thread::set_synchronous(1);
}

LevelWindow::~LevelWindow()
{
	if(gui) 
	{
		gui->set_done(0);
		join();
	}
	if(gui) delete gui;
}

int LevelWindow::create_objects()
{
	gui = new LevelWindowGUI(mwindow, this);
	gui->create_objects();
	return 0;
}


void LevelWindow::run()
{
	if(gui) gui->run_window();
}
