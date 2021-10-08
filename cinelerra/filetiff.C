// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "filetiff.h"
#include "interlacemodes.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "paramlist.h"
#include "paramlistwindow.h"
#include "selection.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define FILETIFF_VCODEC_IX 0
#define TIFF_ENC_CONFIG_NAME "tiff:enc"

#define PARAM_COMPRESSION "compression"
#define PARAM_CMODEL "cmodel"

const struct selection_int FileTIFF::tiff_compression[] =
{
	{ "None", FileTIFF::NONE },
	{ "LZW", FileTIFF::LZW },
	{ "Pack Bits", FileTIFF::PACK_BITS },
	{ "Deflate", FileTIFF::DEFLATE },
	{ "JPEG", FileTIFF::JPEG },
	{ 0, 0 }
};

const struct selection_int FileTIFF::tiff_cmodels[] =
{
	{ "RGB-8 Bit", FileTIFF::RGB_888 },
	{ "RGB-16 Bit", FileTIFF::RGB_161616 },
	{ "RGBA-8 Bit", FileTIFF::RGBA_8888 },
	{ "RGBA-16 Bit", FileTIFF::RGBA_16161616 },
	{ 0, 0 }
};

struct paramlist_defaults FileTIFF::encoder_params[] =
{
	{ PARAM_CMODEL, N_("Colormodel"), PARAMTYPE_INT, FileTIFF::RGB_888 },
	{ PARAM_COMPRESSION, N_("Compression"), PARAMTYPE_INT, FileTIFF::NONE },
	{ 0, 0, 0, 0 }
};


FileTIFF::FileTIFF(Asset *asset, File *file)
 : FileList(asset, file, "TIFFLIST", ".tif", FILE_TIFF, FILE_TIFF_LIST)
{
	asset->video_data = 1;
}

void FileTIFF::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int options)
{
	Param *parm;

	if(options & SUPPORTS_VIDEO)
	{
		asset->encoder_parameters[FILETIFF_VCODEC_IX] =
			Paramlist::construct("FileTIFF",
				asset->encoder_parameters[FILETIFF_VCODEC_IX],
			encoder_params);

		parm = asset->encoder_parameters[FILETIFF_VCODEC_IX]->find(PARAM_CMODEL);
		parm->subparams = Paramlist::construct_from_selection(PARAM_CMODEL,
			parm->subparams, tiff_cmodels);
		parm->subparams->set_selected(parm->intvalue);
		parm->subparams->parent = parm;
		parm = asset->encoder_parameters[FILETIFF_VCODEC_IX]->find(PARAM_COMPRESSION);
		parm->subparams = Paramlist::construct_from_selection(PARAM_COMPRESSION,
			parm->subparams, tiff_compression);
		parm->subparams->set_selected(parm->intvalue);
		parm->subparams->parent = parm;

		ParamlistThread thread(&asset->encoder_parameters[FILETIFF_VCODEC_IX],
			_("TIFF compression"), mwindow_global->get_window_icon(),
			&format_window);

		thread.run();

		if(!thread.win_result)
		{
			parm = asset->encoder_parameters[FILETIFF_VCODEC_IX]->first;
			parm->intvalue = parm->subparams->selectedint;
			parm = parm->next;
			parm->intvalue = parm->subparams->selectedint;
		}
	}
}

