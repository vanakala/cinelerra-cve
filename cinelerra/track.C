
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
#include "automation.h"
#include "bcsignals.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keyframe.h"
#include "keyframes.h"
#include "localsession.h"
#include "plugin.h"
#include "plugindb.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
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
	master = 0;
	track_w = edlsession->output_w;
	track_h = edlsession->output_h;
	one_unit = (ptstime) 1 / 48000;
	edits = new Edits(edl, this);
	id = EDL::next_id();
}

Track::~Track()
{
	delete automation;
	delete edits;
	plugins.remove_all_objects();
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

	int num_plugins = MIN(plugins.total, track->plugins.total);
// Test existing plugin sets
	for(int i = 0; i < num_plugins; i++)
	{
		plugins.values[i]->equivalent_output(
			track->plugins.values[i],
			&result2);
	}

// New EDL has more plugin sets.  Get starting plugin in new plugin sets
	for(int i = num_plugins; i < plugins.total; i++)
	{
		Plugin *current = plugins.values[i];
		if(current)
		{
			if(result2 < 0 || current->get_pts() < result2)
				result2 = current->get_pts();
		}
	}

// New EDL has fewer plugin sets.  Get starting plugin in old plugin set
	for(int i = num_plugins; i < track->plugins.total; i++)
	{
		Plugin *current = track->plugins.values[i];
		if(current)
		{
			if(result2 < 0 || current->get_pts() < result2)
				result2 = current->get_pts();
		}
	}

// Number of plugin sets differs but somehow we didn't find the start of the
// change.  Assume 0
	if(track->plugins.total != plugins.total && result2 < 0)
		result2 = 0;

	if(result2 >= 0 && (*result < 0 || result2 < *result))
		*result = result2;
}


int Track::is_synthesis(ptstime position)
{
	int is_synthesis = 0;
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *plugin = get_current_plugin(position, i, 0);

		if(plugin)
		{
// Assume data from a shared track is synthesized
			if(plugin->plugin_type == PLUGIN_SHAREDMODULE) 
				is_synthesis = 1;
			else
				is_synthesis = plugin->is_synthesis(position);
			if(is_synthesis)
				break;
		}
	}
	return is_synthesis;
}

void Track::copy_from(Track *track)
{
	copy_settings(track);
	edits->copy_from(track->edits);
	plugins.remove_all_objects();

	for(int i = 0; i < track->plugins.total; i++)
	{
		Plugin *new_plugin = plugins.append(new Plugin(edl, this, 0));
		new_plugin->copy_from(track->plugins.values[i]);
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
			plugins.total *
			theme->get_image("plugin_bg_data")->get_h();
	else
		result = edl->local_session->zoom_track;

	if(edlsession->show_titles)
		result += theme->get_image("title_bg_data")->get_h();

	return result;
}

ptstime Track::get_length()
{
	ptstime total_length = 0;
	ptstime length = 0;
	Plugin *plugin;

// Test edits
	if(edits->last)
		total_length = edits->last->get_pts();

// Test plugins
	for(int i = 0; i < plugins.total; i++)
	{
		plugin = plugins.values[i];
		length = plugin->end_pts();
		if(length > total_length)
			total_length = length;
	}
	return total_length;
}

void Track::get_source_dimensions(ptstime position, int &w, int &h)
{
	for(Edit *current = edits->first; current; current = NEXT)
	{
		if(current->get_pts() <= position &&
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
	ptstime startproject = 0;
	int current_plugin = 0;
	Plugin *plugin = 0;

	record = file->tag.get_property("RECORD", record);
	play = file->tag.get_property("PLAY", play);
	gang = file->tag.get_property("GANG", gang);
	draw = file->tag.get_property("DRAW", draw);
	nudge = file->tag.get_property("NUDGE", nudge);
	master = file->tag.get_property("MASTER", master);
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
					while(!file->read_tag())
					{
						if(file->tag.title_is("PLUGIN"))
						{
							posnum length_units = file->tag.get_property("LENGTH", (int64_t)0);
							ptstime length = 0;
							if(length_units)
								length = from_units(length_units);
							length = file->tag.get_property("DURATION", length);
							startproject = file->tag.get_property("POSTIME", startproject);
							int type = file->tag.get_property("TYPE", PLUGIN_NONE);

							if(type != PLUGIN_NONE)
							{
								PluginServer *server = 0;

								if(type == PLUGIN_STANDALONE)
								{
									char string[BCTEXTLEN];
									string[0] = 0;
									file->tag.get_property("TITLE", string);
									server = plugindb.get_pluginserver(string, data_type);
								}
								plugin = new Plugin(edl, this, server);
								plugins.append(plugin);
								plugin->plugin_type = type;
								plugin->set_pts(startproject);
								plugin->set_length(length);
								plugin->on = !file->tag.get_property("OFF", 0);
								plugin->load(file);
								// got length - ignore next plugin pts
								if(length > 0)
									plugin = 0;
							}
							else if(plugin)
							{
								plugin->set_end(startproject);
								plugin = 0;
							}
						}
						if(file->tag.title_is("/PLUGINSET"))
							break;
					}
				}
				else
				if(load_flags & LOAD_AUTOMATION)
				{
					if(current_plugin < plugins.total)
					{
						Plugin *plugin = plugins.values[current_plugin];
						plugin->load(file);
						current_plugin++;
					}
				}
			}
		}
	}while(!result);
}

