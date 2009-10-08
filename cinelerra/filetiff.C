
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
#include "edit.h"
#include "file.h"
#include "filetiff.h"
#include "interlacemodes.h"
#include "language.h"
#include "vframe.h"
#include "mainerror.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>

FileTIFF::FileTIFF(Asset *asset, File *file)
 : FileList(asset, file, "TIFFLIST", ".tif", FILE_TIFF, FILE_TIFF_LIST)
{
	asset->video_data = 1;
	temp = 0;
}

FileTIFF::~FileTIFF()
{
	if(temp) delete temp;
}


void FileTIFF::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(video_options)
	{
		TIFFConfigVideo *window = new TIFFConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}


int FileTIFF::check_sig(Asset *asset)
{
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{
		char test[10];
		fread(test, 10, 1, stream);
		fclose(stream);

		if(test[0] == 'I' && test[1] == 'I')
		{
			return 1;
		}
		else
		if(test[0] == 'T' && test[1] == 'I' && test[2] == 'F' && test[3] == 'F' && 
			test[4] == 'L' && test[5] == 'I' && test[6] == 'S' && test[7] == 'T')
		{
			return 1;
		}
		else
		if(strlen(asset->path) > 4 && 
			!strcasecmp(asset->path + strlen(asset->path) - 4, ".tif"))
		{
			return 1;
		}
		else
		if(strlen(asset->path) > 5 && 
			!strcasecmp(asset->path + strlen(asset->path) - 5, ".tiff"))
		{
			return 1;
		}
	}
	return 0;
}

char* FileTIFF::compression_to_str(int value)
{
	switch(value)
	{
		case FileTIFF::NONE: return "None"; break;
		case FileTIFF::LZW: return "LZW"; break;
		case FileTIFF::PACK_BITS: return "Pack Bits"; break;
		case FileTIFF::DEFLATE: return "Deflate"; break;
		case FileTIFF::JPEG: return "JPEG"; break;
		default: 
			return "None"; 
			break;
	}
}

char* FileTIFF::cmodel_to_str(int value)
{
	switch(value)
	{
		case FileTIFF::GREYSCALE: return "Greyscale"; break;
		case FileTIFF::RGB_888: return "RGB-8 Bit"; break;
		case FileTIFF::RGB_161616: return "RGB-16 Bit"; break;
		case FileTIFF::RGBA_8888: return "RGBA-8 Bit"; break;
		case FileTIFF::RGBA_16161616: return "RGBA-16 Bit"; break;
		case FileTIFF::RGB_FLOAT: return "RGB-FLOAT"; break;
		case FileTIFF::RGBA_FLOAT: return "RGBA-FLOAT"; break;
		default: 
			return "RGB-8 Bit"; 
			break;
	}
}


int FileTIFF::can_copy_from(Edit *edit, int64_t position)
{
	if(edit->asset->format == FILE_TIFF_LIST ||
		edit->asset->format == FILE_TIFF)
		return 1;
	
	return 0;
}