int FileTIFF::check_sig(Asset *asset)
{
	int l;
	FILE *stream = fopen(asset->path, "rb");

	if(stream)
	{
		char test[16];
		l = fread(test, 16, 1, stream);
		fclose(stream);
		if(l < 1) return 0;
		if(test[0] == 'I' && test[1] == 'I' && test[2] == 0x2a)
		{
			// Reject cr2, libtif fails with it
			if(test[4] == 0x10 && !test[5] && !test[6] && !test[7] &&
					test[8] == 'C' && test[9] == 'R')
				return 0;
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

int FileTIFF::read_frame_header(const char *path)
{
	TIFF *stream;
	int result = 0;

	if(!(stream = TIFFOpen(path, "rb")))
	{
		errormsg("Error while opening \"%s\" for reading. \n%m\n", asset->path);
		return 1;
	}

// The raw format for certain cameras deviates from TIFF here.

	int components = 0;
	TIFFGetField(stream, TIFFTAG_SAMPLESPERPIXEL, &components);
	int bitspersample = 0;
	TIFFGetField(stream, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	int sampleformat = 0;
	TIFFGetField(stream, TIFFTAG_SAMPLEFORMAT, &sampleformat);

	if(bitspersample == 8 && components == 3)
		tiff_cmodel = FileTIFF::RGB_888;
	else
	if(bitspersample == 16 && components == 3)
		tiff_cmodel = FileTIFF::RGB_161616;
	else
	if(bitspersample == 8 && components == 4)
		tiff_cmodel = FileTIFF::RGBA_8888;
	else
	if(bitspersample == 16 && components == 4)
		tiff_cmodel = FileTIFF::RGBA_16161616;
	else
	if(bitspersample == 32 && components == 3)
		tiff_cmodel = FileTIFF::RGB_FLOAT;
	else
	if(bitspersample == 32 && components == 4)
		tiff_cmodel = FileTIFF::RGBA_FLOAT;
	else
	if(bitspersample == 8 && (components == 1 || components == 0))
		tiff_cmodel = FileTIFF::GREYSCALE;
	else
	if(bitspersample == 1)
		tiff_cmodel = FileTIFF::BLACKWHITE;

	TIFFClose(stream);

	asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;

	return result;
}

int FileTIFF::colormodel_supported(int colormodel)
{
	switch(tiff_cmodel)
	{
	case FileTIFF::BLACKWHITE:
	case FileTIFF::RGB_888:
		return BC_RGB888;
	case FileTIFF::RGB_161616:
		return BC_RGB161616;
	case FileTIFF::GREYSCALE:
		return BC_RGB888;
	case FileTIFF::RGBA_8888:
		return BC_RGBA8888;
	case FileTIFF::RGBA_16161616:
		return BC_RGBA16161616;
	case FileTIFF::RGB_FLOAT:
		return BC_RGB_FLOAT;
	case FileTIFF::RGBA_FLOAT:
		return BC_RGBA_FLOAT;
	default:
		return BC_RGB888;
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
	for(int i = 0; i < asset->streams[0].height; i++)
	{
		TIFFReadScanline(stream, output->get_row_ptr(i), i, 0);

// For the greyscale model, the output is RGB888 but the input must be expanded
		if(tiff_cmodel == FileTIFF::GREYSCALE)
		{
			unsigned char *row = output->get_row_ptr(i);
			for(int j = output->get_w() - 1; j >= 0; j--)
			{
				unsigned char value = row[j];
				row[j * 3] = value;
				row[j * 3 + 1] = value;
				row[j * 3 + 2] = value;
			}
		}
		else
		if(tiff_cmodel == FileTIFF::BLACKWHITE)
		{
			unsigned char *row = output->get_row_ptr(i);
			for(int j = output->get_w() - 1; j >= 0;)
			{
				unsigned char value = row[j / 8];
				for(int m = 1; m < 0x100; m <<= 1, j--)
				{
					unsigned char v = value & m ? 0 : 255;
					row[j * 3] = v;
					row[j * 3 + 1] = v;
					row[j * 3 + 2] = v;
				}
			}
		}
	}

	TIFFClose(stream);
	delete unit;

	return 0;
}

int FileTIFF::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
	FileTIFFUnit *tiff_unit = (FileTIFFUnit*)unit;
	int result = 0;
	TIFF *stream;
	tiff_unit->offset = 0;
	tiff_unit->data = data;
	tiff_unit->data->set_compressed_size(0);
	int streamix = asset->get_stream_ix(STRDSC_VIDEO);
	int height = asset->streams[streamix].height;
	int width = asset->streams[streamix].width;

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
	int compression_param = FileTIFF::NONE;
	int cmodel_param = FileTIFF::RGB_888;

	if(asset->encoder_parameters[FILETIFF_VCODEC_IX])
	{
		cmodel_param =
			asset->encoder_parameters[FILETIFF_VCODEC_IX]->get(PARAM_CMODEL,
				cmodel_param);
		compression_param =
			asset->encoder_parameters[FILETIFF_VCODEC_IX]->get(PARAM_COMPRESSION,
				compression_param);
	}

	switch(cmodel_param)
	{
	case FileTIFF::RGB_888:
		components = 3;
		color_model = BC_RGB888;
		bits = 8;
		type = TIFF_BYTE;
		bytesperrow = 3 * width;
		break;
	case FileTIFF::RGB_161616:
		components = 3;
		color_model = BC_RGB161616;
		bits = 16;
		type = TIFF_SHORT;
		bytesperrow = 6 * width;
		break;
	case FileTIFF::RGBA_8888: 
		components = 4;
		color_model = BC_RGBA8888;
		bits = 8;
		type = TIFF_BYTE;
		bytesperrow = 4 * width;
		break;
	case FileTIFF::RGBA_16161616: 
		components = 4;
		color_model = BC_RGBA16161616;
		bits = 16;
		type = TIFF_SHORT;
		bytesperrow = 8 * width;
		break;
	default: 
		components = 3;
		color_model = BC_RGB888;
		bits = 8;
		type = TIFF_BYTE;
		bytesperrow = 3 * width;
		break;
	}

	switch(compression_param)
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
		bits = 8;
		type = TIFF_BYTE;
		break;
	default:
		compression = COMPRESSION_NONE;
		break;
	}

	TIFFSetField(stream, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(stream, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(stream, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(stream, TIFFTAG_SAMPLESPERPIXEL, components);
	TIFFSetField(stream, TIFFTAG_BITSPERSAMPLE, bits);
	TIFFSetField(stream, TIFFTAG_SAMPLEFORMAT, sampleformat);
	TIFFSetField(stream, TIFFTAG_COMPRESSION, compression);
	TIFFSetField(stream, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(stream, TIFFTAG_ROWSPERSTRIP, 
		TIFFDefaultStripSize(stream, (uint32_t)-1));
	TIFFSetField(stream, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	if(frame->get_color_model() == color_model)
	{
		for(int i = 0; i < height; i++)
		{
			TIFFWriteScanline(stream, frame->get_row_ptr(i), i, 0);
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
				width,
				height,
				color_model);
		}

		tiff_unit->temp->transfer_from(frame);

		for(int i = 0; i < height; i++)
		{
			TIFFWriteScanline(stream, tiff_unit->temp->get_row_ptr(i), i, 0);
		}
	}

	TIFFClose(stream);

	return result;
}

void FileTIFF::save_render_optios(Asset *asset)
{
	char pathbuf[BCTEXTLEN];

	asset->profile_config_path(TIFF_ENC_CONFIG_NAME, pathbuf);
	strcat(pathbuf, XML_CONFIG_EXT);

	Paramlist::save_paramlist(asset->encoder_parameters[FILETIFF_VCODEC_IX],
		pathbuf, encoder_params);
}

void FileTIFF::get_render_defaults(Asset *asset)
{
	char pathbuf[BCTEXTLEN];

	asset->profile_config_path(TIFF_ENC_CONFIG_NAME, pathbuf);
	strcat(pathbuf, XML_CONFIG_EXT);

	delete asset->encoder_parameters[FILETIFF_VCODEC_IX];
	asset->encoder_parameters[FILETIFF_VCODEC_IX] =
		Paramlist::load_paramlist(pathbuf);
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