void Track::init_shared_pointers()
{
	for(int i = 0; i < plugins.total; i++)
		plugins.values[0]->init_shared_pointers();
}

void Track::insert_asset(Asset *asset,
		ptstime length,
		ptstime position,
		int track_number,
		int overwrite)
{
	edits->insert_asset(asset, 
		length,
		position,
		track_number, overwrite);

	if(!overwrite)
	{
		if(edlsession->plugins_follow_edits)
			shift_effects(position, length);
		automation->paste_silence(position, position + length);
	}
}

// Insert data

// Plugins:  This is an arbitrary behavior
//
// 1) No plugin in source track: Paste silence into destination
// plugin sets.
// 2) Plugin in source track: plugin in source track is inserted into
// existing destination track plugin sets, new sets being added when
// necessary.

void Track::insert_track(Track *track, 
	ptstime length,
	ptstime position,
	int overwrite)
{
	edits->insert_edits(track->edits, position, length, overwrite);

	if(edlsession->plugins_follow_edits)
		insert_plugin(track, position, length, overwrite);

	automation->insert_track(track->automation, 
		position,
		length);
}

// Called by insert_track
void Track::insert_plugin(Track *track, ptstime position,
	ptstime duration, int overwrite)
{
	Plugin *plugin, *new_plugin;
	ptstime end, new_start, new_length;

	if(duration < 0)
		duration = track->get_length();

	end = position + duration;

	if(overwrite)
		clear_plugins(position, end);
	else
		shift_effects(position, duration);

	for(int i = 0; i < track->plugins.total; i++)
	{
		plugin = track->plugins.values[i];

		new_start = position + plugin->get_pts();
		if(new_start < end)
		{
			plugins.append(new_plugin = new Plugin(edl, this, 0));
			new_plugin->copy_from(plugin);
			new_plugin->set_pts(new_start);
			new_length = plugin->get_length();
			if(new_start + new_length > end)
				new_length = end - new_start;
			new_plugin->set_length(new_length);
		}
	}
}

Plugin* Track::insert_effect(PluginServer *server,
	ptstime start,
	ptstime length,
	int plugin_type,
	Plugin *shared_plugin,
	Track *shared_track)
{
	if(length < EPSILON)
		return 0;

	Plugin *plugin = new Plugin(edl, this, server);

	plugins.append(plugin);
	plugin->plugin_type = plugin_type;
	plugin->shared_track = shared_track;
	plugin->shared_plugin = shared_plugin;

// Position is identical to source plugin
	if(plugin_type == PLUGIN_SHAREDPLUGIN)
	{
// From an attach operation
		if(shared_plugin)
		{
			plugin->set_pts(shared_plugin->get_pts());
			plugin->set_length(shared_plugin->get_length());
		}
		else
// From a drag operation
		{
			plugin->set_pts(start);
			plugin->set_length(length);
		}
	}
	else
	{
		plugin->set_pts(start);
		plugin->set_length(length);
	}

	expand_view = 1;
	return plugin;
}

void Track::xchg_plugins(Plugin *plugin1, Plugin *plugin2)
{
	int si1, si2;

	if(plugin1 == plugin2)
		return;

	si1 = si2 = -1;

	for(int i = 0; i < plugins.total; i++)
	{
		if(plugins.values[i] == plugin1)
			si1 = i;
		if(plugins.values[i] == plugin2)
			si2 = i;
	}

	if(si1 >= 0 && si2 >= 0)
	{
		Plugin *temp = plugins.values[si1];
		plugins.values[si1] = plugins.values[si2];
		plugins.values[si2] = temp;
	}
}

void Track::move_plugin_up(Plugin *plugin)
{
	for(int i = 0; i < this->plugins.total; i++)
	{
		if(this->plugins.values[i] == plugin)
		{
			if(i == 0) break;

			Plugin *temp = plugins.values[i - 1];
			plugins.values[i - 1] = plugins.values[i];
			plugins.values[i] = temp;
			break;
		}
	}
}

