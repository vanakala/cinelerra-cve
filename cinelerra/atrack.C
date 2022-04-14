// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "atrack.h"
#include "automation.h"
#include "bcresources.h"
#include "edl.h"
#include "edlsession.h"
#include "clip.h"
#include "datatype.h"
#include "file.h"
#include "filexml.h"
#include "language.h"
#include "theme.h"
#include "tracks.inc"
#include "units.h"

#include <string.h>


ATrack::ATrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_AUDIO;
	one_unit = (ptstime)1.0 / edlsession->sample_rate;
	automation = new Automation(edl, this);
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
	return Units::round(position * edlsession->sample_rate);
}

ptstime ATrack::from_units(posnum position)
{
	return (double)position / edlsession->sample_rate;
}
