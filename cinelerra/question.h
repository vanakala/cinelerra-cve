// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef QUESTION_H
#define QUESTION_H

#include "bcwindow.h"
#include "bcbutton.h"

class QuestionWindow : public BC_Window
{
public:
	QuestionWindow(int use_cancel,
		int absx, int absy, const char *string);
};

class QuestionButton : public BC_GenericButton
{
public:
	QuestionButton(const char *label, int value, int x, int y);

	int handle_event();
	int keypress_event();

	int value;
};

#endif
