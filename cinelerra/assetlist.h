
/*
 * CINELERRA
 * Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef ASSETLIST_H
#define ASSETLIST_H

#include "arraylist.h"
#include "asset.inc"
#include "assetlist.inc"
#include "filexml.inc"
#include "linklist.h"

extern AssetList assetlist_global;

class AssetList : public List<Asset>
{
public:
	AssetList();
	~AssetList();

// Adds asset. Deletes parameter, it asset is alredy in table
	Asset* add_asset(Asset *asset);
// Delete all assets
	void delete_all();
// return the asset containing this path or 0 if not found
	Asset* get_asset(const char *filename, int stream = -1);
// remove asset from table
	void remove_asset(Asset *asset);
// reset inuse of all assets in list
	void reset_inuse();
// increment inuse of the asset
	void asset_inuse(Asset *asset);
// remove unused assets from list
	void remove_unused();
// Load assets from file
	void load_assets(FileXML *file, ArrayList<Asset*> *assets);

	void dump(int indent = 0);
};

#endif
