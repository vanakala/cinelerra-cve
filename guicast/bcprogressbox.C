#include "bcbutton.h"
#include "bcprogress.h"
#include "bcprogressbox.h"
#include "bctitle.h"
#include "bcwindow.h"

BC_ProgressBox::BC_ProgressBox(int x, int y, char *text, long length)
 : Thread()
{
	set_synchronous(1);
	pwindow = new BC_ProgressWindow(x, y);
	pwindow->create_objects(text, length);
	cancelled = 0;
}

BC_ProgressBox::~BC_ProgressBox()
{
	delete pwindow;
}

void BC_ProgressBox::run()
{
	int result = pwindow->run_window();
	if(result) cancelled = 1;
}

int BC_ProgressBox::update(long position)
{
	if(!cancelled)
	{
		pwindow->lock_window();
		pwindow->bar->update(position);
		pwindow->unlock_window();
	}
	return cancelled;
}

int BC_ProgressBox::update_title(char *title)
{
	pwindow->caption->update(title);
	return cancelled;
}

int BC_ProgressBox::update_length(long length)
{
	pwindow->bar->update_length(length);
	return cancelled;
}


int BC_ProgressBox::is_cancelled()
{
	return cancelled;
}

int BC_ProgressBox::stop_progress()
{
	pwindow->set_done(0);
	Thread::join();
	return 0;
}

void BC_ProgressBox::lock_window()
{
	pwindow->lock_window();
}

void BC_ProgressBox::unlock_window()
{
	pwindow->unlock_window();
}



BC_ProgressWindow::BC_ProgressWindow(int x, int y)
 : BC_Window("Progress", x, y, 340, 120, 0, 0, 0)
{
}

BC_ProgressWindow::~BC_ProgressWindow()
{
}

int BC_ProgressWindow::create_objects(char *text, long length)
{
	int x = 10, y = 10;
	this->text = text;
	add_tool(caption = new BC_Title(x, y, text));
	y += caption->get_h() + 20;
	add_tool(bar = new BC_ProgressBar(x, y, get_w() - 20, length));
	y += 40;
	add_tool(new BC_CancelButton(x, y));
	return 0;
}
