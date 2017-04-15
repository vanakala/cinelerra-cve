
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

#ifndef LABEL_H
#define LABEL_H

#include <stdint.h>

#include "datatype.h"
#include "edl.inc"
#include "filexml.inc"
#include "labels.inc"
#include "mwindow.inc"
#include "timebar.inc"

#define LABELSIZE 15

class Label : public ListItem<Label>
{
public:
	Label(EDL *edl, Labels *labels, ptstime position, const char *textstr);

	EDL *edl;
	Labels *labels;
	char textstr[BCTEXTLEN];
// Seconds
	ptstime position;
};

class Labels : public List<Label>
{
public:
	Labels(EDL *edl, const char *xml_tag);
	virtual ~Labels();

	void dump(int indent = 0);

	Labels& operator=(Labels &that);
	void copy_from(Labels *labels);
	void toggle_label(ptstime start, ptstime end);
	void delete_all();
	void save(FileXML *xml);
	void load(FileXML *xml, uint32_t load_flags);
	void insert_labels(Labels *labels, 
		ptstime start,
		ptstime length,
		int paste_silence = 1);

	void modify_handles(double oldposition, 
		double newposition, 
		int currentend, 
		int handle_mode,
		int edit_labels);
	void copy(ptstime start, ptstime end, FileXML *xml);
	void insert(ptstime start, ptstime length);

// Setting follow to 1 causes labels to move forward after clear.
// Setting it to 0 implies ignoring the labels follow edits setting.
	void clear(ptstime start, ptstime end, int follow = 1);
	void paste_silence(ptstime start, ptstime end);
	void optimize();  // delete duplicates
// Get nearest labels or 0 if start or end of timeline
	Label* prev_label(ptstime position);
	Label* next_label(ptstime position);

	Label* label_of(ptstime position); // first label on or after position
	MWindow *mwindow;
	TimeBar *timebar;
	EDL *edl;
	const char *xml_tag;
};

#endif
