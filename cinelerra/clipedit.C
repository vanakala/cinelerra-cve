#include "awindow.h"
#include "awindowgui.h"
#include "clipedit.h"
#include "edl.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vwindow.h"
#include "vwindowgui.h"




ClipEdit::ClipEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow)
 : Thread()
{
	this->mwindow = mwindow;
	this->awindow = awindow;
	this->vwindow = vwindow;
	this->clip = 0;
	this->create_it = 0;
}

ClipEdit::~ClipEdit()
{
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
	if(clip)
	{
		EDL *original = clip;
		if(!create_it)
		{
			clip = new EDL(mwindow->edl);
			clip->create_objects();
			clip->copy_all(original);
		}








		ClipEditWindow *window = new ClipEditWindow(mwindow, this);
		window->create_objects();
		int result = window->run_window();
		
		if(!result)
		{
			EDL *new_edl = 0;
// Add to EDL
			if(create_it)
				new_edl = mwindow->edl->add_clip(window->clip);

// Copy clip to existing clip in EDL
			if(!create_it)
				original->copy_session(clip);


//			mwindow->vwindow->gui->update_sources(mwindow->vwindow->gui->source->get_text());


			mwindow->awindow->gui->lock_window();
			mwindow->awindow->gui->update_assets();
			mwindow->awindow->gui->flush();
			mwindow->awindow->gui->unlock_window();

// Change VWindow to it if vwindow was called
// But this doesn't let you easily create a lot of clips.
			if(vwindow && create_it)
			{
//				vwindow->change_source(new_edl);
			}
		}
		else
		{
			mwindow->session->clip_number--;
		}
		


// For creating new clips, the original was copied in add_clip.
// For editing old clips, the original was transferred to another variable.
		delete window->clip;
		delete window;
		clip = 0;
		create_it = 0;
	}
}







ClipEditWindow::ClipEditWindow(MWindow *mwindow, ClipEdit *thread)
 : BC_Window(PROGRAM_NAME ": Clip Info", 
 	mwindow->gui->get_abs_cursor_x() - 400 / 2,
	mwindow->gui->get_abs_cursor_y() - 350 / 2,
	400, 
	350,
	400,
	430,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

ClipEditWindow::~ClipEditWindow()
{
}

	
void ClipEditWindow::create_objects()
{
	this->clip = thread->clip;
	this->create_it = thread->create_it;

	int x = 10, y = 10;
	int x1 = x;
	BC_TextBox *textbox;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x1, y, _("Title:")));
	y += title->get_h() + 5;
	add_subwindow(textbox = new ClipEditTitle(this, x1, y, get_w() - x1 * 2));
	y += textbox->get_h() + 10;
	add_subwindow(title = new BC_Title(x1, y, _("Comments:")));
	y += title->get_h() + 5;
	add_subwindow(textbox = new ClipEditComments(this, 
		x1, 
		y, 
		get_w() - x1 * 2, 
		BC_TextBox::pixels_to_rows(this, MEDIUMFONT, get_h() - 10 - 40 - y)));



	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
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
