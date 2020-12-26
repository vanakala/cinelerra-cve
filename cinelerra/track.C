// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "aautomation.h"
#include "automation.h"
#include "atrackrender.h"
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
#include "mwindow.h"
#include "plugin.h"
#include "plugindb.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
#include "vautomation.h"
#include "vframe.h"
#include "vtrackrender.h"
#include <string.h>

// Reset plugin if it is inactive more seconds
#define PLUGIN_INACTIVE_TIME 1

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
	renderer = 0;
	track_w = edlsession->output_w;
	track_h = edlsession->output_h;
	one_unit = (ptstime) 1 / 48000;
	edits = new Edits(edl, this);
	id = EDL::next_id();
	if(tracks)
		tracks->reset_plugins();
}

Track::~Track()
{
	for(Track *track = tracks->first; track; track = track->next)
	{
		for(int i = 0; i < track->plugins.total; i++)
		{
			Plugin *plugin = track->plugins.values[i];

			if(plugin->shared_track == this)
				plugin->shared_track = 0;
		}
	}
	plugins.remove_all_objects();
	tracks->reset_plugins();
	delete automation;
	delete edits;
	delete renderer;
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
			!EQUIV(nudge, track->nudge))
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
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *plugin = plugins.values[i];

		if(plugin->get_pts() > position || plugin->end_pts() < position)
			continue;

// Assume data from a shared track is synthesized
		if(plugin->plugin_type == PLUGIN_SHAREDMODULE)
			return 1;
		if(plugin->is_synthesis())
			return 1;
	}
	return 0;
}

void Track::copy_from(Track *track)
{
	copy_settings(track);
	edits->copy_from(track->edits);
	plugins.remove_all_objects();
	id = track->id;

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
	int result;
	int number;

	if(expand_view)
	{
		if(edlsession->shrink_plugin_tracks)
		{
			ptstime start = master_edl->local_session->view_start_pts;
			ptstime end = start + mwindow_global->trackcanvas_visible();

			number = 0;
			for(int i = 0; i < plugins.total; i++)
			{
				if(plugins.values[i]->active_in(start, end))
					number++;
			}
		}
		else
			number = plugins.total;

		result = edl->local_session->zoom_track +
			number *
			theme->get_image("plugin_bg_data")->get_h();
	}
	else
		result = edl->local_session->zoom_track;

	if(edlsession->show_titles)
		result += theme->get_image("title_bg_data")->get_h();

	return result;
}

int Track::get_canvas_number(Plugin *plugin)
{
	int number = -1;

	if(edlsession->shrink_plugin_tracks)
	{
		ptstime start = master_edl->local_session->view_start_pts;
		ptstime end = start + mwindow_global->trackcanvas_visible();

		number = 0;
		for(int i = 0; i < plugins.total; i++)
		{
			if(plugins.values[i]->active_in(start, end))
			{
				if(plugins.values[i] == plugin)
					return number;
				number++;
			}
		}
	}
	else
		number = plugins.number_of(plugin);
	return number;
}

ptstime Track::get_length()
{
	ptstime total_length = 0;
	ptstime length;

// Test edits
	if(edits->last)
		total_length = edits->last->get_pts();
// Test synthesis effects
	if((length = get_effects_length(1)) > total_length)
		total_length = length;
	return total_length;
}

ptstime Track::get_effects_length(int is_synthesis)
{
	ptstime total_length = 0;
	ptstime length;
	Plugin *plugin;

// Test plugins
	for(int i = 0; i < plugins.total; i++)
	{
		plugin = plugins.values[i];
		length = plugin->end_pts();
		if(!is_synthesis || plugin->is_synthesis())
		{
			if(length > total_length)
				total_length = length;
		}
	}
// Test automation
	if(!is_synthesis)
	{
		length = automation->get_length();
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

void Track::update_plugin_guis()
{
	for(int i = 0; i < plugins.total; i++)
		plugins.values[i]->update_plugin_gui();
}

void Track::update_plugin_titles()
{
	for(int i = 0; i < plugins.total; i++)
		plugins.values[i]->update_display_title();
}

void Track::load(FileXML *file)
{
	int result = 0;
	int current_plugin = 0;

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
			if(automation->load(file))
			{
				;
			}
			else
			if(file->tag.title_is("EDITS"))
				edits->load(file);
			else
			if(file->tag.title_is("PLUGINSET"))
				load_pluginset(file, 0);
		}
	}while(!result);
}

