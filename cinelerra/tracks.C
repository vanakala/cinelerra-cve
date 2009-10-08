
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
#include "automation.h"
#include "cursor.h"
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
#include "transportque.inc"
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
}


Tracks::~Tracks()
{
	delete_all_tracks();
}






void Tracks::equivalent_output(Tracks *tracks, double *result)
{
	if(total_playable_vtracks() != tracks->total_playable_vtracks())
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




void Tracks::get_affected_edits(ArrayList<Edit*> *drag_edits, double position, Track *start_track)
{
	drag_edits->remove_all();

	for(Track *track = start_track;
		track;
		track = track->next)
	{
//printf("Tracks::get_affected_edits 1 %p %d %d\n", track, track->data_type, track->record);
		if(track->record)
		{
			for(Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				double startproject = track->from_units(edit->startproject);
//printf("Tracks::get_affected_edits 1 %d\n", edl->equivalent(startproject, position));
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
	double start,
	double end,
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
				current->to_units(start, 0),
				current->to_units(end, 1),
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
printf("Tracks::operator= 1\n");
	copy_from(&tracks);
	return *this;
}

int Tracks::load(FileXML *xml, int &track_offset, uint32_t load_flags)
{
// add the appropriate type of track
	char string[BCTEXTLEN];
	Track *track = 0;
	sprintf(string, "");
	
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

// load it
	if(track) track->load(xml, track_offset, load_flags);

	return 0;
}

Track* Tracks::add_audio_track(int above, Track *dst_track)
{
	int pixel;
	ATrack* new_track = new ATrack(edl, this);
	if(!dst_track)
	{
		dst_track = (above ? first : last);
	}

	if(above)
	{
		insert_before(dst_track, (Track*)new_track);
	}
	else
	{
		insert_after(dst_track, (Track*)new_track);
// Shift effects referenced below the destination track
	}

// Shift effects referenced below the new track
	for(Track *track = last; 
		track && track != new_track; 
		track = track->previous)
	{
		change_modules(number_of(track) - 1, number_of(track), 0);
	}


	new_track->create_objects();
	new_track->set_default_title();

	int current_pan = 0;
	for(Track *current = first; 
		current != (Track*)new_track; 
		current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO) current_pan++;
		if(current_pan >= edl->session->audio_channels) current_pan = 0;
	}



	PanAuto* pan_auto = 
		(PanAuto*)new_track->automation->autos[AUTOMATION_PAN]->default_auto;
	pan_auto->values[current_pan] = 1.0;

	BC_Pan::calculate_stick_position(edl->session->audio_channels, 
		edl->session->achannel_positions, 
		pan_auto->values, 
		MAX_PAN, 
		PAN_RADIUS,
		pan_auto->handle_x,
		pan_auto->handle_y);
	return new_track;
}

Track* Tracks::add_video_track(int above, Track *dst_track)
{
	int pixel;
	VTrack* new_track = new VTrack(edl, this);
	if(!dst_track)
		dst_track = (above ? first : last);

	if(above)
	{
		insert_before(dst_track, (Track*)new_track);
	}
	else
	{
		insert_after(dst_track, (Track*)new_track);
	}



// Shift effects referenced below the new track
	for(Track *track = last; 
		track && track != new_track; 
		track = track->previous)
	{
		change_modules(number_of(track) - 1, number_of(track), 0);
	}



	new_track->create_objects();
	new_track->set_default_title();
	return new_track;
}


int Tracks::delete_track(Track *track)
{
	if (!track)
		return 0;

	int old_location = number_of(track);
	detach_shared_effects(old_location);

// Shift effects referencing effects below the deleted track
	for(Track *current = track; 
		current;
		current = NEXT)
	{
		change_modules(number_of(current), number_of(current) - 1, 0);
	}
	if(track) delete track;

	return 0;
}

int Tracks::detach_shared_effects(int module)
{
	for(Track *current = first; current; current = NEXT)
	{
		current->detach_shared_effects(module);
	}
 
 	return 0;
 }

int Tracks::total_of(int type)
{
	int result = 0;
	IntAuto *mute_keyframe = 0;
	
	for(Track *current = first; current; current = NEXT)
	{
		long unit_start = current->to_units(edl->local_session->get_selectionstart(1), 0);
		mute_keyframe = 
			(IntAuto*)current->automation->autos[AUTOMATION_MUTE]->get_prev_auto(
			unit_start, 
			PLAY_FORWARD,
			(Auto* &)mute_keyframe);

		result += 
			(current->play && type == PLAY) ||
			(current->record && type == RECORD) ||
			(current->gang && type == GANG) ||
			(current->draw && type == DRAW) ||
			(mute_keyframe->value && type == MUTE) ||
			(current->expand_view && type == EXPAND);
	}
	return result;
}

int Tracks::recordable_audio_tracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
		if(current->data_type == TRACK_AUDIO && current->record) result++;
	return result;
}

int Tracks::recordable_video_tracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO && current->record) result++;
	}
	return result;
}


