// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "gwindow.h"
#include "gwindowgui.h"

GWindow::GWindow(BC_WindowBase *win)
 : Thread(THREAD_SYNCHRONOUS)
{
	int w, h;

	GWindowGUI::calculate_extents(win, &w, &h);
	gui = new GWindowGUI(w, h);
}

void GWindow::run()
{
	gui->run_window();
}
