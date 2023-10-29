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