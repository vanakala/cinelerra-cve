// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef NORMALIZEWINDOW_H
#define NORMALIZEWINDOW_H

#include "bctextbox.h"
#include "bctoggle.h"
#include "normalize.h"
#include "pluginwindow.h"

class NormalizeWindow : public PluginWindow
{
public:
	NormalizeWindow(NormalizeMain *plugin, int x, int y);

	NormalizeMain *plugin;
};

class NormalizeWindowOverload : public BC_TextBox
{
public:
	NormalizeWindowOverload(NormalizeMain *plugin, int x, int y);

	int handle_event();

	NormalizeMain *plugin;
};

class NormalizeWindowSeparate : public BC_CheckBox
{
public:
	NormalizeWindowSeparate(NormalizeMain *plugin, int x, int y);

	int handle_event();

	NormalizeMain *plugin;
};

#endif
