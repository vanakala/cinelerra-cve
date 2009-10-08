
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

#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keyframe.h"
#include "keyframes.h"
#include "plugin.h"
#include "pluginautos.h"
#include "pluginset.h"
#include "track.h"

#include <string.h>

PluginSet::PluginSet(EDL *edl, Track *track)
 : Edits(edl, track, create_edit())
{
	record = 1;
}

PluginSet::~PluginSet()
{
	while(last) delete last;
}


PluginSet& PluginSet::operator=(PluginSet& plugins)
{
printf("PluginSet::operator= 1\n");
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

int64_t PluginSet::plugin_change_duration(int64_t input_position, 
	int64_t input_length, 
	int reverse)
{
	int result = input_length;
	Edit *current;

	if(reverse)
	{
		int input_start = input_position - input_length;
		for(current = last; current; current = PREVIOUS)
		{
			int start = current->startproject;
			int end = start + current->length;
			if(end > input_start && end < input_position)
			{
				result = input_position - end;
				return result;
			}
			else
			if(start > input_start && start < input_position)
			{
				result = input_position - start;
				return result;
			}
		}
	}
	else
	{
		int input_end = input_position + input_length;
		for(current = first; current; current = NEXT)
		{
			int start = current->startproject;
			int end = start + current->length;
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

Plugin* PluginSet::insert_plugin(char *title, 
	int64_t unit_position, 
	int64_t unit_length,
	int plugin_type,
	SharedLocation *shared_location,
	KeyFrame *default_keyframe,
	int do_optimize)
{
	Plugin *plugin = (Plugin*)create_and_insert_edit(unit_position, 
		unit_position + unit_length);


	if(title) strcpy(plugin->title, title);

	if(shared_location) plugin->shared_location = *shared_location;

	plugin->plugin_type = plugin_type;

	if(default_keyframe) 
		*plugin->keyframes->default_auto = *default_keyframe;
	plugin->keyframes->default_auto->position = unit_position;

// May delete the plugin we just added so not desirable while loading.
	if(do_optimize) optimize();
	return plugin;
}

Edit* PluginSet::create_edit()
{
	Plugin* result = new Plugin(edl, this, "");
	return result;
}

Edit* PluginSet::insert_edit_after(Edit *previous_edit)
{
	Plugin *current = new Plugin(edl, this, "");
	List<Edit>::insert_after(previous_edit, current);
	return (Edit*)current;
}


int PluginSet::get_number()
{
	return track->plugin_set.number_of(this);
}

void PluginSet::clear(int64_t start, int64_t end)
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

void PluginSet::clear_recursive(int64_t start, int64_t end)
{
//printf("PluginSet::clear_recursive 1\n");
	clear(start, end);
}

void PluginSet::shift_keyframes_recursive(int64_t position, int64_t length)
{
// Plugin keyframes are shifted in shift_effects
}

void PluginSet::shift_effects_recursive(int64_t position, int64_t length)
{
// Effects are shifted in length extension
//	shift_effects(position, length);
}


void PluginSet::clear_keyframes(int64_t start, int64_t end)
{
	for(Plugin *current = (Plugin*)first; current; current = (Plugin*)NEXT)
	{
		current->clear_keyframes(start, end);
	}
}

void PluginSet::copy_keyframes(int64_t start, 
	int64_t end, 
	FileXML *file, 
	int default_only,
	int autos_only)
{
	file->tag.set_title("PLUGINSET");	
	file->append_tag();
	file->append_newline();

	for(Plugin *current = (Plugin*)first; 
		current; 
		current = (Plugin*)NEXT)
	{
		current->copy_keyframes(start, end, file, default_only, autos_only);
	}

	file->tag.set_title("/PLUGINSET");	
	file->append_tag();
	file->append_newline();
}


void PluginSet::paste_keyframes(int64_t start, 
	int64_t length, 
	FileXML *file, 
	int default_only,
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
				int64_t position = file->tag.get_property("POSITION", 0);
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

//				printf("d: a%sb\n", data);
				Plugin *picked_first_target = 0;
				if (!target_pluginset && default_keyframe && default_only && strlen(data_default_keyframe) > 0)
				{
					strcpy(data, data_default_keyframe);
				} 
				if ((!target_pluginset && !default_keyframe && strlen(data) > 0) ||	
				    (!target_pluginset && default_keyframe && default_only && strlen(data_default_keyframe) > 0))	 
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
							if(position >= current->startproject 
							&& position <= current->length + current->startproject 
							&& !strncmp(((KeyFrame *)current->keyframes->default_auto)->data, data, name_len))
							{
								target_pluginset = pluginset;
								first_target_plugin = current;
								break;
							}
							if(position >= current->startproject 
							&& !strncmp(((KeyFrame *)current->keyframes->default_auto)->data, data, name_len))
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
//				printf(" Target: %p\n", target_pluginset);
				if (target_pluginset) 
				{
					unused_pluginsets.remove(target_pluginset);
					if (do_default_keyframe)
					{
// default plugin is always delayed
						KeyFrame *keyframe = (KeyFrame*)first_target_plugin->keyframes->default_auto;
						strcpy(keyframe->data, data_default_keyframe);
						keyframe->position = position;
						do_default_keyframe = 0;
					}
					if (!default_only && !default_keyframe)
					{
						for(current = (Plugin*)target_pluginset->last; 
							current;
							current = (Plugin*)PREVIOUS)
						{
							if(position >= current->startproject)
							{
								KeyFrame *keyframe;
								keyframe = (KeyFrame*)current->keyframes->insert_auto(position);
								strcpy(keyframe->data, data);
								keyframe->position = position;
								break;
							}
						}
					}
				}
				
// Get plugin owning keyframe
/*
				for(current = (Plugin*)last; 
					current;
					current = (Plugin*)PREVIOUS)
				{
// We want keyframes to exist beyond the end of the last plugin to
// make editing intuitive, but it will always be possible to 
// paste keyframes from one plugin into an incompatible plugin.
					if(position >= current->startproject)
					{
						KeyFrame *keyframe;
						if(file->tag.get_property("DEFAULT", 0) || default_only)
						{
							keyframe = (KeyFrame*)current->keyframes->default_auto;
						}
						else
						{
							keyframe = 
								(KeyFrame*)current->keyframes->insert_auto(position);
						}
						keyframe->load(file);
						keyframe->position = position;
						break;
					}
				}
*/
			}
		}
	}
}

void PluginSet::shift_effects(int64_t start, int64_t length)
{
	for(Plugin *current = (Plugin*)first;
		current;
		current = (Plugin*)NEXT)
	{
// Shift beginning of this effect
		if(current->startproject >= start)
		{
			current->startproject += length;
		}
		else
// Extend end of this effect.
// In loading new files, the effect should extend to fill the entire track.
// In muting, the effect must extend to fill the gap if another effect follows.
// The user should use Settings->edit effects to disable this.
		if(current->startproject + current->length >= start)
		{
			current->length += length;
		}

// Shift keyframes in this effect.
// If the default keyframe lands on the starting point, it must be shifted
// since the effect start is shifted.
		if(current->keyframes->default_auto->position >= start)
			current->keyframes->default_auto->position += length;

		current->keyframes->paste_silence(start, start + length);
	}
}

void PluginSet::copy(int64_t start, int64_t end, FileXML *file)
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
	int64_t startproject = 0;

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
				int64_t length = file->tag.get_property("LENGTH", (int64_t)0);
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
						0,
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
	optimize();
}



