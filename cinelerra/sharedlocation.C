#include "edl.h"
#include "filexml.h"
#include "plugin.h"
#include "sharedlocation.h"
#include "track.h"
#include "tracks.h"
#include "transportque.h"

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
	if(
		module == that.module &&
		plugin == that.plugin
	) return 1;
	else
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
	double position, 
	int convert_units,
	int plugin_type)
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
				PLAY_FORWARD,
				convert_units);
		}

		char track_title[BCTEXTLEN];
		char plugin_title[BCTEXTLEN];

		if(track)
			strcpy(track_title, track->title);
		else
			sprintf(track_title, "None");

		if(plugin)
			strcpy(plugin_title, plugin->title);
		else
			sprintf(plugin_title, "None");

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
			sprintf(string, "None");
//printf("SharedLocation::calculate_title %p %s\n", string);
	}
}



