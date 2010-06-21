
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
#include "assets.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "localsession.h"
#include "plugin.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include <string.h>


Edit::Edit()
{
	reset();
}

Edit::Edit(EDL *edl, Track *track)
{
	reset();
	this->edl = edl;
	this->track = track;
	if(track) this->edits = track->edits;
	id = EDL::next_id();
}

Edit::Edit(EDL *edl, Edits *edits)
{
	reset();
	this->edl = edl;
	this->edits = edits;
	if(edits) this->track = edits->track;
	id = EDL::next_id();
}

Edit::~Edit()
{
	if(transition) delete transition;
}

void Edit::reset()
{
	edl = 0;
	track = 0;
	edits = 0;
	startsource = 0;
	startproject = 0;
	length = 0;  
	asset = 0;
	transition = 0;
	channel = 0;
	user_title[0] = 0;
}

int Edit::copy(posnum start, posnum end, FileXML *file, const char *output_path)
{
// variables

	posnum endproject = startproject + length;
	int result;

	if((startproject >= start && startproject <= end) ||  // startproject in range
		 (endproject <= end && endproject >= start) ||	   // endproject in range
		 (startproject <= start && endproject >= end))    // range in project
	{   
// edit is in range
		posnum startproject_in_selection = startproject; // start of edit in selection in project
		posnum startsource_in_selection = startsource; // start of source in selection in source
		posnum endsource_in_selection = startsource + length; // end of source in selection
		posnum length_in_selection = length;             // length of edit in selection

		if(startproject < start)
		{         // start is after start of edit in project
			posnum length_difference = start - startproject;

			startsource_in_selection += length_difference;
			startproject_in_selection += length_difference;
			length_in_selection -= length_difference;
		}

		if(endproject > end)
		{         // end is before end of edit in project
			length_in_selection = end - startproject_in_selection;
		}
		

		if(file)    // only if not counting
		{
			file->tag.set_title("EDIT");
			file->tag.set_property("STARTSOURCE", startsource_in_selection);
			file->tag.set_property("CHANNEL", (int64_t)channel);
			file->tag.set_property("LENGTH", length_in_selection);
			if(user_title[0]) file->tag.set_property("USER_TITLE", user_title);

			copy_properties_derived(file, length_in_selection);

			file->append_tag();

			if(asset)
			{
				char stored_path[1024];
				char asset_directory[1024], output_directory[1024];
				FileSystem fs;

				fs.extract_dir(asset_directory, asset->path);

				if(output_path)
					fs.extract_dir(output_directory, output_path);
				else
					output_directory[0] = 0;

				if(output_path && !strcmp(asset_directory, output_directory))
					fs.extract_name(stored_path, asset->path);
				else
					strcpy(stored_path, asset->path);

				file->tag.set_title("FILE");
				file->tag.set_property("SRC", stored_path);
				file->append_tag();
				file->tag.set_title("/FILE");
				file->append_tag();
			}

			if(transition)
			{
				transition->save_xml(file);
			}

			file->tag.set_title("/EDIT");
			file->append_tag();
			file->append_newline();	
		}
		result = 1;
	}
	else
	{
		result = 0;
	}
	return result;
}


posnum Edit::get_source_end(posnum default_)
{
	return default_;
}

void Edit::insert_transition(const char *title)
{
	transition = new Transition(edl, 
		this, 
		title, 
		track->to_units(edl->session->default_transition_length, 1));
}

void Edit::detach_transition()
{
	if(transition) delete transition;
	transition = 0;
}

int Edit::silence()
{
	if(asset) 
		return 0;
	else
		return 1;
}


void Edit::copy_from(Edit *edit)
{
	this->asset = edl->assets->update(edit->asset);
	this->startsource = edit->startsource;
	this->startproject = edit->startproject;
	this->length = edit->length;
	strcpy (this->user_title, edit->user_title);

	if(edit->transition)
	{
		if(!transition) transition = new Transition(edl, 
			this, 
			edit->transition->title,
			edit->transition->length);
		*this->transition = *edit->transition;
	}
	this->channel = edit->channel;
}

void Edit::equivalent_output(Edit *edit, posnum *result)
{
// End of edit changed
	if(startproject + length != edit->startproject + edit->length)
	{
		int64_t new_length = MIN(startproject + length, edit->startproject + edit->length);
		if(*result < 0 || new_length < *result) 
			*result = new_length;
	}

// Start of edit changed
	if(
// One is silence and one isn't
		edit->asset == 0 && asset != 0 ||
		edit->asset != 0 && asset == 0 ||
// One has transition and one doesn't
		edit->transition == 0 && transition != 0 ||
		edit->transition != 0 && transition == 0 ||
// Position changed
		startproject != edit->startproject ||
		startsource != edit->startsource ||
// Transition changed
		(transition && 
			edit->transition && 
			!transition->identical(edit->transition)) ||
// Asset changed
		(asset && 
			edit->asset &&
			!asset->equivalent(*edit->asset, 1, 1))
		)
	{
		if(*result < 0 || startproject < *result) *result = startproject;
	}
}


