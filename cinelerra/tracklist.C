
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

#include "intauto.h"
#include "intautos.h"
#include "tracklist.h"

TrackList::TrackList(MWindow *mwindow, long position, int reverse)
 : ArrayList<Track*>()
{
	this->mwindow = mwindow;
}

TrackList::get_playable_audio(long position, int reverse)
{
	get_playable_type(position, reverse, TRACK_AUDIO);
}

TrackList::get_playable_video(long position, int reverse)
{
	get_playable_type(position, reverse, TRACK_VIDEO);
}



TrackList::get_recordable_audio()
{
	get_recordable_type(TRACK_AUDIO);
}

TrackList::get_recordable_video()
{
	get_recordable_type(TRACK_VIDEO);
}

TrackList::get_playable_type(long position, int reverse, int data_type)
{
	Track *current_track;
	Patch *current_patch;
	Auto *current_auto;
	
	for(current_track = mwindow->tracks->first, 
		current_patch = mwindow->patches->first; 
		current_track && current_patch; 
		current_track = current_track->next, 
		current_patch = current_patch->next)
	{
		if(current_patch->play && current_track->data_type == data_type)
		{
			current_auto = current_track->play_autos->autoof(position);

// get auto right before position
			if(reverse)
			{
				if(current_auto && current_auto->position < position) 
					current_auto = current_auto->next;
			}
			else
			{
				if(current_auto && current_auto->position > position) 
					current_auto = current_auto->previous;
			}
			
			if(!current_auto || current_auto->value == 1)
				append((ATrack*)current_track);
		}
	}
}

TrackList::get_recordable_type(int data_type)
{
	Track *current_track;
	Patch *current_patch;
	
	for(current_track = tracks->first, current_patch = patches->first; 
		current_track && current_patch; 
		current_track = current_track->next, current_patch = current_patch->next)
	{
		if(current_patch->record && current_track->data_type == TRACK_AUDIO) 
			append((ATrack*)current_track);
	}
}
