// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LABEL_H
#define LABEL_H

#include <stdint.h>

#include "datatype.h"
#include "edl.inc"
#include "filexml.inc"
#include "labels.inc"
#include "linklist.h"

#define LABELSIZE 15

class Label : public ListItem<Label>
{
public:
	Label(ptstime position, const char *textstr = 0);

	size_t get_size();

	char textstr[BCTEXTLEN];
// Seconds
	ptstime position;
};

class Labels : public List<Label>
{
public:
	Labels(EDL *edl, const char *xml_tag);
	~Labels();

	void reset_instance();
	void dump(int indent = 0);

	Labels& operator=(Labels &that);
	void copy_from(Labels *labels);
	void toggle_label(ptstime start, ptstime end);
	void delete_all();
	void load(FileXML *xml);
	void modify_handles(double oldposition, 
		double newposition, 
		int currentend, 
		int handle_mode,
		int edit_labels);
	void save_xml(FileXML *xml);
	void copy(Labels *that, ptstime start, ptstime end);
	void insert(ptstime start, ptstime length);

// Setting follow to 1 causes labels to move forward after clear.
// Setting it to 0 implies ignoring the labels follow edits setting.
	void clear(ptstime start, ptstime end, int follow = 1);
	void paste_silence(ptstime start, ptstime end);
	void optimize();  // delete duplicates
	size_t get_size();

// Get nearest labels or 0 if start or end of timeline
	Label* prev_label(ptstime position);
	Label* next_label(ptstime position);

	Label* label_of(ptstime position); // first label on or after position
	EDL *edl;
	const char *xml_tag;
};

#endif
