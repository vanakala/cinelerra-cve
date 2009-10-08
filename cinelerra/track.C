
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
#include "patch.h"
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
#include "transportque.inc"
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
	id = EDL::next_id();
}

Track::~Track()
{
	delete automation;
	delete edits;
	plugin_set.remove_all_objects();
}

int Track::create_objects()
{
	return 0;
}


int Track::copy_settings(Track *track)
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
	return 0;
}

int Track::get_id()
{
	return id;
}


int Track::load_defaults(BC_Hash *defaults)
{
	return 0;
}

void Track::equivalent_output(Track *track, double *result)
{
	if(data_type != track->data_type ||
		track_w != track->track_w ||
		track_h != track->track_h ||
		play != track->play ||
		nudge != track->nudge)
		*result = 0;

// Convert result to track units
	int64_t result2 = -1;
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
			if(result2 < 0 || current->startproject < result2)
				result2 = current->startproject;
		}
	}

// New EDL has fewer plugin sets.  Get starting plugin in old plugin set
	for(int i = plugin_sets; i < track->plugin_set.total; i++)
	{
		Plugin *current = track->plugin_set.values[i]->get_first_plugin();
		if(current)
		{
			if(result2 < 0 || current->startproject < result2)
				result2 = current->startproject;
		}
	}

// Number of plugin sets differs but somehow we didn't find the start of the
// change.  Assume 0
	if(track->plugin_set.total != plugin_set.total && result2 < 0)
		result2 = 0;

	if(result2 >= 0 && 
		(*result < 0 || from_units(result2) < *result))
		*result = from_units(result2);
}


int Track::is_synthesis(RenderEngine *renderengine, 
	int64_t position, 
	int direction)
{
	int is_synthesis = 0;
	for(int i = 0; i < plugin_set.total; i++)
	{
		Plugin *plugin = get_current_plugin(position,
			i,
			direction,
			0,
			0);
		if(plugin)
		{
// Assume data from a shared track is synthesized
			if(plugin->plugin_type == PLUGIN_SHAREDMODULE) 
				is_synthesis = 1;
			else
				is_synthesis = plugin->is_synthesis(renderengine, 
					position, 
					direction);
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
printf("Track::operator= 1\n");
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

double Track::get_length()
{
	double total_length = 0;
	double length = 0;

// Test edits
	int64_t unit_end;
	unit_end = edits->last->startproject;
	if (edits->last->transition)
		unit_end += edits->last->transition->length + 1; // add one so transition is finished...
	length = from_units(unit_end);
	if(length > total_length) total_length = length;

// Test plugins
	for(int i = 0; i < plugin_set.total; i++)
	{
		length = from_units(plugin_set.values[i]->last->startproject);
		if(length > total_length) total_length = length;
	}

// Test keyframes
	length = from_units(automation->get_length());
	if(length > total_length) total_length = length;
	

	return total_length;
}



void Track::get_source_dimensions(double position, int &w, int &h)
{
	int64_t native_position = to_units(position, 0);
	for(Edit *current = edits->first; current; current = NEXT)
	{
		if(current->startproject <= native_position &&
			current->startproject + current->length > native_position &&
			current->asset)
		{
			w = current->asset->width;
			h = current->asset->height;
			return;
		}
	}
}


int64_t Track::horizontal_span()
{
	return (int64_t)(get_length() * 
		edl->session->sample_rate / 
		edl->local_session->zoom_sample + 
		0.5);
}


int Track::load(FileXML *file, int track_offset, uint32_t load_flags)
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

	load_header(file, load_flags);

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
			if(load_flags && automation->load(file)
			/* strstr(file->tag.get_title(), "AUTOS") */)
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
			else
				load_derived(file, load_flags);
		}
	}while(!result);



	return 0;
}

