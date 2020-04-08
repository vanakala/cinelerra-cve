
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

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

int PluginVClient::is_video()
{
	return 1;
}

void PluginVClient::run_opengl()
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
		plugin->guideframe = mwindow_global->cwindow->new_guideframe(start_pts, end_pts);
	return plugin->guideframe;
}
