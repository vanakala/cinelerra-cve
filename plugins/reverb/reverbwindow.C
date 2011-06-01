
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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "filesystem.h"
#include "language.h"
#include "reverb.h"
#include "reverbwindow.h"

#include <string.h>

PLUGIN_THREAD_OBJECT(Reverb, ReverbThread, ReverbWindow)


ReverbWindow::ReverbWindow(Reverb *reverb, int x, int y)
 : BC_Window(reverb->gui_string, 
	x,
	y, 
	250, 
	230, 
	250, 
	230, 
	0, 
	0,
	1)
{ 
	this->reverb = reverb; 
}

ReverbWindow::~ReverbWindow()
{
}

void ReverbWindow::create_objects()
{
	int x = 170, y = 10;
	VFrame *ico = reverb->new_picon();

	set_icon(ico);
	add_tool(new BC_Title(5, y + 10, _("Initial signal level:")));
	add_tool(level_init = new ReverbLevelInit(reverb, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("ms before reflections:")));
	add_tool(delay_init = new ReverbDelayInit(reverb, x + 35, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("First reflection level:")));
	add_tool(ref_level1 = new ReverbRefLevel1(reverb, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("Last reflection level:")));
	add_tool(ref_level2 = new ReverbRefLevel2(reverb, x + 35, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("Number of reflections:")));
	add_tool(ref_total = new ReverbRefTotal(reverb, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("ms of reflections:")));
	add_tool(ref_length = new ReverbRefLength(reverb, x + 35, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("Start band for lowpass:")));
	add_tool(lowpass1 = new ReverbLowPass1(reverb, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("End band for lowpass:")));
	add_tool(lowpass2 = new ReverbLowPass2(reverb, x + 35, y)); y += 40;
	show_window();
	flush();
	delete ico;
}


ReverbLevelInit::ReverbLevelInit(Reverb *reverb, int x, int y)
 : BC_FPot(x, 
	y,
	reverb->config.level_init, 
	INFINITYGAIN, 
	0)
{
	this->reverb = reverb;
}

ReverbLevelInit::~ReverbLevelInit() 
{
}

int ReverbLevelInit::handle_event()
{
	reverb->config.level_init = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbDelayInit::ReverbDelayInit(Reverb *reverb, int x, int y)
 : BC_IPot(x, 
	y, 
	reverb->config.delay_init, 
	0, 
	1000)
{
	this->reverb = reverb;
}

ReverbDelayInit::~ReverbDelayInit() 
{
}

int ReverbDelayInit::handle_event()
{
	reverb->config.delay_init = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbRefLevel1::ReverbRefLevel1(Reverb *reverb, int x, int y)
 : BC_FPot(x, 
	y, 
	reverb->config.ref_level1, 
	INFINITYGAIN, 
	0)
{
	this->reverb = reverb;
}

ReverbRefLevel1::~ReverbRefLevel1() 
{
}

int ReverbRefLevel1::handle_event()
{
	reverb->config.ref_level1 = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbRefLevel2::ReverbRefLevel2(Reverb *reverb, int x, int y)
 : BC_FPot(x, 
	y, 
	reverb->config.ref_level2, 
	INFINITYGAIN, 
	0)
{
	this->reverb = reverb;
}

ReverbRefLevel2::~ReverbRefLevel2()
{
}

int ReverbRefLevel2::handle_event()
{
	reverb->config.ref_level2 = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbRefTotal::ReverbRefTotal(Reverb *reverb, int x, int y)
 : BC_IPot(x, 
	y,
	reverb->config.ref_total, 
	1, 
	250)
{
	this->reverb = reverb;
}

ReverbRefTotal::~ReverbRefTotal()
{
}

int ReverbRefTotal::handle_event()
{
	reverb->config.ref_total = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbRefLength::ReverbRefLength(Reverb *reverb, int x, int y)
 : BC_IPot(x, 
	y,
	reverb->config.ref_length, 
	0, 
	5000)
{
	this->reverb = reverb;
}

ReverbRefLength::~ReverbRefLength() 
{
}

int ReverbRefLength::handle_event()
{
	reverb->config.ref_length = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbLowPass1::ReverbLowPass1(Reverb *reverb, int x, int y)
 : BC_QPot(x, 
	y, 
	reverb->config.lowpass1)
{
	this->reverb = reverb;
}

ReverbLowPass1::~ReverbLowPass1() 
{
}

int ReverbLowPass1::handle_event()
{
	reverb->config.lowpass1 = get_value();
	reverb->send_configure_change();
	return 1;
}

ReverbLowPass2::ReverbLowPass2(Reverb *reverb, int x, int y)
 : BC_QPot(x, 
	y,
	reverb->config.lowpass2)
{
	this->reverb = reverb;
}

ReverbLowPass2::~ReverbLowPass2() 
{
}

int ReverbLowPass2::handle_event()
{
	reverb->config.lowpass2 = get_value();
	reverb->send_configure_change();
	return 1;
}
