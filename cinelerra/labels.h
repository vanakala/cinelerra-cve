
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

#include "edl.inc"
#include "guicast.h"
#include "filexml.inc"
#include "labels.inc"
#include "mwindow.inc"
#include "recordlabel.inc"
#include "stringfile.inc"
#include "timebar.inc"

#define LABELSIZE 15

class LabelToggle : public BC_Label
{
public:
	LabelToggle(MWindow *mwindow, Label *label, int x, int y, long position);
	~LabelToggle();
	
	int handle_event();
	MWindow *mwindow;
	Label *label;
};

class Label : public ListItem<Label>
{
public:
	Label(EDL *edl, Labels *labels, double position, char *textstr);
	Label();
	~Label();


	EDL *edl;
	Labels *labels;
	char textstr[BCTEXTLEN];
// Seconds
	double position;
};

class Labels : public List<Label>
{
public:
	Labels(EDL *edl, char *xml_tag);
	virtual ~Labels();

	void dump();

	Labels& operator=(Labels &that);
	void copy_from(Labels *labels);
	int toggle_label(double start, double end);
	int delete_all();
	int save(FileXML *xml);
	int load(FileXML *xml, uint32_t load_flags);
	void insert_labels(Labels *labels, 
		double start, 
		double length, 
		int paste_silence = 1);

	int modify_handles(double oldposition, 
		double newposition, 
		int currentend, 
		int handle_mode,
		int edit_labels);
	int copy(double start, double end, FileXML *xml);
	int copy_length(long start, long end); // return number of Labels in selection
	int insert(double start, double length);
	int paste(long start, long end, long total_length, FileXML *xml);
	int paste_output(long startproject, long endproject, long startsource, long endsource, RecordLabels *labels);
// Setting follow to 1 causes labels to move forward after clear.
// Setting it to 0 implies ignoring the labels follow edits setting.
	int clear(double start, double end, int follow = 1);
	int paste_silence(double start, double end);
	int optimize();  // delete duplicates
// Get nearest labels or 0 if start or end of timeline
	Label* prev_label(double position);
	Label* next_label(double position);

	Label* label_of(double position); // first label on or after position
	MWindow *mwindow;
	TimeBar *timebar;
	EDL *edl;
	char *xml_tag;
};

#endif
