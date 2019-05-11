
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

#include "bcsignals.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keyframe.h"
#include "keyframes.h"
#include "language.h"
#include "localsession.h"
#include "plugin.h"
#include "pluginset.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "track.h"
#include "tracks.h"
#include "virtualnode.h"


Plugin::Plugin(EDL *edl, PluginSet *plugin_set, const char *title)
 : Edit(edl, plugin_set)
{
	this->track = plugin_set->track;
	this->plugin_set = plugin_set;
	if(title)
		strcpy(this->title, title);
	else
		this->title[0] = 0;
	plugin_type = PLUGIN_NONE;
	show = 0;
	on = 1;
	guideframe = 0;
	shared_track = 0;
	shared_plugin = 0;
	keyframes = new KeyFrames(edl, track, this);
}

Plugin::~Plugin()
{
	while(keyframes->last) delete keyframes->last;
	delete keyframes;
	delete guideframe;
}

Edit& Plugin::operator=(Edit& edit)
{
	copy_from(&edit);
	return *this;
}

Plugin& Plugin::operator=(Plugin& edit)
{
	copy_from(&edit);
	return *this;
}

int Plugin::operator==(Plugin& that)
{
	return identical(&that);
}

int Plugin::operator==(Edit& that)
{
	return identical((Plugin*)&that);
}

int Plugin::silence()
{
	if(plugin_type != PLUGIN_NONE) 
		return 0;
	else
		return 1;
}

void Plugin::copy_from(Edit *edit)
{
	Plugin *plugin = (Plugin*)edit;

	set_source_pts(edit->get_source_pts());
	set_pts(edit->get_pts());

	plugin_type = plugin->plugin_type;
	show = plugin->show;
	on = plugin->on;
	shared_track = plugin->shared_track;
	shared_plugin = plugin->shared_plugin;
	strcpy(title, plugin->title);

	copy_keyframes(plugin);
}

void Plugin::copy_keyframes(Plugin *plugin)
{
	keyframes->copy_from(plugin->keyframes);
}

void Plugin::synchronize_params(Edit *edit)
{
	Plugin *plugin = (Plugin*)edit;

	this->show = plugin->show;
	this->on = plugin->on;
	strcpy(this->title, plugin->title);
	copy_keyframes(plugin);
	if(!PTSEQU(keyframes->base_pts, get_pts()))
		shift_keyframes(get_pts() - keyframes->base_pts);
}

void Plugin::shift_keyframes(ptstime difference)
{
	keyframes->shift_all(difference);
}

void Plugin::remove_keyframes_after(ptstime pts)
{
	keyframes->remove_after(pts);
}

void Plugin::equivalent_output(Edit *edit, ptstime *result)
{
	Plugin *plugin = (Plugin*)edit;
// End of plugin changed
	ptstime thisend = end_pts();
	ptstime plugend = end_pts();
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
		strcmp(title, plugin->title))
	{
		if(*result < 0 || get_pts() < *result)
			*result = get_pts();
	}

// Test keyframes
	keyframes->equivalent_output(plugin->keyframes, 
		get_pts(), result);
}

int Plugin::is_synthesis(RenderEngine *renderengine, 
		ptstime postime)
{
	switch(plugin_type)
	{
	case PLUGIN_STANDALONE:
		{
			PluginServer *plugin_server = renderengine->scan_plugindb(title,
				track->data_type);
			return plugin_server->synthesis;
		}

// Dereference real plugin and descend another level
	case PLUGIN_SHAREDPLUGIN:
		{
			if(shared_plugin)
				return shared_plugin->is_synthesis(renderengine, postime);
			break;
		}

// Dereference the real track and descend
	case PLUGIN_SHAREDMODULE:
		{
			if(shared_track)
				return shared_track->is_synthesis(renderengine, postime);
			break;
		}
	}
	return 0;
}

