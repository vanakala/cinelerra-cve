
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
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keyframe.h"
#include "keyframes.h"
#include "plugin.h"
#include "pluginset.h"
#include "track.h"

#include <string.h>

PluginSet::PluginSet(EDL *edl, Track *track)
 : Edits(edl, track)
{
	record = 1;
}

PluginSet::~PluginSet()
{
	while(last) delete last;
}

PluginSet& PluginSet::operator=(PluginSet& plugins)
{
	copy_from(&plugins);
	return *this;
}

void PluginSet::copy_from(PluginSet *src)
{
	while(last) delete last;
	for(Plugin *current = (Plugin*)src->first; current; current = (Plugin*)NEXT)
	{
		Plugin *new_plugin;
		append(new_plugin = (Plugin*)create_edit());
		new_plugin->copy_from(current);
	}
	this->record = src->record;
}

Plugin* PluginSet::get_first_plugin()
{
// Called when a new pluginset is added.
// Get first non-silence plugin in the plugin set.
	for(Plugin *current = (Plugin*)first; current; current = (Plugin*)NEXT)
	{
		if(current && current->plugin_type != PLUGIN_NONE)
		{
			return current;
		}
	}
	return 0;
}

ptstime PluginSet::plugin_change_duration(ptstime input_position,
	ptstime input_length)
{
	ptstime result = input_length;
	Edit *current;

	ptstime input_end = input_position + input_length;
	for(current = first; current; current = NEXT)
	{
		ptstime start = current->project_pts;
		ptstime end = current->end_pts();
		if(start > input_position && start < input_end)
		{
			result = start - input_position;
			return result;
		}
		else
		if(end > input_position && end < input_end)
		{
			result = end - input_position;
			return result;
		}
	}
	return input_length;
}

void PluginSet::synchronize_params(PluginSet *plugin_set)
{
	for(Plugin *this_plugin = (Plugin*)first, *that_plugin = (Plugin*)plugin_set->first;
		this_plugin && that_plugin;
		this_plugin = (Plugin*)this_plugin->next, that_plugin = (Plugin*)that_plugin->next)
	{
		this_plugin->synchronize_params(that_plugin);
	}
}

Plugin* PluginSet::insert_plugin(const char *title, 
	ptstime position,
	ptstime length,
	int plugin_type,
	SharedLocation *shared_location,
	KeyFrame *default_keyframe)
{
	Plugin *plugin = (Plugin*)insert_edit(position, length);

	if(title) strcpy(plugin->title, title);

	if(shared_location) plugin->shared_location = *shared_location;

	plugin->plugin_type = plugin_type;
	plugin->keyframes->base_pts = position;

	if(default_keyframe)
	{
		plugin->keyframes->append_auto();
		*((KeyFrame *)plugin->keyframes->first) = *default_keyframe;
		plugin->keyframes->first->pos_time = position;
	}
	return plugin;
}

Edit* PluginSet::create_edit()
{
	return new Plugin(edl, this, "");
}

int PluginSet::get_number()
{
	return track->plugin_set.number_of(this);
}

void PluginSet::clear(ptstime start, ptstime end)
{
// Clear keyframes
	for(Plugin *current = (Plugin*)first;
		current;
		current = (Plugin*)NEXT)
	{
		current->keyframes->clear(start, end, 1);
	}

// Clear edits
	Edits::clear(start, end);
}

void PluginSet::clear_keyframes(ptstime start, ptstime end)
{
	for(Plugin *current = (Plugin*)first; current; current = (Plugin*)NEXT)
	{
		current->clear_keyframes(start, end);
	}
}

void PluginSet::copy_keyframes(ptstime start,
	ptstime end,
	FileXML *file)
{
	file->tag.set_title("PLUGINSET");
	file->append_tag();
	file->append_newline();

	for(Plugin *current = (Plugin*)first; 
		current; 
		current = (Plugin*)NEXT)
	{
		current->copy_keyframes(start, end, file);
	}

	file->tag.set_title("/PLUGINSET");
	file->append_tag();
	file->append_newline();
}

