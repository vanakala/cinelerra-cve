// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "filexml.h"
#include "cropauto.h"
#include "cropautos.h"
#include "track.h"

CropAuto::CropAuto(CropAutos *autos)
 : Auto((Autos*)autos)
{
	left = 0;
	top = 0;
	right = 1.0;
	bottom = 1.0;
	apply_before_plugins = 0;
}

int CropAuto::operator==(Auto &that)
{
	return identical((CropAuto*)&that);
}

int CropAuto::identical(CropAuto *that)
{
	if(this == that)
		return 1;

	return that->left == left && that->right == right &&
		that->top == top && that->bottom == bottom &&
		that->apply_before_plugins == apply_before_plugins;
}

void CropAuto::load(FileXML *file)
{
	int val;

	if((val = file->tag.get_property("LEFT", 0)) > 0)
		left = (double)val / autos->track->track_w;
	left = file->tag.get_property("RLEFT", left);

	if((val = file->tag.get_property("RIGHT", 0)) > 0)
		right = (double)val / autos->track->track_w;
	right = file->tag.get_property("RRIGHT", right);

	if((val = file->tag.get_property("TOP", 0)) > 0)
		top = (double)val / autos->track->track_h;
	top = file->tag.get_property("RTOP", top);

	if((val = file->tag.get_property("BOTTOM", 0)) > 0)
		bottom = (double)val / autos->track->track_h;
	bottom = file->tag.get_property("RBOTTOM", bottom);

	apply_before_plugins = file->tag.get_property("APPLY_BEFORE_PLUGINS",
		apply_before_plugins);
}

void CropAuto::save_xml(FileXML *file)
{
	file->tag.set_title("AUTO");
	file->tag.set_property("POSTIME", pos_time);
	file->tag.set_property("RLEFT", left);
	file->tag.set_property("RRIGHT", right);
	file->tag.set_property("RTOP", top);
	file->tag.set_property("RBOTTOM", bottom);
	file->tag.set_property("APPLY_BEFORE_PLUGINS", apply_before_plugins);
	file->append_tag();
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void CropAuto::copy(Auto *src, ptstime start, ptstime end)
{
	CropAuto *that = (CropAuto*)src;

	pos_time = that->pos_time - start;
	left = that->left;
	right = that->right;
	top = that->top;
	bottom = that->bottom;
	apply_before_plugins = that->apply_before_plugins;
}

void CropAuto::copy_from(Auto *that)
{
	copy_from((CropAuto*)that);
}

void CropAuto::copy_from(CropAuto *that)
{
	Auto::copy_from(that);
	left = that->left;
	right = that->right;
	top = that->top;
	bottom = that->bottom;
	apply_before_plugins = that->apply_before_plugins;
}

size_t CropAuto::get_size()
{
	return sizeof(*this);
}

void CropAuto::dump(int indent)
{
	printf("%*sCropAuto %p: pos_time: %.3f (%.4f,%.4f),(%.4f,%.4f) before %d\n",
		indent, "", this, pos_time, left, top, right, bottom,
		apply_before_plugins);
}
