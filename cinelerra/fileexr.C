
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
#include "clip.h"
#include "fileexr.h"
#include "filesystem.h"
#include "interlacemodes.h"

#include <ImathBox.h>
#include <ImfChannelList.h>
#include <ImfChromaticities.h>
#include <ImfCompression.h>
#include <ImfIO.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfPixelType.h>
#include <ImfRgbaFile.h>
#include <ImfRgbaYca.h>
#include <ImfVersion.h>
#include "mwindow.inc"
#include "vframe.h"


class EXRIStream : public Imf::IStream
{
public:
	EXRIStream(char *data, int size);
	~EXRIStream();

	bool read (char c[], int n);
	Imf::Int64 tellg ();
	void seekg (Imf::Int64 pos);
	void clear ();

private:
	char *data;
	int size;
	int position;
};

class EXROStream : public Imf::OStream
{
public:
	EXROStream(VFrame *data);
	~EXROStream();

    virtual void write(const char c[], int n);
    virtual Imf::Int64 tellp();
    virtual void seekp(Imf::Int64 pos);

private:
	VFrame *data;
	int position;
};



EXRIStream::EXRIStream(char *data, int size)
 : Imf::IStream("mypath")
{
	this->data = data;
	this->size = size;
	position = 0;
}

EXRIStream::~EXRIStream()
{
}

bool EXRIStream::read(char c[], int n)
{
	int fragment = n;
	if(position + fragment > size)
	{
		fragment = size - position;
	}
	memcpy(c, data + position, fragment);
	position += fragment;

	if(n != fragment)
	{
		throw Iex::InputExc ("EXRIStream::read: Unexpected end of file.");
	}
	return position >= size;
}

Imf::Int64 EXRIStream::tellg ()
{
	return position;
}

void EXRIStream::seekg(Imf::Int64 pos)
{
	position = pos;
}

void EXRIStream::clear()
{
}











EXROStream::EXROStream(VFrame *data)
 : Imf::OStream("mypath")
{
	this->data = data;
	position = 0;
}
EXROStream::~EXROStream()
{
}

void EXROStream::write(const char c[], int n)
{
	if(position + n > data->get_compressed_allocated())
		data->allocate_compressed_data(MAX(position + n, data->get_compressed_allocated() * 2));

	memcpy(data->get_data() + position, c, n);
	position += n;
	data->set_compressed_size(MAX(position, data->get_compressed_size()));
}

Imf::Int64 EXROStream::tellp()
{
	return position;
}

void EXROStream::seekp(Imf::Int64 pos)
{
	position = pos;
}











FileEXR::FileEXR(Asset *asset, File *file)
 : FileList(asset, file, "EXRLIST", ".exr", FILE_EXR, FILE_EXR_LIST)
{
	native_cmodel = BC_RGB_FLOAT;
	is_yuv = 0;
	temp_y = 0;
	temp_u = 0;
	temp_v = 0;
}

FileEXR::~FileEXR()
{
	if(temp_y) delete [] temp_y;
	if(temp_u) delete [] temp_u;
	if(temp_v) delete [] temp_v;
}

char* FileEXR::compression_to_str(int compression)
{
	switch(compression)
	{
		case FileEXR::NONE: return "None"; break;
		case FileEXR::PIZ: return "PIZ"; break;
		case FileEXR::ZIP: return "ZIP"; break;
		case FileEXR::ZIPS: return "ZIPS"; break;
		case FileEXR::RLE: return "RLE"; break;
		case FileEXR::PXR24: return "PXR24"; break;
	}
	return "None";
}

int FileEXR::compression_to_exr(int compression)
{
	switch(compression)
	{
		case FileEXR::NONE: return (int)Imf::NO_COMPRESSION; break;
		case FileEXR::PIZ: return (int)Imf::PIZ_COMPRESSION; break;
		case FileEXR::ZIP: return (int)Imf::ZIP_COMPRESSION; break;
		case FileEXR::ZIPS: return (int)Imf::ZIPS_COMPRESSION; break;
		case FileEXR::RLE: return (int)Imf::RLE_COMPRESSION; break;
		case FileEXR::PXR24: return (int)Imf::PXR24_COMPRESSION; break;
	}
	return Imf::NO_COMPRESSION;
}

