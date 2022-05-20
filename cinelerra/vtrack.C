// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "automation.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "clip.h"
#include "datatype.h"
#include "edl.inc"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "filexml.h"
#include "language.h"
#include "theme.h"
#include "tracks.inc"
#include "units.h"
#include "vtrack.h"

VTrack::VTrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_VIDEO;
	draw = 1;
	one_unit = (ptstime)1.0 / edlsession->frame_rate;
	automation = new Automation(edl, this);
}

int VTrack::vertical_span(Theme *theme)
{
	int track_h = Track::vertical_span(theme);
	int patch_h = 0;
	if(expand_view)
	{
		patch_h += theme->title_h + theme->play_h + theme->fade_h + theme->mode_h;
	}
	return MAX(track_h, patch_h);
}

void VTrack::set_default_title()
{
	Track *current = ListItem<Track>::owner->first;
	int i;
	for(i = 0; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO) i++;
	}
	sprintf(title, _("Video %d"), i);
	BC_Resources::encode_to_utf8(title, BCTEXTLEN);
}

posnum VTrack::to_units(ptstime position)
{
	return round(position * edlsession->frame_rate);
}

ptstime VTrack::from_units(posnum position)
{
	return (ptstime)position / edlsession->frame_rate;
}

void VTrack::save_header(FileXML *file)
{
	file->tag.set_property("TYPE", "VIDEO");
}
