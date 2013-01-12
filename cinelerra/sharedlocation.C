
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

#include "edl.h"
#include "filexml.h"
#include "language.h"
#include "plugin.h"
#include "sharedlocation.h"
#include "track.h"
#include "tracks.h"

// plugin locations

#include <string.h>


SharedLocation::SharedLocation()
{
	this->module = -1;
	this->plugin = -1;
}

SharedLocation::SharedLocation(int module, int plugin)
{
	this->module = module;
	this->plugin = plugin;
}

void SharedLocation::save(FileXML *file)
{
	file->tag.set_title("SHARED_LOCATION");
	if(module >= 0) file->tag.set_property("SHARED_MODULE", (int64_t)module);
	if(plugin >= 0) file->tag.set_property("SHARED_PLUGIN", (int64_t)plugin);
	file->append_tag();
	file->tag.set_title("/SHARED_LOCATION");
	file->append_tag();
	file->append_newline();
}

void SharedLocation::load(FileXML *file)
{
	module = -1;
	plugin = -1;

	module = file->tag.get_property("SHARED_MODULE", (int64_t)module);
	plugin = file->tag.get_property("SHARED_PLUGIN", (int64_t)plugin);
}

int SharedLocation::get_type()
{
	if(plugin < 0)
		return PLUGIN_SHAREDMODULE;
	else
		return PLUGIN_SHAREDPLUGIN;
}

int SharedLocation::operator==(const SharedLocation &that)
{
	if(module == that.module && plugin == that.plugin)
		return 1;
	return 0;
}

SharedLocation& SharedLocation::operator=(const SharedLocation &that)
{
	this->plugin = that.plugin;
	this->module = that.module;
	return *this;
}

void SharedLocation::calculate_title(char *string, 
	EDL *edl, 
	ptstime position,
	int plugin_type,
	int use_nudge)
{
	if(plugin_type == PLUGIN_SHAREDPLUGIN)
	{
		Track *track = 0;
		Plugin *plugin = 0;

		track = edl->tracks->get_item_number(module);
		if(track && this->plugin >= 0)
		{
			plugin = track->get_current_plugin(position, 
				this->plugin, 
				use_nudge);
		}

		const char *track_title;
		const char *plugin_title;

		if(track)
			track_title = track->title;
		else
			track_title = _("None");

		if(plugin)
			plugin_title = plugin->title;
		else
			plugin_title = _("None");

		sprintf(string, "%s: %s", track_title, plugin_title);
	}
	else
	if(plugin_type == PLUGIN_SHAREDMODULE)
	{
		Track *track = 0;
		track = edl->tracks->get_item_number(module);

		if(track)
			strcpy(string, track->title);
		else
			strcpy(string, _("None"));
	}
}