int Tracks::playable_audio_tracks()
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO && current->play)
		{
			result++;
		}
	}

	return result;
}

int Tracks::playable_video_tracks()
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO && current->play)
		{
			result++;
		}
	}
	return result;
}

int Tracks::total_audio_tracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
		if(current->data_type == TRACK_AUDIO) result++;
	return result;
}

int Tracks::total_video_tracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
		if(current->data_type == TRACK_VIDEO) result++;
	return result;
}

double Tracks::total_playable_length() 
{
	double total = 0;
	for(Track *current = first; current; current = NEXT)
	{
		double length = current->get_length();
		if(length > total) total = length;
	}
	return total; 
}

double Tracks::total_recordable_length() 
{
	double total = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->record)
		{
			double length = current->get_length();
			if(length > total) total = length;
		}
	}
	return total; 
}

double Tracks::total_length() 
{
	double total = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->get_length() > total) total = current->get_length();
	}
	return total; 
}

double Tracks::total_audio_length() 
{
	double total = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO &&
			current->get_length() > total) total = current->get_length();
	}
	return total; 
}

double Tracks::total_video_length() 
{
	double total = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO &&
			current->get_length() > total) total = current->get_length();
	}
	return total; 
}

double Tracks::total_length_framealigned(double fps)
{
	if (total_audio_tracks() && total_video_tracks())
		return MIN(floor(total_audio_length() * fps), floor(total_video_length() * fps)) / fps;

	if (total_audio_tracks())
		return floor(total_audio_length() * fps) / fps;

	if (total_video_tracks())
		return floor(total_video_length() * fps) / fps;

	return 0;
}

void Tracks::translate_camera(float offset_x, float offset_y)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO)
		{
			((VTrack*)current)->translate(offset_x, offset_y, 1);
		}
	}
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
//printf("Tracks::update_y_pixels %d\n", y);
		current->y_pixel = y;
		y += current->vertical_span(theme);
	}
}

int Tracks::dump()
{
	for(Track* current = first; current; current = NEXT)
	{
		printf("  Track: %x\n", current);
		current->dump();
		printf("\n");
	}
	return 0;
}

void Tracks::select_all(int type,
		int value)
{
	for(Track* current = first; current; current = NEXT)
	{
		double position = edl->local_session->get_selectionstart(1);

		if(type == PLAY) current->play = value;
		if(type == RECORD) current->record = value;
		if(type == GANG) current->gang = value;
		if(type == DRAW) current->draw = value;
		
		if(type == MUTE)
		{
			((IntAuto*)current->automation->autos[AUTOMATION_MUTE]->get_auto_for_editing(position))->value = value;
		}

		if(type == EXPAND) current->expand_view = value;
	}
}










































// ===================================== file operations

int Tracks::popup_transition(int cursor_x, int cursor_y)
{
	int result = 0;
	for(Track* current = first; current && !result; current = NEXT)
	{
		result = current->popup_transition(cursor_x, cursor_y);
	}
	return result;
}


int Tracks::change_channels(int oldchannels, int newchannels)
{
	for(Track *current = first; current; current = NEXT)
	{ current->change_channels(oldchannels, newchannels); }
	return 0;
}



int Tracks::totalpixels()
{
	int result = 0;
	for(Track* current = first; current; current = NEXT)
	{
		result += edl->local_session->zoom_track;
	}
	return result;
}

int Tracks::number_of(Track *track)
{
	int i = 0;
	for(Track *current = first; current && current != track; current = NEXT)
	{
		i++;
	}
	return i;
}

Track* Tracks::number(int number)
{
	Track *current;
	int i = 0;
	for(current = first; current && i < number; current = NEXT)
	{
		i++;
	}
	return current;
}


int Tracks::total_playable_vtracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO && current->play) result++;
	}
	return result;
}
