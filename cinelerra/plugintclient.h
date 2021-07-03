// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLUGINTCLIENT_H
#define PLUGINTCLIENT_H

#include "pluginserver.inc"
#include "pluginclient.h"
#include "theme.inc"

class PluginTClient : public PluginClient
{
public:
	PluginTClient(PluginServer *server);
	virtual ~PluginTClient() {};

	int is_theme() { return 1; };
	virtual Theme* new_theme() { return 0; };
};

#endif
