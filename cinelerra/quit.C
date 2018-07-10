
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

#include "assets.h"
#include "mbuttons.h"
#include "mainerror.h"
#include "glthread.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "quit.h"
#include "question.h"
#include "render.h"
#include "savefile.h"
#include "mainsession.h"


Quit::Quit(MWindow *mwindow, Save *save)
 : BC_MenuItem(_("Quit"), "q", 'q'), Thread() 
{ 
	this->mwindow = mwindow; 
	this->save = save;
}

int Quit::handle_event() 
{
	if(mwindow->session->changes_made ||
		mwindow->render->in_progress) 
	{
		start();
	}
	else 
	{        // quit
		mwindow->interrupt_indexes();
		mwindow->delete_brender();
		mwindow->glthread->quit();
	}
	return 1;
}

void Quit::run()
{
	int result = 0;
// Test execution conditions
	if(mwindow->render->running())
	{
		errorbox(_("Can't quit while a render is in progress."));
		return;
	}

// Quit
	{
		QuestionWindow confirm(mwindow, 1,
			_("Save edit list before exiting?\n( Answering \"No\" will destroy changes )"));
		result = confirm.run_window();
	}

	switch(result)
	{
	case 0:          // quit
		if(mwindow->gui)
		{
			mwindow->interrupt_indexes();
			mwindow->delete_brender();
			mwindow->glthread->quit();
		}
		break;

	case 1:        // cancel
		break;

	case 2:        // save
		save->save_before_quit(); 
		break;
	}
}
