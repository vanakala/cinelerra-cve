#ifndef QUESTION_H
#define QUESTION_H

#include "guicast.h"
#include "mwindow.inc"

class QuestionWindow : public BC_Window
{
public:
	QuestionWindow(MWindow *mwindow);
	~QuestionWindow();

	int create_objects(char *string, int use_cancel);
	MWindow *mwindow;
};

class QuestionYesButton : public BC_GenericButton
{
public:
	QuestionYesButton(MWindow *mwindow, QuestionWindow *window, int x, int y);

	int handle_event();
	int keypress_event();

	QuestionWindow *window;
};

class QuestionNoButton : public BC_GenericButton
{
public:
	QuestionNoButton(MWindow *mwindow, QuestionWindow *window, int x, int y);

	int handle_event();
	int keypress_event();

	QuestionWindow *window;
};

#endif
