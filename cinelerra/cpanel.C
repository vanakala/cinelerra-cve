
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

#include "cpanel.h"
#include "cwindowgui.h"
#include "cwindowtool.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "theme.h"


CPanel::CPanel(MWindow *mwindow, 
	CWindowGUI *subwindow, 
	int x, 
	int y, 
	int w, 
	int h)
{
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	subwindow->add_subwindow(operation[CWINDOW_PROTECT] = new CPanelProtect(mwindow, this, x, y));
	y += operation[CWINDOW_PROTECT]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_ZOOM] = new CPanelMagnify(mwindow, this, x, y));
	y += operation[CWINDOW_ZOOM]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_MASK] = new CPanelMask(mwindow, this, x, y));
	y += operation[CWINDOW_MASK]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_CAMERA] = new CPanelCamera(mwindow, this, x, y));
	y += operation[CWINDOW_CAMERA]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_PROJECTOR] = new CPanelProj(mwindow, this, x, y));
	y += operation[CWINDOW_PROJECTOR]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_CROP] = new CPanelCrop(mwindow, this, x, y));
	y += operation[CWINDOW_CROP]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_EYEDROP] = new CPanelEyedrop(mwindow, this, x, y));
	y += operation[CWINDOW_EYEDROP]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_TOOL_WINDOW] = new CPanelToolWindow(mwindow, this, x, y));
	y += operation[CWINDOW_TOOL_WINDOW]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_TITLESAFE] = new CPanelTitleSafe(mwindow, this, x, y));
}

void CPanel::reposition_buttons(int x, int y)
{
	for(int i = 0; i < CPANEL_OPERATIONS; i++)
	{
		operation[i]->reposition_window(x, y);
		y += operation[i]->get_h();
	}
}

void CPanel::set_operation(int value)
{
	for(int i = 0; i < CPANEL_OPERATIONS; i++)
	{
		if(i == CWINDOW_TOOL_WINDOW)
		{
			operation[i]->update(mwindow->edl->session->tool_window);
		}
		else
		if(i == CWINDOW_TITLESAFE)
		{
			operation[i]->update(mwindow->edl->session->safe_regions);
		}
		else
		{
			if(i != value) 
				operation[i]->update(0);
			else
				operation[i]->update(1);
		}
	}
}


CPanelProtect::CPanelProtect(MWindow *mwindow, CPanel *gui, int x, int y)
 : BC_Toggle(x, 
	y, 
	mwindow->theme->get_image_set("protect"), 
	mwindow->edl->session->cwindow_operation == CWINDOW_PROTECT)
{
	this->gui = gui;
	set_tooltip(_("Protect video from changes"));
}

int CPanelProtect::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_PROTECT);
	return 1;
}


CPanelMask::CPanelMask(MWindow *mwindow, CPanel *gui, int x, int y)
 : BC_Toggle(x, 
	y, 
	mwindow->theme->get_image_set("mask"), 
	mwindow->edl->session->cwindow_operation == CWINDOW_MASK)
{
	this->gui = gui;
	set_tooltip(_("Edit mask"));
}

int CPanelMask::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_MASK);
	return 1;
}


CPanelMagnify::CPanelMagnify(MWindow *mwindow, CPanel *gui, int x, int y)
 : BC_Toggle(x, 
	y,
	mwindow->theme->get_image_set("magnify"), 
	mwindow->edl->session->cwindow_operation == CWINDOW_ZOOM)
{
	this->gui = gui;
	set_tooltip(_("Zoom view"));
}

int CPanelMagnify::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_ZOOM);
	return 1;
}


CPanelCamera::CPanelCamera(MWindow *mwindow, CPanel *gui, int x, int y)
 : BC_Toggle(x, 
	y,
	mwindow->theme->get_image_set("camera"), 
	mwindow->edl->session->cwindow_operation == CWINDOW_CAMERA)
{
	this->gui = gui;
	set_tooltip(_("Adjust camera automation"));
}

int CPanelCamera::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_CAMERA);
	return 1;
}


CPanelProj::CPanelProj(MWindow *mwindow, CPanel *gui, int x, int y)
 : BC_Toggle(x, 
	y,
	mwindow->theme->get_image_set("projector"), 
	mwindow->edl->session->cwindow_operation == CWINDOW_PROJECTOR)
{
	this->gui = gui;
	set_tooltip(_("Adjust projector automation"));
}

int CPanelProj::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_PROJECTOR);
	return 1;
}


CPanelCrop::CPanelCrop(MWindow *mwindow, CPanel *gui, int x, int y)
 : BC_Toggle(x, 
	y,
	mwindow->theme->get_image_set("crop"), 
	mwindow->edl->session->cwindow_operation == CWINDOW_CROP)
{
	this->gui = gui;
	set_tooltip(_("Crop a layer or output"));
}

int CPanelCrop::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_CROP);
	return 1;
}


CPanelEyedrop::CPanelEyedrop(MWindow *mwindow, CPanel *gui, int x, int y)
 : BC_Toggle(x, 
	y,
	mwindow->theme->get_image_set("eyedrop"), 
	mwindow->edl->session->cwindow_operation == CWINDOW_EYEDROP)
{
	this->gui = gui;
	set_tooltip(_("Get color"));
}

int CPanelEyedrop::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_EYEDROP);
	return 1;
}


CPanelToolWindow::CPanelToolWindow(MWindow *mwindow, CPanel *gui, int x, int y)
 : BC_Toggle(x, 
	y,
	mwindow->theme->get_image_set("tool"), 
	mwindow->edl->session->tool_window)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Show tool info"));
}

int CPanelToolWindow::handle_event()
{
	mwindow->edl->session->tool_window = get_value();
	gui->subwindow->tool_panel->update_show_window();
	return 1;
}


CPanelTitleSafe::CPanelTitleSafe(MWindow *mwindow, CPanel *gui, int x, int y)
 : BC_Toggle(x, 
	y,
	mwindow->theme->get_image_set("titlesafe"), 
	mwindow->edl->session->safe_regions)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Show safe regions"));
}

int CPanelTitleSafe::handle_event()
{
	mwindow->edl->session->safe_regions = get_value();
	gui->subwindow->canvas->draw_refresh();
	return 1;
}
