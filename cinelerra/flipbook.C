#include "flipbook.h"
#include "mwindow.h"


FlipBook::FlipBook(MWindow *mwindow)
 : BC_MenuItem("Flipbook...")
{
	this->mwindow = mwindow;
	thread = new FlipBookThread(mwindow);
}

FlipBook::~FlipBook()
{
	delete thread;
}

FlipBook::handle_event()
{
	thread->start();
}





FlipBookThread::FlipBookThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
}

FlipBookThread::~FlipBookThread()
{
}

void FlipBookThread::::run()
{
	
	ArrayList<FlipBookItem*> flipbooklist;
	FlipBookGUI gui(this, mwindow);
	
	int result = gui.run_window();
}





FlipBookGUI::FlipBookGUI(FlipBookThread *thread, MWindow *mwindow)
 : BC_Window()
{
	this->thread = thread;
	this->mwindow = mwindow;
}

FlipBookGUI::~FlipBookGUI()
{
}
