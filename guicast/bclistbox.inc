
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

#ifndef BCLISTBOX_INC
#define BCLISTBOX_INC

// Selection mode
#define LISTBOX_SINGLE   0      // Single selection
#define LISTBOX_MULTIPLE 1      // Multiple selections

// Display format
#define LISTBOX_TEXT     0
#define LISTBOX_ICONS    1

// Icon position
#define ICON_LEFT        0
#define ICON_TOP         1

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
