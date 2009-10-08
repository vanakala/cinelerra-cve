
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

#ifndef IVTCWINDOW_H
#define IVTCWINDOW_H

#include "guicast.h"

class IVTCThread;
class IVTCWindow;

#include "filexml.h"
#include "mutex.h"
#include "ivtc.h"

#define TOTAL_PATTERNS 3

PLUGIN_THREAD_HEADER(IVTCMain, IVTCThread, IVTCWindow)


class IVTCOffset;
class IVTCFieldOrder;
class IVTCAuto;
class IVTCAutoThreshold;
class IVTCPattern;

class IVTCWindow : public BC_Window
{
public:
	IVTCWindow(IVTCMain *client, int x, int y);
	~IVTCWindow();
	
	int create_objects();
	int close_event();
	
	IVTCMain *client;
	IVTCOffset *frame_offset;
	IVTCFieldOrder *first_field;
//	IVTCAuto *automatic;
	IVTCAutoThreshold *threshold;
	IVTCPattern *pattern[TOTAL_PATTERNS];
};

class IVTCOffset : public BC_TextBox
{
public:
	IVTCOffset(IVTCMain *client, int x, int y);
	~IVTCOffset();
	int handle_event();
	IVTCMain *client;
};

class IVTCFieldOrder : public BC_CheckBox
{
public:
	IVTCFieldOrder(IVTCMain *client, int x, int y);
	~IVTCFieldOrder();
	int handle_event();
	IVTCMain *client;
};

class IVTCAuto : public BC_CheckBox
{
public:
	IVTCAuto(IVTCMain *client, int x, int y);
	~IVTCAuto();
	int handle_event();
	IVTCMain *client;
};

class IVTCPattern : public BC_Radial
{
public:
	IVTCPattern(IVTCMain *client, 
		IVTCWindow *window, 
		int number, 
		char *text, 
		int x, 
		int y);
	~IVTCPattern();
	int handle_event();
	IVTCWindow *window;
	IVTCMain *client;
	int number;
};

class IVTCAutoThreshold : public BC_TextBox
{
public:
	IVTCAutoThreshold(IVTCMain *client, int x, int y);
	~IVTCAutoThreshold();
	int handle_event();
	IVTCMain *client;
};

#endif
