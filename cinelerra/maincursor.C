
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

#include "bcresources.h"
#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindowgui.h"
#include "trackcanvas.h"


MainCursor::MainCursor(MWindowGUI *gui)
{
	this->gui = gui;
	visible = 0;
	active = 0;
	playing_back = 0;
	enabled = 0;
	pixel1 = 0;
	pixel2 = 0;
	cursor_lock.title = "MainCursor::lock";
}

void MainCursor::focus_out_event()
{
	cursor_lock.lock("MainCursor::focus_out_event");
	if(enabled && !visible)
	{
		draw(1);
		flash();
	}
	cursor_lock.unlock();
}

void MainCursor::activate()
{
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
		cursor_lock.lock("MainCursor::deactvate");
		if(enabled && !visible)
		{
			draw(1);
			flash();
		}
		cursor_lock.unlock();
	}
}

void MainCursor::repeat_event(int duration)
{
	if(!enabled || !active || !gui->get_has_focus() ||
			duration != BC_WindowBase::get_resources()->blink_rate)
		return;

// Only flash a single sample selection
	if(pixel1 == pixel2)
	{
		cursor_lock.lock("MainCursor::repeat_event");
		if(!playing_back || (playing_back && !visible))
		{
			draw(1);
			flash();
		}
		cursor_lock.unlock();
	}
}

void MainCursor::draw(int do_plugintoggles)
{
	ptstime view_start;
	ptstime selectionstart, selectionend;

	if(!visible)
	{
		selectionstart = master_edl->local_session->get_selectionstart(1);
		selectionend = master_edl->local_session->get_selectionend(1);
		view_start = master_edl->local_session->view_start_pts;

		pixel1 = round((selectionstart - view_start) /
			master_edl->local_session->zoom_time);
		pixel2 = round((selectionend - view_start) /
			master_edl->local_session->zoom_time);
		if(pixel1 < -10)
			pixel1 = -10;
		if(pixel2 > gui->canvas->get_w() + 10)
			pixel2 = gui->canvas->get_w() + 10;
		if(pixel2 < pixel1) pixel2 = pixel1;
	}
	visible = !visible;
	gui->canvas->set_color(WHITE);
	gui->canvas->set_inverse();
	gui->canvas->draw_box(pixel1, 0, pixel2 - pixel1 + 1, gui->canvas->get_h());
	gui->canvas->set_opaque();
	if(do_plugintoggles)
		gui->canvas->refresh_plugintoggles();
}

// Draw the cursor in a new location
void MainCursor::update()
{
	cursor_lock.lock("MainCursor::update");
	if(enabled)
	{
		if(visible)
			draw(0);

		draw(1);
		flash();
	}
	cursor_lock.unlock();
}

void MainCursor::flash()
{
	gui->canvas->flash(pixel1, 0, pixel2 - pixel1 + 1, gui->canvas->get_h());
}

void MainCursor::invisible()
{
	cursor_lock.lock("MainCursor::invisible");
	visible = 0;
	enabled = 0;
	cursor_lock.unlock();
}

void MainCursor::hide()
{
	cursor_lock.lock("MainCursor::hide");
	if(enabled)
	{
		if(visible)
		{
			draw(1);
			flash();
		}
		enabled = 0;
	}
	cursor_lock.unlock();
}

void MainCursor::show()
{
	cursor_lock.lock("MainCursor::show");
	if(!enabled)
	{
		if(!visible)
		{
			draw(1);
			flash();
		}
		enabled = 1;
	}
	cursor_lock.unlock();
}
