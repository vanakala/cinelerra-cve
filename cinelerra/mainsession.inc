// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINSESSION_INC
#define MAINSESSION_INC

class MainSession;
extern MainSession *mainsession;

// Current operation in main window
enum
{
	NO_OPERATION,
	DRAG_EDITHANDLE1,    // Waiting to move out of drag threshold
	DRAG_EDITHANDLE2,    // Dragging outside drag threshold
	DRAG_PARTITION,
	SELECT_REGION,
	DRAG_ASSET,
	DRAG_ATRANSITION,
	DRAG_VTRANSITION,
	DRAG_AEFFECT,
	DRAG_VEFFECT,
	DRAG_AEFFECT_COPY,  // 10
	DRAG_VEFFECT_COPY,
	DRAG_EDIT,
	DRAG_FADE,
	DRAG_CZOOM,
	DRAG_PZOOM,
	DRAG_PAN_PRE,
	DRAG_PAN,
	DRAG_PLAY,
	DRAG_MUTE,
	DRAG_CAMERA_X,      // 20
	DRAG_CAMERA_Y,
	DRAG_CAMERA_Z,
	DRAG_MASK_PRE,
	DRAG_MASK,
	DRAG_MODE_PRE,
	DRAG_MODE,
	DRAG_PROJECTOR_X,
	DRAG_PROJECTOR_Y,
	DRAG_PROJECTOR_Z,
	DRAG_PLUGINKEY_PRE, // 30
	DRAG_PLUGINKEY,
	DRAG_PLUGINHANDLE1, // Dragging started inside plugin
	DRAG_PLUGINHANDLE2, // Dragging strtaed outside plugin
	DRAG_NUDGE,
	DRAG_CROP_PRE,
	DRAG_CROP
};


#endif
