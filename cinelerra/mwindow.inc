// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MWINDOW_INC
#define MWINDOW_INC

#define PROGRAM_NAME "Cinelerra-CVE"
#define DEFAULT_THEME "S.U.V."
#define CONFIG_FILE "Cinelerra_rc"

// Behavior of region selections
#define SELECTION_SAMPLES 0
#define SELECTION_EDIT 1

// Behaviour of paste effects operations
#define PASTE_NONE    1
#define PASTE_AUTOS   2
#define PASTE_PLUGINS 3
#define PASTE_ALL     4

class MWindow;
extern MWindow *mwindow_global;

#endif
