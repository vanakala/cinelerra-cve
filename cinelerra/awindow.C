
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "assetedit.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "labeledit.h"
#include "theme.h"


AWindow::AWindow(MWindow *mwindow) : Thread()
{
	gui = new AWindowGUI(mwindow, this);
	gui->async_update_assets();
	asset_edit = new AssetEdit(mwindow);
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
