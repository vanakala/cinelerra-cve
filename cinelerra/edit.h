// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef EDIT_H
#define EDIT_H

#include "asset.inc"
#include "bcwindowbase.inc"
#include "datatype.h"
#include "edl.inc"
#include "edits.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "linklist.h"
#include "mwindow.inc"
#include "plugin.inc"
#include "pluginserver.inc"
#include "track.inc"

// Generic edit of something

class Edit : public ListItem<Edit>
{
public:
	Edit(EDL *edl, Edits *edits);
	Edit(EDL *edl, Track *track);
	~Edit();

	ptstime length(void);
	ptstime end_pts(void);
	void copy_from(Edit *edit);
	Edit& operator=(Edit& edit);
// Called by Edits
	void equivalent_output(Edit *edit, ptstime *result);
// Get size of frame to draw on timeline
	double picon_w(void);
	int picon_h(void);

	void save_xml(FileXML *xml, const char *output_path, int streamno);
// Shift in time
	void shift(ptstime difference);
	void shift_source(ptstime difference);
	Plugin *insert_transition(PluginServer *server);
// Determine if silence depending on existance of asset or plugin title
	int silence(void);
// Returns size of data in bytes
	size_t get_size();

// Media edit information
// Channel or layer of source
	int channel;
// ID for resource pixmaps
	int id;

	ptstime set_pts(ptstime pts);
	ptstime get_pts();
	ptstime set_source_pts(ptstime pts);
	ptstime get_source_pts();
// End of current edit in source
	ptstime source_end_pts();
// End of asset
	ptstime get_source_length();

// Transition if one is present at the beginning of this edit
// This stores the length of the transition
	Plugin *transition;
	EDL *edl;

	Edits *edits;

	Track *track;

// Asset is 0 if silence, otherwise points an object in assetlist
	Asset *asset;
// Stream in asset
	int stream;

// ============================= initialization

	posnum load_properties(FileXML *xml, ptstime project_pts);

	void dump(int indent = 0);
private:
// Start of edit in source file in seconds
	ptstime source_pts;
// Start of edit in project
	ptstime project_pts;

};

#endif
