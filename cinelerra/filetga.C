
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
#include "bcsignals.h"
#include "edit.h"
#include "filetga.h"
#include "language.h"
#include "mwindow.inc"
#include "vframe.h"
#include "mainerror.h"

#include <string.h>
#include <unistd.h>

/* Known image types. */
#define TGA_TYPE_MAPPED      1
#define TGA_TYPE_COLOR       2
#define TGA_TYPE_GRAY        3

/* Only known compression is RLE */
#define TGA_COMP_NONE        0 
#define TGA_COMP_RLE         1 


FileTGA::FileTGA(Asset *asset, File *file)
 : FileList(asset, file, "TGALIST", ".tga", FILE_TGA, FILE_TGA_LIST)
{
	temp = 0;
}

FileTGA::~FileTGA()
{
	if(temp) delete temp;
}

int FileTGA::check_sig(Asset *asset)
{


// Test file extension
	int result = 0;
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
			fread(test, 16, 1, stream);
			fclose(stream);
			if(test[0] == 'T' && test[1] == 'G' && test[2] == 'A' && 
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
		int audio_options,
		int video_options)
{
	if(video_options)
	{
		TGAConfigVideo *window = new TGAConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

#if 0
N_("RGB compressed")
N_("RGBA compressed")
N_("RGB uncompressed")
N_("RGBA uncompressed")
#endif

#define TGA_RGB_RLE "rle "
#define TGA_RGBA_RLE "rlea"
#define TGA_RGB "raw "
#define TGA_RGBA "rawa"

#define TGA_RGB_RLE_NAME "RGB compressed"
#define TGA_RGBA_RLE_NAME "RGBA compressed"
#define TGA_RGB_NAME "RGB uncompressed"
#define TGA_RGBA_NAME "RGBA uncompressed"

char* FileTGA::compression_to_str(char *compression)
{
	if(!strcasecmp(compression, TGA_RGB_RLE)) return _(TGA_RGB_RLE_NAME);
	if(!strcasecmp(compression, TGA_RGBA_RLE)) return _(TGA_RGBA_RLE_NAME);
	if(!strcasecmp(compression, TGA_RGB)) return _(TGA_RGB_NAME);
	if(!strcasecmp(compression, TGA_RGBA)) return _(TGA_RGBA_NAME);
	return TGA_RGB_NAME;
}

char* FileTGA::str_to_compression(char *string)
{
	if(!strcasecmp(compression_to_str(TGA_RGB_RLE), string)) return TGA_RGB_RLE;
	if(!strcasecmp(compression_to_str(TGA_RGBA_RLE), string)) return TGA_RGBA_RLE;
	if(!strcasecmp(compression_to_str(TGA_RGB), string)) return TGA_RGB;
	if(!strcasecmp(compression_to_str(TGA_RGBA), string)) return TGA_RGBA;
	return TGA_RGB;
}

int FileTGA::can_copy_from(Edit *edit, int64_t position)
{
	if(edit->asset->format == FILE_TGA_LIST ||
		edit->asset->format == FILE_TGA)
		return 1;
	
	return 0;
}


int  FileTGA::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileTGA::get_best_colormodel(Asset *asset, int driver)
{
	if(!strcasecmp(asset->vcodec, TGA_RGB_RLE) || 
		!strcasecmp(asset->vcodec, TGA_RGB)) return BC_RGB888;
	if(!strcasecmp(asset->vcodec, TGA_RGBA_RLE) ||
		!strcasecmp(asset->vcodec, TGA_RGBA)) return BC_RGBA8888;
	return BC_RGB888;
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

int64_t FileTGA::get_memory_usage()
{
	int64_t result = FileList::get_memory_usage();
	if(temp) result += temp->get_data_size();
	return result;
}








#define FOOTERSIZE 26
#define HEADERSIZE 18
int FileTGA::read_frame_header(char *path)
{
	int result = 0;

//printf("FileTGA::read_frame_header 1\n");
	FILE *stream;

	if(!(stream = fopen(path, "rb")))
	{
		eprintf("Error while opening \"%s\" for reading. \n%m\n", asset->path);
		return 1;
	}

	unsigned char header[HEADERSIZE];
	fread(header, HEADERSIZE, 1, stream);
	fclose(stream);

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
//printf("FileTGA::read_frame_header 2 %d %d\n", asset->width, asset->height);

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

	if(idlength) file_offset += idlength;

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
			read_line(output_frame->get_rows()[i], 
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
			read_line(output_frame->get_rows()[i], 
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
	{
		cmodel_transfer(frame->get_rows(), 
			output_frame->get_rows(),
			frame->get_y(),
			frame->get_u(),
			frame->get_v(),
			output_frame->get_y(),
			output_frame->get_u(),
			output_frame->get_v(),
			0, 
			0, 
			width, 
			height,
			0, 
			0, 
			frame->get_w(), 
			frame->get_h(),
			output_frame->get_color_model(), 
			frame->get_color_model(),
			0,
			width,
			frame->get_w());
	}
}

void FileTGA::write_tga(Asset *asset, VFrame *frame, VFrame *data, VFrame* &temp)
{
	unsigned char header[18];
	unsigned char footer[26];
	int64_t file_offset = 0;
	int out_bpp = 0;
	int rle = 0;
	int dest_cmodel = BC_RGB888;

//printf("FileTGA::write_tga 1\n");

	header[0] = 0;
	header[1] = 0;
	if(!strcasecmp(asset->vcodec, TGA_RGBA_RLE))
	{
		header[2] = 10;
    	out_bpp = 4;
		rle = 1;
    	header[16] = 32; /* bpp */
    	header[17] = 0x28; /* alpha + orientation */
		dest_cmodel = BC_RGBA8888;
	}
	else
	if(!strcasecmp(asset->vcodec, TGA_RGBA))
	{
		header[2] = 2;
    	out_bpp = 4;
		rle = 0;
    	header[16] = 32; /* bpp */
    	header[17] = 0x28; /* alpha + orientation */
		dest_cmodel = BC_RGBA8888;
	}
	else
	if(!strcasecmp(asset->vcodec, TGA_RGB_RLE))
	{
		header[2] = 10;
    	out_bpp = 3;
		rle = 1;
    	header[16] = 24; /* bpp */
    	header[17] = 0x20; /* alpha + orientation */
		dest_cmodel = BC_RGB888;
	}
	else
	{
		header[2] = 2;
    	out_bpp = 3;
		rle = 0;
    	header[16] = 24; /* bpp */
    	header[17] = 0x20; /* alpha + orientation */
		dest_cmodel = BC_RGB888;
	}
    header[3] = header[4] = header[5] = header[6] = header[7] = 0;
//printf("FileTGA::write_tga 1\n");

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

		cmodel_transfer(input_frame->get_rows(), 
			frame->get_rows(),
			input_frame->get_y(),
			input_frame->get_u(),
			input_frame->get_v(),
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
			input_frame->get_color_model(),
			0,
			frame->get_w(),
			frame->get_w());
	}
//printf("FileTGA::write_tga 1\n");

// xorigin
// yorigin
	header[8]  = header[9]  = 0;
	header[10] = header[11] = 0;

	header[12] = input_frame->get_w() % 256;
	header[13] = input_frame->get_w() / 256;

	header[14] = input_frame->get_h() % 256;
	header[15] = input_frame->get_h() / 256;
//printf("FileTGA::write_tga 1\n");
	
	write_data(header, data, file_offset, sizeof(header));
//printf("FileTGA::write_tga 1\n");

	unsigned char *output = new unsigned char[out_bpp * input_frame->get_w()];
//printf("FileTGA::write_tga 1\n");
	for(int i = 0; i < input_frame->get_h(); i++)
	{
//printf("FileTGA::write_tga 2\n");
		bgr2rgb(output, input_frame->get_rows()[i], input_frame->get_w(), out_bpp, (out_bpp == 4));
//printf("FileTGA::write_tga 3\n");
		
		if(rle)
		{
//printf("FileTGA::write_tga 4\n");
			rle_write(output, 
				input_frame->get_w(), 
				out_bpp,
				data,
				file_offset);
//printf("FileTGA::write_tga 5\n");
		}
		else
		{
//printf("FileTGA::write_tga 6\n");
			write_data(output, 
				data, 
				file_offset, 
				input_frame->get_w() * out_bpp);
//printf("FileTGA::write_tga 7\n");
		}
	}
//printf("FileTGA::write_tga 8\n");
	delete [] output;
//printf("FileTGA::write_tga 9\n");
}

void FileTGA::write_data(unsigned char *buffer, 
	VFrame *data, 
	int64_t &file_offset,
	int64_t len)
{
//printf("FileTGA::write_data 1 %d\n", len);
	if(data->get_compressed_allocated() <= data->get_compressed_size() + len)
	{
		data->allocate_compressed_data((data->get_compressed_size() + len) * 2);
	}
//printf("FileTGA::write_data 1 %d\n", len);

	bcopy(buffer, data->get_data() + file_offset, len);
//printf("FileTGA::write_data 1 %d\n", len);
	file_offset += len;
//printf("FileTGA::write_data 1 %d\n", len);
	data->set_compressed_size(file_offset);
//printf("FileTGA::write_data 2 %d\n", len);
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
	if(file_offset >= data_size) return;
	if(image_compression == TGA_COMP_RLE)
	{
		rle_read(row,
			data,
			file_offset,
			bytes,
			width);
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
			bgr2rgb(row, row, width, bytes, alphabits);
		}
	}
	else
	{
		;
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









TGAUnit::TGAUnit(FileTGA *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	temp = 0;
	this->file = file;
}

TGAUnit::~TGAUnit()
{
	if(temp) delete temp;
}













TGAConfigVideo::TGAConfigVideo(BC_WindowBase *gui, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	gui->get_abs_cursor_x(1),
 	gui->get_abs_cursor_y(1),
	400,
	100)
{
	this->gui = gui;
	this->asset = asset;

	compression_items.append(new BC_ListBoxItem(FileTGA::compression_to_str(TGA_RGB_RLE)));
	compression_items.append(new BC_ListBoxItem(FileTGA::compression_to_str(TGA_RGBA_RLE)));
	compression_items.append(new BC_ListBoxItem(FileTGA::compression_to_str(TGA_RGB)));
	compression_items.append(new BC_ListBoxItem(FileTGA::compression_to_str(TGA_RGBA)));
}

TGAConfigVideo::~TGAConfigVideo()
{
	compression_items.remove_all_objects();
}

int TGAConfigVideo::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Compression:")));
	TGACompression *textbox = new TGACompression(this, 
		x + 110, 
		y, 
		asset, 
		&compression_items);
	textbox->create_objects();
	add_subwindow(new BC_OKButton(this));
	return 0;
}

int TGAConfigVideo::close_event()
{
	set_done(0);
	return 1;
}


TGACompression::TGACompression(TGAConfigVideo *gui,
	int x, 
	int y, 
	Asset *asset, 
	ArrayList<BC_ListBoxItem*> *compression_items)
 : BC_PopupTextBox(gui,
	compression_items,
 	FileTGA::compression_to_str(gui->asset->vcodec),
	x, 
 	y, 
	200,
	200)
{
	this->asset = asset;
}
int TGACompression::handle_event()
{
	strcpy(asset->vcodec, FileTGA::str_to_compression(get_text()));
	return 1;
}
