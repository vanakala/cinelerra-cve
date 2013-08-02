
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

#include "asset.h"
#include "autoconf.h"
#include "automation.h"
#include "bcsignals.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "keyframe.h"
#include "localsession.h"
#include "module.h"
#include "patchbay.h"
#include "plugin.h"
#include "pluginset.h"
#include "mainsession.h"
#include "theme.h"
#include "intautos.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "vedit.h"
#include "vframe.h"
#include <string.h>


Track::Track(EDL *edl, Tracks *tracks) : ListItem<Track>()
{
	this->edl = edl;
	this->tracks = tracks;
	y_pixel = 0;
	expand_view = 0;
	draw = 1;
	gang = 1;
	title[0] = 0;
	record = 1;
	play = 1;
	nudge = 0;
	track_w = edl->session->output_w;
	track_h = edl->session->output_h;
	one_unit = (ptstime) 1 / 48000;
	id = EDL::next_id();
}

Track::~Track()
{
	delete automation;
	delete edits;
	plugin_set.remove_all_objects();
}

void Track::copy_settings(Track *track)
{
	this->expand_view = track->expand_view;
	this->draw = track->draw;
	this->gang = track->gang;
	this->record = track->record;
	this->nudge = track->nudge;
	this->play = track->play;
	this->track_w = track->track_w;
	this->track_h = track->track_h;
	strcpy(this->title, track->title);
}

int Track::get_id()
{
	return id;
}

void Track::equivalent_output(Track *track, ptstime *result)
{
	if(data_type != track->data_type ||
		track_w != track->track_w ||
		track_h != track->track_h ||
		play != track->play ||
		nudge != track->nudge)
		*result = 0;

// Convert result to track units
	ptstime result2 = -1;
	automation->equivalent_output(track->automation, &result2);
	edits->equivalent_output(track->edits, &result2);

	int plugin_sets = MIN(plugin_set.total, track->plugin_set.total);
// Test existing plugin sets
	for(int i = 0; i < plugin_sets; i++)
	{
		plugin_set.values[i]->equivalent_output(
			track->plugin_set.values[i], 
			&result2);
	}

// New EDL has more plugin sets.  Get starting plugin in new plugin sets
	for(int i = plugin_sets; i < plugin_set.total; i++)
	{
		Plugin *current = plugin_set.values[i]->get_first_plugin();
		if(current)
		{
			if(result2 < 0 || current->project_pts < result2)
				result2 = current->project_pts;
		}
	}

// New EDL has fewer plugin sets.  Get starting plugin in old plugin set
	for(int i = plugin_sets; i < track->plugin_set.total; i++)
	{
		Plugin *current = track->plugin_set.values[i]->get_first_plugin();
		if(current)
		{
			if(result2 < 0 || current->project_pts < result2)
				result2 = current->project_pts;
		}
	}

// Number of plugin sets differs but somehow we didn't find the start of the
// change.  Assume 0
	if(track->plugin_set.total != plugin_set.total && result2 < 0)
		result2 = 0;

	if(result2 >= 0 && (*result < 0 || result2 < *result))
		*result = result2;
}


int Track::is_synthesis(RenderEngine *renderengine, 
	ptstime position)
{
	int is_synthesis = 0;
	for(int i = 0; i < plugin_set.total; i++)
	{
		Plugin *plugin = get_current_plugin(position,
			i,
			0);
		if(plugin)
		{
// Assume data from a shared track is synthesized
			if(plugin->plugin_type == PLUGIN_SHAREDMODULE) 
				is_synthesis = 1;
			else
				is_synthesis = plugin->is_synthesis(renderengine, 
					position);
			if(is_synthesis) break;
		}
	}
	return is_synthesis;
}

void Track::copy_from(Track *track)
{
	copy_settings(track);
	edits->copy_from(track->edits);
	for(int i = 0; i < this->plugin_set.total; i++)
		delete this->plugin_set.values[i];
	this->plugin_set.remove_all_objects();

	for(int i = 0; i < track->plugin_set.total; i++)
	{
		PluginSet *new_plugin_set = plugin_set.append(new PluginSet(edl, this));
		new_plugin_set->copy_from(track->plugin_set.values[i]);
	}
	automation->copy_from(track->automation);
	this->track_w = track->track_w;
	this->track_h = track->track_h;
}

