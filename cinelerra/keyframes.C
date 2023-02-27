// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
	return new KeyFrame(this);
}

void KeyFrames::drag_limits(Auto *current, ptstime *prev, ptstime *next)
{
	if(current->previous)
		*prev = unit_round(current->previous->pos_time, 1);
	else
		*prev = unit_round(base_pts, 1);

	if(current->next)
		*next = unit_round(current->next->pos_time, -1);
	else
		*next = unit_round(plugin->end_pts(), -1);
}

void KeyFrames::save_xml(FileXML *file)
{
	for(KeyFrame* current = (KeyFrame*)first; current; current = (KeyFrame*)NEXT)
		current->save_xml(file);
}

KeyFrame *KeyFrames::get_first()
{
	KeyFrame *current;

	if(!first)
	{
		current = (KeyFrame *)append(new_auto());
		current->pos_time = base_pts;
	}
	return (KeyFrame *)first;
}

size_t KeyFrames::get_size()
{
	size_t size = sizeof(*this);

	for(KeyFrame* current = (KeyFrame*)first; current; current = (KeyFrame*)NEXT)
		size += current->get_size();
	return size;
}

void KeyFrames::dump(int indent)
{
	printf("%*sKeyFrames %p dump(%d): base %.3f\n", indent, " ", this, total(), base_pts);
	for(KeyFrame *current = (KeyFrame*)first; current; current = (KeyFrame*)NEXT)
		current->dump(indent + 2);
}