void Track::load_pluginset(FileXML *file, ptstime start)
{
	ptstime startproject = 0;
	Plugin *plugin = 0;

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
					server = plugindb.get_pluginserver(string, data_type, 0);
				}
				plugin = new Plugin(edl, this, server);
				plugins.append(plugin);
				plugin->plugin_type = type;
				plugin->set_pts(startproject + start);
				plugin->set_length(length);
				plugin->on = !file->tag.get_property("OFF", 0);
				plugin->load(file, start);
				if(!server && type == PLUGIN_STANDALONE)
					plugin->on = 0;
				// got length - ignore next plugin pts
				if(length > 0)
					plugin = 0;
			}
			else if(plugin)
			{
				plugin->set_end(startproject + start);
				plugin = 0;
			}
		}
		if(file->tag.title_is("/PLUGINSET"))
			break;
	}
}

void Track::init_shared_pointers()
{
	for(int i = 0; i < plugins.total; i++)
		plugins.values[i]->init_shared_pointers();
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
		shift_effects(position, length);
		automation->paste_silence(position, position + length);
	}
}

// Insert data
void Track::insert_track(Track *track, 
	ptstime length,
	ptstime position,
	int overwrite)
{
	edits->insert_edits(track->edits, position, length, overwrite);

	insert_plugin(track, position, length, overwrite);

	automation->insert_track(track->automation, position,
		length, overwrite);
}

// Called by insert_track
void Track::insert_plugin(Track *track, ptstime position,
	ptstime duration, int overwrite)
{
	Plugin *plugin, *new_plugin;
	ptstime end, new_start;

	if(duration < 0)
		duration = track->get_length();

	end = position + duration;

	if(overwrite)
	{
// Do not clear plugins, if only effects are pasted
		if(track->tracks->edl)
			clear_plugins(position, end);
	}
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
			new_plugin->shift(position);
			if(new_plugin->end_pts() > end)
				new_plugin->set_end(end);
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

void Track::reset_plugins(ptstime pts)
{
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *current = plugins.values[i];

		if(current->end_pts() < pts - PLUGIN_INACTIVE_TIME ||
				current->get_pts() > pts + PLUGIN_INACTIVE_TIME)
			current->reset_plugin();
	}
	for(Edit *edit = edits->first; edit; edit = edit->next)
	{
		if(edit->transition)
		{
			ptstime pos = edit->get_pts();
			ptstime end = edit->get_pts() + edit->transition->get_length();

			if(end < pts - PLUGIN_INACTIVE_TIME ||
					pos > pts + PLUGIN_INACTIVE_TIME)
				edit->transition->reset_plugin();
		}
	}
}

void Track::reset_renderers()
{
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *current = plugins.values[i];

		if(current->plugin_server && current->client)
		{
			delete current->guideframe;
			current->guideframe = 0;
			current->plugin_server->close_plugin(current->client);
		}
	}
	delete renderer;
	renderer = 0;
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
	printf("%*sType: %s %c-%c-%c-%c-%c nudge %.2f id %d\n", indent, "",
		tp, master ? 'M' : 'm', play ? 'P' : 'p',
		record ? 'A' : 'a', gang ? 'G' : 'g', draw ? 'D' : 'd', nudge, id);
	if(data_type == TRACK_VIDEO)
	{
		printf("%*sTitle: '%s' [%d,%d] renderer %p\n", indent, "",
			title, track_w, track_h, renderer);
		if(renderer)
			((VTrackRender*)renderer)->dump(indent);
	}
	else
	{
		printf("%*sTitle: '%s' renderer %p\n", indent, "", title, renderer);
		if(renderer)
			((ATrackRender*)renderer)->dump(indent);
	}
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

	file->tag.set_title("/TRACK");
	file->append_tag();
	file->append_newline();
}

void Track::load_effects(FileXML *file, int operation)
{
// Only used for pasting effects alone.
	while(!file->read_tag())
	{
		if(file->tag.title_is("/TRACK"))
			break;
		else
		{
			automation->load(file, operation);

			if(file->tag.title_is("PLUGINSETS"))
				load_pluginsets(file, operation);
		}
	}
}

void Track::load_pluginsets(FileXML *file, int operation)
{
	while(!file->read_tag())
	{
		if(file->tag.title_is("/PLUGINSETS"))
			break;
		else
		if(file->tag.title_is("PLUGINSET") &&
				(operation == PASTE_ALL || operation == PASTE_PLUGINS))
			load_pluginset(file, 0);
	}
}

