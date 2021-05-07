
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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
		{
			return new_clip;
		}
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