Edit& Edit::operator=(Edit& edit)
{
	copy_from(&edit);
	return *this;
}

void Edit::synchronize_params(Edit *edit)
{
	copy_from(edit);
}


// Comparison for ResourcePixmap drawing
int Edit::identical(Edit &edit)
{
	int result = (this->asset == edit.asset &&
	this->startsource == edit.startsource &&
	this->startproject == edit.startproject &&
	this->length == edit.length &&
	this->transition == edit.transition &&
	this->channel == edit.channel);
	return result;
}

int Edit::operator==(Edit &edit)
{
	return identical(edit);
}

double Edit::frames_per_picon()
{
	return Units::round(picon_w()) / frame_w();
}

double Edit::frame_w()
{
	return track->from_units(1) * 
		edl->session->sample_rate / 
		edl->local_session->zoom_sample;
}

double Edit::picon_w()
{
	return (double)edl->local_session->zoom_track * 
		asset->width / 
		asset->height;
}

int Edit::picon_h()
{
	return edl->local_session->zoom_track;
}


int Edit::dump()
{
	printf("     EDIT %p\n", this);
	printf("      asset %p\n", asset);
	printf("      channel %d\n", channel);
	if(transition) 
	{
		printf("      TRANSITION %p\n", transition);
		transition->dump();
	}
	printf("      startsource %lld startproject %lld length %lld\n", startsource, startproject, length);
	fflush(stdout);
	return 0;
}

int Edit::load_properties(FileXML *file, posnum &startproject)
{
	startsource = file->tag.get_property("STARTSOURCE", (int64_t)0);
	length = file->tag.get_property("LENGTH", (int64_t)0);
	user_title[0] = 0;
	file->tag.get_property("USER_TITLE", user_title);
	this->startproject = startproject;
	load_properties_derived(file);
	return 0;
}

void Edit::shift(posnum difference)
{
	startproject += difference;
}

