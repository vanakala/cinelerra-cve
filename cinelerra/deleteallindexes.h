// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DELETEALLINDEXES_H
#define DELETEALLINDEXES_H

#include "bcbutton.h"
#include "preferencesthread.inc"
#include "thread.h"

class DeleteAllIndexes : public BC_GenericButton, public Thread
{
public:
	DeleteAllIndexes(PreferencesWindow *pwindow, int x, int y);

	void run();
	int handle_event();
	PreferencesWindow *pwindow;
};


#endif
