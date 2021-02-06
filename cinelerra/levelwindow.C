// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "levelwindow.h"
#include "levelwindowgui.h"

LevelWindow::LevelWindow()
 : Thread(THREAD_SYNCHRONOUS)
{
	gui = new LevelWindowGUI();
}

LevelWindow::~LevelWindow()
{
	gui->set_done(0);
	join();
	delete gui;
}

void LevelWindow::run()
{
	gui->run_window();
}
