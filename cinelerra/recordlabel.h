
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

#ifndef RECORDLABELS_H
#define RECORDLABELS_H

#include "labels.inc"
#include "linklist.h"


class RecordLabel : public ListItem<RecordLabel>
{
public:
	RecordLabel() {};
	RecordLabel(double position);
	~RecordLabel();
	
	double position;
};


class RecordLabels : public List<RecordLabel>
{
public:
	RecordLabels();
	~RecordLabels();
	
	int delete_new_labels();
	int toggle_label(double position);
	double get_prev_label(double position);
	double get_next_label(double position);
	double goto_prev_label(double position);
	double goto_next_label(double position);
};

#endif
