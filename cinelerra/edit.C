// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "keyframe.h"
#include "localsession.h"
#include "plugin.h"
#include "mainsession.h"
#include "tracks.h"
#include <string.h>

Edit::Edit(EDL *edl, Track *track)
 : ListItem<Edit>()
{
	edits = 0;
	source_pts = 0;
	project_pts = 0;
	asset = 0;
	stream = -1;
	transition = 0;
	channel = 0;
	this->edl = edl;
	this->track = track;
	if(track) this->edits = track->edits;
	id = EDL::next_id();
}

Edit::Edit(EDL *edl, Edits *edits)
 : ListItem<Edit>()
{
	track = 0;
	source_pts = 0;
	project_pts = 0;
	asset = 0;
	stream = -1;
	transition = 0;
	channel = 0;
	this->edl = edl;
	this->edits = edits;
	if(edits) this->track = edits->track;
	id = EDL::next_id();
}

Edit::~Edit()
{
	if(transition) delete transition;
}

void Edit::save_xml(FileXML *file, const char *output_path, int track_type)
{
	if(file)    // only if not counting
	{
		file->tag.set_title("EDIT");
		file->tag.set_property("PTS", project_pts);

		if(asset)
		{
			file->tag.set_property("SOURCE_PTS", source_pts);
			file->tag.set_property("CHANNEL", channel);
			file->tag.set_property("STREAMNO", stream + 1);
		}

		file->append_tag();

		if(asset)
		{
			char *store_path = asset->path;
			FileSystem fs;

			if(output_path)
			{
				int pathlen;
				char output_directory[BCTEXTLEN];

				fs.extract_dir(output_directory, output_path);
				pathlen = strlen(output_directory);

				if(strncmp(asset->path, output_path, pathlen) == 0)
					store_path = &asset->path[pathlen];
			}

			file->tag.set_title("FILE");
			file->tag.set_property("SRC", store_path);
			file->append_tag();
			file->tag.set_title("/FILE");
			file->append_tag();
			file->append_newline();
		}

		if(transition)
			transition->save_xml(file);

		file->tag.set_title("/EDIT");
		file->append_tag();
		file->append_newline();
	}
}

ptstime Edit::source_duration()
{
	if(asset && stream >= 0)
		return asset->stream_duration(stream);
	return 0;
}

Plugin *Edit::insert_transition(PluginServer *server)
{
	if(!transition)
	{
		transition = new Plugin(edl,
			track,
			server);
		transition->plugin_type = PLUGIN_TRANSITION;
		transition->set_length(edlsession->default_transition_length);
	}
	else
		transition->get_keyframe(0)->set_data(0);

	transition->plugin_server = server;
	return transition;
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
	if(edit == this)
		return;

	if(edl)
		edl->update_assets(edit->asset);
	this->asset = edit->asset;
	this->stream = edit->stream;
	this->source_pts = edit->source_pts;
	this->project_pts = edit->project_pts;

	if(edit->transition)
	{
		if(!transition)
			transition = new Plugin(edl,
				track,
				0);
		transition->copy_from(edit->transition);
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
			!asset->equivalent(*edit->asset, STRDSC_ALLTYP))
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

double Edit::picon_w()
{
	if(!asset)
		return 0;

	int width = asset->streams[stream].width;
	int height = asset->streams[stream].height;

	if(width && height)
		return (double)edl->local_session->zoom_track *
			width / height;
	return 0.0;
}

int Edit::picon_h()
{
	return edl->local_session->zoom_track;
}

ptstime Edit::length()
{
	if(next)
		return next->project_pts - project_pts;
	return 0;
}

ptstime Edit::end_pts()
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
		printf("%*sasset %p stream %d channel %d\n",
			indent, "", asset, stream, channel);
	if(transition)
		transition->dump(indent);
}

posnum Edit::load_properties(FileXML *file, ptstime project_pts)
{
	posnum startsource;
	posnum length = -1;
	ptstime cur_pts;
	int streamno;

	startsource = file->tag.get_property("STARTSOURCE", (posnum)0);
	if(startsource)
		source_pts = track->from_units(startsource);
	source_pts = file->tag.get_property("SOURCE_PTS", source_pts);
	cur_pts = file->tag.get_property("PTS", -1.0);
	if(cur_pts < -EPSILON)
	{
		length = file->tag.get_property("LENGTH", length);
		this->project_pts = project_pts;
	}
	else
		this->project_pts = cur_pts;
	channel = file->tag.get_property("CHANNEL", 0);
	streamno = file->tag.get_property("STREAMNO", -1);
	if(streamno > 0)
		stream = streamno - 1;
	return length;
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
	if(asset)
		source_pts += difference;
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
	return source_pts = pts;
}

ptstime Edit::get_source_pts()
{
	return source_pts;
}

ptstime Edit::source_end_pts()
{
	if(asset && next)
		return source_pts + next->project_pts - project_pts;
	return 0;
}

size_t Edit::get_size()
{
	size_t size = sizeof(*this);

	if(transition)
		size += transition->get_size();
	return size;
}
