// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "assetedit.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "labeledit.h"
#include "theme.h"


AWindow::AWindow() : Thread()
{
	gui = new AWindowGUI(this);
	gui->async_update_assets();
	asset_edit = new AssetEdit();
	label_edit = new LabelEdit(this);
}

AWindow::~AWindow()
{
	delete asset_edit;
	delete label_edit;
	delete gui;
}

void AWindow::run()
{
	gui->run_window();
}

VFrame *AWindow::get_window_icon()
{
	return theme_global->get_image("awindow_icon");
}
