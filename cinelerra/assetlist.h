// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef ASSETLIST_H
#define ASSETLIST_H

#include "arraylist.h"
#include "asset.inc"
#include "assetlist.inc"
#include "filexml.inc"
#include "linklist.h"

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
// Fill properties of asset
	int check_asset(Asset *asset);
// Remove listed assets
	void remove_assets(ArrayList<Asset*> *assets);

	void dump(int indent = 0);
};

#endif
