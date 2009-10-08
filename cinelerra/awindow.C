
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
#include "assetremove.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "labeledit.h"

AWindow::AWindow(MWindow *mwindow) : Thread()
{
	this->mwindow = mwindow;
	current_folder[0] = 0;
}


AWindow::~AWindow()
{
	delete asset_edit;
	if (label_edit) delete label_edit;
}

int AWindow::create_objects()
{
	gui = new AWindowGUI(mwindow, this);
	gui->create_objects();
	gui->async_update_assets();
	asset_remove = new AssetRemoveThread(mwindow);
	asset_edit = new AssetEdit(mwindow);
	clip_edit = new ClipEdit(mwindow, this, 0);
	label_edit = new LabelEdit(mwindow, this, 0);
	return 0;
}


void AWindow::run()
{
	gui->run_window();
}