void Track::insert_asset(Asset *asset, 
		double length, 
		double position, 
		int track_number)
{
//printf("Track::insert_asset %f\n", length);
	edits->insert_asset(asset, 
		to_units(length, 1), 
		to_units(position, 0), 
		track_number);
	edits->loaded_length += to_units(length, 1);
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
	double position, 
	int replace_default,
	int edit_plugins)
{

// Decide whether to copy settings based on load_mode
	if(replace_default) copy_settings(track);

	edits->insert_edits(track->edits, to_units(position, 0));

	if(edit_plugins)
		insert_plugin_set(track, position);

	automation->insert_track(track->automation, 
		to_units(position, 0), 
		to_units(track->get_length(), 1),
		replace_default);

	optimize();

}

// Called by insert_track
void Track::insert_plugin_set(Track *track, double position)
{
// Extend plugins if no incoming plugins
	if(!track->plugin_set.total)
	{
		shift_effects(position, 
			track->get_length(), 
			1);
	}
	else
	for(int i = 0; i < track->plugin_set.total; i++)
	{
		if(i >= plugin_set.total)
			plugin_set.append(new PluginSet(edl, this));

		plugin_set.values[i]->insert_edits(track->plugin_set.values[i], 
			to_units(position, 0));
	}
}


Plugin* Track::insert_effect(char *title, 
		SharedLocation *shared_location, 
		KeyFrame *default_keyframe,
		PluginSet *plugin_set,
		double start,
		double length,
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
				PLAY_FORWARD, 
				1,
				0);

// From an attach operation
			if(source_plugin)
			{
				plugin = plugin_set->insert_plugin(title, 
					source_plugin->startproject, 
					source_plugin->length,
					plugin_type, 
					shared_location,
					default_keyframe,
					1);
			}
			else
// From a drag operation
			{
				plugin = plugin_set->insert_plugin(title, 
					to_units(start, 0), 
					to_units(length, 1),
					plugin_type, 
					shared_location,
					default_keyframe,
					1);
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
//printf("Track::insert_effect %f %f %d %d\n", start, length, to_units(start, 0), 
//			to_units(length, 0));

		plugin = plugin_set->insert_plugin(title, 
			to_units(start, 0), 
			to_units(length, 1),
			plugin_type, 
			shared_location,
			default_keyframe,
			1);
	}
//printf("Track::insert_effect 2 %f %f\n", start, length);

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

void Track::shift_keyframes(double position, double length, int convert_units)
{
	if(convert_units)
	{
		position = to_units(position, 0);
		length = to_units(length, 1);
	}

	automation->paste_silence(Units::to_int64(position), 
		Units::to_int64(position + length));
// Effect keyframes are shifted in shift_effects
}

void Track::shift_effects(double position, double length, int convert_units)
{
	if(convert_units)
	{
		position = to_units(position, 0);
		length = to_units(length, 1);
	}

	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->shift_effects(Units::to_int64(position), Units::to_int64(length));
	}
}

void Track::detach_effect(Plugin *plugin)
{
//printf("Track::detach_effect 1\n");		
	for(int i = 0; i < plugin_set.total; i++)
	{
		PluginSet *plugin_set = this->plugin_set.values[i];
		for(Plugin *dest = (Plugin*)plugin_set->first; 
			dest; 
			dest = (Plugin*)dest->next)
		{
			if(dest == plugin)
			{
				int64_t start = plugin->startproject;
				int64_t end = plugin->startproject + plugin->length;

				plugin_set->clear(start, end);
				plugin_set->paste_silence(start, end);

// Delete 0 length pluginsets	
				plugin_set->optimize();
//printf("Track::detach_effect 2 %d\n", plugin_set->length());
				if(plugin_set->last == plugin_set->first && plugin_set->last->silence())
					remove_pluginset(plugin_set);
				return;
			}
		}
	}
}

