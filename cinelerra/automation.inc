
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

#ifndef AUTOMATION_INC
#define AUTOMATION_INC

class Automation;

// These must match the following arrays:
// xml_titles in autoconf.C and automation.C.
// auto_colors in trackcanvas.C
// auto_operations in trackcanvas.C
// auto_pixmaps in trackcanvas.C
// auto_text in gwindowgui.C
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
//	AUTOMATION_NUDGE,
	AUTOMATION_TOTAL
};



// Automation group types (used by the gui to control the ranges)

// These must match the following arrays:
// xml_autogrouptypes_* in localsession.C
// autogrouptypes_fixedrange in automation.C
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
	AUTOMATION_TYPE_PAN
};


#endif
