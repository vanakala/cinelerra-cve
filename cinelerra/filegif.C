
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

#include "assets.h"
#include "file.h"
#include "filetiff.h"
#include "interlacemodes.h"
#include "vframe.h"


FileGIF::FileGIF(Asset *asset)
 : FileBase(asset)
{
	reset_parameters();
	asset->video_data = 1;
	asset->format = FILE_GIF;
}

FileGIF::~FileGIF()
{
	close_file();
}

int FileGIF::reset_parameters_derived()
{
	data = 0;
}

int FileGIF::open_file(int rd, int wr)
{
	this->rd = rd;
	this->wr = wr;

// skip header for write
	if(wr)
	{
	}
	else
	if(rd)
	{
		return read_header();
	}
	return 0;
}

int64_t FileGIF::get_video_length()
{
	return -1;    // infinity
// should be determined by whether the GIF is animated
}

int64_t FileGIF::get_memory_usage()
{
// give buffer length plus padding
	if(data)
		return (int64_t)asset->width * asset->height * sizeof(VPixel);
	else
		return 256;
}

int FileGIF::close_file_derived()
{
	if(data) delete data;
	reset_parameters();
}

int FileGIF::read_header()
{
	GIF *stream;

	if(!(stream = GIFOpen(asset->path, "r")))
	{
		eprintf("Error while opening \"%s\" for reading. \n%m\n", asset->path);
		return 1;
	}
	
	GIFGetField(stream, GIFTAG_IMAGEWIDTH, &(asset->width));
	GIFGetField(stream, GIFTAG_IMAGELENGTH, &(asset->height));
	asset->interlacemode = BC_ILACE_MODE_NOTINTERLACED;
	asset->layers = 1;

	GIFClose(stream);
	return 0;
}


VFrame* FileGIF::read_frame(int use_alpha, int use_float)
{
	read_raw();
	return data;
}

int FileGIF::read_raw()
{
	if(!data)
	{
// read the raw data
		GIF *stream;
		unsigned char *raw_data;
		int i;

		if(!(stream = GIFOpen(asset->path, "r")))
		{
			eprintf("Error while opening \"%s\" for writing. \n%m\n", asset->path);
			return 1;
		}

		raw_data = new unsigned char[asset->width * asset->height * 4];
		GIFReadRGBAImage(stream, asset->width, asset->height, (uint32*)raw_data, 0);

		GIFClose(stream);

// convert to a Bcast 2000 Frame
		data = new VFrame(asset->width, asset->height);

		for(i = 0; i < asset->height; i++)
		{
			import_row(data->rows[asset->height - i - 1], &raw_data[i * asset->width * 4]);
		}

// delete temporary buffers
		delete raw_data;
	}
	return 0;
}

int FileGIF::import_row(VPixel *output, unsigned char *row_pointer)
{
	for(int i = 0, j = 0; j < asset->width; j++)
	{
#if (VMAX == 65535)
		output[j].red =  ((VWORD)row_pointer[i++]) << 8;
		output[j].green =  ((VWORD)row_pointer[i++]) << 8;
		output[j].blue = ((VWORD)row_pointer[i++]) << 8;
		output[j].alpha = ((VWORD)row_pointer[i++]) << 8;
#else
		output[j].red =  (VWORD)row_pointer[i++];
		output[j].green =  (VWORD)row_pointer[i++];
		output[j].blue = (VWORD)row_pointer[i++];
		output[j].alpha = (VWORD)row_pointer[i++];
#endif
	}
}
