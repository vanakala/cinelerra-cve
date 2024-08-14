// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "asset.h"
#include "assetlist.h"
#include "bcsignals.h"
#include "filetoc.h"
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
	int stream;

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
		if(current->check_programs(asset))
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
		if(current->check_stream(filename, stream))
		{
			current->global_inuse = 1;
			return current;
		}
	}
	return 0;
}

void AssetList::remove_asset(Asset *asset)
{
	FileTOC *tocfile = asset->tocfile;

	remove(asset);
	// tocfiles of multiprogram assets are shared
	for(Asset* current = first; current; current = current->next)
	{
		if(current->tocfile == tocfile)
		{
			tocfile = 0;
			break;
		}
	}
	delete tocfile;
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
		if(!current->global_inuse)
		{
			int inuse = 0;
			int stream = -1;

			while((stream = current->get_stream_ix(STRDSC_AUDIO, stream)) >= 0)
			{
				if(current->indexfiles[stream].status != INDEX_READY)
					inuse++;
			}

			if(!inuse)
			{
				asset = current->next;
				remove_asset(current);
				current = asset;
			}
			else
				current = current->next;
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

	return new_file.probe_file(asset);
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
