// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2006 Pierre Dumuid

#ifndef LABELEDIT_H
#define LABELEDIT_H

#include "awindow.inc"
#include "bctextbox.h"
#include "bcwindow.h"
#include "thread.h"


class LabelEdit : public Thread
{
public:
	LabelEdit(AWindow *awindow);

	void run();
	void edit_label(Label *label);

	AWindow *awindow;

	Label *label;
};


class LabelEditWindow : public BC_Window
{
public:
	LabelEditWindow(LabelEdit *thread, int absx, int absy);

	Label *label;
	BC_TextBox *textbox;
};


class LabelEditComments : public BC_TextBox
{
public:
	LabelEditComments(LabelEditWindow *window, int x, int y, int w, int rows);
};

#endif
