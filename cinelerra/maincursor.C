#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "trackcanvas.h"


MainCursor::MainCursor(MWindow *mwindow, MWindowGUI *gui)
{
	this->mwindow = mwindow;
	this->gui = gui;
	visible = 0;
	active = 0;
	playing_back = 0;
}

MainCursor::~MainCursor()
{
}

void MainCursor::create_objects()
{
//	draw();
}

void MainCursor::focus_in_event()
{
}

void MainCursor::focus_out_event()
{
	show();
	flash();
}

void MainCursor::activate()
{
//printf("MainCursor::activate 1 %d\n", BC_WindowBase::get_resources()->blink_rate);
	if(!active)
	{
		active = 1;
		gui->set_repeat(BC_WindowBase::get_resources()->blink_rate);
	}
}

void MainCursor::deactivate()
{
	if(active)
	{
		active = 0;
		gui->unset_repeat(BC_WindowBase::get_resources()->blink_rate);
		show();
		flash();
	}
}

int MainCursor::repeat_event(int64_t duration)
{
	if(!active || !gui->get_has_focus()) return 0;
	if(duration != BC_WindowBase::get_resources()->blink_rate) return 0;

// Only flash a single sample selection
	if(selectionstart == selectionend)
	{
		if(!playing_back || (playing_back && !visible))
		{
			draw();
			flash();
		}
	}
	return 1;
}

void MainCursor::draw()
{
	if(!visible)
	{
//printf("MainCursor::draw 1\n");
		selectionstart = mwindow->edl->local_session->selectionstart;
		selectionend = mwindow->edl->local_session->selectionend;
		view_start = mwindow->edl->local_session->view_start;
		zoom_sample = mwindow->edl->local_session->zoom_sample;

		pixel1 = Units::to_int64((selectionstart * 
			mwindow->edl->session->sample_rate / 
			zoom_sample - 
			view_start));
		pixel2 = Units::to_int64((selectionend *
			mwindow->edl->session->sample_rate / 
			zoom_sample - 
			view_start));
		if(pixel1 < -10) pixel1 = -10;
		if(pixel2 > gui->canvas->get_w() + 10) pixel2 = gui->canvas->get_w() + 10;
		if(pixel2 < pixel1) pixel2 = pixel1;
//printf("MainCursor::draw 2\n");
	}

	gui->canvas->set_color(WHITE);
	gui->canvas->set_inverse();
	gui->canvas->draw_box(pixel1, 0, pixel2 - pixel1 + 1, gui->canvas->get_h());
	gui->canvas->set_opaque();
	visible = !visible;
}

// Draw the cursor in a new location
void MainCursor::update()
{
	if(visible)
	{
		hide();
		flash();
	}
	show();
	flash();
}


void MainCursor::flash()
{
	gui->canvas->flash(pixel1, 0, pixel2 - pixel1 + 1, gui->canvas->get_h());
	gui->flush();
}

void MainCursor::hide()
{
	if(visible) draw();
}

void MainCursor::show()
{
	if(!visible) draw();
}

// Constitutively redraw the cursor after it is overwritten by a draw
void MainCursor::restore()
{
	if(visible)
	{
		draw();
		visible = 1;
	}
}