int FileTIFF::read_frame_header(char *path)
{
	TIFF *stream;
	int result = 0;

	if(!(stream = TIFFOpen(path, "rb")))
	{
		eprintf("Error while opening \"%s\" for reading. \n%m\n", asset->path);
		return 1;
	}

	char *ptr = 0;
	TIFFGetField(stream, TIFFTAG_MODEL, &ptr);
//printf("FileTIFF::read_frame_header 1 %s\n", ptr);
	if(ptr && !strcmp(ptr, "Canon EOS-1DS"))       // FIXME: Does this have a purpose?
	{
		printf("FileTIFF::read_frame_header: got a %s.\n",
			ptr);
	}

// The raw format for certain cameras deviates from TIFF here.

	TIFFGetField(stream, TIFFTAG_IMAGEWIDTH, &(asset->width));
	TIFFGetField(stream, TIFFTAG_IMAGELENGTH, &(asset->height));

	int components = 0;
	TIFFGetField(stream, TIFFTAG_SAMPLESPERPIXEL, &components);
	int bitspersample = 0;
	TIFFGetField(stream, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	int sampleformat = 0;
	TIFFGetField(stream, TIFFTAG_SAMPLEFORMAT, &sampleformat);

	if(bitspersample == 8 && components == 3)
		asset->tiff_cmodel = FileTIFF::RGB_888;
	else
	if(bitspersample == 16 && components == 3)
		asset->tiff_cmodel = FileTIFF::RGB_161616;
	else
	if(bitspersample == 8 && components == 4)
		asset->tiff_cmodel = FileTIFF::RGBA_8888;
	else
	if(bitspersample == 16 && components == 4)
		asset->tiff_cmodel = FileTIFF::RGBA_16161616;
	else
	if(bitspersample == 32 && components == 3)
		asset->tiff_cmodel = FileTIFF::RGB_FLOAT;
	else
	if(bitspersample == 32 && components == 4)
		asset->tiff_cmodel = FileTIFF::RGBA_FLOAT;
	else
	if(bitspersample == 8 && (components == 1 || components == 0))
		asset->tiff_cmodel = FileTIFF::GREYSCALE;

//printf("FileTIFF::read_frame_header %d %d %d\n", bitspersample, components, asset->tiff_cmodel);
	TIFFClose(stream);

	asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;

	return result;
}

int FileTIFF::colormodel_supported(int colormodel)
{
	switch(asset->tiff_cmodel)
	{
		case FileTIFF::RGB_888: return BC_RGB888; break;
		case FileTIFF::RGB_161616: return BC_RGB_FLOAT; break;
		case FileTIFF::GREYSCALE: return BC_RGB888; break;
		case FileTIFF::RGBA_8888: return BC_RGBA8888; break;
		case FileTIFF::RGBA_16161616: return BC_RGBA_FLOAT; break;
		case FileTIFF::RGB_FLOAT: return BC_RGB_FLOAT; break;
		case FileTIFF::RGBA_FLOAT: return BC_RGBA_FLOAT; break;
		default: return BC_RGB888; break;
	}
}

int FileTIFF::get_best_colormodel(Asset *asset, int driver)
{
	switch(asset->tiff_cmodel)
	{
		case FileTIFF::GREYSCALE: return BC_RGB888; break;
		case FileTIFF::RGB_888: return BC_RGB888; break;
		case FileTIFF::RGB_161616: return BC_RGB_FLOAT; break;
		case FileTIFF::RGBA_8888: return BC_RGBA8888; break;
		case FileTIFF::RGBA_16161616: return BC_RGBA_FLOAT; break;
		case FileTIFF::RGB_FLOAT: return BC_RGB_FLOAT; break;
		case FileTIFF::RGBA_FLOAT: return BC_RGBA_FLOAT; break;
		default: return BC_RGB888; break;
	}
}


static tsize_t tiff_read(thandle_t ptr, tdata_t buf, tsize_t size)
{
	FileTIFFUnit *tiff_unit = (FileTIFFUnit*)ptr;
	if(tiff_unit->data->get_compressed_size() < tiff_unit->offset + size)
		return 0;
	memcpy(buf, tiff_unit->data->get_data() + tiff_unit->offset, size);
	tiff_unit->offset += size;
	return size;
}

static tsize_t tiff_write(thandle_t ptr, tdata_t buf, tsize_t size)
{
	FileTIFFUnit *tiff_unit = (FileTIFFUnit*)ptr;
	if(tiff_unit->data->get_compressed_allocated() < tiff_unit->offset + size)
	{
		tiff_unit->data->allocate_compressed_data((tiff_unit->offset + size) * 2);
	}


	if(tiff_unit->data->get_compressed_size() < tiff_unit->offset + size)
		tiff_unit->data->set_compressed_size(tiff_unit->offset + size);
	memcpy(tiff_unit->data->get_data() + tiff_unit->offset,
		buf,
		size);
	tiff_unit->offset += size;
	return size;
}

static toff_t tiff_seek(thandle_t ptr, toff_t off, int whence)
{
	FileTIFFUnit *tiff_unit = (FileTIFFUnit*)ptr;
	switch(whence)
	{
		case SEEK_SET:
			tiff_unit->offset = off;
			break;
		case SEEK_CUR:
			tiff_unit->offset += off;
			break;
		case SEEK_END:
			tiff_unit->offset = tiff_unit->data->get_compressed_size() + off;
			break;
	}
	return tiff_unit->offset;
}

static int tiff_close(thandle_t ptr)
{
	return 0;
}

static toff_t tiff_size(thandle_t ptr)
{
	FileTIFFUnit *tiff_unit = (FileTIFFUnit*)ptr;
	return tiff_unit->data->get_compressed_size();
}

static int tiff_mmap(thandle_t ptr, tdata_t* pbase, toff_t* psize)
{
	FileTIFFUnit *tiff_unit = (FileTIFFUnit*)ptr;
	*pbase = tiff_unit->data->get_data();
	*psize = tiff_unit->data->get_compressed_size();
	return 0;
}

void tiff_unmap(thandle_t ptr, tdata_t base, toff_t size)
{
}

int FileTIFF::read_frame(VFrame *output, VFrame *input)
{
	FileTIFFUnit *unit = new FileTIFFUnit(this, 0);
	TIFF *stream;
	unit->offset = 0;
	unit->data = input;

	stream = TIFFClientOpen("FileTIFF", 
		"r",
	    (void*)unit,
	    tiff_read, 
		tiff_write,
	    tiff_seek, 
		tiff_close,
	    tiff_size,
	    tiff_mmap, 
		tiff_unmap);

// This loads the original TIFF data into each scanline of the output frame, 
// assuming the output scanlines are bigger than the input scanlines.
// Then it expands the input data in reverse to fill the row.
	for(int i = 0; i < asset->height; i++)
	{
		TIFFReadScanline(stream, output->get_rows()[i], i, 0);

// For the greyscale model, the output is RGB888 but the input must be expanded
		if(asset->tiff_cmodel == FileTIFF::GREYSCALE)
		{
			unsigned char *row = output->get_rows()[i];
			for(int j = output->get_w() - 1; j >= 0; j--)
			{
				unsigned char value = row[j];
				row[j * 3] = value;
				row[j * 3 + 1] = value;
				row[j * 3 + 2] = value;
			}
		}
// For the 16 bit models, the output is floating point.
		else
		if(asset->tiff_cmodel == FileTIFF::RGB_161616)
		{
			uint16_t *input_row = (uint16_t*)output->get_rows()[i];
			float *output_row = (float*)output->get_rows()[i];
			for(int j = output->get_w() - 1; j >= 0; j--)
			{
				uint16_t r = input_row[j * 3];
				uint16_t g = input_row[j * 3 + 1];
				uint16_t b = input_row[j * 3 + 2];
				output_row[j * 3] = (float)r / 65535;
				output_row[j * 3 + 1] = (float)g / 65535;
				output_row[j * 3 + 2] = (float)b / 65535;
			}
		}
		else
		if(asset->tiff_cmodel == FileTIFF::RGBA_16161616)
		{
			uint16_t *input_row = (uint16_t*)output->get_rows()[i];
			float *output_row = (float*)output->get_rows()[i];
			for(int j = output->get_w() - 1; j >= 0; j--)
			{
				uint16_t r = input_row[j * 4];
				uint16_t g = input_row[j * 4 + 1];
				uint16_t b = input_row[j * 4 + 2];
				output_row[j * 4] = (float)r / 65535;
				output_row[j * 4 + 1] = (float)g / 65535;
				output_row[j * 4 + 2] = (float)b / 65535;
			}
		}
	}

	TIFFClose(stream);
	delete unit;

	return 0;
}

int FileTIFF::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
//printf("FileTIFF::write_frame 1\n");
	FileTIFFUnit *tiff_unit = (FileTIFFUnit*)unit;
	int result = 0;
	TIFF *stream;
	tiff_unit->offset = 0;
	tiff_unit->data = data;
	tiff_unit->data->set_compressed_size(0);

	stream = TIFFClientOpen("FileTIFF", 
		"w",
	    (void*)tiff_unit,
	    tiff_read, 
		tiff_write,
	    tiff_seek, 
		tiff_close,
	    tiff_size,
	    tiff_mmap, 
		tiff_unmap);

	int components, color_model, bits, type, compression;
	int sampleformat = SAMPLEFORMAT_UINT;
	int bytesperrow;
	switch(asset->tiff_cmodel)
	{
		case FileTIFF::RGB_888: 
			components = 3;
			color_model = BC_RGB888;
			bits = 8;
			type = TIFF_BYTE;
			bytesperrow = 3 * asset->width;
			break;
		case FileTIFF::RGB_161616: 
			components = 3;
			color_model = BC_RGB_FLOAT;
			bits = 16;
			type = TIFF_SHORT;
			bytesperrow = 6 * asset->width;
			break;
		case FileTIFF::RGBA_8888: 
			components = 4;
			color_model = BC_RGBA8888;
			bits = 8;
			type = TIFF_BYTE;
			bytesperrow = 4 * asset->width;
			break;
		case FileTIFF::RGBA_16161616: 
			components = 4;
			color_model = BC_RGBA_FLOAT;
			bits = 16;
			type = TIFF_SHORT;
			bytesperrow = 8 * asset->width;
			break;
		case FileTIFF::RGB_FLOAT: 
			components = 3;
			color_model = BC_RGB_FLOAT;
			bits = 32;
			type = TIFF_FLOAT;
			sampleformat = SAMPLEFORMAT_IEEEFP;
			bytesperrow = 12 * asset->width;
			break;
		case FileTIFF::RGBA_FLOAT: 
			components = 4;
			color_model = BC_RGBA_FLOAT;
			bits = 32;
			type = TIFF_FLOAT;
			sampleformat = SAMPLEFORMAT_IEEEFP;
			bytesperrow = 16 * asset->width;
			break;
		default: 
			components = 3;
			color_model = BC_RGB888;
			bits = 8;
			type = TIFF_BYTE;
			bytesperrow = 3 * asset->width;
			break;
	}


	switch(asset->tiff_compression)
	{
		case FileTIFF::LZW:
			compression = COMPRESSION_LZW;
			break;
		case FileTIFF::PACK_BITS:
			compression = COMPRESSION_PACKBITS;
			break;
		case FileTIFF::DEFLATE:
			compression = COMPRESSION_DEFLATE;
			break;
		case FileTIFF::JPEG:
			compression = COMPRESSION_JPEG;
			break;
		default:
			compression = COMPRESSION_NONE;
			break;
	}

	TIFFSetField(stream, TIFFTAG_IMAGEWIDTH, asset->width);
	TIFFSetField(stream, TIFFTAG_IMAGELENGTH, asset->height);
	TIFFSetField(stream, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(stream, TIFFTAG_SAMPLESPERPIXEL, components);
	TIFFSetField(stream, TIFFTAG_BITSPERSAMPLE, bits);
    TIFFSetField(stream, TIFFTAG_SAMPLEFORMAT, sampleformat);
	TIFFSetField(stream, TIFFTAG_COMPRESSION, compression);
	TIFFSetField(stream, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
 	TIFFSetField(stream, TIFFTAG_ROWSPERSTRIP, 
 		TIFFDefaultStripSize(stream, (uint32_t)-1));
//  	TIFFSetField(stream, TIFFTAG_ROWSPERSTRIP, 
// 		(8 * 1024) / bytesperrow);
	TIFFSetField(stream, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	if(frame->get_color_model() == color_model)
	{
		for(int i = 0; i < asset->height; i++)
		{
			TIFFWriteScanline(stream, frame->get_rows()[i], i, 0);
		}
	}
	else
	{
		if(tiff_unit->temp &&
			tiff_unit->temp->get_color_model() != color_model)
		{
			delete tiff_unit->temp;
			tiff_unit->temp = 0;
		}
		if(!tiff_unit->temp)
		{
			tiff_unit->temp = new VFrame(0,
				asset->width,
				asset->height,
				color_model);
		}

		cmodel_transfer(tiff_unit->temp->get_rows(), 
			frame->get_rows(),
			tiff_unit->temp->get_y(),
			tiff_unit->temp->get_u(),
			tiff_unit->temp->get_v(),
			frame->get_y(),
			frame->get_u(),
			frame->get_v(),
			0, 
			0, 
			frame->get_w(), 
			frame->get_h(),
			0, 
			0, 
			frame->get_w(), 
			frame->get_h(),
			frame->get_color_model(), 
			color_model,
			0,
			frame->get_w(),
			frame->get_w());
		for(int i = 0; i < asset->height; i++)
		{
			TIFFWriteScanline(stream, tiff_unit->temp->get_rows()[i], i, 0);
		}
	}

	TIFFClose(stream);

//printf("FileTIFF::write_frame 10\n");
	return result;
}

FrameWriterUnit* FileTIFF::new_writer_unit(FrameWriter *writer)
{
	return new FileTIFFUnit(this, writer);
}








FileTIFFUnit::FileTIFFUnit(FileTIFF *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	this->file = file;
	temp = 0;
}

FileTIFFUnit::~FileTIFFUnit()
{
	if(temp) delete temp;
}












TIFFConfigVideo::TIFFConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	400,
	200)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

TIFFConfigVideo::~TIFFConfigVideo()
{
}

int TIFFConfigVideo::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, "Colorspace:"));
	TIFFColorspace *menu1;
	add_subwindow(menu1 = new TIFFColorspace(this, x + 150, y, 200));
	menu1->create_objects();
	y += 40;
	add_subwindow(new BC_Title(x, y, "Compression:"));
	TIFFCompression *menu2;
	add_subwindow(menu2 = new TIFFCompression(this, x + 150, y, 200));
	menu2->create_objects();

	add_subwindow(new BC_OKButton(this));
	return 0;
}

