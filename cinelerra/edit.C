
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

Edit::Edit(EDL *edl, Track *track)
{
	edits = 0;
	source_pts = 0;
	project_pts = 0;
	asset = 0;
	transition = 0;
	channel = 0;
	user_title[0] = 0;
	this->edl = edl;
	this->track = track;
	if(track) this->edits = track->edits;
	id = EDL::next_id();
}

Edit::Edit(EDL *edl, Edits *edits)
{
	track = 0;
	source_pts = 0;
	project_pts = 0;
	asset = 0;
	transition = 0;
	channel = 0;
	user_title[0] = 0;
	this->edl = edl;
	this->edits = edits;
	if(edits) this->track = edits->track;
	id = EDL::next_id();
}

Edit::~Edit()
{
	if(transition) delete transition;
}

void Edit::copy(FileXML *file, const char *output_path)
{
	if(file)    // only if not counting
	{
		file->tag.set_title("EDIT");
		file->tag.set_property("PTS", project_pts);

		if(asset)
		{
			file->tag.set_property("SOURCE_PTS", source_pts);
			file->tag.set_property("CHANNEL", channel);
		}
		if(user_title[0]) file->tag.set_property("USER_TITLE", user_title);

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
			transition->save_xml(file);

		file->tag.set_title("/EDIT");
		file->append_tag();
		file->append_newline();
	}
}


ptstime Edit::get_source_end(ptstime default_value)
{
	return default_value;
}

void Edit::insert_transition(const char *title, KeyFrame *default_keyframe)
{
	transition = new Transition(edl, 
		this, 
		title, 
		edl->session->default_transition_length, default_keyframe);
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
			edit->transition->length(), 0);
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
	if(asset && asset->width && asset->height)
		return (double)edl->local_session->zoom_track *
			asset->width / asset->height;
	return 0.0;
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

void Edit::dump(int indent)
{
	printf("%*sEdit %p: project_pts %.3f source_pts %.3f length %.3f\n",
		indent, "", this, project_pts,  source_pts, length());
	indent += 2;
	if(asset)
		printf("%*sasset %p channel %d\n", indent, "", asset, channel);
	if(transition)
		transition->dump(indent);
}

ptstime Edit::load_properties(FileXML *file, ptstime project_pts)
{
	posnum startsource, length;
	ptstime length_time = -1;
	ptstime cur_pts;

	startsource = file->tag.get_property("STARTSOURCE", (posnum)0);
	if(startsource)
		source_pts = track->from_units(startsource);
	source_pts = file->tag.get_property("SOURCE_PTS", source_pts);
	cur_pts = file->tag.get_property("PTS", -1.0);
	if(cur_pts < -EPSILON)
	{
		length = file->tag.get_property("LENGTH", (int64_t)0);
		if(length)
			length_time = track->from_units(length);
		length_time = file->tag.get_property("LENGTH_TIME", length_time);
		this->project_pts = project_pts;
	}
	else
		this->project_pts = cur_pts;
	user_title[0] = 0;
	file->tag.get_property("USER_TITLE", user_title);
	channel = file->tag.get_property("CHANNEL", (int32_t)0);
	return length_time;
}

void Edit::shift(ptstime difference)
{
	ptstime new_pts = project_pts + difference;

	if(edl)
		project_pts = edl->align_to_frame(new_pts);
	else
		project_pts = new_pts;
}

void Edit::shift_source(ptstime difference)
{
	ptstime new_pts = source_pts += difference;

	if(asset && track)
		source_pts = asset->align_to_frame(new_pts, track->data_type);
	else
		source_pts = new_pts;
}

ptstime Edit::set_pts(ptstime pts)
{
	if(edl)
		project_pts = edl->align_to_frame(pts);
	else
		project_pts = pts;
	return project_pts;
}

ptstime Edit::get_pts()
{
	return project_pts;
}

ptstime Edit::set_source_pts(ptstime pts)
{
	if(asset && track)
		source_pts = asset->align_to_frame(pts, track->data_type);
	else
		source_pts = pts;
	return source_pts;
}

ptstime Edit::get_source_pts()
{
	return source_pts;
}
