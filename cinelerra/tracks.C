
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

#include "atrack.h"
#include "bcpan.h"
#include "automation.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "intauto.h"
#include "intautos.h"
#include "localsession.h"
#include "module.h"
#include "panauto.h"
#include "panautos.h"
#include "patchbay.h"
#include "mainsession.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "vtrack.h"
#include <string.h>

Tracks::Tracks(EDL *edl)
 : List<Track>()
{
	this->edl = edl;
}

Tracks::Tracks()
 : List<Track>()
{
    edl = 0;
}

Tracks::~Tracks()
{
	delete_all_tracks();
}

void Tracks::reset_instance()
{
	delete_all_tracks();
}

void Tracks::equivalent_output(Tracks *tracks, ptstime *result)
{
	if(playable_tracks_of(TRACK_VIDEO) != tracks->playable_tracks_of(TRACK_VIDEO))
	{
		*result = 0;
	}
	else
	{
		Track *current = first;
		Track *that_current = tracks->first;
		while(current || that_current)
		{
// Get next video track
			while(current && current->data_type != TRACK_VIDEO)
				current = NEXT;

			while(that_current && that_current->data_type != TRACK_VIDEO)
				that_current = that_current->next;

// One doesn't exist but the other does
			if((!current && that_current) ||
				(current && !that_current))
			{
				*result = 0;
				break;
			}
			else
// Both exist
			if(current && that_current)
			{
				current->equivalent_output(that_current, result);
				current = NEXT;
				that_current = that_current->next;
			}
		}
	}
}

void Tracks::get_affected_edits(ArrayList<Edit*> *drag_edits, ptstime position, Track *start_track)
{
	drag_edits->remove_all();

	for(Track *track = start_track;
		track;
		track = track->next)
	{
		if(track->record)
		{
			for(Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				ptstime startproject = edit->get_pts();
				if(edl->equivalent(startproject, position))
				{
					drag_edits->append(edit);
					break;
				}
			}
		}
	}

}

void Tracks::get_automation_extents(float *min, 
	float *max,
	ptstime start,
	ptstime end,
	int autogrouptype)
{
	*min = 0;
	*max = 0;
	int coords_undefined = 1;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->automation->get_extents(min,
				max,
				&coords_undefined,
				start,
				end,
				autogrouptype);
		}
	}
}

void Tracks::copy_from(Tracks *tracks)
{
	Track *new_track;

	delete_all_tracks();
	for(Track *current = tracks->first; current; current = NEXT)
	{
		switch(current->data_type)
		{
		case TRACK_AUDIO: 
			new_track = add_audio_track(0, 0); 
			break;
		case TRACK_VIDEO: 
			new_track = add_video_track(0, 0); 
			break;
		}
		new_track->copy_from(current);
	}
}

Tracks& Tracks::operator=(Tracks &tracks)
{
	copy_from(&tracks);
	return *this;
}

void Tracks::load(FileXML *xml, int &track_offset, uint32_t load_flags)
{
// add the appropriate type of track
	char string[BCTEXTLEN];
	Track *track = 0;

	string[0] = 0;

	xml->tag.get_property("TYPE", string);

	if((load_flags & LOAD_ALL) == LOAD_ALL ||
		(load_flags & LOAD_EDITS))
	{
		if(!strcmp(string, "VIDEO"))
		{
			add_video_track(0, 0);
		}
		else
		{
			add_audio_track(0, 0);    // default to audio
		}
		track = last;
	}
	else
	{
		track = get_item_number(track_offset);
		track_offset++;
	}

	if(track) track->load(xml, track_offset, load_flags);
}

