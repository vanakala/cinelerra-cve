// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcwindowbase.inc"
#include "clip.h"
#include "filexml.h"
#include "maskauto.h"
#include "maskautos.h"
#include "track.h"

#include <stdlib.h>
#include <string.h>

MaskPoint::MaskPoint()
{
	submask_x = 0;
	submask_y = 0;
	control_x1 = 0;
	control_y1 = 0;
	control_x2 = 0;
	control_y2 = 0;
}

int MaskPoint::operator==(MaskPoint& ptr)
{
	return EQUIV(submask_x, ptr.submask_x) &&
		EQUIV(submask_y, ptr.submask_y) &&
		EQUIV(control_x1, ptr.control_x1) &&
		EQUIV(control_y1, ptr.control_y1) &&
		EQUIV(control_x2, ptr.control_x2) &&
		EQUIV(control_y2, ptr.control_y2);
}

size_t MaskPoint::get_size()
{
	return sizeof(*this);
}

void MaskPoint::dump(int indent)
{
	printf("%*spoint %p (%.4f %.4f) in:(%.2f,%.2f) out:(%.2f,%.2f)\n",
		indent, " ", this, submask_x, submask_y,
		control_x1, control_y1, control_x2, control_y2);
}


SubMask::SubMask(MaskAuto *keyframe)
{
	this->keyframe = keyframe;
}

void SubMask::copy_from(SubMask& ptr)
{
	points.remove_all_objects();

	for(int i = 0; i < ptr.points.total; i++)
	{
		MaskPoint *point = new MaskPoint;
		*point = *ptr.points.values[i];
		points.append(point);
	}
}

MaskPoint *SubMask::append_point(MaskPoint *templ)
{
	MaskPoint *point = new MaskPoint;

	if(templ)
		*point = *templ;
	points.append(point);
	return point;
}

void SubMask::load(FileXML *file, Track *track)
{
	points.remove_all_objects();

	while(!file->read_tag())
	{
		if(file->tag.title_is("/MASK"))
			break;
		else
		if(file->tag.title_is("POINT"))
		{
			char string[BCTEXTLEN];
			string[0] = 0;
			file->read_text_until("/POINT", string, BCTEXTLEN);

			MaskPoint *point = new MaskPoint;
			char *ptr = string;

			point->submask_x = atof(ptr);
			ptr = strchr(ptr, ',');

			if(ptr)
			{
				point->submask_y = atof(ptr + 1);
				ptr = strchr(ptr + 1, ',');

				if(ptr)
				{
					point->control_x1 = atof(ptr + 1);
					ptr = strchr(ptr + 1, ',');
					if(ptr)
					{
						point->control_y1 = atof(ptr + 1);
						ptr = strchr(ptr + 1, ',');
						if(ptr)
						{
							point->control_x2 = atof(ptr + 1);
							ptr = strchr(ptr + 1, ',');
							if(ptr) point->control_y2 = atof(ptr + 1);
						}
					}
				}
			}
			// Backward copatibility
			if(point->submask_x > 1 || point->submask_y > 1)
			{
				point->submask_x /= track->track_w;
				point->submask_y /= track->track_h;
				point->control_x1 /= track->track_w;
				point->control_x2 /= track->track_w;
				point->control_y1 /= track->track_h;
				point->control_y2 /= track->track_h;
			}
			points.append(point);
		}
	}
}

void SubMask::save_xml(FileXML *file)
{
	if(points.total)
	{
		file->tag.set_title("MASK");
		file->append_tag();

		for(int i = 0; i < points.total; i++)
		{
			file->append_newline();
			file->tag.set_title("POINT");
			file->append_tag();
			char string[BCTEXTLEN];
			double mask_x = CLIP(points.values[i]->submask_x, 0.0, 1.0);
			double mask_y = CLIP(points.values[i]->submask_y, 0.0, 1.0);

			sprintf(string, "%.7g, %.7g, %.7g, %.7g, %.7g, %.7g",
				mask_x, mask_y,
				points.values[i]->control_x1,
				points.values[i]->control_y1,
				points.values[i]->control_x2,
				points.values[i]->control_y2);
			file->append_text(string);
			file->tag.set_title("/POINT");
			file->append_tag();
		}
		file->append_newline();

		file->tag.set_title("/MASK");
		file->append_tag();
		file->append_newline();
	}
}

size_t SubMask::get_size()
{
	size_t size = sizeof(*this);

	for(int i = 0; i < points.total; i++)
		size += points.values[i]->get_size();
	return size;
}

void SubMask::dump(int indent)
{
	printf("%*sSubmask %p dump(%d):\n", indent, " ", this, points.total);
	indent += 2;

	for(int i = 0; i < points.total; i++)
		points.values[i]->dump(indent);
}


MaskAuto::MaskAuto(EDL *edl, MaskAutos *autos)
 : Auto(edl, autos)
{
	feather = 0;
	value = 100;
	apply_before_plugins = 0;
}

MaskAuto::~MaskAuto()
{
	masks.remove_all_objects();
}

void MaskAuto::copy_from(Auto *src)
{
	copy_from((MaskAuto*)src);
}

