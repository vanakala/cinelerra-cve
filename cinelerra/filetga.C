// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "edit.h"
#include "filetga.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "selection.h"
#include "paramlist.h"
#include "paramlistwindow.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>

// Known image types.
#define TGA_TYPE_MAPPED      1
#define TGA_TYPE_COLOR       2
#define TGA_TYPE_GRAY        3

// Only known compression is RLE
#define TGA_COMP_NONE        0 
#define TGA_COMP_RLE         1 

#define TGA_RGB_RLE "rle"
#define TGA_RGBA_RLE "rlea"
#define TGA_RGB "raw"
#define TGA_RGBA "rawa"

#define RGB_RLE  0
#define RGBA_RLE 1
#define RGB_NC   2
#define RGBA_NC  3

#define FILETGA_VCODEC_IX 0
#define TGA_ENC_CONFIG_NAME "tga:enc"

#define PARAM_COMPRESSION "compression"

const struct selection_int FileTGA::tga_compression[] =
{
	{ "RGB compressed", RGB_RLE },
	{ "RGBA compressed", RGBA_RLE },
	{ "RGB uncompressed", RGB_NC },
	{ "RGBA uncompressed", RGBA_NC },
	{ 0, 0 }
};

struct paramlist_defaults FileTGA::encoder_params[] =
{
	{ PARAM_COMPRESSION, N_("Compression"), PARAMTYPE_INT, RGB_RLE },
	{ 0, 0, 0, 0 }
};

FileTGA::FileTGA(Asset *asset, File *file)
 : FileList(asset, file, "TGALIST", ".tga", FILE_TGA, FILE_TGA_LIST)
{
	temp = 0;
}

FileTGA::~FileTGA()
{
	delete temp;
}

int FileTGA::check_sig(Asset *asset)
{
// Test file extension
	int result = 0;
	int l;
	char *ext = strrchr(asset->path, '.');

	if(ext)
	{
		if(!strncasecmp(ext, ".tga", 4)) result = 1;
	}

// Test for list
	if(!result)
	{
		FILE *stream;
		if(!(stream = fopen(asset->path, "rb")))
		{
// file not found
			result = 0;
		}
		else
		{
			char test[16];
			l = fread(test, 16, 1, stream);
			fclose(stream);
			if(l && test[0] == 'T' && test[1] == 'G' && test[2] == 'A' && 
				test[3] == 'L' && test[4] == 'I' && test[5] == 'S' && 
				test[6] == 'T')
			{
				result = 1;
			}
			
		}
	}

	return result;
}

void FileTGA::get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int options)
{
	Param *parm;

	if(options & SUPPORTS_VIDEO)
	{
		asset->encoder_parameters[FILETGA_VCODEC_IX] =
			Paramlist::construct("FileTGA",
				asset->encoder_parameters[FILETGA_VCODEC_IX],
			encoder_params);
		parm = asset->encoder_parameters[FILETGA_VCODEC_IX]->find(PARAM_COMPRESSION);
		parm->subparams = Paramlist::construct_from_selection(PARAM_COMPRESSION,
			parm->subparams, tga_compression);
		parm->subparams->set_selected(parm->intvalue);

		ParamlistThread thread(&asset->encoder_parameters[FILETGA_VCODEC_IX],
			_("TGA compression"), mwindow_global->get_window_icon(),
			&format_window);

		thread.run();

		if(!thread.win_result)
		{
			parm = asset->encoder_parameters[FILETGA_VCODEC_IX]->first;
			parm->intvalue = parm->subparams->selectedint;
		}
	}
}

int FileTGA::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileTGA::read_frame(VFrame *frame, VFrame *data)
{
	read_tga(asset, frame, data, temp);
	return 0;
}

int FileTGA::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
	TGAUnit *tga_unit = (TGAUnit*)unit;

	write_tga(asset, frame, data, tga_unit->temp);
	return 0;
}