int PluginSet::optimize()
{
	int result = 1;
	Plugin *current_edit;


// Delete keyframes out of range
	for(current_edit = (Plugin*)first;
		current_edit; 
		current_edit = (Plugin*)current_edit->next)
	{
		current_edit->keyframes->default_auto->position = 0;
		for(KeyFrame *current_keyframe = (KeyFrame*)current_edit->keyframes->last;
			current_keyframe; )
		{
			KeyFrame *previous_keyframe = (KeyFrame*)current_keyframe->previous;
			if(current_keyframe->position > 
				current_edit->startproject + current_edit->length ||
				current_keyframe->position < current_edit->startproject)
			{
				delete current_keyframe;
			}
			current_keyframe = previous_keyframe;
		}
	}

// Insert silence between plugins
	for(Plugin *current = (Plugin*)last; current; current = (Plugin*)PREVIOUS)
	{
		if(current->previous)
		{
			Plugin *previous = (Plugin*)PREVIOUS;

			if(current->startproject - 
				previous->startproject - 
				previous->length > 0)
			{
				Plugin *new_plugin = (Plugin*)create_edit();
				insert_before(current, new_plugin);
				new_plugin->startproject = previous->startproject + 
					previous->length;
				new_plugin->length = current->startproject - 
					previous->startproject - 
					previous->length;
			}
		}
		else
		if(current->startproject > 0)
		{
			Plugin *new_plugin = (Plugin*)create_edit();
			insert_before(current, new_plugin);
			new_plugin->length = current->startproject;
		}
	}


// delete 0 length plugins
	while(result)
	{
		result = 0;

		for(current_edit = (Plugin*)first; 
			current_edit && !result; )
		{
			if(current_edit->length == 0)
			{
				Plugin* next = (Plugin*)current_edit->next;
				delete current_edit;
				result = 1;
				current_edit = next;
			}
			else
				current_edit = (Plugin*)current_edit->next;
		}


// merge identical plugins with same keyframes
		for(current_edit = (Plugin*)first; 
			current_edit && current_edit->next && !result; )
		{
			Plugin *next_edit = (Plugin*)current_edit->next;


// plugins identical
   			if(next_edit->identical(current_edit))
        	{
        		current_edit->length += next_edit->length;
// Merge keyframes
				for(KeyFrame *source = (KeyFrame*)next_edit->keyframes->first;
					source;
					source = (KeyFrame*)source->next)
				{
					KeyFrame *dest = new KeyFrame(edl, current_edit->keyframes);
					*dest = *source;
					current_edit->keyframes->append(dest);
				}
        		remove(next_edit);
        		result = 1;
        	}

    		current_edit = (Plugin*)current_edit->next;
		}

	}
	if (!last || !last->silence())
	{
// No last empty edit available... create one
		Edit *empty_edit = create_edit();
		if (!last) 
			empty_edit->startproject = 0;
		else
			empty_edit->startproject = last->startproject + last->length;
		empty_edit->length = LAST_VIRTUAL_LENGTH;
		insert_after(last, empty_edit);
	} else
	{
		last->length = LAST_VIRTUAL_LENGTH;
	}


	return 0;
}





void PluginSet::dump()
{
	printf("   PLUGIN_SET:\n");
	for(Plugin *current = (Plugin*)first; current; current =  (Plugin*)NEXT)
		current->dump();
}
