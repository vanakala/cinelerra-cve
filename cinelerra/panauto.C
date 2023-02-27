// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "apatchgui.inc"
#include "bcpan.h"
#include "edlsession.h"
#include "filexml.h"
#include "panauto.h"

PanAuto::PanAuto(PanAutos *autos)
 : Auto((Autos*)autos)
{
	memset(values, 0, MAXCHANNELS * sizeof(double));
	handle_x = handle_y = 0;
}

int PanAuto::operator==(Auto &that)
{
	PanAuto *panauto = (PanAuto*)&that;

	return handle_x == panauto->handle_x &&
		handle_y == panauto->handle_y;
}

void PanAuto::rechannel()
{
	BC_Pan::stick_to_values(values,
		edlsession->audio_channels,
		edlsession->achannel_positions,
		handle_x, 
		handle_y,
		PAN_RADIUS,
		1);
}

void PanAuto::load(FileXML *file)
{
	memset(values, 0, MAXCHANNELS * sizeof(double));
	handle_x = file->tag.get_property("HANDLE_X", handle_x);
	handle_y = file->tag.get_property("HANDLE_Y", handle_y);
	for(int i = 0; i < edlsession->audio_channels; i++)
	{
		char string[BCTEXTLEN];
		sprintf(string, "VALUE%d", i);
		values[i] = file->tag.get_property(string, values[i]);
	}
}

void PanAuto::save_xml(FileXML *file)
{
	file->tag.set_title("AUTO");
	file->tag.set_property("POSTIME", pos_time);
	file->tag.set_property("HANDLE_X", handle_x);
	file->tag.set_property("HANDLE_Y", handle_y);
	for(int i = 0; i < edlsession->audio_channels; i++)
	{
		char  string[BCTEXTLEN];
		sprintf(string, "VALUE%d", i);
		file->tag.set_property(string, values[i]);
	}
	file->append_tag();
	file->tag.set_title("/AUTO");
	file->append_tag();
}

void PanAuto::copy(Auto *src, ptstime start, ptstime end)
{
	PanAuto *that = (PanAuto*)src;

	pos_time = that->pos_time - start;
	handle_x = that->handle_x;
	handle_y = that->handle_y;

	for(int i = 0; i < edlsession->audio_channels; i++)
		values[i] = that->values[i];
}

void PanAuto::copy_from(Auto *that)
{
	Auto::copy_from(that);

	PanAuto *pan_auto = (PanAuto*)that;
	memcpy(this->values, pan_auto->values, MAXCHANNELS * sizeof(double));
	this->handle_x = pan_auto->handle_x;
	this->handle_y = pan_auto->handle_y;
}

size_t PanAuto::get_size()
{
	return sizeof(*this);
}

void PanAuto::dump(int indent)
{
	printf("%*sPanAuto %p: %.3f handles: %d %d\n",
		indent, "", this, pos_time, handle_x, handle_y);
	printf("%*svalues:", indent + 2, "");
	for(int i = 0; i < edlsession->audio_channels; i++)
		printf(" %.1f", values[i]);
	putchar('\n');
}
