// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "cpanel.h"
#include "cwindowgui.h"
#include "cwindowtool.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mbuttons.h"
#include "theme.h"


CPanel::CPanel(CWindowGUI *subwindow,
	int x, 
	int y, 
	int w, 
	int h)
{
	this->subwindow = subwindow;
	subwindow->add_subwindow(operation[CWINDOW_PROTECT] =
		new CPanelProtect(this, x, y));
	y += operation[CWINDOW_PROTECT]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_ZOOM] =
		new CPanelMagnify(this, x, y));
	y += operation[CWINDOW_ZOOM]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_MASK] =
		new CPanelMask(this, x, y));
	y += operation[CWINDOW_MASK]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_RULER] =
		new CPanelRuler(this, x, y));
	y += operation[CWINDOW_RULER]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_CAMERA] =
		new CPanelCamera(this, x, y));
	y += operation[CWINDOW_CAMERA]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_PROJECTOR] =
		new CPanelProj(this, x, y));
	y += operation[CWINDOW_PROJECTOR]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_CROP] =
		new CPanelCrop(this, x, y));
	y += operation[CWINDOW_CROP]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_EYEDROP] =
		new CPanelEyedrop(this, x, y));
	y += operation[CWINDOW_EYEDROP]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_TOOL_WINDOW] =
		new CPanelToolWindow(this, x, y));
	y += operation[CWINDOW_TOOL_WINDOW]->get_h();
	subwindow->add_subwindow(operation[CWINDOW_TITLESAFE] =
		new CPanelTitleSafe(this, x, y));
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
	switch(value)
	{
	case CWINDOW_PROTECT:
	case CWINDOW_ZOOM:
		edlsession->tool_window = 0;
		break;
	}

	for(int i = 0; i < CPANEL_OPERATIONS; i++)
	{
		if(i == CWINDOW_TOOL_WINDOW)
		{
			operation[i]->update(edlsession->tool_window);
		}
		else
		if(i == CWINDOW_TITLESAFE)
		{
			operation[i]->update(edlsession->safe_regions);
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


CPanelProtect::CPanelProtect(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("protect"),
	edlsession->cwindow_operation == CWINDOW_PROTECT)
{
	this->gui = gui;
	set_tooltip(_("Protect video from changes"));
}

int CPanelProtect::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_PROTECT);
	return 1;
}


CPanelMask::CPanelMask(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("mask"),
	edlsession->cwindow_operation == CWINDOW_MASK)
{
	this->gui = gui;
	set_tooltip(_("Edit mask"));
}

int CPanelMask::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_MASK);
	return 1;
}


CPanelRuler::CPanelRuler(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("ruler"),
	edlsession->cwindow_operation == CWINDOW_RULER)
{
	this->gui = gui;
	set_tooltip(_("Ruler"));
}

int CPanelRuler::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_RULER);
	return 1;
}

CPanelMagnify::CPanelMagnify(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("magnify"),
	edlsession->cwindow_operation == CWINDOW_ZOOM)
{
	this->gui = gui;
	set_tooltip(_("Zoom view"));
}

int CPanelMagnify::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_ZOOM);
	return 1;
}


CPanelCamera::CPanelCamera(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("camera"),
	edlsession->cwindow_operation == CWINDOW_CAMERA)
{
	this->gui = gui;
	set_tooltip(_("Adjust camera automation"));
}

int CPanelCamera::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_CAMERA);
	return 1;
}


CPanelProj::CPanelProj(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("projector"),
	edlsession->cwindow_operation == CWINDOW_PROJECTOR)
{
	this->gui = gui;
	set_tooltip(_("Adjust projector automation"));
}

int CPanelProj::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_PROJECTOR);
	return 1;
}


CPanelCrop::CPanelCrop(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("crop"),
	edlsession->cwindow_operation == CWINDOW_CROP)
{
	this->gui = gui;
	set_tooltip(_("Crop a layer or output"));
}

int CPanelCrop::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_CROP);
	return 1;
}


CPanelEyedrop::CPanelEyedrop(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("eyedrop"),
	edlsession->cwindow_operation == CWINDOW_EYEDROP)
{
	this->gui = gui;
	set_tooltip(_("Get color"));
}

int CPanelEyedrop::handle_event()
{
	gui->subwindow->set_operation(CWINDOW_EYEDROP);
	return 1;
}


CPanelToolWindow::CPanelToolWindow(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("tool"),
	edlsession->tool_window)
{
	this->gui = gui;
	set_tooltip(_("Show tool info"));
}

int CPanelToolWindow::handle_event()
{
	edlsession->tool_window = 1;
	edlsession->tool_window =
		gui->subwindow->tool_panel->update_show_window();
	set_value(edlsession->tool_window, 0);
	return 1;
}


CPanelTitleSafe::CPanelTitleSafe(CPanel *gui, int x, int y)
 : BC_Toggle(x, y, theme_global->get_image_set("titlesafe"),
	edlsession->safe_regions)
{
	this->gui = gui;
	set_tooltip(_("Show safe regions"));
}

int CPanelTitleSafe::handle_event()
{
	edlsession->safe_regions = get_value();
	gui->subwindow->canvas->draw_refresh();
	return 1;
}
