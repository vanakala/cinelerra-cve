
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

#include "featheredits.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)



FeatherEdits::FeatherEdits(MWindow *mwindow, int audio, int video)
 : BC_MenuItem("Feather Edits..."), Thread()
{ 
	this->mwindow = mwindow; 
	this->audio = audio; 
	this->video = video; 
}

int FeatherEdits::handle_event()
{
	set_synchronous(0);
	Thread::start();
}


void FeatherEdits::run()
{
	int result;
	long feather_samples;

	{
		feather_samples = mwindow->get_feather(audio, video);

		FeatherEditsWindow window(mwindow, feather_samples);

		window.create_objects(audio, video);
		result = window.run_window();
		feather_samples = window.feather_samples;
	}

	if(!result)
	{
		mwindow->gui->lock_window();
//		mwindow->undo->update_undo_edits(_("Feather"), 0);
		
		mwindow->feather_edits(feather_samples, audio, video);

//		mwindow->undo->update_undo_edits();
		mwindow->gui->unlock_window();
	}
}


FeatherEditsWindow::FeatherEditsWindow(MWindow *mwindow, long feather_samples)
 : BC_Window(PROGRAM_NAME ": Feather Edits", 
 	mwindow->gui->get_abs_cursor_x(), 
	mwindow->gui->get_abs_cursor_y(), 
	340, 
	140)
{
	this->feather_samples = feather_samples;
}

FeatherEditsWindow::~FeatherEditsWindow()
{
	delete text;
}

int FeatherEditsWindow::create_objects(int audio, int video)
{
	int x = 10;
	int y = 10;
	this->audio = audio;
	this->video = video;

	if(audio)
		add_subwindow(new BC_Title(x, y, _("Feather by how many samples:")));
	else
		add_subwindow(new BC_Title(x, y, _("Feather by how many frames:")));

	y += 20;
	char string[1024];
	sprintf(string, "%d", feather_samples);
	add_subwindow(text = new FeatherEditsTextBox(this, string, x, y));

	y += 20;
	add_subwindow(new BC_OKButton(x, y));
	add_subwindow(new BC_CancelButton(x, y));
	return 0;
}

FeatherEditsTextBox::FeatherEditsTextBox(FeatherEditsWindow *window, char *text, int x, int y)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->window = window;
}

int FeatherEditsTextBox::handle_event()
{
	window->feather_samples = atol(get_text());
	return 0;
}