int Plugin::identical(Plugin *that)
{
// Test type
	if(plugin_type != that->plugin_type) return 0;

// Test title or location
	switch(plugin_type)
	{
	case PLUGIN_STANDALONE:
		if(strcmp(title, that->title))
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

int Plugin::identical_location(Plugin *that)
{
	if(!plugin_set || !plugin_set->track) return 0;
	if(!that->plugin_set || !that->plugin_set->track) return 0;

	if(plugin_set->track->number_of() == that->plugin_set->track->number_of() &&
		plugin_set->get_number() == that->plugin_set->get_number() &&
		PTSEQU(get_pts(), that->get_pts())) return 1;

	return 0;
}

void Plugin::change_plugin(const char *title, int plugin_type,
	Plugin *shared_plugin, Track *shared_track)
{
	if(title)
		strcpy(this->title, title);

	this->shared_plugin = shared_plugin;
	this->shared_track = shared_track;
	this->plugin_type = plugin_type;
	if(guideframe)
		guideframe->clear();
}

KeyFrame* Plugin::get_prev_keyframe(ptstime postime)
{
	KeyFrame *current = 0;

	if(!keyframes->first)
		return (KeyFrame*)keyframes->insert_auto(0);

// This doesn't work because edl->selectionstart doesn't change during
// playback at the same rate as PluginClient::source_position.
	if(postime < 0)
	{
		postime = edl->local_session->get_selectionstart(1);
	}

// Get keyframe on or before current position
	for(current = (KeyFrame*)keyframes->last;
		current;
		current = (KeyFrame*)PREVIOUS)
	{
		if(current->pos_time <= postime) break;
	}

// Nothing before current position
	if(!current && keyframes->first)
	{
		current = (KeyFrame*)keyframes->first;
	}
	return current;
}

KeyFrame* Plugin::get_next_keyframe(ptstime postime)
{
	KeyFrame *current;

	if(!keyframes->first)
		return (KeyFrame*)keyframes->insert_auto(0);

// This doesn't work for playback because edl->selectionstart doesn't 
// change during playback at the same rate as PluginClient::source_position.
	if(postime < 0)
	{
		postime = edl->local_session->get_selectionstart(1);
	}

// Get keyframe after current position
	for(current = (KeyFrame*)keyframes->first;
		current;
		current = (KeyFrame*)NEXT)
	{
		if(current->pos_time > postime) break;
	}

// Nothing after current position
	if(!current && keyframes->last)
	{
		current = (KeyFrame*)keyframes->last;
	}
	return current;
}

KeyFrame* Plugin::first_keyframe()
{
	if(!keyframes->first)
		return (KeyFrame*)keyframes->insert_auto(0);
	return (KeyFrame*)keyframes->first;
}

KeyFrame* Plugin::get_keyframe()
{
// Search for keyframe on or before selection
	KeyFrame *result = 
		get_prev_keyframe(edl->local_session->get_selectionstart(1));

// Return nearest keyframe if not in automatic keyframe generation
	if(!edlsession->auto_keyframes)
	{
		return result;
	}
	else
// Return new keyframe
	if(!PTSEQU(result->pos_time, edl->local_session->get_selectionstart(1)))
	{
		return (KeyFrame*)keyframes->insert_auto(edl->local_session->get_selectionstart(1));
	}
	else
// Return existing keyframe
	{
		return result;
	}

	return 0;
}

void Plugin::save_xml(FileXML *file)
{
// Plugins don't store silence
	file->tag.set_title("PLUGIN");
	file->tag.set_property("POSTIME", get_pts());
	file->tag.set_property("TYPE", plugin_type);
	if(title[0])
		file->tag.set_property("TITLE", title);
	file->append_tag();
	file->append_newline();

	if(plugin_type == PLUGIN_SHAREDPLUGIN ||
			plugin_type == PLUGIN_SHAREDMODULE)
	{
		save_shared_location(file);
	}
	if(show)
	{
		file->tag.set_title("SHOW");
		file->append_tag();
		file->tag.set_title("/SHOW");
		file->append_tag();
	}
	if(on)
	{
		file->tag.set_title("ON");
		file->append_tag();
		file->tag.set_title("/ON");
		file->append_tag();
	}
	file->append_newline();

// Keyframes
	keyframes->save_xml(file);

	file->tag.set_title("/PLUGIN");
	file->append_tag();
	file->append_newline();
}

void Plugin::save_shared_location(FileXML *file)
{
	int numtrack, numplugin;

	numtrack = numplugin = -1;

	file->tag.set_title("SHARED_LOCATION");
	if(shared_track)
		numtrack = shared_track->number_of();
	else if(shared_plugin)
	{
		numplugin = shared_plugin->plugin_set->get_number();
		numtrack = shared_plugin->plugin_set->track->number_of();
	}
	if(numtrack >= 0)
		file->tag.set_property("SHARED_MODULE", numtrack);
	if(numplugin >= 0)
		file->tag.set_property("SHARED_PLUGIN", numplugin);
	file->append_tag();
	file->tag.set_title("/SHARED_LOCATION");
	file->append_tag();
	file->append_newline();
}

void Plugin::load(FileXML *file)
{
	int result = 0;

// Currently show is ignored when loading
	show = 0;
	on = 0;
	shared_track_num = -1;
	shared_plugin_num = -1;

	while(keyframes->last) delete keyframes->last;

	do{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/PLUGIN"))
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
					postime = file->tag.get_property("POSTIME", postime);
				}
				KeyFrame *keyframe;

				if(!(keyframe = (KeyFrame *)keyframes->get_auto_at_position(postime)))
				{
					keyframe = (KeyFrame*)keyframes->append(new KeyFrame(edl, keyframes));
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
		other_track = edl->tracks->get_item_number(shared_track_num);
	if(shared_plugin_num >= 0 && other_track &&
			other_track->plugin_set.total > shared_plugin_num)
		other_plugin = other_track->plugin_set.values[shared_plugin_num]->get_first_plugin();

	if(other_plugin && plugin_type == PLUGIN_SHAREDPLUGIN)
		shared_plugin = other_plugin;
	else
		shared_track = other_track;
}

void Plugin::calculate_title(char *string, int use_nudge)
{
	string[0] = 0;

	if(plugin_type == PLUGIN_STANDALONE || plugin_type == PLUGIN_NONE)
	{
		strcpy(string, _(title));
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
			strcat(string, shared_plugin->title);
		}
	}
	else
		strcpy(string, _("None"));
}

void Plugin::shift(ptstime difference)
{
	Edit::shift(difference);
	shift_keyframes(difference);
	if(guideframe)
		guideframe->shift(difference);
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
	default:
		s = "Unknown";
		break;
	}

	printf("%*sPlugin %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*sType: '%s' title: \"%s\" on: %d", indent, "",
		s, title, on);
	if(shared_track)
		printf(" shared_track %p", shared_track);
	if(shared_plugin)
		printf("  shared_plugin %p", shared_plugin);
	printf("\n%*sproject_pts %.3f length %.3f\n", indent, "",
		get_pts(), length());

	keyframes->dump(indent);
}