int TIFFConfigVideo::close_event()
{
	set_done(0);
	return 1;
}






TIFFColorspace::TIFFColorspace(TIFFConfigVideo *gui, int x, int y, int w)
 : BC_PopupMenu(x,
 	y,
	w,
	FileTIFF::cmodel_to_str(gui->asset->tiff_cmodel))
{
	this->gui = gui;
}
int TIFFColorspace::handle_event()
{
	return 1;
}
void TIFFColorspace::create_objects()
{
	add_item(new TIFFColorspaceItem(gui, FileTIFF::RGB_888));
//	add_item(new TIFFColorspaceItem(gui, FileTIFF::RGB_16161616));
	add_item(new TIFFColorspaceItem(gui, FileTIFF::RGBA_8888));
//	add_item(new TIFFColorspaceItem(gui, FileTIFF::RGBA_16161616));
	add_item(new TIFFColorspaceItem(gui, FileTIFF::RGB_FLOAT));
	add_item(new TIFFColorspaceItem(gui, FileTIFF::RGBA_FLOAT));
}


TIFFColorspaceItem::TIFFColorspaceItem(TIFFConfigVideo *gui, int value)
 : BC_MenuItem(FileTIFF::cmodel_to_str(value))
{
	this->gui = gui;
	this->value = value;
}
int TIFFColorspaceItem::handle_event()
{
	gui->asset->tiff_cmodel = value;
	return 0;
}







TIFFCompression::TIFFCompression(TIFFConfigVideo *gui, int x, int y, int w)
 : BC_PopupMenu(x, y, w, FileTIFF::compression_to_str(gui->asset->tiff_compression))
{
	this->gui = gui;
}
int TIFFCompression::handle_event()
{
	return 1;
}
void TIFFCompression::create_objects()
{
	add_item(new TIFFCompressionItem(gui, FileTIFF::NONE));
//	add_item(new TIFFCompressionItem(gui, FileTIFF::LZW));
	add_item(new TIFFCompressionItem(gui, FileTIFF::PACK_BITS));
//	add_item(new TIFFCompressionItem(gui, FileTIFF::DEFLATE));
//	add_item(new TIFFCompressionItem(gui, FileTIFF::JPEG));
}





TIFFCompressionItem::TIFFCompressionItem(TIFFConfigVideo *gui, int value)
 : BC_MenuItem(FileTIFF::compression_to_str(value))
{
	this->gui = gui;
	this->value = value;
}
int TIFFCompressionItem::handle_event()
{
	gui->asset->tiff_compression = value;
	return 0;
}



