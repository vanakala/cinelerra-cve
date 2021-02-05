// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef EDITPANEL_INC
#define EDITPANEL_INC

class EditPanel;

// use_flag bits
#define EDTP_MWINDOW      0x0001
#define EDTP_EDITING_MODE 0x0002
#define EDTP_KEYFRAME     0x0004
#define EDTP_SPLICE       0x0008
#define EDTP_OVERWRITE    0x0010
#define EDTP_COPY         0x0020
#define EDTP_PASTE        0x0040
#define EDTP_UNDO         0x0080
#define EDTP_FIT          0x0100
#define EDTP_LOCKLABELS   0x0200
#define EDTP_LABELS       0x0400
#define EDTP_TOCLIP       0x0800
#define EDTP_CUT          0x1000

#endif
