
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
#include "bcsignals.h"
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


Edit::Edit(void)
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

void Edit::reset(void)
{
	edl = 0;
	track = 0;
	edits = 0;
	source_pts = 0;
	project_pts = 0;
	asset = 0;
	transition = 0;
	channel = 0;
	user_title[0] = 0;
}

int Edit::copy(ptstime start, ptstime end, FileXML *file, const char *output_path)
{
	ptstime endpts = end_pts();
	int result;

	if((project_pts >= start && project_pts <= end) ||  // startproject in range
		(endpts <= end && endpts >= start) ||   // endproject in range
		(project_pts <= start && endpts >= end))    // range in project
	{
// edit is in range
		ptstime startproj_in_selection = project_pts; // start of edit in selection in project
		ptstime startsrc_in_selection = source_pts; // start of source in selection in source
		ptstime len_in_selection = length();             // length of edit in selection

		if(project_pts < start)
		{         // start is after start of edit in project
			ptstime len_difference = start - project_pts;

			startsrc_in_selection += len_difference;
			startproj_in_selection += len_difference;
			len_in_selection -= len_difference;
		}

		if(endpts > end)
		{         // end is before end of edit in project
			len_in_selection = end - startproj_in_selection;
		}

		if(file)    // only if not counting
		{
			file->tag.set_title("EDIT");
			file->tag.set_property("SOURCE_PTS", startsrc_in_selection);
			file->tag.set_property("CHANNEL", (int64_t)channel);
			file->tag.set_property("LENGTH_TIME", len_in_selection);
			if(user_title[0]) file->tag.set_property("USER_TITLE", user_title);

			copy_properties_derived(file, len_in_selection);

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


ptstime Edit::get_source_end(ptstime default_value)
{
	return default_value;
}

void Edit::insert_transition(const char *title)
{
	transition = new Transition(edl, 
		this, 
		title, 
		edl->session->default_transition_length);
}

void Edit::detach_transition(void)
{
	if(transition) delete transition;
	transition = 0;
}

int Edit::silence(void)
{
	if(asset) 
		return 0;
	else
		return 1;
}


void Edit::copy_from(Edit *edit)
{
	this->asset = edl->assets->update(edit->asset);
	this->source_pts = edit->source_pts;
	this->project_pts = edit->project_pts;
	strcpy (this->user_title, edit->user_title);

	if(edit->transition)
	{
		if(!transition) transition = new Transition(edl, 
			this, 
			edit->transition->title,
			edit->transition->length());
		*this->transition = *edit->transition;
	}
	this->channel = edit->channel;
}

void Edit::equivalent_output(Edit *edit, ptstime *result)
{
// End of edit changed
	ptstime curend = end_pts();
	ptstime edend = edit->end_pts();

	if(!PTSEQU(curend, edend))
	{
		ptstime new_length = MIN(curend, edend);
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
		!PTSEQU(project_pts, edit->project_pts) ||
		!PTSEQU(source_pts, edit->source_pts) ||
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
		if(*result < 0 || project_pts < *result)
			*result = project_pts;
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
	ptstime len = this->length();
	ptstime elen = edit.length();

	int result = (this->asset == edit.asset &&
		PTSEQU(this->source_pts, edit.source_pts) &&
		PTSEQU(this->project_pts, edit.project_pts) &&
		PTSEQU(len, elen) &&
		this->transition == edit.transition &&
		this->channel == edit.channel);
	return result;
}

int Edit::operator==(Edit &edit)
{
	return identical(edit);
}

double Edit::picon_w(void)
{
	return (double)edl->local_session->zoom_track * 
		asset->width / 
		asset->height;
}

int Edit::picon_h(void)
{
	return edl->local_session->zoom_track;
}

ptstime Edit::length(void)
{
	if(next)
		return next->project_pts - project_pts;
	return 0;
}

ptstime Edit::end_pts(void)
{
	if(next)
		return next->project_pts;
	return project_pts;
}

void Edit::dump(void)
{
	printf("     EDIT %p\n", this);
	printf("      asset %p\n", asset);
	printf("      channel %d\n", channel);
	if(transition) 
	{
		printf("      TRANSITION %p\n", transition);
		transition->dump();
	}
	printf("      source_pts %.3f project_pts %.3f length %.3f\n",
		source_pts, project_pts, length());
	fflush(stdout);
}

ptstime Edit::load_properties(FileXML *file, ptstime project_pts)
{
	posnum startsource, length;
	ptstime length_time;

	startsource = file->tag.get_property("STARTSOURCE", (posnum)0);
	if(startsource)
		source_pts = track->from_units(startsource);
	source_pts = file->tag.get_property("SOURCE_PTS", source_pts);
	length = file->tag.get_property("LENGTH", (int64_t)0);
	if(length)
		length_time = track->from_units(length);
	length_time = file->tag.get_property("LENGTH_TIME", length_time);
	user_title[0] = 0;
	file->tag.get_property("USER_TITLE", user_title);
	this->project_pts = project_pts;
	load_properties_derived(file);
	return length_time;
}

void Edit::shift(ptstime difference)
{
	project_pts += difference;
}

void Edit::shift_start_in(int edit_mode, 
	ptstime newpostime,
	ptstime oldpostime,
	int actions,
	Edits *trim_edits)
{
	ptstime cut_length = newpostime - oldpostime;
	ptstime end_previous_source, end_source;

	if(edit_mode == MOVE_ALL_EDITS)
	{
		if(cut_length < length())
		{        // clear partial 
			edits->clear_recursive(oldpostime,
				newpostime,
				actions,
				trim_edits);
		}
		else
		{        // clear entire
			edits->clear_recursive(oldpostime,
				end_pts(),
				actions,
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
			new_edit->project_pts = this->project_pts;
			edits->insert_before(this, 
				new_edit);
		}

		end_previous_source = previous->get_source_end(previous->source_pts + previous->length() + cut_length);
		if(end_previous_source > 0 && 
			previous->source_pts + previous->length() + cut_length > end_previous_source)
			cut_length = end_previous_source - previous->end_pts();

		if(cut_length < length())
		{		// Move in partial
			project_pts += cut_length;
			source_pts += cut_length;
		}
		else
		{		// Clear entire edit
			cut_length = length();
			for(Edit* current_edit = this; current_edit; current_edit = current_edit->next)
			{
				current_edit->project_pts += cut_length;
			}
			edits->clear_recursive(oldpostime + cut_length, 
				project_pts + cut_length,
				actions,
				trim_edits);
		}
	}
	else
	if(edit_mode == MOVE_NO_EDITS)
	{
		end_source = get_source_end(end_pts() + cut_length);
		if(end_source > 0 && end_pts() + cut_length > end_source)
			cut_length = end_source - source_pts - length();

		source_pts += cut_length;
	}
}

void Edit::shift_start_out(int edit_mode, 
	ptstime newpostime,
	ptstime oldpostime,
	int actions,
	Edits *trim_edits)
{
	ptstime cut_length = oldpostime - newpostime;

	if(asset)
	{
		ptstime end_source = get_source_end(1);

		if(end_source > 0 && source_pts < cut_length)
		{
			cut_length = source_pts;
		}
	}

	if(edit_mode == MOVE_ALL_EDITS)
	{
		source_pts -= cut_length;

		edits->shift_keyframes_recursive(project_pts, 
			cut_length);
		if(actions & EDIT_PLUGINS)
			edits->shift_effects_recursive(project_pts,
				cut_length);

		for(Edit* current_edit = next; current_edit; current_edit = current_edit->next)
		{
			current_edit->project_pts += cut_length;
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
		if(previous)
		{
			if(cut_length < previous->length())
			{   // Cut into previous edit
				project_pts -= cut_length;
				source_pts -= cut_length;
			}
			else
			{   // Clear entire previous edit
				cut_length = previous->length();
				source_pts -= cut_length;
				project_pts -= cut_length;
			}
		}
	}
	else
	if(edit_mode == MOVE_NO_EDITS)
	{
		source_pts -= cut_length;
	}

// Fix infinite length files
	if(source_pts < 0) 
		source_pts = 0;
}

void Edit::shift_end_in(int edit_mode, 
	ptstime newpostime,
	ptstime oldpostime,
	int actions,
	Edits *trim_edits)
{
	ptstime cut_length = oldpostime - newpostime;

	if(edit_mode == MOVE_ALL_EDITS)
	{
		if(newpostime > project_pts)
		{        // clear partial edit
			edits->clear_recursive(newpostime,
				oldpostime,
				actions,
				trim_edits);
		}
		else
		{        // clear entire edit
			edits->clear_recursive(project_pts,
				oldpostime,
				actions,
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
				ptstime end_source = next->get_source_end(1);

				if(end_source > 0 && next->source_pts - cut_length < 0)
				{
					cut_length = next->source_pts;
				}
			}

			if(cut_length < length())
			{
				next->project_pts -= cut_length;
				next->source_pts -= cut_length;
			}
			else
			{
				cut_length = length();
				next->source_pts -= cut_length;
				next->project_pts -= cut_length;
			}
		}
		else
		{
			edits->clear_recursive(project_pts,
				oldpostime,
				actions,
				trim_edits);
		}
	}
	else
// Does nothing for plugins
	if(edit_mode == MOVE_NO_EDITS)
	{
		ptstime end_source = get_source_end(1);
		if(end_source > 0 && source_pts < cut_length)
		{
			cut_length = source_pts;
		}
		source_pts -= cut_length;
	}
}

void Edit::shift_end_out(int edit_mode, 
	ptstime newpostime,
	ptstime oldpostime,
	int actions,
	Edits *trim_edits)
{
	ptstime cut_length = newpostime - oldpostime;
	ptstime endsource = get_source_end(source_pts + length() + cut_length);

// check end of edit against end of source file
	if(endsource > 0 && source_pts + length() + cut_length > endsource)
		cut_length = endsource - source_pts - length();

	if(edit_mode == MOVE_ALL_EDITS)
	{

// Effects are shifted in length extension
		if(actions & EDIT_PLUGINS)
			edits->shift_effects_recursive(oldpostime,
				cut_length);
		edits->shift_keyframes_recursive(oldpostime,
			cut_length);

		for(Edit* current_edit = next; current_edit; current_edit = current_edit->next)
		{
			current_edit->project_pts += cut_length;
		}
	}
	else
	if(edit_mode == MOVE_ONE_EDIT)
	{
		if(next)
		{
			if(cut_length < next->length())
			{
				next->project_pts += cut_length;
				next->source_pts += cut_length;
			}
			else
			{
				cut_length = next->length();
				next->project_pts += cut_length;
			}
		}
	}
	else
	if(edit_mode == MOVE_NO_EDITS)
	{
		source_pts += cut_length;
	}
}
