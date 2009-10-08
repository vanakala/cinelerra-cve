
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

#ifndef PICTURE_H
#define PICTURE_H


// Container for picture controls like brightness.

#include "arraylist.h"
#include "bcwindowbase.inc"
#include "bchash.inc"
#include "mwindow.inc"

class PictureItem
{
public:
	PictureItem();
	~PictureItem();
	
	void copy_from(PictureItem *src);
	char* get_default_string(char *string);
	char name[BCTEXTLEN];
	int device_id;
	int min;
	int max;
	int default_;
	int step;
	int type;
	int value;
};


class PictureConfig
{
public:
	PictureConfig(BC_Hash *defaults);
	~PictureConfig();
	void copy_settings(PictureConfig *picture);
	void copy_usage(PictureConfig *picture);
	void load_defaults();
	void save_defaults();
	void set_item(int device_id, int value);
	void dump();

	int brightness;
	int hue;
	int color;
	int contrast;
	int whiteness;

// Flags for picture settings the device uses
	int use_brightness;
	int use_contrast;
	int use_color;
	int use_hue;
	int use_whiteness;

// For the latest APIs the controls are defined by the driver
// Search for existing driver with name.  If none exists, create it.
	PictureItem* new_item(const char *name);
	PictureItem* get_item(const char *name, int id);
	ArrayList<PictureItem*> controls;
// Pointer to MWindow::defaults get defaults
	BC_Hash *defaults;
};


#endif