FrameWriterUnit* FileTGA::new_writer_unit(FrameWriter *writer)
{
	return new TGAUnit(this, writer);
}

size_t FileTGA::get_memory_usage()
{
	size_t result = FileList::get_memory_usage();
	if(temp) result += temp->get_data_size();
	return result;
}

#define FOOTERSIZE 26
#define HEADERSIZE 18

int FileTGA::read_frame_header(const char *path)
{
	int result = 0;

	FILE *stream;

	if(!(stream = fopen(path, "rb")))
	{
		errormsg("Error while opening \"%s\" for reading. \n%m\n", asset->path);
		return 1;
	}

	unsigned char header[HEADERSIZE];
	if(fread(header, HEADERSIZE, 1, stream) < 1)
		result = 1;
	fclose(stream);

	if(result) return 1;

	asset->width = header[12] | (header[13] << 8);
	asset->height = header[14] | (header[15] << 8);
	int bpp = header[16];
	int rle = header[2] & 0x8;
	switch(bpp)
	{
	case 32:
		if(rle) 
			strcpy(asset->vcodec, TGA_RGBA_RLE);
		else
			strcpy(asset->vcodec, TGA_RGBA);
		break;
	case 24:
		if(rle) 
			strcpy(asset->vcodec, TGA_RGB_RLE);
		else
			strcpy(asset->vcodec, TGA_RGB);
		break;
	}

	return result;
}

void FileTGA::read_tga(Asset *asset, VFrame *frame, VFrame *data, VFrame* &temp)
{
// Read header
	unsigned char *footer, *header;
	int input_cmodel;
	int64_t file_offset = 0;

	footer = data->get_data() + 
		data->get_compressed_size() - 
		FOOTERSIZE;
	header = data->get_data();
	file_offset += HEADERSIZE;

	int image_type;
	int image_compression;
	switch(header[2])
	{
	case 1:
		image_type = TGA_TYPE_MAPPED;
		image_compression = TGA_COMP_NONE;
		break;
	case 2:
		image_type = TGA_TYPE_COLOR;
		image_compression = TGA_COMP_NONE;
		break;
	case 3:
		image_type = TGA_TYPE_GRAY;
		image_compression = TGA_COMP_NONE;
		break;
	case 9:
		image_type = TGA_TYPE_MAPPED;
		image_compression = TGA_COMP_RLE;
		break;
	case 10:
		image_type = TGA_TYPE_COLOR;
		image_compression = TGA_COMP_RLE;
		break;
	case 11:
		image_type = TGA_TYPE_GRAY;
		image_compression = TGA_COMP_RLE;
		break;
	default:
		image_type = 0;
	}
	int idlength = header[0];
	int colormaptype = header[1];
	int colormapindex = header[3] + header[4] * 256;
	int colormaplength = header[5] + header[6] * 256;
	int colormapsize = header[7];
	int xorigin = header[8] + header[9] * 256;
	int yorigin = header[10] + header[11] * 256;
	int width = header[12] + header[13] * 256;
	int height = header[14] + header[15] * 256;
	int bpp = header[16];
	int bytes = (bpp + 7) / 8;
	int alphabits = header[17] & 0x0f;
	int fliphoriz = (header[17] & 0x10) ? 1 : 0;
	int flipvert = (header[17] & 0x20) ? 0 : 1;
	int data_size = data->get_compressed_size();

	file_offset += idlength;

// Get colormap
	unsigned char *tga_cmap;
	unsigned char colormap[4 * 256];

	if(colormaptype == 1)
	{
		int cmap_bytes = (colormapsize + 7) / 8;
		tga_cmap = data->get_data() + file_offset;
		file_offset += colormaplength * cmap_bytes;

		switch(colormapsize)
		{
		case 32:
			bgr2rgb(colormap, tga_cmap, colormaplength, cmap_bytes, 1);
			break;
		case 24:
			bgr2rgb(colormap, tga_cmap, colormaplength, cmap_bytes, 0);
			break;
		case 16:
			upsample(colormap, tga_cmap, colormaplength, cmap_bytes);
			break;
		}
	}

	int source_cmodel = BC_RGB888;
	switch(bpp)
	{
	case 32:
		source_cmodel = BC_RGBA8888;
		break;
	case 24:
		source_cmodel = BC_RGB888;
		break;
	}

// Read image
	VFrame *output_frame;
	if(frame->get_color_model() == source_cmodel)
	{
		output_frame = frame;
	}
	else
	{
		if(temp && temp->get_color_model() != source_cmodel)
		{
			delete temp;
			temp = 0;
		}

		if(!temp)
		{
			temp = new VFrame(0, width, height, source_cmodel);
		}
		output_frame = temp;
	}

	if(flipvert)
	{
		for(int i = height - 1; i >= 0; i--)
		{
			read_line(output_frame->get_row_ptr(i),
				data->get_data(), 
				file_offset,
				image_type,
				bpp,
				image_compression,
				bytes,
				width,
				fliphoriz,
				alphabits,
				data_size);
		}
	}
	else
	{
		for(int i = 0; i < height; i++)
		{
			read_line(output_frame->get_row_ptr(i),
				data->get_data(), 
				file_offset,
				image_type,
				bpp,
				image_compression,
				bytes,
				width,
				fliphoriz,
				alphabits,
				data_size);
		}
	}

	if(output_frame != frame)
		frame->transfer_from(output_frame);
}

