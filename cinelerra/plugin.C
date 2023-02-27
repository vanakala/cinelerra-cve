// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "atmpframecache.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "guidelines.h"
#include "keyframe.h"
#include "keyframes.h"
#include "language.h"
#include "plugin.h"
#include "pluginserver.h"
#include "pluginclient.h"
#include "preferences.inc"
#include "track.h"
#include "tracks.h"
#include "trackplugin.h"
#include "tmpframecache.h"
#include "vframe.h"


Plugin::Plugin(EDL *edl, Track *track, PluginServer *server)
{
	this->edl = edl;
	this->track = track;
	plugin_type = PLUGIN_NONE;
	pts = 0;
	length = 0;
	show = 0;
	on = 1;
	idle = 0;
	guideframe = 0;
	shared_track = 0;
	shared_plugin = 0;
	keyframes = new KeyFrames(edl, track, this);
	id = EDL::next_id();
	plugin_server = server;
	shared_track_id = shared_plugin_id = -1;
	shared_track_num = shared_plugin_num = -1;
	client = 0;
	trackplugin = 0;
	if(server)
		apiversion = server->apiversion;
	else
		apiversion = 0;
}

Plugin::~Plugin()
{
	if(plugin_type != PLUGIN_TRANSITION)
	{
		for(Track *current = track->tracks->first; current;
			current = current->next)
		{
			for(int i = 0; i < current->plugins.total; i++)
			{
				if(current->plugins.values[i]->shared_plugin == this)
				{
					current->plugins.values[i]->shared_plugin = 0;
					plugin_server->close_plugin(
						current->plugins.values[i]->client);
				}
			}
		}
	}
	while(keyframes->last) delete keyframes->last;
	delete keyframes;
	track->tracks->reset_plugins();
	delete guideframe;
	delete trackplugin;
	plugin_server->close_plugin(client);
}

int Plugin::silence()
{
	if(plugin_type != PLUGIN_NONE) 
		return 0;
	else
		return 1;
}

void Plugin::copy(Plugin *plugin, ptstime start, ptstime end)
{
	copy_from(plugin);

	if(pts < start)
	{
		length -= start - pts;
		shift_keyframes(start - pts);
		pts = start;
	}

	if(end_pts() > end)
		length -= end - pts;

	keyframes->clear_after(pts + length);
	shift(-start);
}

void Plugin::copy_from(Plugin *plugin)
{
	plugin->calculate_ref_ids();
	pts = plugin->get_pts();
	length = plugin->duration();

	plugin_type = plugin->plugin_type;
	show = plugin->show;
	on = plugin->on;
	shared_track = plugin->shared_track;
	shared_plugin = plugin->shared_plugin;
	plugin_server = plugin->plugin_server;
	shared_track_id = plugin->shared_track_id;
	shared_plugin_id = plugin->shared_plugin_id;
	id = plugin->id;
	client = 0;

	copy_keyframes(plugin);
}

void Plugin::set_pts(ptstime pts)
{
	this->pts = pts;
	this->keyframes->base_pts = pts;
}

void Plugin::set_duration(ptstime duration)
{
	this->length = duration;
}

void Plugin::set_end(ptstime end)
{
	length = end - pts;
}

Plugin *Plugin::active_in(ptstime start, ptstime end)
{
	if(pts < end && pts + length > start)
		return this;
	return 0;
}

void Plugin::copy_keyframes(Plugin *plugin)
{
	keyframes->copy_from(plugin->keyframes);
}

void Plugin::shift_keyframes(ptstime difference)
{
	keyframes->shift_all(difference);
}

void Plugin::remove_keyframes_after(ptstime pts)
{
	keyframes->clear_after(pts);
}

