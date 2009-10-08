
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

#include "asset.h"
#include "assets.h"
#include "awindowgui.inc"
#include "batch.h"
#include "cache.h"
#include "bchash.h"
#include "edl.h"
#include "file.h"
#include "filexml.h"
#include "filesystem.h"
#include "indexfile.h"
#include "quicktime.h"
#include "mainsession.h"
#include "threadindexer.h"
#include <string.h>

Assets::Assets(EDL *edl) : List<Asset>()
{
	this->edl = edl;
}

Assets::~Assets()
{
	delete_all();
}

int Assets::load(ArrayList<PluginServer*> *plugindb, 
	FileXML *file, 
	uint32_t load_flags)
{
	int result = 0;

//printf("Assets::load 1\n");
	while(!result)
	{
		result = file->read_tag();
		if(!result)
		{
			if(file->tag.title_is("/ASSETS"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("ASSET"))
			{
//printf("Assets::load 2\n");
				char *path = file->tag.get_property("SRC");
//printf("Assets::load 3\n");
				Asset *new_asset = new Asset(path ? path : SILENCE);
//printf("Assets::load 4\n");
				new_asset->read(file);
//printf("Assets::load 5\n");
				update(new_asset);
				Garbage::delete_object(new_asset);
//printf("Assets::load 6\n");
			}
		}
	}
//printf("Assets::load 7\n");
	return 0;
}

int Assets::save(ArrayList<PluginServer*> *plugindb, FileXML *file, char *path)
{
	file->tag.set_title("ASSETS");
	file->append_tag();
	file->append_newline();

	for(Asset* current = first; current; current = NEXT)
	{
		current->write(file, 
			0, 
			path);
	}

	file->tag.set_title("/ASSETS");
	file->append_tag();
	file->append_newline();	
	file->append_newline();	
	return 0;
}

void Assets::copy_from(Assets *assets)
{
	delete_all();

	for(Asset *current = assets->first; current; current = NEXT)
	{
		Asset *new_asset;
		append(new_asset = new Asset);
		new_asset->copy_from(current, 1);
	}
}

Assets& Assets::operator=(Assets &assets)
{
printf("Assets::operator= 1\n");
	copy_from(&assets);
	return *this;
}


void Assets::update_index(Asset *asset)
{
	for(Asset* current = first; current; current = NEXT)
	{
		if(current->test_path(asset->path))
		{
			current->update_index(asset);
		}
	}
}

Asset* Assets::update(Asset *asset)
{
	if(!asset) return 0;

	for(Asset* current = first; current; current = NEXT)
	{
// Asset already exists.
		if(current->test_path(asset->path)) 
		{
			return current;
		}
	}

// Asset doesn't exist.
	Asset *asset_copy = new Asset(*asset);
	append(asset_copy);
	return asset_copy;
}

int Assets::delete_all()
{
	while(first) 
	{
		remove_asset(first);
	}
	return 0;
}

Asset* Assets::update(const char *path)
{
	Asset* current = first;

	while(current)
	{
		if(current->test_path(path)) 
		{
			return current;
		}
		current = NEXT;
	}

	return append(new Asset(path));
}

Asset* Assets::get_asset(const char *filename)
{
	Asset* current = first;
	Asset* result = 0;

	while(current)
	{
//printf("Assets::get_asset %p %s\n", filename, filename);
		if(current->test_path(filename))
		{
			result = current;
			break;
		}
		current = current->next;
	}

	return result;	
}

Asset* Assets::remove_asset(Asset *asset)
{
	remove_pointer(asset);
	Garbage::delete_object(asset);
}


int Assets::number_of(Asset *asset)
{
	int i;
	Asset *current;

	for(i = 0, current = first; current && current != asset; i++, current = NEXT)
		;

	return i;
}

Asset* Assets::asset_number(int number)
{
	int i;
	Asset *current;

	for(i = 0, current = first; i < number && current; i++, current = NEXT)
		;
	
	return current;
}

int Assets::update_old_filename(char *old_filename, char *new_filename)
{
	for(Asset* current = first; current; current = NEXT)
	{
		if(!strcmp(current->path, old_filename))
		{
			current->update_path(new_filename);
		}
	}
	return 0;
}


int Assets::dump()
{
	for(Asset *current = first; current; current = NEXT)
	{
		current->dump();
	}
	return 0;
}