void FileTGA::write_tga(Asset *asset, VFrame *frame, VFrame *data, VFrame* &temp)
{
	unsigned char header[18];
	unsigned char footer[26];
	int64_t file_offset = 0;
	int out_bpp = 0;
	int rle = 0;
	int dest_cmodel = BC_RGB888;
	int tga_mode = RGB_RLE;

	if(asset->encoder_parameters[FILETGA_VCODEC_IX])
		tga_mode = asset->encoder_parameters[FILETGA_VCODEC_IX]->get(PARAM_COMPRESSION, tga_mode);

	header[0] = 0;
	header[1] = 0;
	switch(tga_mode)
	{
	case RGBA_RLE:
		header[2] = 10;
		out_bpp = 4;
		rle = 1;
		header[16] = 32; /* bpp */
		header[17] = 0x28; /* alpha + orientation */
		dest_cmodel = BC_RGBA8888;
		break;

	case RGBA_NC:
		header[2] = 2;
		out_bpp = 4;
		rle = 0;
		header[16] = 32; /* bpp */
		header[17] = 0x28; /* alpha + orientation */
		dest_cmodel = BC_RGBA8888;
		break;

	case RGB_RLE:
		header[2] = 10;
		out_bpp = 3;
		rle = 1;
		header[16] = 24; /* bpp */
		header[17] = 0x20; /* alpha + orientation */
		dest_cmodel = BC_RGB888;
		break;

	default:
		header[2] = 2;
		out_bpp = 3;
		rle = 0;
		header[16] = 24; /* bpp */
		header[17] = 0x20; /* alpha + orientation */
		dest_cmodel = BC_RGB888;
		break;
	}

	header[3] = header[4] = header[5] = header[6] = header[7] = 0;

	VFrame *input_frame;
	if(frame->get_color_model() == dest_cmodel)
	{
		input_frame = frame;
	}
	else
	{
		if(temp && temp->get_color_model() != dest_cmodel)
		{
			delete temp;
			temp = 0;
		}

		if(!temp)
		{
			temp = new VFrame(0, frame->get_w(), frame->get_h(), dest_cmodel);
		}
		input_frame = temp;
		input_frame->transfer_from(frame);
	}

// xorigin
// yorigin
	header[8]  = header[9]  = 0;
	header[10] = header[11] = 0;

	header[12] = input_frame->get_w() % 256;
	header[13] = input_frame->get_w() / 256;

	header[14] = input_frame->get_h() % 256;
	header[15] = input_frame->get_h() / 256;

	write_data(header, data, file_offset, sizeof(header));

	unsigned char *output = new unsigned char[out_bpp * input_frame->get_w()];

	for(int i = 0; i < input_frame->get_h(); i++)
	{
		bgr2rgb(output, input_frame->get_row_ptr(i), input_frame->get_w(), out_bpp, (out_bpp == 4));

		if(rle)
		{
			rle_write(output, 
				input_frame->get_w(), 
				out_bpp,
				data,
				file_offset);
		}
		else
		{
			write_data(output, 
				data, 
				file_offset, 
				input_frame->get_w() * out_bpp);
		}
	}
	delete [] output;
}

