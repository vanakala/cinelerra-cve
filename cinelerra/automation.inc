// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AUTOMATION_INC
#define AUTOMATION_INC

class Automation;

// These must match the following array:
// Automation::automation_tbl in automation.C
enum
{
	AUTOMATION_MUTE,
	AUTOMATION_CAMERA_X,
	AUTOMATION_CAMERA_Y,
	AUTOMATION_CAMERA_Z,
	AUTOMATION_PROJECTOR_X,
	AUTOMATION_PROJECTOR_Y,
	AUTOMATION_PROJECTOR_Z,
	AUTOMATION_FADE,
	AUTOMATION_PAN,
	AUTOMATION_MODE,
	AUTOMATION_MASK,
	AUTOMATION_CROP,
	AUTOMATION_TOTAL
};

// Automation group types (used by the gui to control the ranges)
// These must match the following array:
// Automation::autogrouptypes in automation.C
enum
{
	AUTOGROUPTYPE_AUDIO_FADE,
	AUTOGROUPTYPE_VIDEO_FADE,
	AUTOGROUPTYPE_ZOOM,
	AUTOGROUPTYPE_X,
	AUTOGROUPTYPE_Y,
	AUTOGROUPTYPE_INT255,
	AUTOGROUPTYPE_COUNT
};

// Automation types
enum
{
	AUTOMATION_TYPE_FLOAT,
	AUTOMATION_TYPE_MASK,
	AUTOMATION_TYPE_INT,
	AUTOMATION_TYPE_PAN,
	AUTOMATION_TYPE_CROP
};

#endif
