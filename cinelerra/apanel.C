
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

#include "apanel.h"
#include "cwindowgui.h"
#include "language.h"


APanel::APanel(MWindow *mwindow, CWindowGUI *subwindow, int x, int y, int w, int h)
{
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
}

APanel::~APanel()
{
}

int APanel::create_objects()
{
	int x = this->x + 5, y = this->y + 10;
	char string[BCTEXTLEN];
	int x1 = x;

	subwindow->add_subwindow(new BC_Title(x, y, _("Automation")));
	y += 20;
	for(int i = 0; i < PLUGINS; i++)
	{
		sprintf(string, _("Plugin %d"), i + 1);
		subwindow->add_subwindow(new BC_Title(x, y, string, SMALLFONT));
		subwindow->add_subwindow(plugin_autos[i] = new APanelPluginAuto(mwindow, this, x, y + 20));
		if(x == x1)
		{
			x += plugin_autos[i]->get_w();
		}
		else
		{
			x = x1;
			y += plugin_autos[i]->get_h() + 20;
		}
	}
	
	subwindow->add_subwindow(mute = new APanelMute(mwindow, this, x, y));
	y += mute->get_h();
	subwindow->add_subwindow(play = new APanelPlay(mwindow, this, x, y));
	return 0;
}




APanelPluginAuto::APanelPluginAuto(MWindow *mwindow, APanel *gui, int x, int y)
 : BC_FPot(x, 
		y, 
		0, 
		-1, 
		1)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int APanelPluginAuto::handle_event()
{
	return 1;
}

APanelMute::APanelMute(MWindow *mwindow, APanel *gui, int x, int y)
 : BC_CheckBox(x, y, 0, _("Mute"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int APanelMute::handle_event()
{
	return 1;
}


APanelPlay::APanelPlay(MWindow *mwindow, APanel *gui, int x, int y)
 : BC_CheckBox(x, y, 1, _("Play"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int APanelPlay::handle_event()
{
	return 1;
}


