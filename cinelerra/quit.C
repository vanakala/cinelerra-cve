// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcmenuitem.h"
#include "edl.h"
#include "glthread.h"
#include "language.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mwindow.h"
#include "quit.h"
#include "question.h"
#include "render.h"
#include "savefile.h"


Quit::Quit(Save *save)
 : BC_MenuItem(_("Quit"), "q", 'q'), Thread()
{
	this->save = save;
}

int Quit::handle_event()
{
	if((master_edl->duration() > EPSILON &&
		mainsession->changes_made) ||
		mwindow_global->render->in_progress)
	{
		if(running())
			return 0;
		start();
	}
	else
	{        // quit
		mwindow_global->stop_playback();
		mwindow_global->interrupt_indexes();
		mwindow_global->delete_brender();
		mwindow_global->glthread->quit();
	}
	return 1;
}

void Quit::run()
{
	int result = 0;
	int cx, cy;

// Test execution conditions
	if(mwindow_global->render->running())
	{
		errorbox(_("Can't quit while a render is in progress."));
		return;
	}
// Quit
	mwindow_global->get_abs_cursor_pos(&cx, &cy);
	QuestionWindow confirm(1, cx, cy,
		_("Save edit list before exiting?\n( Answering \"No\" will destroy changes )"));
	result = confirm.run_window();

	switch(result)
	{
	case 0:          // quit
		mwindow_global->stop_playback();
		mwindow_global->interrupt_indexes();
		mwindow_global->delete_brender();
		mwindow_global->glthread->quit();
		break;

	case 1:        // cancel
		break;

	case 2:        // save
		save->save_before_quit(); 
		break;
	}
}