void Track::move_plugin_down(Plugin *plugin)
{
	for(int i = 0; i < plugins.total; i++)
	{
		if(plugins.values[i] == plugin)
		{
			if(i == plugins.total - 1)
				break;

			Plugin *temp = plugins.values[i + 1];
			plugins.values[i + 1] = plugins.values[i];
			plugins.values[i] = temp;
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
}

void Track::remove_plugin(Plugin *plugin)
{
	int i;

	for(i = 0; i < plugins.total; i++)
	{
		if(plugin == plugins.values[i])
		{
			plugins.remove_object(plugin);
			break;
		}
	}
	tracks->detach_shared_effects(plugin, 0);
}

void Track::shift_keyframes(ptstime position, ptstime length)
{
	automation->paste_silence(position, position + length);
// Effect keyframes are shifted in shift_effects
}

void Track::shift_effects(ptstime position, ptstime length)
{
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *plugin = plugins.values[i];

		if(plugin->get_pts() >= position)
			plugin->shift(length);
	}
}

void Track::detach_shared_effects(Plugin *plugin, Track *track)
{
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *dest = plugins.values[i];

		if(plugin && dest->shared_plugin == plugin)
			dest->shared_plugin = 0;
		if(track && dest->shared_track == track)
			dest->shared_track = 0;
		if((dest->plugin_type == PLUGIN_SHAREDPLUGIN &&
				!dest->shared_plugin) ||
			(dest->plugin_type == PLUGIN_SHAREDMODULE &&
				!dest->shared_track))
		{
			plugins.remove_object_number(i);
			--i;
		}
	}
}

Plugin* Track::get_current_plugin(ptstime postime,
	int num,
	int use_nudge)
{
	if(use_nudge) postime += nudge;

	if(num >= 0 && num < plugins.total)
	{
		Plugin *current = plugins.values[num];

		if(current->get_pts() <= postime &&
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
		if(current->get_pts() <= position && current->end_pts() > position)
		{
			if(current->transition && position <
				current->get_pts() + current->transition->get_length())
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

	for(int i = 0; i < plugins.total && i < track->plugins.total; i++)
		plugins.values[i]->synchronize_params(plugins.values[i]);

	automation->copy_from(track->automation);
	this->nudge = track->nudge;
}


void Track::dump(int indent)
{
	const char *tp;

	printf("%*sTrack %p dump:\n", indent, "", this);
	indent += 2;
	switch(data_type)
	{
	case TRACK_AUDIO:
		tp = "Audio";
		break;
	case TRACK_VIDEO:
		tp = "Video";
		break;
	default:
		tp = "Unknown";
		break;
	}
	printf("%*sType: %s %c-%c-%c-%c-%c nudge %.2f\n", indent, "",
		tp, master ? 'M' : 'm', play ? 'P' : 'p',
		record ? 'A' : 'a', gang ? 'G' : 'g', draw ? 'D' : 'd', nudge);
	if(data_type == TRACK_VIDEO)
		printf("%*sTitle: '%s' [%d,%d]\n", indent, "", title, track_w, track_h);
	else
		printf("%*sTitle: '%s'\n", indent, "", title);
	edits->dump(indent);
	automation->dump(indent);
	printf("%*sPlugins: %d\n", indent, "", plugins.total);

	for(int i = 0; i < plugins.total; i++)
		plugins.values[i]->dump(indent + 2);
}

// ======================================== accounting

int Track::number_of() 
{ 
	return tracks->number_of(this);
}

// ================================================= editing

// used for copying automation alone
void Track::automation_xml(FileXML *file)
{
	file->tag.set_title("TRACK");
// Video or audio
	save_header(file);
	file->append_tag();
	file->append_newline();

	automation->save_xml(file);

	if(edlsession->auto_conf->plugins)
	{
		file->tag.set_title("PLUGINSETS");
		file->append_tag();
		file->append_newline();

		for(int i = 0; i < plugins.total; i++)
		{
// For historical reasons we have pluginsets
			file->tag.set_title("PLUGINSET");
			file->append_tag();
			file->append_newline();
			plugins.values[i]->save_xml(file);
			file->tag.set_title("/PLUGINSET");
			file->append_tag();
			file->append_newline();
		}
		file->tag.set_title("/PLUGINSETS");
		file->append_tag();
		file->append_newline();
	}

	file->tag.set_title("/TRACK");
	file->append_tag();
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
		scale = edlsession->sample_rate / sample_rate;
	else
		scale = edlsession->frame_rate / frame_rate;

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
				paste_keyframes(selectionstart,
					total_length, 
					file);
			}
		}
	}
}

