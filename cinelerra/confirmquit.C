#include "confirmquit.h"
#include "keys.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"

ConfirmQuitWindow::ConfirmQuitWindow(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Question", 
 	mwindow->gui->get_abs_cursor_x(), 
	mwindow->gui->get_abs_cursor_y(), 
	375, 
	160)
{
	this->mwindow = mwindow;
}

ConfirmQuitWindow::~ConfirmQuitWindow()
{
}

int ConfirmQuitWindow::create_objects(char *string)
{
	int x = 10, y = 10;
	BC_Title *title;

//printf("ConfirmQuitWindow::create_objects 1\n");
	add_subwindow(title = new BC_Title(x, y, string));
	y += title->get_h();
//printf("ConfirmQuitWindow::create_objects 1\n");
	add_subwindow(title = new BC_Title(x, y, "( Answering ""No"" will destroy changes )"));
	y = get_h() - 40;
//printf("ConfirmQuitWindow::create_objects 1\n");
	add_subwindow(new ConfirmQuitYesButton(mwindow, x, y));
	x = 150;
//printf("ConfirmQuitWindow::create_objects 1\n");
	add_subwindow(new ConfirmQuitNoButton(mwindow, x, y));
	x = get_w() - 110;
//printf("ConfirmQuitWindow::create_objects 1\n");
	add_subwindow(new ConfirmQuitCancelButton(mwindow, x, y));
//printf("ConfirmQuitWindow::create_objects 1\n");
	return 0;
}

ConfirmQuitYesButton::ConfirmQuitYesButton(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, y, "Yes")
{
	set_underline(0);
}

int ConfirmQuitYesButton::handle_event()
{
	set_done(2);
	return 1;
}

int ConfirmQuitYesButton::keypress_event()
{;
	if(get_keypress() == 'y') return handle_event();
	return 0;
}

ConfirmQuitNoButton::ConfirmQuitNoButton(MWindow *mwindow, int x, int y)
 : BC_GenericButton(x, y, "No")
{
	set_underline(0);
}

int ConfirmQuitNoButton::handle_event()
{
	set_done(0);
	return 1;
}

int ConfirmQuitNoButton::keypress_event()
{
	if(get_keypress() == 'n') return handle_event(); 
	return 0;
}

ConfirmQuitCancelButton::ConfirmQuitCancelButton(MWindow *mwindow, int x, int y)
 : BC_CancelButton(x, y)
{
}

int ConfirmQuitCancelButton::handle_event()
{
	set_done(1);
	return 1;
}

int ConfirmQuitCancelButton::keypress_event()
{
	if(get_keypress() == ESC) return handle_event();
	return 0;
}