void FileTGA::write_data(unsigned char *buffer, 
	VFrame *data, 
	int64_t &file_offset,
	int len)
{
	if(data->get_compressed_allocated() <= data->get_compressed_size() + len)
	{
		data->allocate_compressed_data((data->get_compressed_size() + len) * 2);
	}

	memcpy(data->get_data() + file_offset, buffer, len);
	file_offset += len;
	data->set_compressed_size(file_offset);
}

void FileTGA::read_line(unsigned char *row,
	unsigned char *data,
	int64_t &file_offset,
	int image_type,
	int bpp,
	int image_compression,
	int bytes,
	int width,
	int fliphoriz,
	int alphabits,
	int data_size)
{
	if(file_offset >= data_size)
		return;

	if(image_compression == TGA_COMP_RLE)
	{
		rle_read(row, data, file_offset,
			bytes, width);
	}
	else
	{
		if(file_offset + bytes * width <= data_size)
			bcopy(data + file_offset, row, bytes * width);
		file_offset += bytes * width;
	}

	if(fliphoriz)
	{
		flip_line(row, bytes, width);
	}

	if(image_type == TGA_TYPE_COLOR)
	{
		if(bpp == 16)
		{
			upsample(row, row, width, bytes);
		}
		else
		{
			bgr2rgb(row, row, width, bytes, bpp == 32);
		}
	}
}

void FileTGA::flip_line(unsigned char *row, int bytes, int width)
{
	unsigned char temp;
	unsigned char *alt;
	int x, s;

	alt = row + (bytes * (width - 1));

	for (x = 0; x * 2 <= width; x++)
	{
		for(s = 0; s < bytes; ++s)
		{
			temp = row[s];
			row[s] = alt[s];
			alt[s] = temp;
		}

		row += bytes;
		alt -= bytes;
	}
}

void FileTGA::rle_read(unsigned char *row,
	unsigned char *data,
	int64_t &file_offset,
	int bytes,
	int width)
{
	int repeat = 0;
	int direct = 0;
	unsigned char sample[4];
	int head;

	for(int x = 0; x < width; x++)
	{
		if(repeat == 0 && direct == 0)
		{
			head = data[file_offset++];
			if(head == EOF)
			{
				return;
			}
			else
			if(head >= 128)
			{
				repeat = head - 127;
				bcopy(data + file_offset, sample, bytes);
				file_offset += bytes;
			}
			else
			{
				direct = head + 1;
			}
		}

		if(repeat > 0)
		{
			for(int k = 0; k < bytes; k++)
			{
				row[k] = sample[k];
			}

			repeat--;
		}
		else
		{
			bcopy(data + file_offset, row, bytes);
			file_offset += bytes;
			direct--;
		}

		row += bytes;
	}
}

