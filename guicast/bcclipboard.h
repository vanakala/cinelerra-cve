
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

#ifndef BCCLIPBOARD_H
#define BCCLIPBOARD_H

#include "thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

// The primary selection is filled by highlighting a region
#define PRIMARY_SELECTION 0
// The secondary selection is filled by copying
#define SECONDARY_SELECTION 1

class BC_Clipboard : public Thread
{
public:
	BC_Clipboard(char *display_name);
	~BC_Clipboard();

	int start_clipboard();
	void run();
	int stop_clipboard();
	long clipboard_len(int clipboard_num);
	int to_clipboard(char *data, long len, int clipboard_num);
	int from_clipboard(char *data, long maxlen, int clipboard_num);

private:
	void handle_selectionrequest(XSelectionRequestEvent *request);
	int handle_request_string(XSelectionRequestEvent *request);
	int handle_request_targets(XSelectionRequestEvent *request);

	Display *in_display, *out_display;
	Atom completion_atom, primary, secondary;
	Atom targets_atom;
	Window in_win, out_win;
	char *data[2];
	long length[2];
};

#endif