int Edit::shift_start_in(int edit_mode, 
	posnum newposition, 
	posnum oldposition,
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	Edits *trim_edits)
{
	posnum cut_length = newposition - oldposition;
	posnum end_previous_source, end_source;

	if(edit_mode == MOVE_ALL_EDITS)
	{
		if(cut_length < length)
		{        // clear partial 
			edits->clear_recursive(oldposition, 
				newposition,
				edit_edits,
				edit_labels,
				edit_plugins,
				trim_edits);
		}
		else
		{        // clear entire
			edits->clear_recursive(oldposition, 
				startproject + length,
				edit_edits,
				edit_labels,
				edit_plugins,
				trim_edits);
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
// Paste silence and cut
		if(!previous)
		{
			Edit *new_edit = edits->create_edit();
			new_edit->startproject = this->startproject;
			new_edit->length = 0;
			edits->insert_before(this, 
				new_edit);
		}

		end_previous_source = previous->get_source_end(previous->startsource + previous->length + cut_length);
		if(end_previous_source > 0 && 
			previous->startsource + previous->length + cut_length > end_previous_source)
			cut_length = end_previous_source - previous->startsource - previous->length;

		if(cut_length < length)
		{		// Move in partial
			startproject += cut_length;
			startsource += cut_length;
			length -= cut_length;
			previous->length += cut_length;
		}
		else
		{		// Clear entire edit
			cut_length = length;
			previous->length += cut_length;
			for(Edit* current_edit = this; current_edit; current_edit = current_edit->next)
			{
				current_edit->startproject += cut_length;
			}
			edits->clear_recursive(oldposition + cut_length, 
				startproject + cut_length,
				edit_edits,
				edit_labels,
				edit_plugins,
				trim_edits);
		}
	}
	else
	if(edit_mode == MOVE_NO_EDITS)
	{
		end_source = get_source_end(startsource + length + cut_length);
		if(end_source > 0 && startsource + length + cut_length > end_source)
			cut_length = end_source - startsource - length;
		
		startsource += cut_length;
	}
	return 0;
}

int Edit::shift_start_out(int edit_mode, 
	posnum newposition,
	posnum oldposition,
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	Edits *trim_edits)
{
	posnum cut_length = oldposition - newposition;

	if(asset)
	{
		posnum end_source = get_source_end(1);

		if(end_source > 0 && startsource < cut_length)
		{
			cut_length = startsource;
		}
	}

	if(edit_mode == MOVE_ALL_EDITS)
	{
		startsource -= cut_length;
		length += cut_length;

		edits->shift_keyframes_recursive(startproject, 
			cut_length);
		if(edit_plugins)
			edits->shift_effects_recursive(startproject, 
				cut_length);

		for(Edit* current_edit = next; current_edit; current_edit = current_edit->next)
		{
			current_edit->startproject += cut_length;
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
		if(previous)
		{
			if(cut_length < previous->length)
			{   // Cut into previous edit
				previous->length -= cut_length;
				startproject -= cut_length;
				startsource -= cut_length;
				length += cut_length;
			}
			else
			{   // Clear entire previous edit
				cut_length = previous->length;
				previous->length = 0;
				length += cut_length;
				startsource -= cut_length;
				startproject -= cut_length;
			}
		}
	}
	else
	if(edit_mode == MOVE_NO_EDITS)
	{
		startsource -= cut_length;
	}

// Fix infinite length files
	if(startsource < 0) startsource = 0;
	return 0;
}

int Edit::shift_end_in(int edit_mode, 
	posnum newposition, 
	posnum oldposition,
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	Edits *trim_edits)
{
	posnum cut_length = oldposition - newposition;

	if(edit_mode == MOVE_ALL_EDITS)
	{
		if(newposition > startproject)
		{        // clear partial edit
			edits->clear_recursive(newposition, 
				oldposition,
				edit_edits,
				edit_labels,
				edit_plugins,
				trim_edits);
		}
		else
		{        // clear entire edit
			edits->clear_recursive(startproject, 
				oldposition,
				edit_edits,
				edit_labels,
				edit_plugins,
				trim_edits);
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
		if(next)
		{
			if(next->asset)
			{
				posnum end_source = next->get_source_end(1);

				if(end_source > 0 && next->startsource - cut_length < 0)
				{
					cut_length = next->startsource;
				}
			}

			if(cut_length < length)
			{
				length -= cut_length;
				next->startproject -= cut_length;
				next->startsource -= cut_length;
				next->length += cut_length;
			}
			else
			{
				cut_length = length;
				next->length += cut_length;
				next->startsource -= cut_length;
				next->startproject -= cut_length;
				length -= cut_length;
			}
		}
		else
		{
			if(cut_length < length)
			{
				length -= cut_length;
			}
			else
			{
				cut_length = length;
				edits->clear_recursive(startproject, 
					oldposition,
					edit_edits,
					edit_labels,
					edit_plugins,
					trim_edits);
			}
		}
	}
	else
// Does nothing for plugins
	if(edit_mode == MOVE_NO_EDITS)
	{
		posnum end_source = get_source_end(1);
		if(end_source > 0 && startsource < cut_length)
		{
			cut_length = startsource;
		}
		startsource -= cut_length;
	}
	return 0;
}

int Edit::shift_end_out(int edit_mode, 
	posnum newposition, 
	posnum oldposition,
	int edit_edits,
	int edit_labels,
	int edit_plugins,
	Edits *trim_edits)
{
	posnum cut_length = newposition - oldposition;
	posnum endsource = get_source_end(startsource + length + cut_length);

// check end of edit against end of source file
	if(endsource > 0 && startsource + length + cut_length > endsource)
		cut_length = endsource - startsource - length;

	if(edit_mode == MOVE_ALL_EDITS)
	{
// Extend length
		this->length += cut_length;

// Effects are shifted in length extension
		if(edit_plugins)
			edits->shift_effects_recursive(oldposition /* startproject */, 
				cut_length);
		edits->shift_keyframes_recursive(oldposition /* startproject */, 
			cut_length);

		for(Edit* current_edit = next; current_edit; current_edit = current_edit->next)
		{
			current_edit->startproject += cut_length;
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
		if(next)
		{
			if(cut_length < next->length)
			{
				length += cut_length;
				next->startproject += cut_length;
				next->startsource += cut_length;
				next->length -= cut_length;
			}
			else
			{
				cut_length = next->length;
				next->length = 0;
				length += cut_length;
			}
		}
		else
		{
			length += cut_length;
		}
	}
	else
	if(edit_mode == MOVE_NO_EDITS)
	{
		startsource += cut_length;
	}
	return 0;
}






int Edit::popup_transition(float view_start, float zoom_units, int cursor_x, int cursor_y)
{
	posnum left, right, left_unit, right_unit;
	if(!transition) return 0;
	get_handle_parameters(left, right, left_unit, right_unit, view_start, zoom_units);

	if(cursor_x > left && cursor_x < right)
	{
		return 1;
	}
	return 0;
}

int Edit::select_handle(float view_start, float zoom_units, int cursor_x, int cursor_y, int64_t &selection)
{
	posnum left, right, left_unit, right_unit;
	get_handle_parameters(left, right, left_unit, right_unit, view_start, zoom_units);

	posnum pixel1, pixel2;
	pixel1 = left;
	pixel2 = pixel1 + 10;

// test left edit
// cursor_x is faked in acanvas
	if(cursor_x >= pixel1 && cursor_x <= pixel2)
	{
		selection = left_unit;
		return 1;     // left handle
	}

	posnum endproject = startproject + length;
	pixel2 = right;
	pixel1 = pixel2 - 10;

// test right edit
	if(cursor_x >= pixel1 && cursor_x <= pixel2)
	{
		selection = right_unit;
		return 2;     // right handle
	}
	return 0;
}