Track& Track::operator=(Track& track)
{
	copy_from(&track);
	return *this;
}

int Track::vertical_span(Theme *theme)
{
	int result = 0;
	if(expand_view)
		result = edl->local_session->zoom_track + 
			plugin_set.total * 
			theme->get_image("plugin_bg_data")->get_h();
	else
		result = edl->local_session->zoom_track;

	if(edl->session->show_titles)
		result += theme->get_image("title_bg_data")->get_h();

	return result;
}

ptstime Track::get_length()
{
	ptstime total_length = 0;
	ptstime length = 0;
	Edit *plugin;

// Test edits
	if(edits->last)
		total_length = edits->last->project_pts;

// Test plugins
	for(int i = 0; i < plugin_set.total; i++)
	{
		if(plugin = plugin_set.values[i]->last)
		{
			length = plugin->project_pts;
			if(length > total_length)
				total_length = length;
		}
	}
	return total_length;
}

void Track::get_source_dimensions(ptstime position, int &w, int &h)
{
	for(Edit *current = edits->first; current; current = NEXT)
	{
		if(current->project_pts <= position &&
			current->end_pts() > position &&
			current->asset)
		{
			w = current->asset->width;
			h = current->asset->height;
			return;
		}
	}
}

void Track::load(FileXML *file, int track_offset, uint32_t load_flags)
{
	int result = 0;
	int current_channel = 0;
	int current_plugin = 0;

	record = file->tag.get_property("RECORD", record);
	play = file->tag.get_property("PLAY", play);
	gang = file->tag.get_property("GANG", gang);
	draw = file->tag.get_property("DRAW", draw);
	nudge = file->tag.get_property("NUDGE", nudge);
	expand_view = file->tag.get_property("EXPAND", expand_view);
	track_w = file->tag.get_property("TRACK_W", track_w);
	track_h = file->tag.get_property("TRACK_H", track_h);

	do{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/TRACK"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("TITLE"))
			{
				file->read_text_until("/TITLE", title, BCTEXTLEN);
			}
			else
			if(load_flags && automation->load(file))
			{
				;
			}
			else
			if(file->tag.title_is("EDITS"))
			{
				if(load_flags & LOAD_EDITS)
					edits->load(file, track_offset);
			}
			else
			if(file->tag.title_is("PLUGINSET"))
			{
				if(load_flags & LOAD_EDITS)
				{
					PluginSet *plugin_set = new PluginSet(edl, this);
					this->plugin_set.append(plugin_set);
					plugin_set->load(file, load_flags);
				}
				else
				if(load_flags & LOAD_AUTOMATION)
				{
					if(current_plugin < this->plugin_set.total)
					{
						PluginSet *plugin_set = this->plugin_set.values[current_plugin];
						plugin_set->load(file, load_flags);
						current_plugin++;
					}
				}
			}
		}
	}while(!result);
}

void Track::insert_asset(Asset *asset, 
		ptstime length,
		ptstime position, 
		int track_number)
{
	edits->insert_asset(asset, 
		length,
		position,
		track_number);
	edits->loaded_length += length;
}

// Insert data

// Default keyframes: We don't replace default keyframes in pasting but
// when inserting the first EDL of a load operation we need to replace
// the default keyframes.

// Plugins:  This is an arbitrary behavior
//
// 1) No plugin in source track: Paste silence into destination
// plugin sets.
// 2) Plugin in source track: plugin in source track is inserted into
// existing destination track plugin sets, new sets being added when
// necessary.

void Track::insert_track(Track *track, 
	ptstime position,
	int replace_default,
	int edit_plugins)
{
// Decide whether to copy settings based on load_mode
	if(replace_default) copy_settings(track);

	edits->insert_edits(track->edits, position);

	if(edit_plugins)
		insert_plugin_set(track, position);

	automation->insert_track(track->automation, 
		position,
		track->get_length());
	optimize();

}

// Called by insert_track
void Track::insert_plugin_set(Track *track, ptstime position)
{
// Extend plugins if no incoming plugins
	if(!track->plugin_set.total)
	{
		shift_effects(position, 
			track->get_length());
	}
	else
	for(int i = 0; i < track->plugin_set.total; i++)
	{
		if(i >= plugin_set.total)
			plugin_set.append(new PluginSet(edl, this));

		plugin_set.values[i]->insert_edits(track->plugin_set.values[i], position);
	}
}


