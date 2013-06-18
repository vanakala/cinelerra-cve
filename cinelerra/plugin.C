
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
#include "localsession.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginpopup.h"
#include "pluginset.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "track.h"
#include "tracks.h"
#include "virtualnode.h"


Plugin::Plugin(EDL *edl, 
		Track *track, 
		const char *title)
 : Edit(edl, track)
{
	this->track = track;
	this->plugin_set = 0;
	strcpy(this->title, title);
	plugin_type = PLUGIN_NONE;
	in = 1;
	out = 1;
	show = 0;
	on = 1;
	keyframes = new KeyFrames(edl, track, this);
}


Plugin::Plugin(EDL *edl, PluginSet *plugin_set, const char *title)
 : Edit(edl, plugin_set)
{
	this->track = plugin_set->track;
	this->plugin_set = plugin_set;
	strcpy(this->title, title);
	plugin_type = PLUGIN_NONE;
	in = 1;
	out = 1;
	show = 0;
	on = 1;
	keyframes = new KeyFrames(edl, track, this);
}

Plugin::~Plugin()
{
	while(keyframes->last) delete keyframes->last;
	delete keyframes;
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

void Plugin::clear_keyframes(ptstime start, ptstime end)
{
	keyframes->clear(start, end, 0);
}


void Plugin::copy_from(Edit *edit)
{
	Plugin *plugin = (Plugin*)edit;

	this->source_pts = edit->source_pts;
	this->project_pts = edit->project_pts;

	this->plugin_type = plugin->plugin_type;
	this->in = plugin->in;
	this->out = plugin->out;
	this->show = plugin->show;
	this->on = plugin->on;
// Should reconfigure this based on where the first track is now.
	this->shared_location = plugin->shared_location;
	strcpy(this->title, plugin->title);

	copy_keyframes(plugin);
}

void Plugin::copy_keyframes(Plugin *plugin)
{
	keyframes->copy_from(plugin->keyframes);
}

void Plugin::copy_keyframes(ptstime start,
	ptstime end,
	FileXML *file)
{
	keyframes->copy(start, end, file);
}

void Plugin::synchronize_params(Edit *edit)
{
	Plugin *plugin = (Plugin*)edit;
	this->in = plugin->in;
	this->out = plugin->out;
	this->show = plugin->show;
	this->on = plugin->on;
	strcpy(this->title, plugin->title);
	copy_keyframes(plugin);
}

void Plugin::shift_keyframes(ptstime postime)
{
	keyframes->shift_all(postime);
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
	if(!PTSEQU(project_pts, plugin->project_pts) ||
		plugin_type != plugin->plugin_type ||
		on != plugin->on ||
		!(shared_location == plugin->shared_location) ||
		strcmp(title, plugin->title))
	{
		if(*result < 0 || project_pts < *result)
			*result = project_pts;
	}

// Test keyframes
	keyframes->equivalent_output(plugin->keyframes, 
		project_pts, result);
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
			int real_module_number = shared_location.module;
			int real_plugin_number = shared_location.plugin;
			Track *track = edl->tracks->number(real_module_number);
// Get shared plugin from master track
			Plugin *plugin = track->get_current_plugin(postime, 
				real_plugin_number, 
				0);

			if(plugin)
				return plugin->is_synthesis(renderengine, postime);
		}

// Dereference the real track and descend
	case PLUGIN_SHAREDMODULE:
		{
			int real_module_number = shared_location.module;
			Track *track = edl->tracks->number(real_module_number);
			return track->is_synthesis(renderengine, postime);
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
		if(strcmp(title, that->title)) return 0;
		break;
	case PLUGIN_SHAREDPLUGIN:
		if(shared_location.module != that->shared_location.module ||
			shared_location.plugin != that->shared_location.plugin) return 0;
		break;
	case PLUGIN_SHAREDMODULE:
		if(shared_location.module != that->shared_location.module) return 0;
		break;
	}

// Test remaining fields
	return (this->on == that->on &&
		((KeyFrame*)keyframes->first)->identical(
			((KeyFrame*)that->keyframes->first)));
}

int Plugin::identical_location(Plugin *that)
{
	if(!plugin_set || !plugin_set->track) return 0;
	if(!that->plugin_set || !that->plugin_set->track) return 0;

	if(plugin_set->track->number_of() == that->plugin_set->track->number_of() &&
		plugin_set->get_number() == that->plugin_set->get_number() &&
		PTSEQU(project_pts, that->project_pts)) return 1;

	return 0;
}

void Plugin::change_plugin(const char *title, 
		SharedLocation *shared_location, 
		int plugin_type)
{
	strcpy(this->title, title);
	this->shared_location = *shared_location;
	this->plugin_type = plugin_type;
}