void Track::clear_automation(ptstime selectionstart,
	ptstime selectionend)
{
	automation->clear(selectionstart, selectionend, edlsession->auto_conf, 0);
}

void Track::straighten_automation(ptstime selectionstart, ptstime selectionend)
{
	automation->straighten(selectionstart, selectionend);
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

	Edit *current = edits->editof(start);

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
	ptstime end)
{
	automation->clear(start, end, 0, 1);
	clear_plugins(start, end);
	edits->clear(start, end);
}

void Track::clear_plugins(ptstime start, ptstime end)
{
	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *plugin = plugins.values[i];
		ptstime pl_pts = plugin->get_pts();
		ptstime pl_end = plugin->end_pts();

		if(start > pl_end) // plugin before selection
			continue;
		if(end < pl_pts)      // plugin after selection
			plugin->shift(start - end);
		else if(start <= pl_pts && pl_end <= end) // plugin in selection
		{
			plugins.remove_object(plugin);
			i--;
		}
		else if(start > pl_pts && end < pl_end) // selection in plugin
		{
			plugin->keyframes->clear(start, end, 1);
			plugin->set_length(plugin->get_length() - end + start);
		}
		else if(end >= pl_end && start > pl_pts) // plugin end in selection
		{
			plugin->keyframes->clear_after(start);
			plugin->set_length(start - pl_pts);
		}
		else if(start <= pl_pts && end < pl_end) // plugin start in selection
		{
			plugin->keyframes->clear(pl_pts, end, 1);
			plugin->set_length(pl_end - end);
			plugin->shift(start - pl_pts);
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
		{
			plugin->shift_keyframes(newpos - plugin->get_pts());
			plugin->set_pts(newpos);
		}
		else if(edl->equivalent(plugin->end_pts(), oldposition) &&
				newpos > plugin->get_pts())
		{
			plugin->set_length(newpos - plugin->get_pts());
			plugin->remove_keyframes_after(newpos);
		}
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

void Track::paste_silence(ptstime start, ptstime end)
{
	edits->paste_silence(start, end);
	shift_keyframes(start, end - start);
	shift_effects(start, end - start);
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

void Track::cleanup()
{
	// Remove orphaned shared plugins
	for(int i = 0; i < this->plugins.total; i++)
	{
		Plugin *current_plugin = plugins.values[i];

		if((current_plugin->plugin_type == PLUGIN_SHAREDPLUGIN &&
				!current_plugin->shared_plugin) ||
			(current_plugin->plugin_type == PLUGIN_SHAREDMODULE &&
				!current_plugin->shared_track))
		{
			plugins.remove_object(current_plugin);
			i--;
		}
	}
}

ptstime Track::plugin_max_start(Plugin *plugin)
{
	if(master && plugin->plugin_server &&
			plugin->plugin_server->synthesis)
		return get_length();
	else
		return tracks->length() - plugin->get_length();
}

Edit *Track::editof(ptstime postime)
{
	return edits->editof(postime);
}

Plugin *Track::get_shared_multichannel(ptstime start, ptstime end)
{
	if(PTSEQU(start, end))
	{
		start = 0;
		end = get_length();
	}

	for(int j = 0; j < plugins.total; j++)
	{
		Plugin *plugin = plugins.values[j];

		if(plugin->plugin_type == PLUGIN_SHAREDPLUGIN &&
				plugin->shared_plugin &&
				plugin->shared_plugin->plugin_server &&
				plugin->shared_plugin->plugin_server->multichannel &&
				plugin->get_pts() < end && plugin->end_pts() > start)
			return plugin;
	}
	return 0;
}

Plugin *Track::get_shared_track(ptstime start, ptstime end)
{
	if(PTSEQU(start, end))
	{
		start = 0;
		end = get_length();
	}

	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *plugin = plugins.values[i];

		if(plugin->plugin_type == PLUGIN_SHAREDMODULE &&
				plugin->get_pts() < end && plugin->end_pts() > start)
			return plugin;
	}
	return 0;
}

size_t Track::get_size()
{
	size_t size = sizeof(*this);

	for(int i = 0; i < plugins.total; i++)
	{
		Plugin *current_plugin = plugins.values[i];

		size += current_plugin->get_size();
	}
	size += edits->get_size();

	switch(data_type)
	{
	case TRACK_AUDIO:
		size += ((AAutomation*)automation)->get_size();
		break;
	case TRACK_VIDEO:
		size += ((VAutomation*)automation)->get_size();
		break;
	}
	return size;
}