Plugin* Track::insert_effect(const char *title, 
		SharedLocation *shared_location, 
		KeyFrame *default_keyframe,
		PluginSet *plugin_set,
		ptstime start,
		ptstime length,
		int plugin_type)
{
	if(!plugin_set)
	{
		plugin_set = new PluginSet(edl, this);
		this->plugin_set.append(plugin_set);
	}

	Plugin *plugin = 0;

// Position is identical to source plugin
	if(plugin_type == PLUGIN_SHAREDPLUGIN)
	{
		Track *source_track = tracks->get_item_number(shared_location->module);
		if(source_track)
		{
			Plugin *source_plugin = source_track->get_current_plugin(
				edl->local_session->get_selectionstart(), 
				shared_location->plugin, 
				0);

// From an attach operation
			if(source_plugin)
			{
				plugin = plugin_set->insert_plugin(title, 
					source_plugin->project_pts,
					source_plugin->length(),
					plugin_type, 
					shared_location,
					default_keyframe);
			}
			else
// From a drag operation
			{
				plugin = plugin_set->insert_plugin(title, 
					start,
					length,
					plugin_type,
					shared_location,
					default_keyframe);
			}
		}
	}
	else
	{
// This should be done in the caller
		if(EQUIV(length, 0))
		{
			if(edl->local_session->get_selectionend() > 
				edl->local_session->get_selectionstart())
			{
				start = edl->local_session->get_selectionstart();
				length = edl->local_session->get_selectionend() - start;
			}
			else
			{
				start = 0;
				length = get_length();
			}
		}

		plugin = plugin_set->insert_plugin(title, 
			start,
			length,
			plugin_type, 
			shared_location,
			default_keyframe);
	}

	expand_view = 1;
	return plugin;
}

void Track::move_plugins_up(PluginSet *plugin_set)
{
	for(int i = 0; i < this->plugin_set.total; i++)
	{
		if(this->plugin_set.values[i] == plugin_set)
		{
			if(i == 0) break;

			PluginSet *temp = this->plugin_set.values[i - 1];
			this->plugin_set.values[i - 1] = this->plugin_set.values[i];
			this->plugin_set.values[i] = temp;

			SharedLocation old_location, new_location;
			new_location.module = old_location.module = tracks->number_of(this);
			old_location.plugin = i;
			new_location.plugin = i - 1;
			tracks->change_plugins(old_location, new_location, 1);
			break;
		}
	}
}

void Track::move_plugins_down(PluginSet *plugin_set)
{
	for(int i = 0; i < this->plugin_set.total; i++)
	{
		if(this->plugin_set.values[i] == plugin_set)
		{
			if(i == this->plugin_set.total - 1) break;

			PluginSet *temp = this->plugin_set.values[i + 1];
			this->plugin_set.values[i + 1] = this->plugin_set.values[i];
			this->plugin_set.values[i] = temp;

			SharedLocation old_location, new_location;
			new_location.module = old_location.module = tracks->number_of(this);
			old_location.plugin = i;
			new_location.plugin = i + 1;
			tracks->change_plugins(old_location, new_location, 1);
			break;
		}
	}
}


void Track::remove_asset(Asset *asset)
{
	for(Edit *edit = edits->first; edit; edit = edit->next)
	{
		if(edit->asset && edit->asset == asset)
		{
			edit->asset = 0;
		}
	}
	optimize();
}

void Track::remove_pluginset(PluginSet *plugin_set)
{
	int i;
	for(i = 0; i < this->plugin_set.total; i++)
		if(plugin_set == this->plugin_set.values[i]) break;

	this->plugin_set.remove_object(plugin_set);
	for(i++ ; i < this->plugin_set.total; i++)
	{
		SharedLocation old_location, new_location;
		new_location.module = old_location.module = tracks->number_of(this);
		old_location.plugin = i;
		new_location.plugin = i - 1;
		tracks->change_plugins(old_location, new_location, 0);
	}
}

void Track::shift_keyframes(ptstime position, ptstime length)
{
	automation->paste_silence(position, position + length);
// Effect keyframes are shifted in shift_effects
}

void Track::shift_effects(ptstime position, ptstime length)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->shift_effects(position, length);
	}
}

