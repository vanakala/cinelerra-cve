#include "confirmquit.h"
#include "keys.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"




ConfirmQuitWindow::ConfirmQuitWindow(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Question", 
 	mwindow->gui->get_abs_cursor_x(1), 
	mwindow->gui->get_abs_cursor_y(1), 
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

	add_subwindow(title = new BC_Title(x, y, string));
	y += title->get_h();
	add_subwindow(title = new BC_Title(x, y, _("( Answering ""No"" will destroy changes )")));

	add_subwindow(new ConfirmQuitYesButton(mwindow, this));
	add_subwindow(new ConfirmQuitNoButton(mwindow, this));
	add_subwindow(new ConfirmQuitCancelButton(mwindow, this));
	return 0;
}

ConfirmQuitYesButton::ConfirmQuitYesButton(MWindow *mwindow, 
	ConfirmQuitWindow *gui)
 : BC_GenericButton(10, 
 	gui->get_h() - BC_GenericButton::calculate_h() - 10, 
	_("Yes"))
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

ConfirmQuitNoButton::ConfirmQuitNoButton(MWindow *mwindow, 
	ConfirmQuitWindow *gui)
 : BC_GenericButton(gui->get_w() / 2 - BC_GenericButton::calculate_w(gui, _("No")) / 2, 
 	gui->get_h() - BC_GenericButton::calculate_h() - 10, 
	_("No"))
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

ConfirmQuitCancelButton::ConfirmQuitCancelButton(MWindow *mwindow, 
	ConfirmQuitWindow *gui)
 : BC_GenericButton(gui->get_w() - BC_GenericButton::calculate_w(gui, _("Cancel")) - 10, 
 	gui->get_h() - BC_GenericButton::calculate_h() - 10, 
	_("Cancel"))
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

