#ifndef CONFIRMSAVE_H
#define CONFIRMSAVE_H

#include "guicast.h"
#include "mwindow.inc"

class ConfirmQuitYesButton;
class ConfirmQuitNoButton;
class ConfirmQuitCancelButton;

class ConfirmQuitWindow : public BC_Window
{
public:
	ConfirmQuitWindow(MWindow *mwindow);
	~ConfirmQuitWindow();

	int create_objects(char *string);

	MWindow *mwindow;
};

class ConfirmQuitYesButton : public BC_GenericButton
{
public:
	ConfirmQuitYesButton(MWindow *mwindow, int x, int y);

	int handle_event();
	int keypress_event();
};

class ConfirmQuitNoButton : public BC_GenericButton
{
public:
	ConfirmQuitNoButton(MWindow *mwindow, int x, int y);

	int handle_event();
	int keypress_event();
};

class ConfirmQuitCancelButton : public BC_CancelButton
{
public:
	ConfirmQuitCancelButton(MWindow *mwindow, int x, int y);

	int handle_event();
	int keypress_event();
};

#endif
