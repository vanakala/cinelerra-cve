#include "assets.h"
#include "autoconf.h"
#include "automation.h"
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
#include "selection.h"
#include "selections.h"
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
	track_w = edl->session->output_w;
	track_h = edl->session->output_h;
	id = EDL::next_id();
}

Track::~Track()
{
	delete automation;
	delete edits;
//for(int i = 0; i < plugin_set.total; i++)
//	printf("Track::~Track %p %p\n", plugin_set.values[i], plugin_set.values[i]->last);
	plugin_set.remove_all_objects();
}

int Track::create_objects()
{
	selections = new Selections(edl, this);
	return 0;
}


int Track::copy_settings(Track *track)
{
	this->expand_view = track->expand_view;
	this->draw = track->draw;
	this->gang = track->gang;
	this->record = track->record;
	this->play = track->play;
	this->track_w = track->track_w;
	this->track_h = track->track_h;
	strcpy(this->title, track->title);
	return 0;
}

int Track::load_defaults(Defaults *defaults)
{
	return 0;
}

void Track::equivalent_output(Track *track, double *result)
{
	if(data_type != track->data_type ||
		track_w != track->track_w ||
		track_h != track->track_h ||
		play != track->play)
		*result = 0;

// Convert result to track units
	long result2 = -1;
	automation->equivalent_output(track->automation, &result2);
//printf("Track::equivalent_output 3 %d\n", result2);
	edits->equivalent_output(track->edits, &result2);
//printf("Track::equivalent_output 4 %d\n", result2);

	int plugin_sets = MIN(plugin_set.total, track->plugin_set.total);
// Test existing plugin sets
	for(int i = 0; i < plugin_sets; i++)
	{
		plugin_set.values[i]->equivalent_output(
			track->plugin_set.values[i], 
			&result2);
	}
//printf("Track::equivalent_output 5 %d\n", result2);

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

//printf("Track::equivalent_output 6 %f %d\n", *result, result2);
	if(result2 >= 0 && 
		(*result < 0 || from_units(result2) < *result))
		*result = from_units(result2);
//printf("Track::equivalent_output 7 %f\n", *result);
}


int Track::is_synthesis(RenderEngine *renderengine, 
	long position, 
	int direction)
{
	int is_synthesis = 0;
	for(int i = 0; i < plugin_set.total; i++)
	{
		Plugin *plugin = get_current_plugin(position,
			i,
			direction,
			0);
		if(plugin)
		{
			is_synthesis = plugin->is_synthesis(renderengine, 
				position, 
				direction);
			if(is_synthesis) break;
		}
	}
	return is_synthesis;
}

Track& Track::operator=(Track& track)
{
//printf("Track::operator= 1\n");
	copy_settings(&track);
//printf("Track::operator= 1\n");
	*this->edits = *track.edits;
//printf("Track::operator= 1\n");
	for(int i = 0; i < this->plugin_set.total; i++)
		delete this->plugin_set.values[i];
//printf("Track::operator= 1\n");
	this->plugin_set.remove_all_objects();
//printf("Track::operator= 1\n");

	for(int i = 0; i < track.plugin_set.total; i++)
	{
		PluginSet *new_plugin_set = plugin_set.append(new PluginSet(edl, this));
		*new_plugin_set = *track.plugin_set.values[i];
	}
//printf("Track::operator= 1\n");
	*this->automation = *track.automation;
//printf("Track::operator= 1\n");
	*this->selections = *track.selections;
	this->track_w = track.track_w;
	this->track_h = track.track_h;
//printf("Track::operator= 2\n");
	return *this;
}

int Track::vertical_span(Theme *theme)
{
	int result = 0;
	if(expand_view)
		result = edl->local_session->zoom_track + 
			plugin_set.total * 
			theme->plugin_bg_data->get_h();
	else
		result = edl->local_session->zoom_track;

	if(edl->session->show_titles)
		result += theme->title_bg_data->get_h();

	return result;
}

double Track::get_length()
{
	double total_length = 0;
	double length = 0;

// Test edits
	if(edits->last)
	{
		length = from_units(edits->last->startproject + edits->last->length);
		if(length > total_length) total_length = length;
	}

// Test plugins
	for(int i = 0; i < plugin_set.total; i++)
	{
		if(plugin_set.values[i]->last)
		{
			length = from_units(plugin_set.values[i]->last->startproject + 
				plugin_set.values[i]->last->length);
			if(length > total_length) total_length = length;
		}
	}

// Test keyframes
	length = from_units(automation->get_length());
	if(length > total_length) total_length = length;
	

	return total_length;
}