void FileTGA::rle_write(unsigned char *buffer, 
	int width, 
	int bytes, 
	VFrame *frame, 
	int64_t &file_offset)
{
	int repeat = 0;
	int direct = 0;
	unsigned char *from = buffer;
	unsigned char output;
	int x;

	for(x = 1; x < width; ++x)
	{
/* next pixel is different */
		if(memcmp(buffer, buffer + bytes, bytes))
		{
			if(repeat)
			{
				output = 128 + repeat;
				write_data(&output, frame, file_offset, 1);
				write_data(from, frame, file_offset, bytes);
				from = buffer + bytes;
				repeat = 0;
				direct = 0;
			}
			else
			{
				direct++;
			}
		}
		else
/* next pixel is the same */
		{
			if(direct)
			{
				output = direct - 1;
				write_data(&output, frame, file_offset, 1);
				write_data(from, frame, file_offset, bytes * direct);
				from = buffer;
				direct = 0;
				repeat = 1;
			}
			else
			{
				repeat++;
			}
		}

		if(repeat == 128)
		{
			output = 255;
			write_data(&output, frame, file_offset, 1);
			write_data(from, frame, file_offset, bytes);
			from = buffer + bytes;
			direct = 0;
			repeat = 0;
		}
		else
		if(direct == 128)
		{
			output = 127;
			write_data(&output, frame, file_offset, 1);
			write_data(from, frame, file_offset, direct * bytes);
			from = buffer + bytes;
			direct = 0;
			repeat = 0;
		}
		buffer += bytes;
	}

	if(repeat > 0)
	{
		output = 128 + repeat;
		write_data(&output, frame, file_offset, 1);
		write_data(from, frame, file_offset, bytes);
	}
	else
	{
		output = direct;
		write_data(&output, frame, file_offset, 1);
		write_data(from, frame, file_offset, bytes * (direct + 1));
	}
}

void FileTGA::bgr2rgb(unsigned char *dest,
	unsigned char *src,
	int width,
	int bytes,
	int alpha)
{
	int x;
	unsigned char r, g, b;

	if(alpha)
	{
		for(x = 0; x < width; x++)
		{
			r = src[2];
			g = src[1];
			b = src[0];
			*(dest++) = r;
			*(dest++) = g;
			*(dest++) = b;
			*(dest++) = src[3];

			src += bytes;
		}
	}
	else
	{
		for(x = 0; x < width; x++)
		{
			r = src[2];
			g = src[1];
			b = src[0];
			*(dest++) = r;
			*(dest++) = g;
			*(dest++) = b;

			src += bytes;
		}
	}
}

void FileTGA::upsample(unsigned char *dest,
		unsigned char *src,
		int width,
		int bytes)
{
	int x;

	dest += (width - 1) * 3;
	src += (width - 1) * bytes;
	for(x = width - 1; x >= 0; x--)
	{
		dest[0] =  ((src[1] << 1) & 0xf8);
		dest[0] += (dest[0] >> 5);

		dest[1] =  ((src[0] & 0xe0) >> 2) + ((src[1] & 0x03) << 6);
		dest[1] += (dest[1] >> 5);

		dest[2] =  ((src[0] << 3) & 0xf8);
		dest[2] += (dest[2] >> 5);

		dest -= 3;
		src -= bytes;
	}
}

void FileTGA::save_render_optios(Asset *asset)
{
	char pathbuf[BCTEXTLEN];

	asset->profile_config_path(TGA_ENC_CONFIG_NAME, pathbuf);
	strcat(pathbuf, XML_CONFIG_EXT);

	Paramlist::save_paramlist(asset->encoder_parameters[FILETGA_VCODEC_IX],
		pathbuf, encoder_params);
}

void FileTGA::get_render_defaults(Asset *asset)
{
	char pathbuf[BCTEXTLEN];

	asset->profile_config_path(TGA_ENC_CONFIG_NAME, pathbuf);
	strcat(pathbuf, XML_CONFIG_EXT);

	delete asset->encoder_parameters[FILETGA_VCODEC_IX];
	asset->encoder_parameters[FILETGA_VCODEC_IX] =
		Paramlist::load_paramlist(pathbuf);
}

TGAUnit::TGAUnit(FileTGA *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	temp = 0;
	this->file = file;
}

TGAUnit::~TGAUnit()
{
	delete temp;
}
