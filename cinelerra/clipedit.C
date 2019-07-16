
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

#include "awindow.h"
#include "awindowgui.h"
#include "cliplist.h"
#include "clipedit.h"
#include "bctitle.h"
#include "edl.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "mainerror.h"
#include "theme.h"


ClipEdit::ClipEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow)
 : Thread()
{
	this->mwindow = mwindow;
	this->awindow = awindow;
	this->vwindow = vwindow;
	this->clip = 0;
	this->create_it = 0;
}

void ClipEdit::edit_clip(EDL *clip)
{
// Allow more than one window so we don't have to delete the clip in handle_event
	if(clip)
	{
		this->clip = clip;
		this->create_it = 0;
		Thread::start();
	}
}

void ClipEdit::create_clip(EDL *clip)
{
// Allow more than one window so we don't have to delete the clip in handle_event
	if(clip)
	{
		this->clip = clip;
		this->create_it = 1;
		Thread::start();
	}
}

void ClipEdit::run()
{
	int cx, cy;

	if(clip)
	{
		EDL *original = clip;
		if(!create_it)
		{
			clip = new EDL(1);
			clip->copy_all(original);
			clip->copy_session(original);
		}
		mwindow->get_abs_cursor_pos(&cx, &cy);
		ClipEditWindow *window = new ClipEditWindow(mwindow, this, cx, cy);

		int  name_ok_or_cancel = 0;
		int result;
		while (!name_ok_or_cancel)
		{ 
			result = window->run_window();
			if (result)
				name_ok_or_cancel = 1;
			else
			{
				// Check if clip name is unique
				name_ok_or_cancel = 1;
				for (int i = 0; i < cliplist_global.total; i++)
				{
					if (!strcasecmp(clip->local_session->clip_title,
						cliplist_global.values[i]->local_session->clip_title) &&
						(create_it || strcasecmp(clip->local_session->clip_title,
						original->local_session->clip_title)))
					
						name_ok_or_cancel = 0;
				}
				if (!name_ok_or_cancel)
				{
					errorbox(_("A clip with that name already exists."));
					window->titlebox->activate();
				}
			}
		}

		if(!result)
		{
// Add to cliplist
			if(create_it)
				cliplist_global.add_clip(clip);

// Copy clip to existing clip in EDL
			if(!create_it)
			{
				original->copy_session(clip);
				delete clip;
			}

			mwindow->awindow->gui->async_update_assets();

		}
		else
		{
			if(create_it)
			{
				delete clip;
				mainsession->clip_number--;
			}
		}

// For creating new clips, the original was copied in add_clip.
// For editing old clips, the original was transferred to another variable.
		delete window;
		clip = 0;
		create_it = 0;
	}
}


ClipEditWindow::ClipEditWindow(MWindow *mwindow, ClipEdit *thread, int absx, int absy)
 : BC_Window(MWindow::create_title(N_("Clip Info")),
	absx - 400 / 2,
	absy - 350 / 2,
	400, 
	350,
	400,
	430,
	0,
	0,
	1)
{
	int x = 10, y = 10;
	int x1 = x;
	BC_TextBox *textbox;
	BC_Title *title;

	this->mwindow = mwindow;
	this->thread = thread;
	this->clip = thread->clip;
	this->create_it = thread->create_it;

	set_icon(mwindow->theme->get_image("awindow_icon"));
	add_subwindow(title = new BC_Title(x1, y, _("Title:")));
	y += title->get_h() + 5;
	add_subwindow(titlebox = new ClipEditTitle(this, x1, y, get_w() - x1 * 2));

	int end = strlen(titlebox->get_text());
	titlebox->set_selection(0, end, end);

	y += titlebox->get_h() + 10;
	add_subwindow(title = new BC_Title(x1, y, _("Comments:")));
	y += title->get_h() + 5;
	add_subwindow(textbox = new ClipEditComments(this, 
		x1, 
		y, 
		get_w() - x1 * 2, 
		BC_TextBox::pixels_to_rows(this, 
			MEDIUMFONT, 
			get_h() - 10 - BC_OKButton::calculate_h() - y)));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	titlebox->activate();
}


ClipEditTitle::ClipEditTitle(ClipEditWindow *window, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, window->clip->local_session->clip_title)
{
	this->window = window;
}

int ClipEditTitle::handle_event()
{
	strcpy(window->clip->local_session->clip_title, get_text());
	return 1;
}


ClipEditComments::ClipEditComments(ClipEditWindow *window, int x, int y, int w, int rows)
 : BC_TextBox(x, y, w, rows, window->clip->local_session->clip_notes)
{
	this->window = window;
}

int ClipEditComments::handle_event()
{
	strcpy(window->clip->local_session->clip_notes, get_text());
	return 1;
}