void Track::paste_keyframes(ptstime start, ptstime length,
	FileXML *file)
{
	char data[MESSAGESIZE];

	int result = 0;

	Plugin *target_plugin = 0;
	Plugin *second_choice_plugin = 0;
	ArrayList<Plugin*> unused_plugins;
// get all available target plugins, we will be removing them one by one
//    when we will paste into them
	for(int i = 0; i < plugins.total; i++)
		unused_plugins.append(plugins.values[i]);

	while(!result)
	{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/PLUGINSETS"))
				result = 1;
			else
			if(file->tag.title_is("PLUGINSET"))
			{
				target_plugin = 0;
			}
			else
			if(file->tag.title_is("KEYFRAME"))
			{
				ptstime position = file->tag.get_property("POSTIME", 0);
				position += start;
				file->read_text_until("/KEYFRAME",
					data, MESSAGESIZE);

				if(!target_plugin && data[0])
				{
// now try to find the target
					int name_len = strchr(data, ' ') - data + 1;

					second_choice_plugin = 0;
					for(int i = 0; i < unused_plugins.total; i++)
					{
						Plugin *current = unused_plugins.values[i];

						if(position >= current->get_pts() &&
							position <= current->end_pts() &&
							!strncmp(((KeyFrame *)current->keyframes->first)->data, data, name_len))
						{
							target_plugin = current;
							break;
						}

						if(position >= current->get_pts() &&
							!strncmp(((KeyFrame *)current->keyframes->first)->data, data, name_len))
						{
							second_choice_plugin = current;
							break;
						}
					}
				}

				if(!target_plugin)
					target_plugin = second_choice_plugin;

				if(target_plugin)
				{
					KeyFrame *keyframe;
					keyframe = (KeyFrame*)target_plugin->keyframes->insert_auto(position);
					strcpy(keyframe->data, data);
					keyframe->pos_time = position;
					unused_plugins.remove(target_plugin);
				}
			}
		}
	}
}

void Track::clear_automation(ptstime selectionstart,
	ptstime selectionend,
	int shift_autos,
	int default_only)
{
	automation->clear(selectionstart, selectionend, edlsession->auto_conf, 0);

	if(edlsession->auto_conf->plugins)
	{
		for(int i = 0; i < plugins.total; i++)
		{
			plugins.values[i]->keyframes->clear(selectionstart,
				selectionend, 0);
		}
	}

}

void Track::straighten_automation(ptstime selectionstart, ptstime selectionend)
{
	automation->straighten(selectionstart, selectionend, edlsession->auto_conf);
}


void Track::save_xml(FileXML *file, const char *output_path)
{
	file->tag.set_title("TRACK");
	file->tag.set_property("RECORD", record);
	file->tag.set_property("NUDGE", nudge);
	file->tag.set_property("PLAY", play);
	file->tag.set_property("GANG", gang);
	file->tag.set_property("DRAW", draw);
	file->tag.set_property("EXPAND", expand_view);
	if(master)
		file->tag.set_property("MASTER", master);
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

	edits->save_xml(file, output_path);
	automation->save_xml(file);

	for(int i = 0; i < plugins.total; i++)
	{
// For historical reasons we have pluginsets
		file->tag.set_title("PLUGINSET");
		file->append_tag();
		file->append_newline();
		plugins.values[i]->save_xml(file);
		file->tag.set_title("/PLUGINSET");
		file->append_tag();
		file->append_newline();
	}

	file->tag.set_title("/TRACK");
	file->append_tag();
	file->append_newline();
}

void Track::copy(Track *track, ptstime start, ptstime end)
{
	record = track->record;
	nudge = track->nudge;
	play = track->play;
	gang = track->gang;
	draw = track->draw;
	master = track->master;
	expand_view = track->expand_view;
	track_w = track_w;
	track_h = track_h;
	strcpy(title, track->title);
	edits->copy(track->edits, start, end);
	automation->copy(track->automation, start, end);

	for(int i = 0; i < track->plugins.total; i++)
	{
		if(track->plugins.values[i]->active_in(start, end))
		{
			Plugin *new_plugin = plugins.append(new Plugin(edl, this, 0));
			new_plugin->copy(track->plugins.values[i], start, end);
		}
	}
}

void Track::copy_assets(ptstime start,
	ptstime end,
	ArrayList<Asset*> *asset_list)
{
	int i, result = 0;

	Edit *current = edits->editof(start, 0);

// Search all edits
	while(current && current->get_pts() < end)
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
	{
		clear_plugins(start, end);
	}

	if(actions & EDIT_EDITS)
		edits->clear(start, end);
}