KeyFrame* Plugin::get_prev_keyframe(ptstime postime)
{
	KeyFrame *current = 0;

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
		current =  (KeyFrame*)keyframes->last;
	}
	return current;
}

KeyFrame* Plugin::get_keyframe()
{
// Search for keyframe on or before selection
	KeyFrame *result = 
		get_prev_keyframe(edl->local_session->get_selectionstart(1));

// Return nearest keyframe if not in automatic keyframe generation
	if(!edl->session->auto_keyframes)
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

void Plugin::copy(ptstime start, ptstime end, FileXML *file)
{
	ptstime endproject = end_pts();

	if((project_pts >= start && project_pts <= end) ||  // startproject in range
		(endproject <= end && endproject >= start) ||   // endproject in range
		(project_pts <= start && project_pts >= end))    // range in project
	{
// edit is in range
		ptstime startprj_in_selection = project_pts; // start of edit in selection in project
		ptstime len_in_selection = length();             // length of edit in selection

		if(project_pts < start)
		{         // start is after start of edit in project
			startprj_in_selection += start - project_pts;
		}

// Plugins don't store silence
		file->tag.set_title("PLUGIN");
		file->tag.set_property("POSTIME", startprj_in_selection);
		file->tag.set_property("TYPE", plugin_type);
		file->tag.set_property("TITLE", title);
		file->append_tag();
		file->append_newline();

		if(plugin_type == PLUGIN_SHAREDPLUGIN ||
			plugin_type == PLUGIN_SHAREDMODULE)
		{
			shared_location.save(file);
		}

		if(in)
		{
			file->tag.set_title("IN");
			file->append_tag();
			file->tag.set_title("/IN");
			file->append_tag();
		}
		if(out)
		{
			file->tag.set_title("OUT");
			file->append_tag();
			file->tag.set_title("/OUT");
			file->append_tag();
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
		keyframes->copy(start, end, file);

		file->tag.set_title("/PLUGIN");
		file->append_tag();
		file->append_newline();
	}
}

void Plugin::load(FileXML *file)
{
	int result = 0;
	in = 0;
	out = 0;
// Currently show is ignored when loading
	show = 0;
	on = 0;
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
				shared_location.load(file);
			}
			else
			if(file->tag.title_is("IN"))
			{
				in = 1;
			}
			else
			if(file->tag.title_is("OUT"))
			{
				out = 1;
			}
			else
			if(file->tag.title_is("ON"))
			{
				on = 1;
			}
			else
			if(file->tag.title_is("KEYFRAME"))
			{
				ptstime postime;

				if(file->tag.get_property("DEFAULT", 0))
					postime = keyframes->base_pts;
				else
				{
// Convert from old position
					posnum position = file->tag.get_property("POSITION", (posnum)0);
					postime = 0;
					if(position)
						postime = keyframes->pos2pts(position);
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

void Plugin::get_shared_location(SharedLocation *result)
{
	if(plugin_type == PLUGIN_STANDALONE && plugin_set)
	{
		result->module = edl->tracks->number_of(track);
		result->plugin = track->plugin_set.number_of(plugin_set);
	}
	else
	{
		*result = this->shared_location;
	}
}

Track* Plugin::get_shared_track()
{
	return edl->tracks->get_item_number(shared_location.module);
}


void Plugin::calculate_title(char *string, int use_nudge)
{
	if(plugin_type == PLUGIN_STANDALONE || plugin_type == PLUGIN_NONE)
	{
		strcpy(string, _(title));
	}
	else
	if(plugin_type == PLUGIN_SHAREDPLUGIN || plugin_type == PLUGIN_SHAREDMODULE)
	{
		shared_location.calculate_title(string, 
			edl, 
			project_pts,
			plugin_type,
			use_nudge);
	}
}

void Plugin::shift(ptstime difference)
{
	Edit::shift(difference);
	shift_keyframes(difference);
}

void Plugin::dump(int indent)
{
	const char *s;

	switch(plugin_type)
	{
	case 0:
		s = "None";
		break;
	case 1:
		s = "Standalone";
		break;
	case 2:
		s = "SharedPlugin";
		break;
	case 3:
		s = "SharedModule";
		break;
	default:
		s = "Unknown";
		break;
	}

	printf("%*sPlugin %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*stype=%s title=\"%s\" on=%d track=%d plugin=%d\n", indent, "",
		s,
		title, 
		on, 
		shared_location.module,
		shared_location.plugin);
	printf("%*sproject_pts %.3f length %.3f\n", indent, "",
		project_pts, length());

	keyframes->dump(indent);
}

