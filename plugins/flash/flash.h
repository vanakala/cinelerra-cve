// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FLASH_H
#define FLASH_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_TRANSITION

#define PLUGIN_TITLE N_("Flash")
#define PLUGIN_CLASS FlashMain
#define PLUGIN_USES_TMPFRAME

#include "pluginmacros.h"

class FlashMain;

#include "language.h"
#include "pluginvclient.h"
#include "vframe.inc"

class FlashMain : public PluginVClient
{
public:
	FlashMain(PluginServer *server);
	~FlashMain();

	PLUGIN_CLASS_MEMBERS
// required for all realtime plugins
	void process_realtime(VFrame *input_ptr, VFrame *output_ptr);
};

#endif
