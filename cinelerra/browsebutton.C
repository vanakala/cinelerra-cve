#include "browsebutton.h"
#include "mwindow.h"
#include "theme.h"

BrowseButton::BrowseButton(MWindow *mwindow, 
	BC_WindowBase *parent_window, 
	BC_TextBox *textbox, 
	int x, 
	int y, 
	char *init_directory, 
	char *title, 
	char *caption, 
	int want_directory)
 : BC_Button(x, y, mwindow->theme->magnify_button_data), Thread()
{
	this->parent_window = parent_window;
	this->want_directory = want_directory;
	this->title = title;
	this->caption = caption;
	this->init_directory = init_directory;
	this->textbox = textbox;
	this->mwindow = mwindow;
	set_tooltip("Look for file");
}

BrowseButton::~BrowseButton()  
{ }

int BrowseButton::handle_event() { start(); }

void BrowseButton::run()
{
	BrowseButtonWindow browsewindow(mwindow,
		parent_window, 
		textbox->get_text(), 
		title, 
		caption, 
		want_directory);
	browsewindow.create_objects();
	int result2 = browsewindow.run_window();

	if(!result2)
	{
// 		if(want_directory)
// 		{
// 			textbox->update(browsewindow.get_directory());
// 		}
// 		else
// 		{
// 			textbox->update(browsewindow.get_filename());
// 		}

		textbox->update(browsewindow.get_path());
		parent_window->flush();
		textbox->handle_event();
	}
}






BrowseButtonWindow::BrowseButtonWindow(MWindow *mwindow, BC_WindowBase *parent_window, 
	char *init_directory, 
	char *title, 
	char *caption, 
	int want_directory)
 : BC_FileBox(parent_window->get_abs_cursor_x() - BC_WindowBase::get_resources()->filebox_w / 2, 
 	parent_window->get_abs_cursor_y() - BC_WindowBase::get_resources()->filebox_h / 2,
	init_directory,
	title,
	caption,
// Set to 1 to get hidden files. 
	want_directory,
// Want only directories
	want_directory,
	0,
	mwindow->theme->browse_pad)
{
}

BrowseButtonWindow::~BrowseButtonWindow() 
{
}
