
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

#ifndef SHARPENWINDOW_H
#define SHARPENWINDOW_H

#include "guicast.h"

class SharpenThread;
class SharpenWindow;
class SharpenInterlace;

#include "filexml.h"
#include "mutex.h"
#include "sharpen.h"





PLUGIN_THREAD_HEADER(SharpenMain, SharpenThread, SharpenWindow)

class SharpenSlider;
class SharpenHorizontal;
class SharpenLuminance;

class SharpenWindow : public BC_Window
{
public:
	SharpenWindow(SharpenMain *client, int x, int y);
	~SharpenWindow();
	
	int create_objects();
	int close_event();
	
	SharpenMain *client;
	SharpenSlider *sharpen_slider;
	SharpenInterlace *sharpen_interlace;
	SharpenHorizontal *sharpen_horizontal;
	SharpenLuminance *sharpen_luminance;
};

class SharpenSlider : public BC_ISlider
{
public:
	SharpenSlider(SharpenMain *client, float *output, int x, int y);
	~SharpenSlider();
	int handle_event();

	SharpenMain *client;
	float *output;
};

class SharpenInterlace : public BC_CheckBox
{
public:
	SharpenInterlace(SharpenMain *client, int x, int y);
	~SharpenInterlace();
	int handle_event();

	SharpenMain *client;
};

class SharpenHorizontal : public BC_CheckBox
{
public:
	SharpenHorizontal(SharpenMain *client, int x, int y);
	~SharpenHorizontal();
	int handle_event();

	SharpenMain *client;
};

class SharpenLuminance : public BC_CheckBox
{
public:
	SharpenLuminance(SharpenMain *client, int x, int y);
	~SharpenLuminance();
	int handle_event();

	SharpenMain *client;
};


#endif