void Track::resample(double old_rate, double new_rate)
{
	edits->resample(old_rate, new_rate);
	automation->resample(old_rate, new_rate);
	for(int i = 0; i < plugin_set.total; i++)
		plugin_set.values[i]->resample(old_rate, new_rate);
	nudge = (int64_t)(nudge * new_rate / old_rate);
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
				int64_t start = dest->startproject;
				int64_t end = dest->startproject + dest->length;

				plugin_set->clear(start, end);
				plugin_set->paste_silence(start, end);

// Delete 0 length pluginsets	
				plugin_set->optimize();
				if(plugin_set->last == plugin_set->first && plugin_set->last->silence())
				{
					this->plugin_set.remove_object_number(i);
					--i;
				}
			}
		}
	}
}


void Track::optimize()
{
	edits->optimize();
	for(int i = 0; i < plugin_set.total; i++)
	{
		PluginSet *plugin_set = this->plugin_set.values[i];
		plugin_set->optimize();
//printf("Track::optimize %d\n", plugin_set.values[i]->total());
// new definition of empty track...
		if(plugin_set->last == plugin_set->first && plugin_set->last->silence())
		{
			remove_pluginset(plugin_set);
			i--;
		}
	}
}

Plugin* Track::get_current_plugin(double position, 
	int plugin_set, 
	int direction, 
	int convert_units,
	int use_nudge)
{
	Plugin *current;
	if(convert_units) position = to_units(position, 0);
	if(use_nudge) position += nudge;
	
	if(plugin_set >= this->plugin_set.total || plugin_set < 0) return 0;

//printf("Track::get_current_plugin 1 %d %d %d\n", position, this->plugin_set.total, direction);
	if(direction == PLAY_FORWARD)
	{
		for(current = (Plugin*)this->plugin_set.values[plugin_set]->last; 
			current; 
			current = (Plugin*)PREVIOUS)
		{
// printf("Track::get_current_plugin 2 %d %ld %ld\n", 
// current->startproject, 
// current->startproject + current->length, 
// position);
			if(current->startproject <= position && 
				current->startproject + current->length > position)
			{
				return current;
			}
		}
	}
	else
	if(direction == PLAY_REVERSE)
	{
		for(current = (Plugin*)this->plugin_set.values[plugin_set]->first; 
			current; 
			current = (Plugin*)NEXT)
		{
			if(current->startproject < position && 
				current->startproject + current->length >= position)
			{
				return current;
			}
		}
	}

	return 0;
}

