// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcdisplayinfo.h"
#include "bcprogress.h"
#include "bcresources.h"
#include "language.h"
#include "mwindow.h"
#include "splashgui.h"
#include "vframe.h"

#include <unistd.h>


SplashGUI::SplashGUI(VFrame *bg, int x, int y)
 : BC_Window(MWindow::create_title(N_("Loading")), x, y, bg->get_w(), bg->get_h(),
	-1, -1, 0, 0, 1, -1, "", 0, WINDOW_SPLASH)
{
	draw_vframe(bg, 0, 0);
	flash();
	show_window();

	add_subwindow(progress = new BC_ProgressBar(5,
		get_h() - get_resources()->progress_images[0]->get_h() - 5,
		get_w() - 10, 0, 0));
	add_subwindow(operation = 
		new BC_Title(5,
			progress->get_y() - get_text_height(MEDIUMFONT) - 5,
			_("Loading...")));
}
