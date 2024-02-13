// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2023 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bcsignals.h"
#include "bcwindowbase.h"
#include <stdio.h>

// Debugging helpers

void BC_WindowBase::dump_XVisualInfo(XVisualInfo *visinfo, int indent)
{
	printf("%*sXVisualInfo %p dump:\n", indent, "", visinfo);
	indent += 2;
	printf("%*sVisual %p VisualID %#lx screen %d depth %d\n", indent, "",
		visinfo->visual, visinfo->visualid, visinfo->screen, visinfo->depth);
	printf("%*sclass '%s' masks red %#lx green %#lx blue %#lx\n", indent, "",
		visual_class_name(visinfo->c_class), visinfo->red_mask, visinfo->green_mask,
		visinfo->blue_mask);
	printf("%*scolormap_size %d bits_per_rgb %d\n", indent, "",
		visinfo->colormap_size, visinfo->bits_per_rgb);
}

const char *BC_WindowBase::visual_class_name(int c_class)
{
	static char bufr[64];

	switch(c_class)
	{
	case StaticGray:
		return "StaticGray";
	case GrayScale:
		return "GrayScale";
	case StaticColor:
		return "StaticColor";
	case PseudoColor:
		return "PseudoColor";
	case TrueColor:
		return "TrueColor";
	case DirectColor:
		return "DirectColor";
	default:
		sprintf(bufr, "Unknown (%d)", c_class);
		return bufr;
	}
}

void BC_WindowBase::dump_XSizeHints(XSizeHints *sizehints, int indent)
{
	printf("%*sXSizeHints %p dump:\n", indent, "", sizehints);
	indent++;
	printf("%*sFlags %s\n", indent, "", size_hints_mask(sizehints->flags));
	if(sizehints->flags & (PPosition | PSize | USPosition | USSize | PMinSize | PMaxSize))
		printf("%*s(%d,%d) [%d,%d] min:[%d,%d] max:[%d,%d]\n", indent, "",
			sizehints->x, sizehints->y, sizehints->width, sizehints->height,
			sizehints->min_width, sizehints->min_height,
			sizehints->max_width, sizehints->max_height);
	if(sizehints->flags & (PResizeInc | PAspect))
		printf("%*sincs: [%d,%d] aspects %d/%d, %d/%d\n", indent, "",
			sizehints->width_inc, sizehints->height_inc,
			sizehints->min_aspect.x, sizehints->min_aspect.y,
			sizehints->max_aspect.y, sizehints->max_aspect.y);
	if(sizehints->flags & (PBaseSize | PWinGravity))
		printf("%*sbase [%d,%d] gravity %d\n", indent, "",
			sizehints->base_width, sizehints->base_height,
			sizehints->win_gravity);
}

char *BC_WindowBase::size_hints_mask(long mask)
{
	static char bufr[128];
	char *p;

	p = bufr;

	if(mask & USPosition)
		p = append_bufr(p, "USPosition|");
	if(mask & USSize)
		p = append_bufr(p, "USSize|");
	if(mask & PPosition)
		p = append_bufr(p, "PPosition|");
	if(mask & PSize)
		p = append_bufr(p, "PSize|");
	if(mask & PMinSize)
		p = append_bufr(p, "PMinSize|");
	if(mask & PMaxSize)
		p = append_bufr(p, "PMaxSize|");
	if(mask & PResizeInc)
		p = append_bufr(p, "PResizeInc|");
	if(mask & PAspect)
		p = append_bufr(p, "PAspect|");
	if(mask & PBaseSize)
		p = append_bufr(p, "PBaseSize|");
	if(mask & PWinGravity)
		p = append_bufr(p, "PWinGravity|");

	while(--p > bufr && (*p == '|' || *p == ' '));
	*++p = 0;
	return bufr;
}

char *BC_WindowBase::append_bufr(char *bufr, const char *str)
{
	char *p;

	strcpy(bufr, str);
	for(p = bufr; *p; p++);
	return p;
}
