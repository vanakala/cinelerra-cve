// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "edl.h"
#include "filexml.h"
#include "cliplist.h"

ClipList::ClipList()
 : ArrayList<EDL*>()
{
}

ClipList::~ClipList()
{
	remove_all_objects();
}

EDL *ClipList::add_clip(EDL *new_clip)
{
	for(int i = 0; i < total; i++)
	{
		if(values[i] == new_clip)
			return new_clip;
	}
	return append(new_clip);
}

void ClipList::remove_clip(EDL *clip)
{
	remove(clip);
}

void ClipList::remove_from_project(ArrayList<EDL*> *clips)
{
	for(int i = 0; i < clips->total; i++)
	{
		for(int j = 0; j < total; j++)
		{
			if(values[j] == clips->values[i])
			{
				remove_object(clips->values[i]);
				break;
			}
		}
	}
}

void ClipList::save_xml(FileXML *file, const char *output_path)
{
	if(!total)
		return;

	for(int i = 0; i < total; i++)
	{
		file->tag.set_title("CLIP_EDL");
		file->append_tag();
		file->append_newline();

		values[i]->save_xml(file, output_path, EDL_CLIP);

		file->tag.set_title("/CLIP_EDL");
		file->append_tag();
		file->append_newline();
	}
}

void ClipList::dump(int indent)
{
	printf("%*sClipList %p dump(%d):\n", indent, "", this, total);
	indent += 2;
	for(int i = 0; i < total; i++)
		values[i]->dump(indent);
}
