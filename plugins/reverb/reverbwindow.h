// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef REVERBWINDOW_H
#define REVERBWINDOW_H

#define TOTAL_LOADS 5

class ReverbThread;
class ReverbWindow;

#include "bcpot.h"
#include "reverb.h"
#include "pluginwindow.h"

PLUGIN_THREAD_HEADER


class ReverbLevelInit;
class ReverbDelayInit;
class ReverbRefLevel1;
class ReverbRefLevel2;
class ReverbRefTotal;
class ReverbRefLength;
class ReverbLowPass1;
class ReverbLowPass2;
class ReverbMenu;

class ReverbWindow : public PluginWindow
{
public:
	ReverbWindow(Reverb *reverb, int x, int y);

	void update();

	Reverb *reverb;
	ReverbLevelInit *level_init;
	ReverbDelayInit *delay_init;
	ReverbRefLevel1 *ref_level1;
	ReverbRefLevel2 *ref_level2;
	ReverbRefTotal *ref_total;
	ReverbRefLength *ref_length;
	ReverbLowPass1 *lowpass1;
	ReverbLowPass2 *lowpass2;
	ReverbMenu *menu;
	PLUGIN_GUI_CLASS_MEMBERS
};

class ReverbLevelInit : public BC_FPot
{
public:
	ReverbLevelInit(Reverb *reverb, int x, int y);

	int handle_event();
	Reverb *reverb;
};

class ReverbDelayInit : public BC_IPot
{
public:
	ReverbDelayInit(Reverb *reverb, int x, int y);

	int handle_event();
	Reverb *reverb;
};

class ReverbRefLevel1 : public BC_FPot
{
public:
	ReverbRefLevel1(Reverb *reverb, int x, int y);

	int handle_event();
	Reverb *reverb;
};

class ReverbRefLevel2 : public BC_FPot
{
public:
	ReverbRefLevel2(Reverb *reverb, int x, int y);

	int handle_event();
	Reverb *reverb;
};

class ReverbRefTotal : public BC_IPot
{
public:
	ReverbRefTotal(Reverb *reverb, int x, int y);

	int handle_event();
	Reverb *reverb;
};

class ReverbRefLength : public BC_IPot
{
public:
	ReverbRefLength(Reverb *reverb, int x, int y);

	int handle_event();
	Reverb *reverb;
};

class ReverbLowPass1 : public BC_QPot
{
public:
	ReverbLowPass1(Reverb *reverb, int x, int y);

	int handle_event();
	Reverb *reverb;
};

class ReverbLowPass2 : public BC_QPot
{
public:
	ReverbLowPass2(Reverb *reverb, int x, int y);

	int handle_event();
	Reverb *reverb;
};

#endif
