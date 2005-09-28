#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "tipwindow.h"



// Table of tips of the day
static char *tips[] = 
{
	"When configuring slow effects, disable playback for the track.  After configuring it,\n"
	"re-enable playback to process a single frame.",

	"Ctrl + any transport command causes playback to only cover\n"
	"the region defined by the in/out points.",

	"Shift + clicking a patch causes all other patches except the\n"
	"selected one to toggle.",

	"Clicking on a patch and dragging across other tracks causes\n"
	"the other patches to match the first one.",

	"Shift + clicking on an effect boundary causes dragging to affect\n"
	"just the one effect.",

	"Load multiple files by clicking on one file and shift + clicking on\n"
	"another file.  Ctrl + clicking toggles individual files.",

	"Ctrl + left clicking on the time bar cycles forward a time format.\n"
	"Ctrl + middle clicking on the time bar cycles backward a time format.",

	"Use the +/- keys in the Compositor window to zoom in and out.\n",

	"Pressing Alt while clicking in the cropping window causes translation of\n"
	"all 4 points.\n",

	"Pressing Tab over a track toggles the Record status.\n"
	"Pressing Shift-Tab over a track toggles the Record status of all the other tracks.\n"
};

static int total_tips = sizeof(tips) / sizeof(char*);




TipWindow::TipWindow(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
}

BC_Window* TipWindow::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x();
	int y = display_info.get_abs_cursor_y();
	TipWindowGUI *gui = this->gui = new TipWindowGUI(mwindow, 
		this,
		x,
		y);
	gui->create_objects();
	return gui;
}

char* TipWindow::get_current_tip()
{
	CLAMP(mwindow->session->current_tip, 0, total_tips - 1);
	char *result = tips[mwindow->session->current_tip];
	mwindow->session->current_tip++;
	if(mwindow->session->current_tip >= total_tips)
		mwindow->session->current_tip = 0;
	mwindow->save_defaults();
	return result;
}

void TipWindow::next_tip()
{
	gui->tip_text->update(get_current_tip());
}

void TipWindow::prev_tip()
{
	for(int i = 0; i < 2; i++)
	{
		mwindow->session->current_tip--;
		if(mwindow->session->current_tip < 0)
			mwindow->session->current_tip = total_tips - 1;
	}

	gui->tip_text->update(get_current_tip());
}






TipWindowGUI::TipWindowGUI(MWindow *mwindow, 
	TipWindow *thread,
	int x,
	int y)
 : BC_Window(PROGRAM_NAME ": Tip of the day",
 	x,
	y,
 	640,
	100,
	640,
	100,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void TipWindowGUI::create_objects()
{
	int x = 10, y = 10;
SET_TRACE
	add_subwindow(tip_text = new BC_Title(x, y, thread->get_current_tip()));
	y = get_h() - 30;
SET_TRACE
	BC_CheckBox *checkbox; 
	add_subwindow(checkbox = new TipDisable(mwindow, this, x, y));
SET_TRACE
	BC_Button *button;
	y = get_h() - TipClose::calculate_h(mwindow) - 10;
	x = get_w() - TipClose::calculate_w(mwindow) - 10;
	add_subwindow(button = new TipClose(mwindow, this, x, y));
SET_TRACE
	x -= TipNext::calculate_w(mwindow) + 10;
	add_subwindow(button = new TipNext(mwindow, this, x, y));
SET_TRACE
	x -= TipPrev::calculate_w(mwindow) + 10;
	add_subwindow(button = new TipPrev(mwindow, this, x, y));
SET_TRACE
	x += button->get_w() + 10;

	show_window();
	raise_window();
}

int TipWindowGUI::keypress_event()
{
	switch(get_keypress())
	{
		case RETURN:
		case ESC:
		case 'w':
			set_done(0);
			break;
	}
	return 0;
}







TipDisable::TipDisable(MWindow *mwindow, TipWindowGUI *gui, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	mwindow->preferences->use_tipwindow, 
	_("Show tip of the day."))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int TipDisable::handle_event()
{
	mwindow->preferences->use_tipwindow = get_value();
	return 1;
}



TipNext::TipNext(MWindow *mwindow, TipWindowGUI *gui, int x, int y)
 : BC_Button(x, 
 	y, 
	mwindow->theme->get_image_set("next_tip"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Next tip"));
}
int TipNext::handle_event()
{
	gui->thread->next_tip();
	return 1;
}

int TipNext::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("next_tip")[0]->get_w();
}






TipPrev::TipPrev(MWindow *mwindow, TipWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("prev_tip"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Previous tip"));
}

int TipPrev::handle_event()
{
	gui->thread->prev_tip();
	return 1;
}
int TipPrev::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("prev_tip")[0]->get_w();
}









TipClose::TipClose(MWindow *mwindow, TipWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("close_tip"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Close"));
}

int TipClose::handle_event()
{
	gui->set_done(0);
	return 1;
}

int TipClose::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("close_tip")[0]->get_w();
}
int TipClose::calculate_h(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("close_tip")[0]->get_h();
}