Plugin* Track::get_current_transition(double position, 
	int direction, 
	int convert_units,
	int use_nudge)
{
	Edit *current;
	Plugin *result = 0;
	if(convert_units) position = to_units(position, 0);
	if(use_nudge) position += nudge;

	if(direction == PLAY_FORWARD)
	{
		for(current = edits->last; current; current = PREVIOUS)
		{
			if(current->startproject <= position && current->startproject + current->length > position)
			{
//printf("Track::get_current_transition %p\n", current->transition);
				if(current->transition && position < current->startproject + current->transition->length)
				{
					result = current->transition;
					break;
				}
			}
		}
	}
	else
	if(direction == PLAY_REVERSE)
	{
		for(current = edits->first; current; current = NEXT)
		{
			if(current->startproject < position && current->startproject + current->length >= position)
			{
				if(current->transition && position <= current->startproject + current->transition->length)
				{
					result = current->transition;
					break;
				}
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





int Track::dump()
{
	printf("   Data type %d\n", data_type);
	printf("   Title %s\n", title);
	printf("   Edits:\n");
	for(Edit* current = edits->first; current; current = NEXT)
	{
		current->dump();
	}
	automation->dump();
	printf("   Plugin Sets: %d\n", plugin_set.total);

	for(int i = 0; i < plugin_set.total; i++)
		plugin_set.values[i]->dump();
//printf("Track::dump 2\n");
	return 0;
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

int Track::select_auto(AutoConf *auto_conf, int cursor_x, int cursor_y)
{
	return 0;
}

int Track::move_auto(AutoConf *auto_conf, int cursor_x, int cursor_y, int shift_down)
{
	return 0;
}

int Track::release_auto()
{
	return 0;
}

// used for copying automation alone
int Track::copy_automation(double selectionstart, 
	double selectionend, 
	FileXML *file,
	int default_only,
	int autos_only)
{
	int64_t start = to_units(selectionstart, 0);
	int64_t end = to_units(selectionend, 1);

	file->tag.set_title("TRACK");
// Video or audio
    save_header(file);
	file->append_tag();
	file->append_newline();

	automation->copy(start, end, file, default_only, autos_only);

	if(edl->session->auto_conf->plugins)
	{
		file->tag.set_title("PLUGINSETS");
		file->append_tag();
		file->append_newline();
		for(int i = 0; i < plugin_set.total; i++)
		{
		
			plugin_set.values[i]->copy_keyframes(start, 
				end, 
				file, 
				default_only,
				autos_only);
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

	return 0;
}

int Track::paste_automation(double selectionstart, 
	double total_length, 
	double frame_rate,
	int64_t sample_rate,
	FileXML *file,
	int default_only)
{
// Only used for pasting automation alone.
	int64_t start;
	int64_t length;
	int result;
	double scale;

	if(data_type == TRACK_AUDIO)
		scale = edl->session->sample_rate / sample_rate;
	else
		scale = edl->session->frame_rate / frame_rate;

	total_length *= scale;
	start = to_units(selectionstart, 0);
	length = to_units(total_length, 1);
	result = 0;
//printf("Track::paste_automation 1\n");

	while(!result)
	{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/TRACK"))
				result = 1;
			else
			if(automation->paste(start, 
					length, 
					scale,
					file,
					default_only,
					0))
			/* strstr(file->tag.get_title(), "AUTOS")) */
			{
				;
			}
			else
			if(file->tag.title_is("PLUGINSETS"))
			{
//printf("Track::paste_automation 2 %d\n", current_pluginset);
				PluginSet::paste_keyframes(start, 
					length, 
					file,
					default_only,
					this);
			}
		}
	}
//printf("Track::paste_automation 3\n");
	

	return 0;
}

void Track::clear_automation(double selectionstart, 
	double selectionend, 
	int shift_autos,
	int default_only)
{
	int64_t start = to_units(selectionstart, 0);
	int64_t end = to_units(selectionend, 1);

	automation->clear(start, end, edl->session->auto_conf, 0);

	if(edl->session->auto_conf->plugins)
	{
		for(int i = 0; i < plugin_set.total; i++)
		{
			plugin_set.values[i]->clear_keyframes(start, end);
		}
	}

}

void Track::straighten_automation(double selectionstart, 
	double selectionend)
{
	int64_t start = to_units(selectionstart, 0);
	int64_t end = to_units(selectionend, 1);

	automation->straighten(start, end, edl->session->auto_conf);
}




int Track::copy(double start, 
	double end, 
	FileXML *file, 
	char *output_path)
{
// Use a copy of the selection in converted units
// So copy_automation doesn't reconvert.
	int64_t start_unit = to_units(start, 0);
	int64_t end_unit = to_units(end, 1);




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
	save_derived(file);

	file->tag.set_title("TITLE");
	file->append_tag();
	file->append_text(title);
	file->tag.set_title("/TITLE");
	file->append_tag();
	file->append_newline();

// 	if(data_type == TRACK_AUDIO)
// 		file->tag.set_property("TYPE", "AUDIO");
// 	else
// 		file->tag.set_property("TYPE", "VIDEO");
// 
// 	file->append_tag();
// 	file->append_newline();

	edits->copy(start_unit, end_unit, file, output_path);

	AutoConf auto_conf;
	auto_conf.set_all(1);
	automation->copy(start_unit, end_unit, file, 0, 0);


	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->copy(start_unit, end_unit, file);
	}

	copy_derived(start_unit, end_unit, file);

	file->tag.set_title("/TRACK");
	file->append_tag();
	file->append_newline();
	file->append_newline();
	file->append_newline();
	file->append_newline();

	return 0;
}

int Track::copy_assets(double start, 
	double end, 
	ArrayList<Asset*> *asset_list)
{
	int i, result = 0;

	start = to_units(start, 0);
	end = to_units(end, 1);

	Edit *current = edits->editof((int64_t)start, PLAY_FORWARD, 0);

// Search all edits
	while(current && current->startproject < end)
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

	return 0;
}





int Track::clear(double start, 
	double end, 
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	int convert_units,
	Edits *trim_edits)
{
// Edits::move_auto calls this routine after the units are converted to the track
// format.
//printf("Track::clear 1 %d %d %d\n", edit_edits, edit_labels, edit_plugins);
	if(convert_units)
	{
		start = to_units(start, 0);
		end = to_units(end, 1);
	}

	if(edit_edits)
		automation->clear((int64_t)start, (int64_t)end, 0, 1);

	if(edit_plugins)
		for(int i = 0; i < plugin_set.total; i++)
		{
			if(!trim_edits || trim_edits == (Edits*)plugin_set.values[i])
				plugin_set.values[i]->clear((int64_t)start, (int64_t)end);
		}

	if(edit_edits)
		edits->clear((int64_t)start, (int64_t)end);
	return 0;
}

int Track::clear_handle(double start, 
	double end, 
	int clear_labels,
	int clear_plugins, 
	double &distance)
{
	edits->clear_handle(start, end, clear_plugins, distance);
}

int Track::popup_transition(int cursor_x, int cursor_y)
{
	return 0;
}



int Track::modify_edithandles(double oldposition, 
	double newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels,
	int edit_plugins)
{
	edits->modify_handles(oldposition, 
		newposition, 
		currentend,
		handle_mode,
		1,
		edit_labels,
		edit_plugins,
		0);


	return 0;
}

int Track::modify_pluginhandles(double oldposition, 
	double newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels,
	Edits *trim_edits)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		if(!trim_edits || trim_edits == (Edits*)plugin_set.values[i])
			plugin_set.values[i]->modify_handles(oldposition, 
				newposition, 
				currentend, 
				handle_mode,
// Don't allow plugin tweeks to affect edits.
				0,
				edit_labels,
				1,
				trim_edits);
	}
	return 0;
}


int Track::paste_silence(double start, double end, int edit_plugins)
{
	start = to_units(start, 0);
	end = to_units(end, 1);

	edits->paste_silence((int64_t)start, (int64_t)end);
	shift_keyframes(start, end - start, 0);
	if(edit_plugins) shift_effects(start, end - start, 0);

	edits->optimize();
	return 0;
}

int Track::select_edit(int cursor_x, 
	int cursor_y, 
	double &new_start, 
	double &new_end)
{
	return 0;
}

int Track::scale_time(float rate_scale, int scale_edits, int scale_autos, int64_t start, int64_t end)
{
	return 0;
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


int Track::playable_edit(int64_t position, int direction)
{
	int result = 0;
	if(direction == PLAY_REVERSE) position--;
	for(Edit *current = edits->first; current && !result; current = NEXT)
	{
		if(current->startproject <= position && 
			current->startproject + current->length > position)
		{
//printf("Track::playable_edit %p %p\n", current->transition, current->asset);
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

int64_t Track::plugin_change_duration(int64_t input_position,
	int64_t input_length,
	int reverse,
	int use_nudge)
{
	if(use_nudge) input_position += nudge;
	for(int i = 0; i < plugin_set.total; i++)
	{
		int64_t new_duration = plugin_set.values[i]->plugin_change_duration(
			input_position, 
			input_length, 
			reverse);
		if(new_duration < input_length) input_length = new_duration;
	}
	return input_length;
}

int64_t Track::edit_change_duration(int64_t input_position, 
	int64_t input_length, 
	int reverse, 
	int test_transitions,
	int use_nudge)
{
	Edit *current;
	int64_t edit_length = input_length;
	if(use_nudge) input_position += nudge;

	if(reverse)
	{
// ================================= Reverse playback
// Get first edit on or after position
		for(current = edits->first; 
			current && current->startproject + current->length <= input_position;
			current = NEXT)
			;

		if(current)
		{
			if(current->startproject > input_position)
			{
// Before first edit
				;
			}
			else
			if(need_edit(current, test_transitions))
			{
// Over an edit of interest.
				if(input_position - current->startproject < input_length)
					edit_length = input_position - current->startproject + 1;
			}
			else
			{
// Over an edit that isn't of interest.
// Search for next edit of interest.
				for(current = PREVIOUS ; 
					current && 
					current->startproject + current->length > input_position - input_length &&
					!need_edit(current, test_transitions);
					current = PREVIOUS)
					;

					if(current && 
						need_edit(current, test_transitions) &&
						current->startproject + current->length > input_position - input_length)
                    	edit_length = input_position - current->startproject - current->length + 1;
			}
		}
		else
		{
// Not over an edit.  Try the last edit.
			current = edits->last;
			if(current && 
				((test_transitions && current->transition) ||
				(!test_transitions && current->asset)))
				edit_length = input_position - edits->length() + 1;
		}
	}
	else
	{
// =================================== forward playback
// Get first edit on or before position
		for(current = edits->last; 
			current && current->startproject > input_position;
			current = PREVIOUS)
			;

		if(current)
		{
			if(current->startproject + current->length <= input_position)
			{
// Beyond last edit.
				;
			}
			else
			if(need_edit(current, test_transitions))
			{
// Over an edit of interest.
// Next edit is going to require a change.
				if(current->length + current->startproject - input_position < input_length)
					edit_length = current->startproject + current->length - input_position;
			}
			else
			{
// Over an edit that isn't of interest.
// Search for next edit of interest.
				for(current = NEXT ; 
					current && 
					current->startproject < input_position + input_length &&
					!need_edit(current, test_transitions);
					current = NEXT)
					;

					if(current && 
						need_edit(current, test_transitions) &&
						current->startproject < input_position + input_length) 
						edit_length = current->startproject - input_position;
			}
		}
		else
		{
// Not over an edit.  Try the first edit.
			current = edits->first;
			if(current && 
				((test_transitions && current->transition) ||
				(!test_transitions && current->asset)))
				edit_length = edits->first->startproject - input_position;
		}
	}

	if(edit_length < input_length)
		return edit_length;
	else
		return input_length;
}

int Track::purge_asset(Asset *asset)
{
	return 0;
}

int Track::asset_used(Asset *asset)
{
	Edit* current_edit;
	int result = 0;

	for(current_edit = edits->first; current_edit; current_edit = current_edit->next)
	{
		if(current_edit->asset == asset)
		{
			result++;
		}
	}
	return result;
}

int Track::is_playable(int64_t position, int direction)
{
	return 1;
}


int Track::plugin_used(int64_t position, int64_t direction)
{
//printf("Track::plugin_used 1 %d\n", this->plugin_set.total);
	for(int i = 0; i < this->plugin_set.total; i++)
	{
		Plugin *current_plugin = get_current_plugin(position, 
			i, 
			direction, 
			0,
			0);

//printf("Track::plugin_used 2 %p\n", current_plugin);
		if(current_plugin && 
			(current_plugin->on && 
			current_plugin->plugin_type != PLUGIN_NONE))
		{
			return 1;
		}
	}
//printf("Track::plugin_used 3 %p\n", current_plugin);
	return 0;
}

// Audio is always rendered through VConsole
int Track::direct_copy_possible(int64_t start, int direction, int use_nudge)
{
	return 1;
}

int64_t Track::to_units(double position, int round)
{
	return (int64_t)position;
}

double Track::to_doubleunits(double position)
{
	return position;
}

double Track::from_units(int64_t position)
{
	return (double)position;
}
