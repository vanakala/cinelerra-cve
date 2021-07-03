// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcresources.h"
#include "cwindow.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginserver.inc"
#include "pluginvclient.h"

#include <string.h>

PluginVClient::PluginVClient(PluginServer *server)
 : PluginClient(server)
{
}

ArrayList<BC_FontEntry*> *PluginVClient::get_fontlist()
{
	return BC_Resources::fontlist;
}

BC_FontEntry *PluginVClient::find_fontentry(const char *displayname, int style,
	int mask, int preferred_style)
{
	return BC_Resources::find_fontentry(displayname, style, mask, preferred_style);
}

int PluginVClient::find_font_by_char(FT_ULong char_code, char *path_new, const FT_Face oldface)
{
	return BC_Resources::find_font_by_char(char_code, path_new, oldface);
}

GuideFrame *PluginVClient::get_guides()
{
	if(!plugin->guideframe && mwindow_global)
		plugin->guideframe = mwindow_global->cwindow->new_guideframe(get_start(), get_end());
	return plugin->guideframe;
}
