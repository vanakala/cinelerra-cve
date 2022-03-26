// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "edl.inc"
#include "filexml.h"
#include "intauto.h"
#include "intautos.h"

IntAuto::IntAuto(EDL *edl, IntAutos *autos)
 : Auto(edl, (Autos*)autos)
{
	value = 0;
}

void IntAuto::load(FileXML *file)
{
	value = file->tag.get_property("VALUE", value);
}

void IntAuto::save_xml(FileXML *file)
{
	file->tag.set_title("AUTO");
	file->tag.set_property("POSTIME", pos_time);
	file->tag.set_property("VALUE", value);
	file->append_tag();
	file->tag.set_title("/AUTO");
	file->append_tag();
	file->append_newline();
}

void IntAuto::copy(Auto *src, ptstime start, ptstime end)
{
	IntAuto *that = (IntAuto*)src;

	pos_time = that->pos_time - start;
	value = that->value;
}

void IntAuto::copy_from(Auto *that)
{
	copy_from((IntAuto*)that);
}

void IntAuto::copy_from(IntAuto *that)
{
	Auto::copy_from(that);
	this->value = that->value;
}

size_t IntAuto::get_size()
{
	return sizeof(*this);
}

void IntAuto::dump(int indent)
{
	printf("%*sIntaAuto %p: pos_time: %.3f value: %d\n", indent, "",
		this, pos_time, value);
}
