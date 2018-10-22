
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
#include "aautomation.h"
#include "bcresources.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "cache.h"
#include "clip.h"
#include "datatype.h"
#include "file.h"
#include "filexml.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "units.h"

#include <string.h>



ATrack::ATrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_AUDIO;
	one_unit = (ptstime)1.0 / edl->session->sample_rate;
	automation = new AAutomation(edl, this);
}

// Used by PlaybackEngine
void ATrack::synchronize_params(Track *track)
{
	Track::synchronize_params(track);

	ATrack *atrack = (ATrack*)track;
}

void ATrack::copy_settings(Track *track)
{
	Track::copy_settings(track);

	ATrack *atrack = (ATrack*)track;
}

void ATrack::save_header(FileXML *file)
{
	file->tag.set_property("TYPE", "AUDIO");
}

int ATrack::vertical_span(Theme *theme)
{
	int track_h = Track::vertical_span(theme);
	int patch_h = 0;
	if(expand_view)
	{
		patch_h += theme->title_h + theme->play_h + theme->fade_h + theme->meter_h + theme->pan_h;
	}
	return MAX(track_h, patch_h);
}

void ATrack::set_default_title()
{
	Track *current = ListItem<Track>::owner->first;
	int i;
	for(i = 0; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO) i++;
	}
	sprintf(title, _("Audio %d"), i);
	BC_Resources::encode_to_utf8(title, BCTEXTLEN);
}

posnum ATrack::to_units(ptstime position, int round)
{
	if(round)
		return Units::round(position * edl->session->sample_rate);
	else
		return Units::to_int64(position * edl->session->sample_rate);
}

ptstime ATrack::from_units(posnum position)
{
	return (double)position / edl->session->sample_rate;
}
