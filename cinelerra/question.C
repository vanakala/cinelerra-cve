#include "mwindow.h"
#include "mwindowgui.h"
#include "question.h"
#include "theme.h"


#define WIDTH 375
#define HEIGHT 160

QuestionWindow::QuestionWindow(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Question", 
 	mwindow->gui->get_abs_cursor_x() - WIDTH / 2, 
	mwindow->gui->get_abs_cursor_y() - HEIGHT / 2, 
	WIDTH, 
	HEIGHT)
{
	this->mwindow = mwindow;
}

QuestionWindow::~QuestionWindow()
{
}

int QuestionWindow::create_objects(char *string, int use_cancel)
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(10, 10, string));
	y += 30;
	add_subwindow(new QuestionYesButton(mwindow, this, x, y));
	x += get_w() / 2;
	add_subwindow(new QuestionNoButton(mwindow, this, x, y));
	x = get_w() - 100;
	if(use_cancel) add_subwindow(new BC_CancelButton(x, y));
	return 0;
}

QuestionYesButton::QuestionYesButton(MWindow *mwindow, QuestionWindow *window, int x, int y)
 : BC_GenericButton(x, y, "Yes")
{
	this->window = window;
	set_underline(0);
}

int QuestionYesButton::handle_event()
{
	set_done(2);
}

int QuestionYesButton::keypress_event()
{
	if(get_keypress() == 'y') { handle_event(); return 1; }
	return 0;
}

QuestionNoButton::QuestionNoButton(MWindow *mwindow, QuestionWindow *window, int x, int y)
 : BC_GenericButton(x, y, "No")
{
	this->window = window;
	set_underline(0);
}

int QuestionNoButton::handle_event()
{
	set_done(0);
}

int QuestionNoButton::keypress_event()
{
	if(get_keypress() == 'n') { handle_event(); return 1; }
	return 0;
}
