#include "labelnavigate.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "theme.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

LabelNavigate::LabelNavigate(MWindow *mwindow, MButtons *gui, int x, int y)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->x = x;
	this->y = y;
}

LabelNavigate::~LabelNavigate()
{
	delete prev_label;
	delete next_label;
}

void LabelNavigate::create_objects()
{
	gui->add_subwindow(prev_label = new PrevLabel(mwindow, 
		this, 
		x, 
		y));
	gui->add_subwindow(next_label = new NextLabel(mwindow, 
		this, 
		x + prev_label->get_w(), 
		y));
}


PrevLabel::PrevLabel(MWindow *mwindow, LabelNavigate *navigate, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("prevlabel"))
{ 
	this->mwindow = mwindow; 
	this->navigate = navigate;
	set_tooltip(_("Previous label"));
}

PrevLabel::~PrevLabel() {}

int PrevLabel::handle_event()
{
	mwindow->prev_label();
	return 1;
}



NextLabel::NextLabel(MWindow *mwindow, LabelNavigate *navigate, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("nextlabel"))
{ 
	this->mwindow = mwindow; 
	this->navigate = navigate; 
	set_tooltip(_("Next label"));
}

NextLabel::~NextLabel() {}

int NextLabel::handle_event()
{
	mwindow->next_label();
	return 1;
}