void Track::clear_plugins(ptstime start, ptstime end)
{
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *plugin = plugins.values[i];
		ptstime pts = plugin->get_pts();
		ptstime pl_end = plugin->end_pts();

		if(start > pl_end)
			continue;
		if(end < pts)
			plugin->shift(end - start);
		else if(end < pl_end && end >= pts)
		{
			plugin->keyframes->clear_after(end);
			plugin->set_length(end - start);
		}
		else if(start <= pts && end < pl_end)
		{
			plugin->set_length(end - pts);
			plugin->shift(end - pts);
		}
		else // start < pts && end > pl_end
		{
			plugins.remove_object(plugin);
			i--;
		}
	}
}

void Track::clear_after(ptstime pts)
{
	automation->clear_after(pts);

	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *plugin = plugins.values[i];
		ptstime plugin_pts = plugin->get_pts();
		ptstime plugin_end = plugin->end_pts();

		if(plugin_end < pts)
			continue;
		if(plugin_pts < pts && plugin_end > pts)
			plugin->set_length(pts - plugin_pts);
		if(plugin->get_pts() > pts)
		{
			plugins.remove_object(plugin);
			i--;
		}
	}
	edits->clear_after(pts);
}

void Track::clear_handle(ptstime start,
	ptstime end,
	int actions,
	ptstime &distance)
{
	edits->clear_handle(start, end, actions, distance);
}

ptstime Track::adjust_position(ptstime oldposition, ptstime newposition,
	int currentend, int handle_mode)
{
	ptstime newpos = edits->adjust_position(oldposition, newposition,
		currentend, handle_mode);
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *plugin = plugins.values[i];

		if(edl->equivalent(plugin->get_pts(), oldposition) &&
				newpos < plugin->end_pts())
			plugin->set_pts(newpos);
		else if(edl->equivalent(plugin->end_pts(), oldposition) &&
				newpos > plugin->get_pts())
			plugin->set_length(newpos -plugin->get_pts());
	}
	return newpos;
}

void Track::modify_edithandles(ptstime oldposition,
	ptstime newposition,
	int currentend,
	int handle_mode)
{
	edits->modify_handles(oldposition, newposition, currentend, handle_mode);
}

void Track::modify_pluginhandles(ptstime oldposition,
	ptstime newposition,
	int currentend, 
	int handle_mode,
	int edit_labels)
{
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *plugin = plugins.values[i];

		if(edl->equivalent(plugin->get_pts(), oldposition) &&
				newposition < plugin->end_pts())
			plugin->set_pts(newposition);
		else if(edl->equivalent(plugin->end_pts(), oldposition) &&
				newposition > plugin->get_pts())
			plugin->set_length(newposition - plugin->get_pts());
	}
}

void Track::paste_silence(ptstime start, ptstime end, int edit_plugins)
{
	edits->paste_silence(start, end);
	shift_keyframes(start, end - start);
	if(edit_plugins) shift_effects(start, end - start);
}

int Track::playable_edit(ptstime position)
{
	int result = 0;

	for(Edit *current = edits->first; current && !result; current = NEXT)
	{
		if(current->get_pts() <= position && 
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
	for(int i = 0; i < plugins.total; i++)
	{
		ptstime new_duration = plugins.values[i]->plugin_change_duration(
			input_position, 
			input_length);

		if(new_duration < input_length)
			input_length = new_duration;
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
		current && current->get_pts() > input_position;
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
				current->get_pts() < input_position + input_length &&
				!need_edit(current, test_transitions);
				current = NEXT);

			if(current && need_edit(current, test_transitions) &&
					current->get_pts() < input_position + input_length)
				edit_length = current->get_pts() - input_position;
		}
	}
	else
	{
// Not over an edit.  Try the first edit.
		current = edits->first;
		if(current && ((test_transitions && current->transition) ||
				(!test_transitions && current->asset)))
			edit_length = edits->first->get_pts() - input_position;
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
	for(int i = 0; i < this->plugins.total; i++)
	{
		Plugin *current_plugin = plugins.values[i];

		if(current_plugin && 
			(current_plugin->on && 
			current_plugin->plugin_type != PLUGIN_NONE))
		{
			return 1;
		}
	}
	return 0;
}

void Track::detach_transition(Plugin *transition)
{
	for(Edit *current = edits->first; current; current = current->next)
	{
		if(current->transition == transition)
		{
			delete transition;
			current->transition = 0;
			break;
		}
	}
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
