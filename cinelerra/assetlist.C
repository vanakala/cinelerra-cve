
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

#include "asset.h"
#include "assetlist.h"
#include "bcsignals.h"
#include "filexml.h"
#include "file.h"

#include <stdio.h>

AssetList::AssetList()
 : List<Asset>()
{
}

AssetList::~AssetList()
{
	delete_all();
}

Asset* AssetList::add_asset(Asset *asset)
{
	if(!asset)
		return 0;

	for(Asset* current = first; current; current = NEXT)
	{
// Asset already exists.
		if(current == asset)
		{
			current->global_inuse = 1;
			return current;
		}
		if(current->test_path(asset))
		{
			current->global_inuse = 1;
			delete asset;
			return current;
		}
	}
// Asset doesn't exist.
	append(asset);
	if(asset->format == FILE_UNKNOWN || !asset->nb_streams)
		check_asset(asset);
	asset->global_inuse = 1;
	return asset;
}

void AssetList::delete_all()
{
	while(first)
		remove_asset(first);
}

Asset* AssetList::get_asset(const char *filename, int stream)
{
	for(Asset* current = first; current; current = current->next)
	{
		if(current->test_path(filename, stream))
		{
			current->global_inuse = 1;
			return current;
		}
	}
	return 0;
}

void AssetList::remove_asset(Asset *asset)
{
	remove(asset);
}

void AssetList::reset_inuse()
{
	for(Asset *current = first; current; current = current->next)
		current->global_inuse = 0;
}

void AssetList::asset_inuse(Asset *asset)
{
	asset->global_inuse++;
}

void AssetList::remove_unused()
{
	Asset *asset;

	for(Asset *current = first; current;)
	{
		if(!current->global_inuse && current->index_status == INDEX_READY)
		{
			asset = current->next;
			remove_asset(current);
			current = asset;
		}
		else
			current = current->next;
	}
}

void AssetList::load_assets(FileXML *file, ArrayList<Asset*> *assets)
{
	int i;

	while(!file->read_tag())
	{
		if(file->tag.title_is("/ASSETS"))
			break;
		if(file->tag.title_is("ASSET"))
		{
			Asset *new_asset = new Asset(file->tag.get_property("SRC"));
			new_asset->read(file);
			new_asset = add_asset(new_asset);
			if(assets)
			{
				for(i = 0; i < assets->total; i++)
				{
					if(assets->values[i] == new_asset)
						break;
				}
				if(i >= assets->total)
				{
					assets->append(new_asset);
				}
			}
		}
	}
}

int AssetList::check_asset(Asset *asset)
{
	File new_file;

	if(asset->format != FILE_UNKNOWN && asset->nb_streams)
		return FILE_OK;

	return new_file.open_file(asset, FILE_OPEN_READ | FILE_OPEN_ALL);
}

void AssetList::remove_assets(ArrayList<Asset*> *assets)
{
	for(int i = 0; i < assets->total; i++)
	{
		Asset *asset = assets->values[i];

		for(Asset* current = first; current; current = current->next)
		{
			if(asset == current)
			{
				remove_asset(current);
				break;
			}
		}
	}
}


void AssetList::dump(int indent)
{
	printf("%*sAssetList %p dump(%d)\n", indent, "", this, total());
	indent += 1;
	for(Asset *current = first; current; current = NEXT)
	{
		current->dump(indent);
	}
}