void Track::detach_effect(Plugin *plugin)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		PluginSet *plugin_set = this->plugin_set.values[i];
		for(Plugin *dest = (Plugin*)plugin_set->first; 
			dest; 
			dest = (Plugin*)dest->next)
		{
			if(dest == plugin)
			{
				remove_pluginset(plugin_set);
				return;
			}
		}
	}
}

void Track::detach_shared_effects(int module)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		PluginSet *plugin_set = this->plugin_set.values[i];
		for(Plugin *dest = (Plugin*)plugin_set->first; 
			dest; 
			dest = (Plugin*)dest->next)
		{
			if ((dest->plugin_type == PLUGIN_SHAREDPLUGIN ||
				dest->plugin_type == PLUGIN_SHAREDMODULE)
				&&
				dest->shared_location.module == module)
			{
				this->plugin_set.remove_object_number(i);
				--i;
			}
		}
	}
}


void Track::optimize()
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		PluginSet *plugin_set = this->plugin_set.values[i];

// new definition of empty track...
		if(plugin_set->last == plugin_set->first && plugin_set->last->silence())
		{
			remove_pluginset(plugin_set);
			i--;
		}
	}
}

Plugin* Track::get_current_plugin(ptstime postime,
	int plugin_set, 
	int use_nudge)
{
	Plugin *current;

	if(use_nudge) postime += nudge;

	if(plugin_set >= this->plugin_set.total || plugin_set < 0) return 0;

	for(current = (Plugin*)this->plugin_set.values[plugin_set]->last; 
		current; 
		current = (Plugin*)PREVIOUS)
	{
		if(current->project_pts <= postime && 
			current->end_pts() > postime)
		{
			return current;
		}
	}

	return 0;
}

Plugin* Track::get_current_transition(ptstime position)
{
	Edit *current;
	Plugin *result = 0;

	for(current = edits->last; current; current = PREVIOUS)
	{
		if(current->project_pts <= position && current->end_pts() > position)
		{
			if(current->transition && position < current->project_pts + current->transition->length())
			{
				result = current->transition;
				break;
			}
		}
	}

	return result;
}

void Track::synchronize_params(Track *track)
{
	for(Edit *this_edit = edits->first, *that_edit = track->edits->first;
		this_edit && that_edit;
		this_edit = this_edit->next, that_edit = that_edit->next)
	{
		this_edit->synchronize_params(that_edit);
	}

	for(int i = 0; i < plugin_set.total && i < track->plugin_set.total; i++)
		plugin_set.values[i]->synchronize_params(track->plugin_set.values[i]);

	automation->copy_from(track->automation);
	this->nudge = track->nudge;
}


void Track::dump(int indent)
{
	printf("%*sTrack %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*sData type %d\n", indent, "", data_type);
	printf("%*sTitle %s\n", indent, "", title);
	edits->dump(indent);
	automation->dump(indent);
	printf("%*sPlugin Sets: %d\n", indent, "", plugin_set.total);

	for(int i = 0; i < plugin_set.total; i++)
		plugin_set.values[i]->dump(indent + 2);
}


Track::Track() : ListItem<Track>()
{
	y_pixel = 0;
}

// ======================================== accounting

int Track::number_of() 
{ 
	return tracks->number_of(this); 
}


// ================================================= editing

// used for copying automation alone
void Track::copy_automation(ptstime selectionstart, 
	ptstime selectionend,
	FileXML *file)
{
	file->tag.set_title("TRACK");
// Video or audio
	save_header(file);
	file->append_tag();
	file->append_newline();

	automation->copy(selectionstart, selectionend, file);

	if(edl->session->auto_conf->plugins)
	{
		file->tag.set_title("PLUGINSETS");
		file->append_tag();
		file->append_newline();
		for(int i = 0; i < plugin_set.total; i++)
		{
			plugin_set.values[i]->copy_keyframes(selectionstart,
				selectionend,
				file);
		}
		file->tag.set_title("/PLUGINSETS");
		file->append_tag();
		file->append_newline();
	}

	file->tag.set_title("/TRACK");
	file->append_tag();
	file->append_newline();
	file->append_newline();
	file->append_newline();
	file->append_newline();
}