void Track::get_source_dimensions(double position, int &w, int &h)
{
	long native_position = to_units(position, 0);
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


long Track::horizontal_span()
{
	return (long)(get_length() * 
		edl->session->sample_rate / 
		edl->local_session->zoom_sample + 
		0.5);
}


int Track::load(FileXML *file, int track_offset, unsigned long load_flags)
{
	int result = 0;
	int current_channel = 0;
	int current_plugin = 0;


	record = file->tag.get_property("RECORD", record);
	play = file->tag.get_property("PLAY", play);
	gang = file->tag.get_property("GANG", gang);
	draw = file->tag.get_property("DRAW", draw);
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
				file->read_text_until("/TITLE", title);
			}
			else
			if(strstr(file->tag.get_title(), "AUTOS"))
			{
				if(load_flags)
				{
					automation->load(file);
				}
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
				1);

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
					to_units(length, 0),
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
			to_units(length, 0),
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
		length = to_units(length, 0);
	}

	automation->paste_silence(Units::to_long(position), 
		Units::to_long(position + length));
// Effect keyframes are shifted in shift_effects
}

void Track::shift_effects(double position, double length, int convert_units)
{
	if(convert_units)
	{
		position = to_units(position, 0);
		length = to_units(length, 0);
	}

	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->shift_effects(Units::to_long(position), Units::to_long(length));
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
				long start = plugin->startproject;
				long end = plugin->startproject + plugin->length;

				plugin_set->clear(start, end);
				plugin_set->paste_silence(start, end);

// Delete 0 length pluginsets	
				plugin_set->optimize();
//printf("Track::detach_effect 2 %d\n", plugin_set->length());
				if(!plugin_set->length()) 
					this->plugin_set.remove_object(plugin_set);

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
}



void Track::optimize()
{
	edits->optimize();
	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->optimize();
//printf("Track::optimize %d\n", plugin_set.values[i]->total());
		if(plugin_set.values[i]->total() <= 0)
		{
			remove_pluginset(plugin_set.values[i]);
		}
	}
}

