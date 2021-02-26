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
	LabelEditWindow(Label *label, int absx, int absy);

	BC_TextBox *textbox;
	Label *label;
};


class LabelEditComments : public BC_TextBox
{
public:
	LabelEditComments(Label *label, int x, int y, int w, int rows);
};

#endif
