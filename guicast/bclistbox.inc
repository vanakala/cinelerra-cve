// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef BCLISTBOX_INC
#define BCLISTBOX_INC

// Option bits
#define LISTBOX_ICONS     0x0001   // Display text list or icons
#define LISTBOX_POPUP     0x0002   // If this listbox is a popup window with a button
#define LISTBOX_MULTIPLE  0x0004   // Select one item or multiple items
#define LISTBOX_ICON_TOP  0x0008   // Position of icon relative to text of each item (left or top)
#define LISTBOX_DRAG      0x0010   // Allow user to drag icons around
#define LISTBOX_SROW      0x0020   // Icons should be in single vertical row
#define LISTBOX_SMALLFONT 0x0040   // Use small font
#define LISTBOX_SHRINK    0x0080   // Shrink listbox to useful size

// White space around text
#define LISTBOX_MARGIN   4
// Distance from sides of window
#define LISTBOX_BORDER   2
#define ICON_MARGIN      1
#define LISTBOX_INDENT   10

// Justification for popup listbox
#define LISTBOX_LEFT     0
#define LISTBOX_RIGHT    1

class BC_ListBoxItem;
class BC_ListBox;

#endif