void Track::paste_automation(ptstime selectionstart,
	ptstime total_length, 
	double frame_rate,
	int sample_rate,
	FileXML *file)
{
// Only used for pasting automation alone.
	int result;
	double scale;

	if(data_type == TRACK_AUDIO)
		scale = edl->session->sample_rate / sample_rate;
	else
		scale = edl->session->frame_rate / frame_rate;

	total_length *= scale;
	result = 0;

	while(!result)
	{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/TRACK"))
				result = 1;
			else
			if(automation->paste(selectionstart,
					total_length,
					scale,
					file,
					0))
			{
				;
			}
			else
			if(file->tag.title_is("PLUGINSETS"))
			{
				PluginSet::paste_keyframes(selectionstart, 
					total_length, 
					file,
					this);
			}
		}
	}
}

void Track::clear_automation(ptstime selectionstart,
	ptstime selectionend,
	int shift_autos,
	int default_only)
{
	automation->clear(selectionstart, selectionend, edl->session->auto_conf, 0);

	if(edl->session->auto_conf->plugins)
	{
		for(int i = 0; i < plugin_set.total; i++)
		{
			plugin_set.values[i]->clear_keyframes(selectionstart, selectionend);
		}
	}

}

void Track::straighten_automation(ptstime selectionstart, ptstime selectionend)
{
	automation->straighten(selectionstart, selectionend, edl->session->auto_conf);
}


void Track::copy(ptstime start,
	ptstime end,
	FileXML *file, 
	const char *output_path)
{
	file->tag.set_title("TRACK");
	file->tag.set_property("RECORD", record);
	file->tag.set_property("NUDGE", nudge);
	file->tag.set_property("PLAY", play);
	file->tag.set_property("GANG", gang);
	file->tag.set_property("DRAW", draw);
	file->tag.set_property("EXPAND", expand_view);
	file->tag.set_property("TRACK_W", track_w);
	file->tag.set_property("TRACK_H", track_h);
	save_header(file);
	file->append_tag();
	file->append_newline();

	file->tag.set_title("TITLE");
	file->append_tag();
	file->append_text(title);
	file->tag.set_title("/TITLE");
	file->append_tag();
	file->append_newline();

	edits->copy(start, end, file, output_path);

	AutoConf auto_conf;
	auto_conf.set_all(1);
	automation->copy(start, end, file);

	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->copy(start, end, file);
	}

	file->tag.set_title("/TRACK");
	file->append_tag();
	file->append_newline();
	file->append_newline();
	file->append_newline();
	file->append_newline();
}

void Track::copy_assets(ptstime start,
	ptstime end,
	ArrayList<Asset*> *asset_list)
{
	int i, result = 0;

	Edit *current = edits->editof(start, 0);

// Search all edits
	while(current && current->project_pts < end)
	{
// Check for duplicate assets
		if(current->asset)
		{
			for(i = 0, result = 0; i < asset_list->total; i++)
			{
				if(asset_list->values[i] == current->asset) result = 1;
			}
// append pointer to new asset
			if(!result) asset_list->append(current->asset);
		}

		current = NEXT;
	}
}

void Track::clear(ptstime start,
	ptstime end,
	int actions)
{
	if(actions & EDIT_EDITS)
		automation->clear(start, end, 0, 1);

	if(actions & EDIT_PLUGINS)
		for(int i = 0; i < plugin_set.total; i++)
		{
			plugin_set.values[i]->clear(start, end);
		}

	if(actions & EDIT_EDITS)
		edits->clear(start, end);
}

void Track::clear_handle(ptstime start,
	ptstime end,
	int actions,
	ptstime &distance)
{
	edits->clear_handle(start, end, actions, distance);
}

void Track::modify_edithandles(ptstime &oldposition,
	ptstime &newposition,
	int currentend,
	int handle_mode,
	int actions)
{
	edits->modify_handles(oldposition, newposition, handle_mode);
}

void Track::modify_pluginhandles(ptstime oldposition,
	ptstime newposition,
	int currentend, 
	int handle_mode,
	int edit_labels)
{
	for(int i = 0; i < plugin_set.total; i++)
		plugin_set.values[i]->modify_handles(oldposition, newposition,
			handle_mode);
}

void Track::paste_silence(ptstime start, ptstime end, int edit_plugins)
{
	edits->paste_silence(start, end);
	shift_keyframes(start, end - start);
	if(edit_plugins) shift_effects(start, end - start);
}