int FileEXR::str_to_compression(char *string)
{
	if(!strcmp(compression_to_str(FileEXR::NONE), string)) 	
		return FileEXR::NONE;
	if(!strcmp(compression_to_str(FileEXR::PIZ), string)) 	
		return FileEXR::PIZ;
	if(!strcmp(compression_to_str(FileEXR::ZIP), string)) 	
		return FileEXR::ZIP;
	if(!strcmp(compression_to_str(FileEXR::ZIPS), string)) 	
		return FileEXR::ZIPS;
	if(!strcmp(compression_to_str(FileEXR::RLE), string)) 	
		return FileEXR::RLE;
	if(!strcmp(compression_to_str(FileEXR::PXR24), string)) 	
		return PXR24;
	return FileEXR::NONE;
}

int FileEXR::check_sig(Asset *asset, char *test)
{
	if(Imf::isImfMagic(test)) return 1;
	if(test[0] == 'E' && test[1] == 'X' && test[2] == 'R' && 
		test[3] == 'L' && test[4] == 'I' && test[5] == 'S' && test[6] == 'T')
	{
		return 1;
	}

	return 0;
}

void FileEXR::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(video_options)
	{
		EXRConfigVideo *window = new EXRConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileEXR::colormodel_supported(int colormodel)
{
	return native_cmodel;
}

int FileEXR::get_best_colormodel(Asset *asset, int driver)
{
	if(asset->exr_use_alpha)
		return BC_RGBA_FLOAT;
	else
		return BC_RGB_FLOAT;
}

int64_t FileEXR::get_memory_usage()
{
	int64_t result = FileList::get_memory_usage();
	if(temp_y) result += (int64_t)asset->width * asset->height * 3 / 2;
	return result;
}


int FileEXR::read_frame_header(char *path)
{
	int result = 0;

// This may have been used by VFS
// 	FILE *stream;
// 
// 	if(!(stream = fopen(path, "rb")))
// 	{
// 		perror("FileEXR::read_frame_header");
// 		return 1;
// 	}
// 	int size = FileSystem::get_size(path);
// 	char *buffer = new char[size];
// 	fread(buffer, size, 1, stream);
// 	fclose(stream);
// 
// 	EXRIStream exr_stream(buffer, size);
// 	Imf::InputFile file(exr_stream);


	Imf::InputFile file(path);

	Imath::Box2i dw = file.header().dataWindow();
	
	asset->width = dw.max.x - dw.min.x + 1;
	asset->height = dw.max.y - dw.min.y + 1;
	asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;

	const Imf::ChannelList &channels = file.header().channels();

	if(channels.findChannel("A"))
		native_cmodel = BC_RGBA_FLOAT;
	else
		native_cmodel = BC_RGB_FLOAT;

	if(channels.findChannel("Y"))
		is_yuv = 1;
// for (Imf::ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
// {
// printf("%s\n", i.name());
// }

//	delete [] buffer;
	return result;
}

int FileEXR::read_frame(VFrame *frame, VFrame *data)
{
	EXRIStream exr_stream((char*)data->get_data(), data->get_compressed_size());
	Imf::InputFile file(exr_stream);
	Imath::Box2i dw = file.header().dataWindow();
    int dx = dw.min.x;
    int dy = dw.min.y;
	Imf::FrameBuffer framebuffer;
	float **rows = (float**)frame->get_rows();
	int components = cmodel_components(frame->get_color_model());

	if(is_yuv)
	{
		if(!temp_y) temp_y = new float[asset->width * asset->height];
		if(!temp_u) temp_u = new float[asset->width * asset->height / 4];
		if(!temp_v) temp_v = new float[asset->width * asset->height / 4];
		framebuffer.insert("Y", Imf::Slice(Imf::FLOAT, 
			(char*)(temp_y - dy * asset->width - dx),
			sizeof(float),
			sizeof(float) * frame->get_w()));
		framebuffer.insert("BY", Imf::Slice(Imf::FLOAT, 
			(char*)(temp_u - dy * asset->width / 4 - dx / 2),
			sizeof(float),
			sizeof(float) * frame->get_w() / 2,
			2, 
			2));
		framebuffer.insert("RY", Imf::Slice(Imf::FLOAT, 
			(char*)(temp_v - dy * asset->width / 4 - dx / 2),
			sizeof(float),
			sizeof(float) * frame->get_w() / 2,
			2, 
			2));
	}
	else
	{
		framebuffer.insert("R", Imf::Slice(Imf::FLOAT, 
			(char*)(&rows[-dy][-dx * components]),
			sizeof(float) * components,
			sizeof(float) * components * frame->get_w()));
		framebuffer.insert("G", Imf::Slice(Imf::FLOAT, 
			(char*)(&rows[-dy][-dx * components + 1]),
			sizeof(float) * components,
			sizeof(float) * components * frame->get_w()));
		framebuffer.insert("B", Imf::Slice(Imf::FLOAT, 
			(char*)(&rows[-dy][-dx * components + 2]),
			sizeof(float) * components,
			sizeof(float) * components * frame->get_w()));
	}

// Alpha always goes directly to the output frame
	if(components == 4)
	{
		framebuffer.insert("A", Imf::Slice(Imf::FLOAT, 
			(char*)(&rows[-dy][-dx * components + 3]),
			sizeof(float) * components,
			sizeof(float) * components * frame->get_w()));
	}

	file.setFrameBuffer(framebuffer);
	file.readPixels (dw.min.y, dw.max.y);



	if(is_yuv)
	{
// Convert to RGB using crazy ILM equations
		Imath::V3f yw;
		Imf::Chromaticities cr;
		yw = Imf::RgbaYca::computeYw(cr);

		for(int i = 0; i < asset->height - 1; i += 2)
		{
			float *y_row1 = temp_y + i * asset->width;
			float *y_row2 = temp_y + (i + 1) * asset->width;
			float *u_row = temp_u + (i * asset->width / 4);
			float *v_row = temp_v + (i * asset->width / 4);
			float *out_row1 = rows[i];
			float *out_row2 = rows[i + 1];
			for(int j = 0; j < asset->width - 1; j += 2)
			{
				float v = *u_row++;
				float u = *v_row++;
				float y;

				float r, g, b;
				y = *y_row1++;
				r = (u + 1) * y;
				b = (v + 1) * y;
				g = (y - r * yw.x - b * yw.z) / yw.y;
				*out_row1++ = r;
				*out_row1++ = g;
				*out_row1++ = b;
				if(components == 4) out_row1++;

				y = *y_row1++;
				r = (u + 1) * y;
				b = (v + 1) * y;
				g = (y - r * yw.x - b * yw.z) / yw.y;
				*out_row1++ = r;
				*out_row1++ = g;
				*out_row1++ = b;
				if(components == 4) out_row1++;

				y = *y_row2++;
				r = (u + 1) * y;
				b = (v + 1) * y;
				g = (y - r * yw.x - b * yw.z) / yw.y;
				*out_row2++ = r;
				*out_row2++ = g;
				*out_row2++ = b;
				if(components == 4) out_row1++;

				y = *y_row2++;
				r = (u + 1) * y;
				b = (v + 1) * y;
				g = (y - r * yw.x - b * yw.z) / yw.y;
				*out_row2++ = r;
				*out_row2++ = g;
				*out_row2++ = b;
				if(components == 4) out_row1++;
			}
		}
	}
	return 0;
}




int FileEXR::write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit)
{
	EXRUnit *exr_unit = (EXRUnit*)unit;
	int result = 0;

	VFrame *output_frame;
	data->set_compressed_size(0);


	int native_cmodel = asset->exr_use_alpha ? BC_RGBA_FLOAT : BC_RGB_FLOAT;
	int components = cmodel_components(native_cmodel);

	if(frame->get_color_model() != native_cmodel)
	{
		if(!exr_unit->temp_frame) exr_unit->temp_frame = new VFrame(0,
			asset->width,
			asset->height,
			native_cmodel);
		cmodel_transfer(exr_unit->temp_frame->get_rows(), /* Leave NULL if non existent */
			frame->get_rows(),
			exr_unit->temp_frame->get_y(), /* Leave NULL if non existent */
			exr_unit->temp_frame->get_u(),
			exr_unit->temp_frame->get_v(),
			frame->get_y(), /* Leave NULL if non existent */
			frame->get_u(),
			frame->get_v(),
			0,        /* Dimensions to capture from input frame */
			0, 
			asset->width, 
			asset->height,
			0,       /* Dimensions to project on output frame */
			0, 
			asset->width, 
			asset->height,
			frame->get_color_model(), 
			native_cmodel,
			0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			asset->width,       /* For planar use the luma rowspan */
			asset->height);
		output_frame = exr_unit->temp_frame;
	}
	else
		output_frame = frame;

	Imf::Header header(output_frame->get_w(), output_frame->get_h());
	header.compression() = (Imf::Compression)compression_to_exr(
		asset->exr_compression);
	header.channels().insert("R", Imf::Channel(Imf::FLOAT));
	header.channels().insert("G", Imf::Channel(Imf::FLOAT));
	header.channels().insert("B", Imf::Channel(Imf::FLOAT));
	if(asset->exr_use_alpha) header.channels().insert("A", Imf::Channel(Imf::FLOAT));

	EXROStream exr_stream(data);
	Imf::OutputFile file(exr_stream, header);
	Imf::FrameBuffer framebuffer;
	float **rows = (float**)output_frame->get_rows();
	framebuffer.insert("R",
		Imf::Slice(Imf::FLOAT,
			(char*)(rows[0]),
			sizeof(float) * components,
			sizeof(float) * components * output_frame->get_w()));
	framebuffer.insert("G",
		Imf::Slice(Imf::FLOAT,
			(char*)(rows[0] + 1),
			sizeof(float) * components,
			sizeof(float) * components * output_frame->get_w()));
	framebuffer.insert("B",
		Imf::Slice(Imf::FLOAT,
			(char*)(rows[0] + 2),
			sizeof(float) * components,
			sizeof(float) * components * output_frame->get_w()));
	if(asset->exr_use_alpha)
		framebuffer.insert("A",
			Imf::Slice(Imf::FLOAT,
				(char*)(rows[0] + 3),
				sizeof(float) * components,
				sizeof(float) * components * output_frame->get_w()));
	file.setFrameBuffer(framebuffer);
	file.writePixels(asset->height);
	return 0;
}

