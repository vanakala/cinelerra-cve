#include "assetremove.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"



AssetRemoveWindow::AssetRemoveWindow(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Remove assets", 
				mwindow->gui->get_abs_cursor_x(1),
				mwindow->gui->get_abs_cursor_y(1),
				320, 
				120, 
				-1, 
				-1, 
				0,
				0, 
				1)
{
	this->mwindow = mwindow;
}
void AssetRemoveWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Permanently remove from disk?")));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	flush();
}


AssetRemoveThread::AssetRemoveThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	Thread::set_synchronous(0);
}
void AssetRemoveThread::run()
{
	AssetRemoveWindow *window = new AssetRemoveWindow(mwindow);
	window->create_objects();
	int result = window->run_window();
	delete window;
	
	if(!result)
	{
		mwindow->remove_assets_from_disk();
	}
}



