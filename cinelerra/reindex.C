#include "reindex.h"

ReIndex::ReIndex(MWindow *mwindow)
 : BC_MenuItem("Redraw Indexes", "", 0), Thread()
{
	this->mwindow = mwindow;
}

ReIndex::~ReIndex()
{
}

ReIndex::handle_event() { start(); }

void ReIndex::run()
{
	int result;
	
	if(mwindow->gui) mwindow->gui->disable_window();
	if(mwindow->gui) mwindow->lock_resize();

	{
		ReIndexWindow window;
		window.create_objects();
		result = window.run_window();
	}
	
	if(!result)       // user didn't cancel
	{
// ========== need pointers since mainmenu is created after tracks
// ==================================== delete old index files
		mwindow->tracks->delete_index_files();
// ==================================== create new index files
		mwindow->tracks->create_index_files(1);
// ==================================== draw
		mwindow->draw();
	}
	if(mwindow->gui) mwindow->unlock_resize();
	if(mwindow->gui) mwindow->gui->enable_window();
}

ReIndexWindow::ReIndexWindow(char *display = "")
 : BC_Window(display, MEGREY, PROGRAM_NAME ": Redraw Indexes", 340, 140, 340, 140)
{
}

ReIndexWindow::~ReIndexWindow()
{
	delete ok;
	delete cancel;
}

ReIndexWindow::create_objects()
{
	BC_SubWindow *subwindow;
	
	add_subwindow(subwindow = new BC_SubWindow(0, 0, w, h, MEGREY));
	subwindow->add_subwindow(new BC_Title(5, 5, "Redraw all indexes for the current project?"));
	subwindow->add_subwindow(ok = new ReIndexOkButton(this));
	subwindow->add_subwindow(cancel = new ReIndexCancelButton(this));
}

ReIndexOkButton::ReIndexOkButton(ReIndexWindow *window)
 : BC_Button(5, 80, "Yes")
{
	this->window = window;
}

ReIndexOkButton::handle_event()
{
	window->set_done(0);
}

ReIndexOkButton::keypress_event()
{
	if(window->get_keypress() == 13) { handle_event(); return 1; }
	return 0;
}

ReIndexCancelButton::ReIndexCancelButton(ReIndexWindow *window)
 : BC_Button(140, 80, "No")
{
	this->window = window;
}

ReIndexCancelButton::handle_event()
{
	window->set_done(1);
}

ReIndexCancelButton::keypress_event()
{
	if(window->get_keypress() == ESC) { handle_event(); return 1; }
	return 0;
}

