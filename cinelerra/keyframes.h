// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef KEYFRAMES_H
#define KEYFRAMES_H

#include "autos.h"
#include "filexml.inc"
#include "keyframe.inc"
#include "plugin.inc"

// Keyframes inherit from Autos to reuse the editing commands but
// keyframes don't belong to tracks.  Instead they belong to plugins
// because their data is specific to plugins.


class KeyFrames : public Autos
{
public:
	KeyFrames(EDL *edl, Track *track, Plugin *plugin);

	Auto* new_auto();
	void drag_limits(Auto *current, ptstime *prev, ptstime *next);
	void save_xml(FileXML *file);
	// Returns first keyframe, creates if does not exist
	KeyFrame *get_first();
	// Number of bytes used
	size_t get_size();
	void dump(int indent = 0);

private:
	Plugin *plugin;
};

#endif