void Plugin::equivalent_output(Plugin *plugin, ptstime *result)
{
// End of plugin changed
	ptstime thisend = pts + length;
	ptstime plugend = plugin->end_pts();
	if(!PTSEQU(thisend, plugend))
	{
		if(*result < 0 || thisend < *result)
			*result = thisend;
	}

// Start of plugin changed
	if(!PTSEQU(get_pts(), plugin->get_pts()) ||
		plugin_type != plugin->plugin_type ||
		on != plugin->on ||
		shared_track != plugin->shared_track ||
		shared_plugin != plugin->shared_plugin ||
		plugin_server != plugin->plugin_server)
	{
		if(*result < 0 || get_pts() < *result)
			*result = get_pts();
	}

// Test keyframes
	keyframes->equivalent_output(plugin->keyframes, 
		pts, result);
}

int Plugin::is_synthesis()
{
	if(plugin_server)
	{
		switch(plugin_type)
		{
		case PLUGIN_STANDALONE:
			return plugin_server->synthesis;

// Dereference real plugin and descend another level
		case PLUGIN_SHAREDPLUGIN:
			if(shared_plugin)
				return shared_plugin->is_synthesis();
			break;

// Dereference the real track and descend
		case PLUGIN_SHAREDMODULE:
			if(shared_track)
				return shared_track->is_synthesis(pts);
			break;
		}
	}
	return 0;
}

int Plugin::identical(Plugin *that)
{
	if(this == that)
		return 1;
// Test type
	if(plugin_type != that->plugin_type)
		return 0;

// Test length
	if(!PTSEQU(length, that->length))
		return 0;

// Test server or location
	switch(plugin_type)
	{
	case PLUGIN_STANDALONE:
		if(plugin_server != that->plugin_server)
			return 0;
		break;
	case PLUGIN_SHAREDPLUGIN:
		if(shared_plugin != that->shared_plugin)
			return 0;
		break;
	case PLUGIN_SHAREDMODULE:
		if(shared_track != that->shared_track)
			return 0;
		break;
	}

// Test remaining fields
	return (this->on == that->on &&
		((!keyframes->first && !that->keyframes->first) ||
		(keyframes->first && that->keyframes->first &&
		((KeyFrame*)keyframes->first)->identical(
			((KeyFrame*)that->keyframes->first)))));
}

int Plugin::get_number()
{
	return track->plugins.number_of(this);
}

void Plugin::change_plugin(PluginServer *server, int plugin_type,
	Plugin *shared_plugin, Track *shared_track)
{
	PluginClient *newclient;

	plugin_server = server;
	this->shared_plugin = shared_plugin;
	this->shared_track = shared_track;
	this->plugin_type = plugin_type;
	if(guideframe)
		guideframe->clear();

	while(keyframes->last)
		delete keyframes->last;

	server->close_plugin(client);

	if(plugin_type != PLUGIN_SHAREDPLUGIN)
		this->shared_plugin = 0;
	if(plugin_type != PLUGIN_SHAREDMODULE)
		this->shared_track = 0;

	if(plugin_type != PLUGIN_TRANSITION)
	{
		if(plugin_type != PLUGIN_STANDALONE)
			plugin_server = 0;

		for(Track *current = track->tracks->first; current;
			current = current->next)
		{
			for(int i = 0; i < current->plugins.total; i++)
			{
				if(current->plugins.values[i]->shared_plugin == this)
					plugin_server->close_plugin(
						current->plugins.values[i]->client);
			}
		}
	}

	if(plugin_server && (newclient = plugin_server->open_plugin(0, 0)))
	{
		newclient->save_data(keyframes->get_first());
		plugin_server->close_plugin(newclient);
	}

	if(plugin_type != PLUGIN_TRANSITION)
		track->tracks->cleanup_plugins();

	if(trackplugin)
		trackplugin->update();

	for(Track *current = track->tracks->first; current; current = current->next)
	{
		for(int i = 0; i < current->plugins.total; i++)
		{
			Plugin *plugin = current->plugins.values[i];

			if(plugin->shared_plugin == this && plugin->trackplugin)
				plugin->trackplugin->update();
		}
	}
}

