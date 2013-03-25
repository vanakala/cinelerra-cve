
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

#include "filexml.h"
#include "keyframe.h"

#include <stdio.h>
#include <string.h>

KeyFrame::KeyFrame()
 : Auto()
{
	data[0] = 0;
}

KeyFrame::KeyFrame(EDL *edl, KeyFrames *autos)
 : Auto(edl, (Autos*)autos)
{
	data[0] = 0;
}

void KeyFrame::load(FileXML *file)
{
	file->read_text_until("/KEYFRAME", data, MESSAGESIZE);
}

void KeyFrame::copy(ptstime start, ptstime end, FileXML *file)
{
	file->tag.set_title("KEYFRAME");
	file->tag.set_property("POSTIME", pos_time - start);
	file->append_tag();

	file->append_text(data);

	file->tag.set_title("/KEYFRAME");
	file->append_tag();
	file->append_newline();
}

void KeyFrame::copy_from(Auto *that)
{
	copy_from((KeyFrame*)that);
}

void KeyFrame::copy_from(KeyFrame *that)
{
	Auto::copy_from(that);
	KeyFrame *keyframe = (KeyFrame*)that;
	strcpy(data, keyframe->data);
}

int KeyFrame::identical(KeyFrame *src)
{
	return !strcasecmp(src->data, data);
}

int KeyFrame::operator==(Auto &that)
{
	return identical((KeyFrame*)&that);
}

int KeyFrame::operator==(KeyFrame &that)
{
	return identical(&that);
}

void KeyFrame::dump(int indent)
{
	printf("%*sKeyFrame %p pos_time %.3lf\n", indent, " ", this, pos_time);
	printf("%*sdata: %s\n", indent + 2, " ", data);
}
