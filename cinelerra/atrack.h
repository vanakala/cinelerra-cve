// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef ATRACK_H
#define ATRACK_H

#include "edl.inc"
#include "filexml.inc"
#include "track.h"

class ATrack : public Track
{
public:
	ATrack(EDL *edl, Tracks *tracks);

	void set_default_title();
	int vertical_span(Theme *theme);
	void save_header(FileXML *file);
	posnum to_units(ptstime position, int round = 0);
	ptstime from_units(posnum position);
};

#endif
