// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FLOATAUTO_INC
#define FLOATAUTO_INC

class FloatAuto;
// Possible policies to handle the tagents for the
// bézier curves connecting adjacent automation points
enum tgnt_mode
{
	TGNT_SMOOTH,     // tangents are coupled in order to yield a smooth curve
	TGNT_LINEAR,     // tangents always pointing directly to neighbouring automatio
	TGNT_TFREE,      // tangents on both sides coupled but editable by dragging the
	TGNT_FREE        // tangents on both sides are independent and editable via GUI
};

#endif
