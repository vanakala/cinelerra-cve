// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MBUTTONS_H
#define MBUTTONS_H

#include "bcsignals.h"
#include "editpanel.h"
#include "edl.inc"
#include "mbuttons.inc"
#include "playtransport.h"

class MainEditing;

class MButtons : public BC_SubWindow
{
public:
	MButtons();
	~MButtons();

	void show();
	void resize_event();
	int keypress_event();
	void update();

	PlayTransport *transport;
	MainEditing *edit_panel;
};

class MainTransport : public PlayTransport
{
public:
	MainTransport(MButtons *mbuttons, int x, int y);

	void goto_start();
	void goto_end();
	EDL *get_edl();
};

class MainEditing : public EditPanel
{
public:
	MainEditing(MButtons *mbuttons, int x, int y);

	MButtons *mbuttons;
};

#endif
