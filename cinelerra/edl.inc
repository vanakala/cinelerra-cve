// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef EDL_INC
#define EDL_INC

class EDL;

extern EDL *master_edl;
extern EDL *vwindow_edl;
extern EDL *render_edl;

// Load flags for undo
#define LOAD_ALL        0xffffffff
#define LOAD_EDITS      0x00000001
#define LOAD_AUTOMATION 0x00000002
#define LOAD_PATCHES    0x00000004
#define LOAD_CONSOLE    0x00000008
#define LOAD_TIMEBAR    0x00000010
#define LOAD_ACONFIG    0x00000020
#define LOAD_VCONFIG    0x00000040
#define LOAD_ASSETS     0x00000080
#define LOAD_SESSION    0x00000100


#define EDITING_MODES 2

// Editing modes
#define EDITING_ARROW 0
#define EDITING_IBEAM 1

#define HANDLE_MAIN   0
#define HANDLE_LEFT   1
#define HANDLE_RIGHT  2

#define EDIT_HANDLE_MODES 3

// Behavior for the edit handles based on mouse button
#define MOVE_ALL_EDITS 0
#define MOVE_ONE_EDIT 1
#define MOVE_NO_EDITS 2
#define MOVE_EDITS_DISABLED 3


// AWindow folders
#define AW_NO_FOLDER         -1
#define AW_AEFFECT_FOLDER     0
#define AW_VEFFECT_FOLDER     1
#define AW_ATRANSITION_FOLDER 2
#define AW_VTRANSITION_FOLDER 3
#define AW_LABEL_FOLDER       4
#define AW_CLIP_FOLDER        5
#define AW_MEDIA_FOLDER       6
#define AWINDOW_FOLDERS       7

#define AWINDOW_MODES 2

// Modes for AWindow views
#define ASSETS_TEXT 0
#define ASSETS_ICONS 1

#define ASSET_COLUMNS 2

// Active tool in CWindow corresponds to a member of the CPanel array
// and the current drag operation

#define CPANEL_OPERATIONS 10


#define CWINDOW_NONE -1
#define CWINDOW_PROTECT     0
#define CWINDOW_ZOOM        1
#define CWINDOW_MASK        2
#define CWINDOW_RULER       3
#define CWINDOW_CAMERA      4
#define CWINDOW_PROJECTOR   5
#define CWINDOW_CROP        6
#define CWINDOW_EYEDROP     7
#define CWINDOW_TOOL_WINDOW 8
#define CWINDOW_TITLESAFE   9

// Current dragging operations not available in the CPanel
#define CWINDOW_SCROLL 0x10
#define CWINDOW_MASK_CONTROL_IN 0x15
#define CWINDOW_MASK_CONTROL_OUT 0x16
#define CWINDOW_MASK_TRANSLATE 0x17

# define DEFAULT_ZOOM_TIME (exp2(-6))
// Inclusive limits for zoom panel
#define MIN_ZOOM_TIME (exp2(-10))
#define MAX_ZOOM_TIME (exp2(6))
#define MIN_AMP_ZOOM 8
#define MAX_AMP_ZOOM 262144
#define MIN_TRACK_ZOOM 32
#define MAX_TRACK_ZOOM 1024

// Meter delays (in seconds)
#define OVER_DELAY 3
#define PEAK_DELAY 1

#define MAX_PAN 1.0
#define PAN_RADIUS 50

// Flags of save_xml
#define EDL_CLIP      1
#define EDL_VWINDOW   2
#define EDL_CLIPBRD   4
#define EDL_UNDO      8

#endif
