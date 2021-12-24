
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
#include "file.h"
#include "sighandler.h"


SigHandler::SigHandler()
 : BC_Signals()
{
}

void SigHandler::signal_handler(int signum)
{
	printf("SigHandler::signal_handler total files=%d\n", 
		files.total);
	for(int i = 0; i < files.total; i++)
	{
		printf("Closing %s\n", files.values[i]->asset->path);
		files.values[i]->close_file();
	}
}

void SigHandler::push_file(File *file)
{
// Check for duplicate
	for(int i = 0; i < files.total; i++)
	{
		if(files.values[i] == file)
		{
			printf("SigHandler::push_file: file %s already on table.\n",
				file->asset->path);
			return;
		}
	}

// Append file
	files.append(file);
}

void SigHandler::pull_file(File *file)
{
	for(int i = 0; i < files.total; i++)
	{
		if(files.values[i] == file)
		{
			files.remove_number(i);
			return;
		}
	}
	printf("SigHandler::pull_file: file %s not on table.\n",
		file->asset->path);
}











