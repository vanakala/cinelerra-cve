// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VTRACK_H
#define VTRACK_H

#include "edl.inc"
#include "filexml.inc"
#include "theme.inc"
#include "track.h"
#include "tracks.inc"


class VTrack : public Track
{
public:
	VTrack(EDL *edl, Tracks *tracks);

	void set_default_title();
	void save_header(FileXML *file);
	posnum to_units(ptstime position);
	ptstime from_units(posnum position);

	int vertical_span(Theme *theme);
};

#endif
