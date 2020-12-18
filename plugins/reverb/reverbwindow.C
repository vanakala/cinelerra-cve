// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "bchash.h"
#include "filesystem.h"
#include "language.h"
#include "reverb.h"
#include "reverbwindow.h"

#include <string.h>

PLUGIN_THREAD_METHODS


ReverbWindow::ReverbWindow(Reverb *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	250, 
	230)
{ 
	x = 170;
	y = 10;

	add_tool(new BC_Title(5, y + 10, _("Initial signal level:")));
	add_tool(level_init = new ReverbLevelInit(plugin, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("ms before reflections:")));
	add_tool(delay_init = new ReverbDelayInit(plugin, x + 35, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("First reflection level:")));
	add_tool(ref_level1 = new ReverbRefLevel1(plugin, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("Last reflection level:")));
	add_tool(ref_level2 = new ReverbRefLevel2(plugin, x + 35, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("Number of reflections:")));
	add_tool(ref_total = new ReverbRefTotal(plugin, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("ms of reflections:")));
	add_tool(ref_length = new ReverbRefLength(plugin, x + 35, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("Start band for lowpass:")));
	add_tool(lowpass1 = new ReverbLowPass1(plugin, x, y)); y += 25;
	add_tool(new BC_Title(5, y + 10, _("End band for lowpass:")));
	add_tool(lowpass2 = new ReverbLowPass2(plugin, x + 35, y)); y += 40;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void ReverbWindow::update()
{
	level_init->update(plugin->config.level_init);
	delay_init->update(plugin->config.delay_init);
	ref_level1->update(plugin->config.ref_level1);
	ref_level2->update(plugin->config.ref_level2);
	ref_total->update(plugin->config.ref_total);
	ref_length->update(plugin->config.ref_length);
	lowpass1->update(plugin->config.lowpass1);
	lowpass2->update(plugin->config.lowpass2);
}


ReverbLevelInit::ReverbLevelInit(Reverb *reverb, int x, int y)
 : BC_FPot(x, y,reverb->config.level_init, INFINITYGAIN, 0)
{
	this->reverb = reverb;
}

int ReverbLevelInit::handle_event()
{
	reverb->config.level_init = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbDelayInit::ReverbDelayInit(Reverb *reverb, int x, int y)
 : BC_IPot(x, y, reverb->config.delay_init, 0, 1000)
{
	this->reverb = reverb;
}

int ReverbDelayInit::handle_event()
{
	reverb->config.delay_init = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbRefLevel1::ReverbRefLevel1(Reverb *reverb, int x, int y)
 : BC_FPot(x, y, reverb->config.ref_level1, INFINITYGAIN, 0)
{
	this->reverb = reverb;
}

int ReverbRefLevel1::handle_event()
{
	reverb->config.ref_level1 = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbRefLevel2::ReverbRefLevel2(Reverb *reverb, int x, int y)
 : BC_FPot(x, y, reverb->config.ref_level2, INFINITYGAIN, 0)
{
	this->reverb = reverb;
}

int ReverbRefLevel2::handle_event()
{
	reverb->config.ref_level2 = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbRefTotal::ReverbRefTotal(Reverb *reverb, int x, int y)
 : BC_IPot(x, y, reverb->config.ref_total, 1, 250)
{
	this->reverb = reverb;
}

int ReverbRefTotal::handle_event()
{
	reverb->config.ref_total = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbRefLength::ReverbRefLength(Reverb *reverb, int x, int y)
 : BC_IPot(x, y,reverb->config.ref_length, 0, 5000)
{
	this->reverb = reverb;
}

int ReverbRefLength::handle_event()
{
	reverb->config.ref_length = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbLowPass1::ReverbLowPass1(Reverb *reverb, int x, int y)
 : BC_QPot(x, y, reverb->config.lowpass1)
{
	this->reverb = reverb;
}

int ReverbLowPass1::handle_event()
{
	reverb->config.lowpass1 = get_value();
	reverb->send_configure_change();
	return 1;
}


ReverbLowPass2::ReverbLowPass2(Reverb *reverb, int x, int y)
 : BC_QPot(x, y, reverb->config.lowpass2)
{
	this->reverb = reverb;
}

int ReverbLowPass2::handle_event()
{
	reverb->config.lowpass2 = get_value();
	reverb->send_configure_change();
	return 1;
}
