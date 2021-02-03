// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "mbuttons.h"
#include "mainerror.h"
#include "glthread.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainmenu.h"
#include "mwindow.h"
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
	if(mainsession->changes_made ||
		mwindow->render->in_progress) 
	{
		if(running())
			return 0;
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
	int cx, cy;

// Test execution conditions
	if(mwindow->render->running())
	{
		errorbox(_("Can't quit while a render is in progress."));
		return;
	}

// Quit
	{
		mwindow->get_abs_cursor_pos(&cx, &cy);
		QuestionWindow confirm(mwindow, 1, cx, cy,
			_("Save edit list before exiting?\n( Answering \"No\" will destroy changes )"));
		result = confirm.run_window();
	}

	switch(result)
	{
	case 0:          // quit
		if(mwindow)
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
