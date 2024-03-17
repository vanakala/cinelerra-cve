// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LOADMODE_INC
#define LOADMODE_INC

class LoadMode;
class InsertionModeSelection;

// Load modes for loading files, importing recordings, importing effects
#define LOADMODE_NOTHING 0
#define LOADMODE_REPLACE 1
#define LOADMODE_REPLACE_CONCATENATE 2
#define LOADMODE_NEW_TRACKS 3
#define LOADMODE_CONCATENATE 4
#define LOADMODE_PASTE 5
#define LOADMODE_RESOURCESONLY 6

#define LOADMODE_MASK_ALL   0x7f
#define LOADMODE_MASK_NONO  0x7e

#endif
