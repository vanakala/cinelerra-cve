
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

#ifndef BLURWINDOW_H
#define BLURWINDOW_H

#include "bctoggle.h"
#include "bcpot.h"
#include "blur.h"
#include "mutex.h"
#include "pluginwindow.h"

class BlurVertical;
class BlurHorizontal;
class BlurRadius;
class BlurA;
class BlurR;
class BlurG;
class BlurB;

PLUGIN_THREAD_HEADER

class BlurWindow : public PluginWindow
{
public:
	BlurWindow(BlurMain *plugin, int x, int y);

	void update();

	BlurVertical *vertical;
	BlurHorizontal *horizontal;
	BlurRadius *radius;
	BlurA *a;
	BlurR *r;
	BlurG *g;
	BlurB *b;
	PLUGIN_GUI_CLASS_MEMBERS
};

class BlurA : public BC_CheckBox
{
public:
	BlurA(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};

class BlurR : public BC_CheckBox
{
public:
	BlurR(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};

class BlurG : public BC_CheckBox
{
public:
	BlurG(BlurMain *client, int x, int y);
	int handle_event();
	BlurMain *client;
};

class BlurB : public BC_CheckBox
{
public:
	BlurB(BlurMain *client, int x, int y);

	int handle_event();

	BlurMain *client;
};


class BlurRadius : public BC_IPot
{
public:
	BlurRadius(BlurMain *client, int x, int y);

	int handle_event();

	BlurMain *client;
};

class BlurVertical : public BC_CheckBox
{
public:
	BlurVertical(BlurMain *client, int x, int y);

	int handle_event();

	BlurMain *client;
};

class BlurHorizontal : public BC_CheckBox
{
public:
	BlurHorizontal(BlurMain *client, int x, int y);

	int handle_event();

	BlurMain *client;
};

#endif
