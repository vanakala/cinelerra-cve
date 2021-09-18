// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AWINDOW_H
#define AWINDOW_H

#include "assetedit.inc"
#include "awindowgui.inc"
#include "bcwindowbase.inc"
#include "labeledit.inc"
#include "thread.h"
#include "vframe.inc"

class AWindow : public Thread
{
public:
	AWindow();
	~AWindow();

	void run();
	VFrame *get_window_icon();

	AWindowGUI *gui;
	AssetEdit *asset_edit;
	LabelEdit *label_edit;
};

#endif
