
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

#include "bctextbox.h"
#include "bctoggle.h"
#include "filexml.h"
#include "mutex.h"
#include "ivtc.h"
#include "pluginwindow.h"

#define TOTAL_PATTERNS 3

PLUGIN_THREAD_HEADER

class IVTCOffset;
class IVTCFieldOrder;
class IVTCAuto;
class IVTCAutoThreshold;
class IVTCPattern;

class IVTCWindow : public PluginWindow
{
public:
	IVTCWindow(IVTCMain *plugin, int x, int y);
	~IVTCWindow();

	void update();

	IVTCOffset *frame_offset;
	IVTCFieldOrder *first_field;
	IVTCAutoThreshold *threshold;
	IVTCPattern *pattern[TOTAL_PATTERNS];
	PLUGIN_GUI_CLASS_MEMBERS
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

class IVTCPattern : public BC_Radial
{
public:
	IVTCPattern(IVTCMain *client, 
		IVTCWindow *window, 
		int number, 
		const char *text,
		int x, 
		int y);
	~IVTCPattern();
	int handle_event();
	IVTCWindow *window;
	IVTCMain *client;
	int number;
};

#endif
