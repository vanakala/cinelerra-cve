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

	Display *in_display, *out_display;
	Atom completion_atom, primary, secondary;
	Window in_win, out_win;
	char *data[2];
	long length[2];
};

#endif