void Track::change_plugins(SharedLocation &old_location, SharedLocation &new_location, int do_swap)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		for(Plugin *plugin = (Plugin*)plugin_set.values[i]->first; 
			plugin; 
			plugin = (Plugin*)plugin->next)
		{
			if(plugin->plugin_type == PLUGIN_SHAREDPLUGIN)
			{
				if(plugin->shared_location == old_location)
					plugin->shared_location = new_location;
				else
				if(do_swap && plugin->shared_location == new_location)
					plugin->shared_location = old_location;
			}
		}
	}
}

void Track::change_modules(int old_location, int new_location, int do_swap)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		for(Plugin *plugin = (Plugin*)plugin_set.values[i]->first; 
			plugin; 
			plugin = (Plugin*)plugin->next)
		{
			if(plugin->plugin_type == PLUGIN_SHAREDPLUGIN ||
				plugin->plugin_type == PLUGIN_SHAREDMODULE)
			{
				if(plugin->shared_location.module == old_location)
					plugin->shared_location.module = new_location;
				else
				if(do_swap && plugin->shared_location.module == new_location)
					plugin->shared_location.module = old_location;
			}
		}
	}
}

int Track::playable_edit(ptstime position)
{
	int result = 0;

	for(Edit *current = edits->first; current && !result; current = NEXT)
	{
		if(current->project_pts <= position && 
			current->end_pts() > position)
		{
			if(current->transition || current->asset) result = 1;
		}
	}
	return result;
}


int Track::need_edit(Edit *current, int test_transitions)
{
	return ((test_transitions && current->transition) ||
		(!test_transitions && current->asset));
}

ptstime Track::plugin_change_duration(ptstime input_position,
	ptstime input_length)
{
	input_position += nudge;
	for(int i = 0; i < plugin_set.total; i++)
	{
		ptstime new_duration = plugin_set.values[i]->plugin_change_duration(
			input_position, 
			input_length);
		if(new_duration < input_length) input_length = new_duration;
	}
	return input_length;
}

ptstime Track::edit_change_duration(ptstime input_position, 
	ptstime input_length, 
	int test_transitions)
{
	Edit *current;
	ptstime edit_length = input_length;
	input_position += nudge;

// =================================== forward playback
// Get first edit on or before position
	for(current = edits->last; 
		current && current->project_pts > input_position;
		current = PREVIOUS);

	if(current)
	{
		if(current->end_pts() <= input_position)
		{
// Beyond last edit.
			;
		}
		else
		if(need_edit(current, test_transitions))
		{
// Over an edit of interest.
// Next edit is going to require a change.
			if(current->end_pts() - input_position < input_length)
				edit_length = current->end_pts() - input_position;
		}
		else
		{
// Over an edit that isn't of interest.
// Search for next edit of interest.
			for(current = NEXT;
				current && 
				current->project_pts < input_position + input_length &&
				!need_edit(current, test_transitions);
				current = NEXT)
				;

				if(current && 
					need_edit(current, test_transitions) &&
					current->project_pts < input_position + input_length) 
					edit_length = current->project_pts - input_position;
		}
	}
	else
	{
// Not over an edit.  Try the first edit.
		current = edits->first;
		if(current && 
			((test_transitions && current->transition) ||
			(!test_transitions && current->asset)))
			edit_length = edits->first->project_pts - input_position;
	}

	if(edit_length < input_length)
		return edit_length;
	else
		return input_length;
}

int Track::is_playable(ptstime position)
{
	return 1;
}

int Track::plugin_used(ptstime position)
{
	for(int i = 0; i < this->plugin_set.total; i++)
	{
		Plugin *current_plugin = get_current_plugin(position, 
			i, 
			0);

		if(current_plugin && 
			(current_plugin->on && 
			current_plugin->plugin_type != PLUGIN_NONE))
		{
			return 1;
		}
	}
	return 0;
}

posnum Track::to_units(ptstime position, int round)
{
	return (posnum)position;
}

ptstime Track::from_units(posnum position)
{
	return (double)position;
}

int Track::identical(ptstime sample1, ptstime sample2)
{
	if(fabs(sample1 - sample2) <= one_unit) 
		return 1;
	return 0;
}
