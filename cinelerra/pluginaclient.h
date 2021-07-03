// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLUGINACLIENT_H
#define PLUGINACLIENT_H

#include "pluginclient.h"
#include "pluginserver.inc"

class PluginAClient : public PluginClient
{
public:
	PluginAClient(PluginServer *server);
	virtual ~PluginAClient() {};

	int is_audio() { return 1; };
};

#endif