void PluginSet::paste_keyframes(ptstime start,
	ptstime length,
	FileXML *file, 
	Track *track)
{
	int result = 0;
	Plugin *current;

	PluginSet *target_pluginset;
	Plugin *first_target_plugin = 0;

	ArrayList<PluginSet*> unused_pluginsets;

// get all available target pluginsets, we will be removing them one by one when we will paste into them
	for (int i = 0; i < track->plugin_set.total; i++)
	{
		unused_pluginsets.append(track->plugin_set.values[i]);
	}

	char data[MESSAGESIZE];
	char data_default_keyframe[MESSAGESIZE];
	int default_keyframe;
	int do_default_keyframe = 0;

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
				target_pluginset = 0;
				first_target_plugin = 0;
			}
			else
			if(file->tag.title_is("KEYFRAME"))
			{
				ptstime position = file->tag.get_property("POSTIME", 0);
				position += start;
				if(file->tag.get_property("DEFAULT", 0))
				{
// remember the default keyframe, we'll use it later
					default_keyframe = 1; 
					do_default_keyframe = 1;
					file->read_text_until("/KEYFRAME", data_default_keyframe, MESSAGESIZE);
				}
				else
				{
					default_keyframe = 0;
					file->read_text_until("/KEYFRAME", data, MESSAGESIZE);
				}

				Plugin *picked_first_target = 0;
				if (!target_pluginset && default_keyframe && strlen(data_default_keyframe) > 0)
				{
					strcpy(data, data_default_keyframe);
				} 
				if ((!target_pluginset && !default_keyframe && strlen(data) > 0) ||
					(!target_pluginset && default_keyframe && strlen(data_default_keyframe) > 0))
				{
// now try to find the target
					int name_len = strchr(data, ' ') - data + 1;

					PluginSet *second_choice = 0;
					Plugin *second_choice_first_target_plugin = 0;
					for (int i = 0; i < unused_pluginsets.total && !target_pluginset; i++)
					{
						PluginSet *pluginset = unused_pluginsets.values[i];
						Plugin *current;
						for(current = (Plugin*)(pluginset->last); 
							current;
							current = (Plugin*)PREVIOUS)
						{
							if(position >= current->project_pts
								&& position <= current->end_pts()
								&& !strncmp(((KeyFrame *)current->keyframes->first)->data, data, name_len))
							{
								target_pluginset = pluginset;
								first_target_plugin = current;
								break;
							}
							if(position >= current->project_pts
								&& !strncmp(((KeyFrame *)current->keyframes->first)->data, data, name_len))
							{
								second_choice = pluginset;
								second_choice_first_target_plugin = current;
								break;
							}
						}
					}
					if (!target_pluginset) 
					{
						target_pluginset = second_choice;
						first_target_plugin = second_choice_first_target_plugin;
					}
				}
				if (target_pluginset) 
				{
					unused_pluginsets.remove(target_pluginset);
					if (do_default_keyframe)
					{
// default plugin is always delayed
						KeyFrame *keyframe = (KeyFrame*)first_target_plugin->keyframes->first;
						strcpy(keyframe->data, data_default_keyframe);
						keyframe->pos_time = position;
						do_default_keyframe = 0;
					}
					for(current = (Plugin*)target_pluginset->last; 
						current;
						current = (Plugin*)PREVIOUS)
					{
						if(position >= current->project_pts)
						{
							KeyFrame *keyframe;
							keyframe = (KeyFrame*)current->keyframes->insert_auto(position);
							strcpy(keyframe->data, data);
							keyframe->pos_time = position;
							break;
						}
					}
				}
			}
		}
	}
}

void PluginSet::shift_effects(ptstime start, ptstime length)
{
	if(PTSEQU(length, 0))
		return;

	for(Plugin *current = (Plugin*)first;
		current;
		current = (Plugin*)NEXT)
	{
		if(current != first && current->project_pts >= start)
		{
			current->project_pts += length;
			current->keyframes->base_pts = current->project_pts;
			if(current->keyframes->first && current->keyframes->first->pos_time >= start)
				current->keyframes->first->pos_time += length;
		}
// Shift keyframes in this effect.
		current->keyframes->paste_silence(start, start + length);
	}
}

void PluginSet::copy(ptstime start, ptstime end, FileXML *file)
{
	file->tag.set_title("PLUGINSET");
	file->tag.set_property("RECORD", record);
	file->append_tag();
	file->append_newline();

	for(Plugin *current = (Plugin*)first; current; current = (Plugin*)NEXT)
	{
		current->copy(start, end, file);
	}

	file->tag.set_title("/PLUGINSET");
	file->append_tag();
	file->append_newline();
}

void PluginSet::save(FileXML *file)
{
	copy(0, length(), file);
}

void PluginSet::load(FileXML *file, uint32_t load_flags)
{
	int result = 0;
// Current plugin being amended
	Plugin *plugin = (Plugin*)first;
	ptstime startproject = 0;

	record = file->tag.get_property("RECORD", record);
	do{
		result = file->read_tag();


		if(!result)
		{
			if(file->tag.title_is("/PLUGINSET"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("PLUGIN"))
			{
				posnum length_units = file->tag.get_property("LENGTH", (int64_t)0);
				ptstime length = 0;
				if(length_units)
					length = track->from_units(length_units);
				startproject = file->tag.get_property("POSTIME", startproject);
				loaded_length += length;
				int plugin_type = file->tag.get_property("TYPE", 1);
				char title[BCTEXTLEN];
				title[0] = 0;
				file->tag.get_property("TITLE", title);
				SharedLocation shared_location;
				shared_location.load(file);

				if(load_flags & LOAD_EDITS)
				{
					plugin = insert_plugin(title, 
						startproject, 
						length,
						plugin_type,
						&shared_location,
						0);
					plugin->load(file);
					startproject += length;
				}
				else
				if(load_flags & LOAD_AUTOMATION)
				{
					if(plugin)
					{
						plugin->load(file);
						plugin = (Plugin*)plugin->next;
					}
				}
			}
		}
	}while(!result);
}

void PluginSet::dump(int indent)
{
	printf("%*sPlugin Set %p dump:\n", indent, "", this);
	indent += 2;
	for(Plugin *current = (Plugin*)first; current; current =  (Plugin*)NEXT)
		current->dump(indent);
}
