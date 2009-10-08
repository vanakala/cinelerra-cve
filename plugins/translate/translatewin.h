
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

#ifndef TRANSLATEWIN_H
#define TRANSLATEWIN_H

#include "guicast.h"

class TranslateThread;
class TranslateWin;

#include "filexml.h"
#include "mutex.h"
#include "pluginclient.h"
#include "translate.h"


PLUGIN_THREAD_HEADER(TranslateMain, TranslateThread, TranslateWin)

class TranslateCoord;

class TranslateWin : public BC_Window
{
public:
	TranslateWin(TranslateMain *client, int x, int y);
	~TranslateWin();

	int create_objects();
	int close_event();

	TranslateCoord *in_x, *in_y, *in_w, *in_h, *out_x, *out_y, *out_w, *out_h;
	TranslateMain *client;
};

class TranslateCoord : public BC_TumbleTextBox
{
public:
	TranslateCoord(TranslateWin *win, 
		TranslateMain *client, 
		int x, 
		int y, 
		float *value);
	~TranslateCoord();
	int handle_event();

	TranslateMain *client;
	TranslateWin *win;
	float *value;
};


#endif