FrameWriterUnit* FileEXR::new_writer_unit(FrameWriter *writer)
{
	return new EXRUnit(this, writer);
}












EXRUnit::EXRUnit(FileEXR *file, FrameWriter *writer)
 : FrameWriterUnit(writer)
{
	this->file = file;
	temp_frame = 0;
}

EXRUnit::~EXRUnit()
{
	if(temp_frame) delete temp_frame;
}











EXRConfigVideo::EXRConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	300,
	BC_OKButton::calculate_h() + 100)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

EXRConfigVideo::~EXRConfigVideo()
{
}

int EXRConfigVideo::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new EXRUseAlpha(this, x, y));
	y += 30;
	EXRCompression *menu;
	add_subwindow(new BC_Title(x, y, "Compression:"));
	x += 110;
	add_subwindow(menu = new EXRCompression(this, x, y, 100));
	menu->create_objects();
	add_subwindow(new BC_OKButton(this));
	return 0;
}

int EXRConfigVideo::close_event()
{
	set_done(0);
	return 1;
}


EXRUseAlpha::EXRUseAlpha(EXRConfigVideo *gui, int x, int y)
 : BC_CheckBox(x, y, gui->asset->exr_use_alpha, _("Use alpha"))
{
	this->gui = gui;
}

int EXRUseAlpha::handle_event()
{
	gui->asset->exr_use_alpha = get_value();
	return 1;
}



EXRCompression::EXRCompression(EXRConfigVideo *gui, int x, int y, int w)
 : BC_PopupMenu(x, 
 	y, 
	w, 
	FileEXR::compression_to_str(gui->asset->exr_compression))
{
	this->gui = gui;
}
void EXRCompression::create_objects()
{
	add_item(new EXRCompressionItem(gui, FileEXR::NONE));
	add_item(new EXRCompressionItem(gui, FileEXR::PIZ));
	add_item(new EXRCompressionItem(gui, FileEXR::ZIP));
	add_item(new EXRCompressionItem(gui, FileEXR::ZIPS));
	add_item(new EXRCompressionItem(gui, FileEXR::RLE));
	add_item(new EXRCompressionItem(gui, FileEXR::PXR24));
}

int EXRCompression::handle_event()
{
	return 1;
}

EXRCompressionItem::EXRCompressionItem(EXRConfigVideo *gui, int value)
 : BC_MenuItem(FileEXR::compression_to_str(value))
{
	this->gui = gui;
	this->value = value;
}

int EXRCompressionItem::handle_event()
{
	gui->asset->exr_compression = value;
	return 0;
}

