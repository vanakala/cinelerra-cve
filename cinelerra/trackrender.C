
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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
#include "edit.h"
#include "file.h"
#include "mainerror.h"
#include "track.h"
#include "trackrender.h"

TrackRender::TrackRender(Track *track)
{
	this->track = track;
	file = 0;
}

File *TrackRender::media_file(Edit *edit)
{
	int media_type;

	if(edit && edit->asset)
	{
		if(!file || file->asset != edit->asset)
		{
			if(file)
				file->close_file();
			else
				file = new File();
			switch(edit->track->data_type)
			{
			case TRACK_AUDIO:
				media_type = FILE_OPEN_AUDIO;
				break;
			case TRACK_VIDEO:
				media_type = FILE_OPEN_VIDEO;
				break;
			}
			if(file->open_file(edit->asset, FILE_OPEN_READ | media_type))
			{
				errormsg("TrackRender::media_file: Failed to open media %s\n",
					edit->asset->path);
				return 0;
			}
		}
		return file;
	}
	return 0;
}