KeyFrame* Plugin::get_prev_keyframe(ptstime postime)
{
	KeyFrame *current = 0;

// Get keyframe on or before current position
	for(current = (KeyFrame*)keyframes->last;
			current;
			current = (KeyFrame*)PREVIOUS)
		if(current->pos_time <= postime)
			break;

// Nothing before current position
	if(!current && keyframes->first)
		current = (KeyFrame*)keyframes->first;
	return current;
}

KeyFrame* Plugin::get_next_keyframe(ptstime postime)
{
	KeyFrame *current;

// Get keyframe after current position
	for(current = (KeyFrame*)keyframes->first;
			current;
			current = (KeyFrame*)NEXT)
		if(current->pos_time > postime)
			break;

// Nothing after current position
	if(!current && keyframes->last)
		current = (KeyFrame*)keyframes->last;

	return current;
}

KeyFrame* Plugin::get_keyframe(ptstime selpos)
{
	KeyFrame *result = get_prev_keyframe(selpos);

	if(selpos < pts)
		selpos = pts;
	if(!result)
		result = (KeyFrame*)keyframes->insert_auto(pts);
// Return nearest keyframe if not in automatic keyframe generation
	if(!edlsession->auto_keyframes || !plugin_server ||  plugin_server->no_keyframes)
		return result;
	else
	{
		if(selpos > end_pts())
			selpos = end_pts();
// Return new keyframe
		if(!PTSEQU(result->pos_time, selpos))
			return (KeyFrame*)keyframes->insert_auto(selpos);
	}
// Return existing keyframe
	return result;
}

void Plugin::save_xml(FileXML *file)
{
	if(plugin_type == PLUGIN_TRANSITION)
		file->tag.set_title("TRANSITION");
	else
	{
		file->tag.set_title("PLUGIN");
		file->tag.set_property("POSTIME", pts);
		file->tag.set_property("TYPE", plugin_type);
	}
	if(plugin_server)
		file->tag.set_property("TITLE", plugin_server->title);
	file->tag.set_property("DURATION", length);
	if(!on)
		file->tag.set_property("OFF", !on);
	file->append_tag();
	file->append_newline();

	if(plugin_type == PLUGIN_SHAREDPLUGIN ||
			plugin_type == PLUGIN_SHAREDMODULE)
	{
		save_shared_location(file);
	}

// Keyframes
	keyframes->save_xml(file);

	if(plugin_type == PLUGIN_TRANSITION)
		file->tag.set_title("/TRANSITION");
	else
		file->tag.set_title("/PLUGIN");
	file->append_tag();
	file->append_newline();
}

void Plugin::save_shared_location(FileXML *file)
{
	int numtrack, numplugin;

	numtrack = numplugin = -1;

	file->tag.set_title("SHARED_LOCATION");
	calculate_ref_numbers();
	if(shared_track_num >= 0)
		file->tag.set_property("SHARED_MODULE", shared_track_num);
	if(shared_plugin_num >= 0)
		file->tag.set_property("SHARED_PLUGIN", shared_plugin_num);
	file->append_tag();
	file->tag.set_title("/SHARED_LOCATION");
	file->append_tag();
	file->append_newline();
}

void Plugin::calculate_ref_numbers()
{
	shared_plugin_num = shared_track_num = -1;

	if(shared_track)
		shared_track_num = shared_track->number_of();
	else if(shared_plugin)
	{
		shared_plugin_num = shared_plugin->get_number();
		shared_track_num = shared_plugin->track->number_of();
	}
}

void Plugin::calculate_ref_ids()
{
	shared_track_id = shared_plugin_id = -1;

	if(shared_track)
		shared_track_id = shared_track->get_id();
	if(shared_plugin)
		shared_plugin_id = shared_plugin->id;
}

