
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
	data = 0;
}

KeyFrame::KeyFrame(EDL *edl, KeyFrames *autos)
 : Auto(edl, (Autos*)autos)
{
	data = 0;
}

KeyFrame::~KeyFrame()
{
	free(data);
}

void KeyFrame::load(FileXML *file)
{
	size_t txt_size;

	free(data);
	data = 0;

	if(txt_size = file->text_length_until("/KEYFRAME"))
	{
		data = (char*)malloc(txt_size + 1);
		file->read_text_until("/KEYFRAME", data, txt_size + 1);
	}
}

char *KeyFrame::get_data()
{
	return data;
}

size_t KeyFrame::data_size()
{
	if(data)
		return strlen(data);
	return 0;
}

void KeyFrame::set_data(const char *string)
{
	if(string && string[0])
	{
		if(data)
		{
			if(strcmp(string, data))
			{
				free(data);
				data = strdup(string);
			}
		}
		else
			data = strdup(string);
	}
	else
	{
		free(data);
		data = 0;
	}
}

void KeyFrame::save_xml(FileXML *file)
{
	file->tag.set_title("KEYFRAME");
	file->tag.set_property("POSTIME", pos_time);
	file->append_tag();

	if(data)
		file->append_text(data);

	file->tag.set_title("/KEYFRAME");
	file->append_tag();
	file->append_newline();
}

void KeyFrame::copy(Auto *src, ptstime start, ptstime end)
{
	KeyFrame *that = (KeyFrame*)src;

	if(this == that)
		return;

	pos_time = that->pos_time - start;
	if(data)
		free(data);
	data = strdup(that->data);
}

void KeyFrame::copy_from(Auto *that)
{
	copy_from((KeyFrame*)that);
}

void KeyFrame::copy_from(KeyFrame *that)
{
	if(this == that)
		return;

	Auto::copy_from(that);
	if(data)
		free(data);
	data = strdup(that->data);
}

int KeyFrame::identical(KeyFrame *src)
{
	if(!src)
		return 0;
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

size_t KeyFrame::get_size()
{
	size_t size = sizeof(*this);

	if(data)
		size += strlen(data);
	return size;
}

void KeyFrame::dump(int indent)
{
	printf("%*sKeyFrame %p: pos_time %.3f\n", indent, "", this, pos_time);
	if(data)
		printf("%*sdata: %s\n", indent + 2, "", data);
}
