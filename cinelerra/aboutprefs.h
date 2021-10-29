// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef ABOUTPREFS_H
#define ABOUTPREFS_H

#include "preferencesthread.h"

class AboutPrefs : public PreferencesDialog
{
public:
	AboutPrefs(PreferencesWindow *pwindow);

	void show();
};

#endif