void Plugin::init_pointers_by_ids()
{
	if(shared_track_id < 0 && shared_plugin_id < 0)
		return;

	shared_track = 0;
	shared_plugin = 0;

	for(Track *current = track->tracks->first; current; current = current->next)
	{
		if(shared_track_id >= 0 && current->get_id() == shared_track_id)
		{
			shared_track = current;
			shared_track_id = -1;
			break;
		}
		if(shared_plugin_id >= 0)
		{
			for(int i = 0; i < current->plugins.total; i++)
			{
				Plugin *plugin = current->plugins.values[i];

				if(plugin->id == shared_plugin_id)
				{
					shared_plugin = plugin;
					shared_plugin_id = -1;
					break;
				}
			}
			if(shared_plugin)
				break;
		}
	}
}

int Plugin::module_number()
{
	return track->number_of();
}

void Plugin::load(FileXML *file, ptstime start)
{
	int result = 0;

	shared_track_num = -1;
	shared_plugin_num = -1;

	while(keyframes->last) delete keyframes->last;

	do{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/PLUGIN") || file->tag.title_is("/TRANSITION"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("SHARED_LOCATION"))
			{
				shared_track_num = file->tag.get_property("SHARED_MODULE", shared_track_num);
				shared_plugin_num = file->tag.get_property("SHARED_PLUGIN", shared_plugin_num);
			}
			else
			if(file->tag.title_is("ON"))
			{
				on = 1;
			}
			else
			if(file->tag.title_is("KEYFRAME") && plugin_type != PLUGIN_NONE)
			{
				ptstime postime;
// first keyframe should always be on base_pts
				if(file->tag.get_property("DEFAULT", 0) || !keyframes->first)
					postime = keyframes->base_pts;
				else
				{
					postime = 0;
					if(track)
					{
// Convert from old position
						posnum position = file->tag.get_property("POSITION", (posnum)0);
						if(position)
							postime = track->from_units(position);
					}
					postime = file->tag.get_property("POSTIME", postime) + start;
				}
				KeyFrame *keyframe;

				if(!(keyframe = (KeyFrame *)keyframes->get_auto_at_position(postime)))
				{
					keyframe = (KeyFrame*)keyframes->append(new KeyFrame(keyframes));
					keyframe->pos_time = postime;
				}
				keyframe->load(file);
			}
		}
	}while(!result);
}

void Plugin::init_shared_pointers()
{
	Track *other_track = 0;
	Plugin *other_plugin = 0;

	if(shared_track_num >= 0)
		other_track = track->tracks->get_item_number(shared_track_num);
	if(shared_plugin_num >= 0 && other_track &&
			other_track->plugins.total > shared_plugin_num)
		other_plugin = other_track->plugins.values[shared_plugin_num];

	if(other_plugin && plugin_type == PLUGIN_SHAREDPLUGIN)
		shared_plugin = other_plugin;
	else
		shared_track = other_track;
}

int Plugin::shared_slots()
{
	int count = 1;
	int max;

	if(!plugin_server || !plugin_server->multichannel)
		return -1;

	if(!(max = plugin_server->multichannel_max))
		return -1;

	for(Track *cur = track->tracks->first; cur; cur = cur->next)
	{
		for(int i = 0; i < cur->plugins.total; i++)
		{
			if(cur->plugins.values[i]->shared_plugin == this)
				count++;
		}
	}
	return count < max;
}

int Plugin::get_multichannel_number()
{
	int num = 0;

	for(int i = 0; i < track->plugins.total; i++)
	{
		if(track->plugins.values[i] == this)
			return num;

		if(track->plugins.values[i]->is_multichannel())
			num++;
	}
	return num;
}

int Plugin::get_multichannel_count(ptstime start, ptstime end)
{
	int count = 0;

	for(int i = 0; i < track->plugins.total; i++)
	{
		Plugin *cur = track->plugins.values[i];

		if(cur->active_in(start, end) && cur->is_multichannel())
			count++;
	}
	return count;
}

int Plugin::is_multichannel()
{
	return (plugin_type == PLUGIN_STANDALONE &&
		plugin_server && plugin_server->multichannel) ||
		(shared_plugin &&
		shared_plugin->plugin_server &&
		shared_plugin->plugin_server->multichannel);
}

int Plugin::shared_with(Track *track)
{
	return track->tracks->shared_on_track(this, track);
}

