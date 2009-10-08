
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

#include "assetremove.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"



AssetRemoveWindow::AssetRemoveWindow(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Remove assets", 
				mwindow->gui->get_abs_cursor_x(1),
				mwindow->gui->get_abs_cursor_y(1),
				320, 
				120, 
				-1, 
				-1, 
				0,
				0, 
				1)
{
	this->mwindow = mwindow;
}
void AssetRemoveWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("Permanently remove from disk?")));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	flush();
}


AssetRemoveThread::AssetRemoveThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	Thread::set_synchronous(0);
}
void AssetRemoveThread::run()
{
	AssetRemoveWindow *window = new AssetRemoveWindow(mwindow);
	window->create_objects();
	int result = window->run_window();
	delete window;
	
	if(!result)
	{
		mwindow->remove_assets_from_disk();
	}
}