void MaskAuto::copy_from(MaskAuto *src)
{
	Auto::copy_from(src);

	feather = src->feather;
	value = src->value;
	apply_before_plugins = src->apply_before_plugins;

	masks.remove_all_objects();
	for(int i = 0; i < src->masks.total; i++)
	{
		masks.append(new SubMask(this));
		masks.values[i]->copy_from(*src->masks.values[i]);
	}
}

void MaskAuto::interpolate_from(Auto *a1, Auto *a2, ptstime position, Auto *templ)
{
	if(!a1) a1 = previous;
	if(!a2) a2 = next;
	MaskAuto  *mask_auto1 = (MaskAuto *)a1;
	MaskAuto  *mask_auto2 = (MaskAuto *)a2;

	if(!mask_auto2 || !mask_auto1 || mask_auto2->masks.total == 0)
	// can't interpolate, fall back to copying (using template if possible)
	{
		Auto::interpolate_from(a1, a2, position, templ);
		return;
	}
	double frac1 = (position - mask_auto1->pos_time) /
		(mask_auto2->pos_time - mask_auto1->pos_time);
	double frac2 = 1.0 - frac1;

	feather = round(mask_auto1->feather * frac2 + mask_auto2->feather * frac1);
	value = round(mask_auto1->value * frac2  + mask_auto2->value * frac1);
	apply_before_plugins = mask_auto1->apply_before_plugins;
	pos_time = position;
	masks.remove_all_objects();

	for(int i = 0; i < mask_auto1->masks.total; i++)
	{
		SubMask *new_submask = new SubMask(this);
		masks.append(new_submask);
		SubMask *mask1 = mask_auto1->masks.values[i];
		SubMask *mask2 = mask_auto2->masks.values[i];

		// just in case, should never happen
		int total_points = MIN(mask1->points.total, mask2->points.total);
		for(int j = 0; j < total_points; j++)
		{
			MaskPoint *point = new MaskPoint;
			MaskAutos::avg_points(point,
				mask1->points.values[j],
				mask2->points.values[j],
				pos_time,
				mask_auto1->pos_time,
				mask_auto2->pos_time);
			new_submask->points.append(point);
		}
	}
}

void MaskAuto::interpolate_values(ptstime pts, int *new_value, int *new_feather)
{
	if(next)
	{
		MaskAuto *next_mask = (MaskAuto *)next;
		double frac1 = (pts - pos_time) /
			(next->pos_time - pos_time);
		double frac2 = 1.0 - frac1;

		*new_feather = round(feather * frac2 + next_mask->feather * frac1);
		*new_value = round(value * frac2  + next_mask->value * frac1);
	}
	else
	{
		*new_feather = feather;
		*new_value = value;
	}
}

SubMask* MaskAuto::get_submask(int number)
{
	if(masks.total)
	{
		CLAMP(number, 0, masks.total - 1);
		return masks.values[number];
	}
	return 0;
}

SubMask *MaskAuto::create_submask(int number)
{
	if(number < masks.total)
		return masks.values[number];

	SubMask *mask = new SubMask(this);
	masks.append(mask);
	return mask;
}

void MaskAuto::load(FileXML *file)
{
	if(autos && autos->first == this)
		((MaskAutos *)autos)->set_mode(file->tag.get_property("MODE", ((MaskAutos *)autos)->get_mode()));
	feather = file->tag.get_property("FEATHER", feather);
	value = file->tag.get_property("VALUE", value);
	apply_before_plugins = file->tag.get_property("APPLY_BEFORE_PLUGINS", apply_before_plugins);

	for(int i = 0; i < masks.total; i++)
		delete masks.values[i];

	while(!file->read_tag())
	{
		if(file->tag.title_is("/AUTO"))
			break;
		if(file->tag.title_is("MASK"))
		{
			SubMask *mask = new SubMask(this);
			masks.append(mask);
			mask->load(file, autos->track);
		}
	}
}

void MaskAuto::save_xml(FileXML *file)
{
	file->tag.set_title("AUTO");
	if(autos && autos->first == this)
		file->tag.set_property("MODE", ((MaskAutos*)autos)->get_mode());
	file->tag.set_property("VALUE", value);
	file->tag.set_property("FEATHER", feather);
	file->tag.set_property("APPLY_BEFORE_PLUGINS", apply_before_plugins);

	file->tag.set_property("POSTIME", pos_time);
	file->append_tag();
	file->append_newline();

	for(int i = 0; i < masks.total; i++)
		masks.values[i]->save_xml(file);
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void MaskAuto::copy(Auto *src, ptstime start, ptstime end)
{
	MaskAuto *that = (MaskAuto*)src;

	pos_time = that->pos_time - start;
	value = that->value;
	feather = that->feather;
	apply_before_plugins = that-> apply_before_plugins;

	for(int i = 0; i < that->masks.total; i++)
		masks.values[i]->copy_from(*that->masks.values[i]);
}

size_t MaskAuto::get_size()
{
	size_t size = sizeof(*this);

	for(int i = 0; i < masks.total; i++)
		size += masks.values[i]->get_size();
	return size;
}

void MaskAuto::dump(int indent)
{
	printf("%*sMaskauto %p: value: %d feather %d before %d\n", indent, " ",
		this, value, feather, apply_before_plugins);
	printf("%*spos_time %.3f masks %d\n", indent + 2, " ", pos_time, masks.total);
	indent += 4;
	for(int i = 0; i < masks.total; i++)
		masks.values[i]->dump(indent);
}
