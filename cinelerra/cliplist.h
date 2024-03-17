// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef CLIPLIST_H
#define CLIPLIST_H

#include "arraylist.h"
#include "edl.inc"
#include "filexml.inc"
#include "cliplist.inc"


class ClipList : public ArrayList<EDL*>
{
public:
	ClipList();
	~ClipList();

	EDL *add_clip(EDL *new_clip);
	void remove_clip(EDL *clip);
	void remove_from_project(ArrayList<EDL*> *clips);
	void save_xml(FileXML *file, const char *output_path);

	void dump(int indent);
};

#endif