Plugin* Track::get_current_plugin(double position, 
	int plugin_set, 
	int direction, 
	int convert_units)
{
	Plugin *current;
	if(convert_units) position = to_units(position, 0);
	
	
	if(plugin_set >= this->plugin_set.total || plugin_set < 0) return 0;

//printf("Track::get_current_plugin 1 %d %d %d\n", position, plugin_set, direction);
	if(direction == PLAY_FORWARD)
	{
		for(current = (Plugin*)this->plugin_set.values[plugin_set]->last; 
			current; 
			current = (Plugin*)PREVIOUS)
		{
//printf("Track::get_current_plugin 2 %d %ld %ld %f\n", current->startproject + current->length > position, current->startproject, current->length, position);
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

Plugin* Track::get_current_transition(double position, int direction, int convert_units)
{
	Edit *current;
	Plugin *result = 0;
	if(convert_units) position = to_units(position, 0);

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
	long start = to_units(selectionstart, 0);
	long end = to_units(selectionend, 0);

	file->tag.set_title("TRACK");
// Video or audio
    save_header(file);
	file->append_tag();
	file->append_newline();

	automation->copy(start, end, file, default_only, autos_only);

	if(edl->session->auto_conf->plugins)
	{
		for(int i = 0; i < plugin_set.total; i++)
		{
			plugin_set.values[i]->copy_keyframes(start, 
				end, 
				file, 
				default_only,
				autos_only);
		}
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
	long sample_rate,
	FileXML *file,
	int default_only)
{
// Only used for pasting automation alone.
	long start;
	long length;
	int result;
	int current_pluginset;
	double scale;

	if(data_type == TRACK_AUDIO)
		scale = edl->session->sample_rate / sample_rate;
	else
		scale = edl->session->frame_rate / frame_rate;

	total_length *= scale;
	start = to_units(selectionstart, 0);
	length = to_units(total_length, 0);
	result = 0;
	current_pluginset = 0;
//printf("Track::paste_automation 1\n");

	while(!result)
	{
		result = file->read_tag();

		if(!result)
		{
			if(file->tag.title_is("/TRACK"))
				result = 1;
			else
			if(strstr(file->tag.get_title(), "AUTOS"))
			{
				automation->paste(start, 
					length, 
					scale,
					file,
					default_only,
					0);
			}
			else
			if(file->tag.title_is("PLUGINSET"))
			{
				if(current_pluginset < plugin_set.total)
				{
//printf("Track::paste_automation 2 %d\n", current_pluginset);
					plugin_set.values[current_pluginset]->paste_keyframes(start, 
						length, 
						file,
						default_only);
					current_pluginset++;
				}
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
	long start = to_units(selectionstart, 0);
	long end = to_units(selectionend, 0);

	automation->clear(start, end, edl->session->auto_conf, 0);

	if(edl->session->auto_conf->plugins)
	{
		for(int i = 0; i < plugin_set.total; i++)
		{
			plugin_set.values[i]->clear_keyframes(start, end);
		}
	}

}


int Track::copy(double start, 
	double end, 
	FileXML *file, 
	char *output_path)
{
//printf("Track::copy 1\n");
// Use a copy of the selection in converted units
// So copy_automation doesn't reconvert.
	long start_unit = to_units(start, 0);
	long end_unit = to_units(end, 1);




	file->tag.set_title("TRACK");
//	file->tag.set_property("PLAY", play);
	file->tag.set_property("RECORD", record);
	file->tag.set_property("PLAY", play);
	file->tag.set_property("GANG", gang);
//	file->tag.set_property("MUTE", mute);
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

//printf("Track::copy 1\n");
	if(data_type == TRACK_AUDIO)
		file->tag.set_property("TYPE", "AUDIO");
	else
		file->tag.set_property("TYPE", "VIDEO");

//printf("Track::copy 1\n");
	file->append_tag();
	file->append_newline();

//printf("Track::copy 1\n");
	edits->copy(start_unit, end_unit, file, output_path);

//printf("Track::copy 1\n");
	AutoConf auto_conf;
	auto_conf.set_all();
	automation->copy(start_unit, end_unit, file, 0, 0);


	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->copy(start_unit, end_unit, file);
	}

//printf("Track::copy 1\n");
	copy_derived(start_unit, end_unit, file);

//printf("Track::copy 1\n");
	file->tag.set_title("/TRACK");
	file->append_tag();
	file->append_newline();
	file->append_newline();
	file->append_newline();
	file->append_newline();
//printf("Track::copy 2\n");
	return 0;
}

int Track::copy_assets(double start, 
	double end, 
	ArrayList<Asset*> *asset_list)
{
	int i, result = 0;

	start = to_units(start, 0);
	end = to_units(end, 0);

	Edit *current = edits->editof((long)start, PLAY_FORWARD);

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
	int convert_units)
{
// Edits::move_auto calls this routine after the units are converted to the track
// format.
	if(convert_units)
	{
		start = to_units(start, 0);
		end = to_units(end, 0);
	}

	if(edit_edits)
		automation->clear((long)start, (long)end, 0, 1);

	if(edit_plugins)
		for(int i = 0; i < plugin_set.total; i++)
			plugin_set.values[i]->clear((long)start, (long)end);

	if(edit_edits)
		edits->clear((long)start, (long)end);
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
		edit_plugins);


	return 0;
}

int Track::modify_pluginhandles(double oldposition, 
	double newposition, 
	int currentend, 
	int handle_mode,
	int edit_labels)
{
	for(int i = 0; i < plugin_set.total; i++)
	{
		plugin_set.values[i]->modify_handles(oldposition, 
			newposition, 
			currentend, 
			handle_mode,
			0,
			edit_labels,
			1);
	}
	return 0;
}


int Track::paste_silence(double start, double end, int edit_plugins)
{
	start = to_units(start, 0);
	end = to_units(end, 1);

	edits->paste_silence((long)start, (long)end);
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

int Track::scale_time(float rate_scale, int scale_edits, int scale_autos, long start, long end)
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


int Track::delete_module_pointers(int deleted_track)
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
				if(plugin->shared_location.module == deleted_track)
				{
					plugin->on = 0;
				}
				else
				{
					plugin->shared_location.module--;
				}
			}
		}
	}

	return 0;
}

int Track::playable_edit(long position, int direction)
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


int Track::edit_is_interesting(Edit *current, int test_transitions)
{
	return ((test_transitions && current->transition) ||
		(!test_transitions && current->asset));
}

long Track::edit_change_duration(long input_position, long input_length, int reverse, int test_transitions)
{
	Edit *current;
	long edit_length = input_length;

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
			if(edit_is_interesting(current, test_transitions))
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
					!edit_is_interesting(current, test_transitions);
					current = PREVIOUS)
					;

					if(current && 
						edit_is_interesting(current, test_transitions) &&
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
				edit_length = input_position - edits->last->startproject - edits->last->length + 1;
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
			if(edit_is_interesting(current, test_transitions))
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
					!edit_is_interesting(current, test_transitions);
					current = NEXT)
					;

					if(current && 
						edit_is_interesting(current, test_transitions) &&
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

int Track::channel_is_playable(long position, int direction, int *do_channel)
{
	return 1;
}


int Track::plugin_used(long position, long direction)
{
//printf("Track::plugin_used 1 %d\n", this->plugin_set.total);
	for(int i = 0; i < this->plugin_set.total; i++)
	{
		Plugin *current_plugin = get_current_plugin(position, i, direction, 0);

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
int Track::direct_copy_possible(long start, int direction)
{
	return 1;
}

long Track::to_units(double position, int round)
{
	return (long)position;
}

double Track::to_doubleunits(double position)
{
	return position;
}

double Track::from_units(long position)
{
	return (double)position;
}
