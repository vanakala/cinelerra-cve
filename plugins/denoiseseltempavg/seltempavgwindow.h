
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

#ifndef TIMEAVGWINDOW_H
#define TIMEAVGWINDOW_H


class SelTempAvgThread;
class SelTempAvgWindow;

class SelTempAvgParanoid;
class SelTempAvgNoSubtract;
class SelTempAvgStartKeyframe;
class SelTempAvgMask;

class SelTempAvgOffsetRadial;
class SelTempAvgMethodRadial;

class SelTempAvgSlider;

class SelTempAvgThreshSlider;
class SelTempAvgOffsetValue;
class SelTempAvgGainValue;


#include "guicast.h"
#include "mutex.h"
#include "seltempavg.h"

PLUGIN_THREAD_HEADER(SelTempAvgMain, SelTempAvgThread, SelTempAvgWindow)





enum {
  AVG_RY,
  AVG_GU,
  AVG_BV,
  STD_RY,
  STD_GU,
  STD_BV
};

enum {
  MASK_RY,
  MASK_GU,
  MASK_BV
};


class SelTempAvgWindow : public BC_Window
{
public:
	SelTempAvgWindow(SelTempAvgMain *client, int x, int y);
	~SelTempAvgWindow();
	
	int create_objects();
	int close_event();
	
	SelTempAvgMain *client;
	SelTempAvgSlider *total_frames;

	SelTempAvgThreshSlider *avg_threshold_RY, *avg_threshold_GU, *avg_threshold_BV;
	SelTempAvgThreshSlider *std_threshold_RY, *std_threshold_GU, *std_threshold_BV;
	SelTempAvgMask *mask_RY, *mask_GU, *mask_BV;

	SelTempAvgOffsetRadial *offset_fixed, *offset_restartmarker;
	SelTempAvgMethodRadial *method_none, *method_seltempavg, *method_stddev, *method_average;

	SelTempAvgParanoid *paranoid;
	SelTempAvgNoSubtract *no_subtract;
	SelTempAvgStartKeyframe *offset_restartmarker_keyframe;
	BC_TextBox                *offset_restartmarker_pos;

	SelTempAvgOffsetValue *offset_fixed_value;
	SelTempAvgGainValue *gain;
};

class SelTempAvgThreshSlider : public BC_TextBox
{
public:
	SelTempAvgThreshSlider(SelTempAvgMain *client, int x, int y, int type, float curval);
	~SelTempAvgThreshSlider();
	int handle_event();
	int type;
	SelTempAvgMain *client;
};


class SelTempAvgOffsetValue : public BC_TextBox
{
public:
	SelTempAvgOffsetValue(SelTempAvgMain *client, int x, int y);
	~SelTempAvgOffsetValue();
	int handle_event();
	SelTempAvgMain *client;
};


class SelTempAvgGainValue : public BC_TextBox
{
public:
	SelTempAvgGainValue(SelTempAvgMain *client, int x, int y);
	~SelTempAvgGainValue();
	int handle_event();
	SelTempAvgMain *client;
};


class SelTempAvgSlider : public BC_ISlider
{
public:
	SelTempAvgSlider(SelTempAvgMain *client, int x, int y);
	~SelTempAvgSlider();
	int handle_event();

	SelTempAvgMain *client;
};


class SelTempAvgOffsetRadial : public BC_Radial
{
public:
	SelTempAvgOffsetRadial(SelTempAvgMain *client, SelTempAvgWindow *gui, int x, int y, int type, char *caption);
	int handle_event();
	SelTempAvgMain *client;
	SelTempAvgWindow *gui;
	int type;
};


class SelTempAvgMethodRadial : public BC_Radial
{
public:
	SelTempAvgMethodRadial(SelTempAvgMain *client, SelTempAvgWindow *gui, int x, int y, int type, char *caption);
	int handle_event();
	SelTempAvgMain *client;
	SelTempAvgWindow *gui;
	int type;
};

class SelTempAvgParanoid : public BC_CheckBox
{
public:
	SelTempAvgParanoid(SelTempAvgMain *client, int x, int y);
	int handle_event();
	SelTempAvgMain *client;
};

class SelTempAvgNoSubtract : public BC_CheckBox
{
public:
	SelTempAvgNoSubtract(SelTempAvgMain *client, int x, int y);
	int handle_event();
	SelTempAvgMain *client;
};

class SelTempAvgMask : public BC_CheckBox
{
public:
	SelTempAvgMask(SelTempAvgMain *client, int x, int y,int type, int val);
	int handle_event();
	SelTempAvgMain *client;
	int type;
};


class SelTempAvgStartKeyframe : public BC_CheckBox
{
public:
	SelTempAvgStartKeyframe(SelTempAvgMain *client, int x, int y);
	int handle_event();
	SelTempAvgMain *client;
};


#endif
