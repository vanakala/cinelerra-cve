#ifndef LABELNAVIGATE_H
#define LABELNAVIGATE_H

class PrevLabel;
class NextLabel;

#include "guicast.h"
#include "mbuttons.inc"
#include "mwindow.inc"

class LabelNavigate
{
public:
	LabelNavigate(MWindow *mwindow, MButtons *gui, int x, int y);
	~LabelNavigate();

	void create_objects();
	
	PrevLabel *prev_label;
	NextLabel *next_label;
	MWindow *mwindow;
	MButtons *gui;
	int x;
	int y;
};

class PrevLabel : public BC_Button
{
public:
	PrevLabel(MWindow *mwindow, LabelNavigate *navigate, int x, int y);
	~PrevLabel();

	int handle_event();

	MWindow *mwindow;
	LabelNavigate *navigate;
};

class NextLabel : public BC_Button
{
public:
	NextLabel(MWindow *mwindow, LabelNavigate *navigate, int x, int y);
	~NextLabel();

	int handle_event();

	MWindow *mwindow;
	LabelNavigate *navigate;
};

#endif
