
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

#ifndef ASSETS_H
#define ASSETS_H

#include <stdio.h>
#include <stdint.h>

#include "arraylist.h"
#include "asset.inc"
#include "assets.inc"
#include "batch.inc"
#include "bchash.inc"
#include "edl.inc"
#include "filexml.inc"
#include "guicast.h"
#include "linklist.h"
#include "pluginserver.inc"
#include "sharedlocation.h"

class Assets : public List<Asset>
{
public:
	Assets(EDL *edl);
	virtual ~Assets();

	int load(ArrayList<PluginServer*> *plugindb, 
		FileXML *xml, 
		uint32_t load_flags);
	int save(ArrayList<PluginServer*> *plugindb, 
		FileXML *xml, 
		char *output_path);
	Assets& operator=(Assets &assets);
	void copy_from(Assets *assets);

// Enter a new asset into the table.
// If the asset already exists return the asset which exists.
// If the asset doesn't exist, store a copy of the argument and return the copy.
	Asset* update(Asset *asset);
// Update the index information for assets with the same path
	void update_index(Asset *asset);


// Parent EDL
	EDL *edl;
	








	int delete_all();
	int dump();

// return the asset containing this path or create a new asset containing this path
	Asset* update(const char *path);

// Insert the asset into the list if it doesn't exist already but
// don't create a new asset.
	void update_ptr(Asset *asset);

// return the asset containing this path or 0 if not found
	Asset* get_asset(const char *filename);
// remove asset from table
	Asset* remove_asset(Asset *asset);

// return number of the asset
	int number_of(Asset *asset);
	Asset* asset_number(int number);

	int update_old_filename(char *old_filename, char *new_filename);
};




#endif