char *Plugin::calculate_title(char *string)
{
	string[0] = 0;

	if(plugin_type == PLUGIN_STANDALONE && plugin_server)
	{
		strcpy(string, _(plugin_server->title));
	}
	else
	if(plugin_type == PLUGIN_SHAREDMODULE)
	{
		if(shared_track)
			strcpy(string, shared_track->title);
	}
	else
	if(plugin_type == PLUGIN_SHAREDPLUGIN)
	{
		if(shared_plugin)
		{
			strcpy(string, shared_plugin->track->title);
			strcat(string, ": ");
			strcat(string, shared_plugin->plugin_server->title);
		}
	}
	else
		strcpy(string, _("None"));
	return string;
}

void Plugin::shift(ptstime difference)
{
	pts += difference;
	shift_keyframes(difference);
	if(guideframe)
		guideframe->shift(difference);
}

ptstime Plugin::plugin_change_duration(ptstime start, ptstime length)
{
	ptstime ipt_end = start + length;
	ptstime cur_end = end_pts();

	if(pts > start && pts < ipt_end)
		return pts - start;
	if(cur_end > start && cur_end < ipt_end)
		return cur_end - start;
	return length;
}

void Plugin::clear_keyframes()
{
	keyframes->clear_all();
}

void Plugin::get_camera(double *x, double *y, double *z, ptstime postime)
{
	track->automation->get_camera(x, y, z, postime);
}

void Plugin::update_plugin_gui()
{
	if(client)
		client->update_gui();
}

void Plugin::update_display_title()
{
	if(client)
		client->update_display_title();
}

void Plugin::hide_plugin_gui()
{
	show = 0;
	update_toggles();
	if(client)
		client->hide_gui();
}

int Plugin::show_plugin_gui()
{
	int result = 0;

	if(plugin_server && plugin_server->uses_gui)
	{
		if(show)
			client->raise_window();
		else
		{
			if(!client)
			{
				plugin_server->open_plugin(this, 0);
				client->plugin_init(1);
			}
			client->plugin_show_gui();
			result = show = 1;
			update_toggles();
		}
	}
	return result;
}

void Plugin::update_toggles()
{
	if(trackplugin)
		trackplugin->update_toggles();
}

void Plugin::reset_plugin()
{
	if(!idle && client)
	{
		client->plugin_reset();
		idle = 1;
	}
}

size_t Plugin::get_size()
{
	size_t size = sizeof(*this);

	size += keyframes->get_size();
	if(guideframe)
		size += guideframe->get_size();
	return size;
}

void Plugin::dump(int indent)
{
	const char *s;

	switch(plugin_type)
	{
	case PLUGIN_NONE:
		s = "None";
		break;
	case PLUGIN_STANDALONE:
		s = "Standalone";
		break;
	case PLUGIN_SHAREDPLUGIN:
		s = "SharedPlugin";
		break;
	case PLUGIN_SHAREDMODULE:
		s = "SharedModule";
		break;
	case PLUGIN_TRANSITION:
		s = "Transition";
		break;
	default:
		s = "Unknown";
		break;
	}

	printf("%*sPlugin %p dump:\n", indent, "", this);
	indent += 2;
	if(plugin_server)
		printf("%*sType: '%s' title: \"%s\" on: %d show: %d", indent, "",
			s, plugin_server->title, on, show);
	else
		printf("%*sType: '%s' on: %d", indent, "",
			s, on);
	if(shared_track)
		printf(" shared_track %p", shared_track);
	if(shared_plugin)
		printf(" shared_plugin %p", shared_plugin);
	if(shared_track_id >= 0)
		printf(" shared_track_id: %d", shared_track_id);
	if(shared_plugin_id >= 0)
		printf(" shared_plugin_id: %d", shared_plugin_id);
	printf("\n%*sproject_pts %.3f length %.3f id %d idle %d client %p\n", indent, "",
		pts, length, id, idle, client);

	keyframes->dump(indent);
}
