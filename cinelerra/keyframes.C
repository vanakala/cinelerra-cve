
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
#include "keyframes.h"
#include "plugin.h"

KeyFrames::KeyFrames(EDL *edl, Track *track, Plugin *plugin)
 : Autos(edl, track)
{
	this->plugin = plugin;
}

Auto* KeyFrames::new_auto()
{
	return new KeyFrame(edl, this);
}

void KeyFrames::drag_limits(Auto *current, ptstime *prev, ptstime *next)
{
	if(current->previous)
		*prev = unit_round(current->previous->pos_time, 1);
	else
		*prev = base_pts;

	if(current->next)
		*next = unit_round(current->next->pos_time, 1);
	else
		*next = plugin->end_pts();
}

void KeyFrames::copy(ptstime start,
	ptstime end,
	FileXML *file)
{
	for(KeyFrame* current = (KeyFrame*)autoof(start);
		current && current->pos_time <= end;
		current = (KeyFrame*)NEXT)
	{
// Want to copy single keyframes by putting the cursor on them
		if(current->pos_time >= start && current->pos_time <= end)
			current->copy(start, end, file);
	}
}

void KeyFrames::copy(KeyFrames *src, ptstime start, ptstime end)
{
	KeyFrame *current;

	for(current = (KeyFrame*)src->autoof(start); current && current->pos_time <= end;
		current = (KeyFrame*)NEXT)
	{
		if(current->pos_time >= start - EPSILON && current->pos_time <= end)
		{
			KeyFrame *new_keyframe = (KeyFrame*)append_auto();
			new_keyframe->copy(current, start, end);
		}
	}
}

void KeyFrames::dump(int indent)
{
	printf("%*sKeyFrames %p dump(%d): base %.3f\n", indent, " ", this, total(), base_pts);
	for(KeyFrame *current = (KeyFrame*)first; current; current = (KeyFrame*)NEXT)
		current->dump(indent + 2);
}