Track* Tracks::add_audio_track(int above, Track *dst_track)
{
	ATrack* new_track = new ATrack(edl, this);

	if(!dst_track)
		dst_track = (above ? first : last);

	if(above)
		insert_before(dst_track, (Track*)new_track);
	else
		insert_after(dst_track, (Track*)new_track);

// Shift effects referenced below the new track
	for(Track *track = last; track && track != new_track; track = track->previous)
		change_modules(number_of(track) - 1, number_of(track), 0);

	new_track->set_default_title();

	int current_pan = 0;

	for(Track *current = first; 
		current != (Track*)new_track; 
		current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO) current_pan++;
		if(current_pan >= edlsession->audio_channels) current_pan = 0;
	}

	PanAutos* pan_autos = 
		(PanAutos*)new_track->automation->autos[AUTOMATION_PAN];

	pan_autos->default_values[current_pan] = 1.0;

	BC_Pan::calculate_stick_position(edlsession->audio_channels,
		edlsession->achannel_positions,
		pan_autos->default_values,
		MAX_PAN,
		PAN_RADIUS,
		pan_autos->default_handle_x,
		pan_autos->default_handle_y);
	return new_track;
}

Track* Tracks::add_video_track(int above, Track *dst_track)
{
	VTrack* new_track = new VTrack(edl, this);

	if(!dst_track)
		dst_track = (above ? first : last);

	if(above)
		insert_before(dst_track, (Track*)new_track);
	else
		insert_after(dst_track, (Track*)new_track);

// Shift effects referenced below the new track
	for(Track *track = last; track && track != new_track; track = track->previous)
		change_modules(number_of(track) - 1, number_of(track), 0);

	new_track->set_default_title();
	return new_track;
}

void Tracks::delete_track(Track *track)
{
	if(!track)
		return;

	detach_shared_effects(number_of(track));

// Shift effects referencing effects below the deleted track
	for(Track *current = track; current; current = NEXT)
		change_modules(number_of(current), number_of(current) - 1, 0);

	if(track) delete track;
}

void Tracks::detach_shared_effects(int module)
{
	for(Track *current = first; current; current = NEXT)
		current->detach_shared_effects(module);
}

int Tracks::recordable_tracks_of(int type)
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == type && current->record)
			result++;
	}
	return result;
}

int Tracks::playable_tracks_of(int type)
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == type && current->play)
		{
			result++;
		}
	}
	return result;
}

int Tracks::total_tracks_of(int type)
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == type)
			result++;
	}
	return result;
}

ptstime Tracks::total_length()
{
	ptstime total = 0;

	for(Track *current = first; current; current = NEXT)
	{
		ptstime length = current->get_length();
		if(length > total) total = length;
	}
	return total;
}

ptstime Tracks::total_length_of(int type)
{
	ptstime len, total = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == type)
		{
			len = current->get_length();
			if(len > total)
				total = len;
		}
	}
	return total;
}

ptstime Tracks::total_length_framealigned(double fps)
{
	int atracks, vtracks;
	ptstime alen, vlen;

	alen = vlen = 0;

	if(atracks = total_tracks_of(TRACK_AUDIO))
		alen = total_length_of(TRACK_AUDIO);

	if(vtracks = total_tracks_of(TRACK_VIDEO))
		vlen = total_length_of(TRACK_VIDEO);

	if(atracks && vtracks)
		return MIN(floor(alen * fps), floor(vlen * fps)) / fps;

	if(atracks)
		return floor(alen * fps) / fps;

	if(vtracks)
		return floor(vlen * fps) / fps;

	return 0;
}

void Tracks::translate_projector(float offset_x, float offset_y)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO)
		{
			((VTrack*)current)->translate(offset_x, offset_y, 0);
		}
	}
}

void Tracks::update_y_pixels(Theme *theme)
{
	int y = -edl->local_session->track_start;
	for(Track *current = first; current; current = NEXT)
	{
		current->y_pixel = y;
		y += current->vertical_span(theme);
	}
}

void Tracks::dump(int indent)
{
	printf("%*sTracks %p dump(%d):\n", indent, "", this, total());
	indent += 2;
	for(Track* current = first; current; current = NEXT)
	{
		current->dump(indent);
	}
}

int Tracks::totalpixels()
{
	int result = 0;

	for(Track* current = first; current; current = NEXT)
		result += edl->local_session->zoom_track;

	return result;
}
