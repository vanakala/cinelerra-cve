#include "assets.h"
#include "edit.h"
#include "file.h"
#include "filetiff.h"
#include "vframe.h"

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
	}
	return 0;
}

int FileTIFF::can_copy_from(Edit *edit, int64_t position)
{
	if(edit->asset->format == FILE_TIFF_LIST ||
		edit->asset->format == FILE_TIFF)
		return 1;
	
	return 0;
}

#define TIFF_RGB "rgb "
#define TIFF_RGBA "rgba"


int FileTIFF::read_frame_header(char *path)
{
	TIFF *stream;
	int result = 0;

	if(!(stream = TIFFOpen(path, "rb")))
	{
		perror("FileTIFF::read_header");
		return 1;
	}

	TIFFGetField(stream, TIFFTAG_IMAGEWIDTH, &(asset->width));
	TIFFGetField(stream, TIFFTAG_IMAGELENGTH, &(asset->height));
	int depth = 3;
//	TIFFGetField(stream, TIFFTAG_IMAGEDEPTH, &depth);
	TIFFGetField(stream, TIFFTAG_SAMPLESPERPIXEL, &depth);
	if(depth == 3)
		strcpy(asset->vcodec, TIFF_RGB);
	else
		strcpy(asset->vcodec, TIFF_RGBA);

//printf("FileTIFF::read_frame_header 1 %d\n", depth);
	TIFFClose(stream);


	return result;
}

int FileTIFF::colormodel_supported(int colormodel)
{
	if(!strcmp(asset->vcodec, TIFF_RGB))
		return BC_RGB888;
	else
		return BC_RGBA8888;
}

int FileTIFF::get_best_colormodel(Asset *asset, int driver)
{
	if(!strcmp(asset->vcodec, TIFF_RGB))
		return BC_RGB888;
	else
		return BC_RGBA8888;
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
//printf("tiff_write 1 %d\n", size);
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
//printf("tiff_write 2\n");
	return size;
}

static toff_t tiff_seek(thandle_t ptr, toff_t off, int whence)
{
	FileTIFFUnit *tiff_unit = (FileTIFFUnit*)ptr;
//printf("tiff_seek 1 %d %d\n", off, whence);
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
//printf("tiff_seek 2\n");
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
//printf("tiff_mmap 1\n");
	FileTIFFUnit *tiff_unit = (FileTIFFUnit*)ptr;
	*pbase = tiff_unit->data->get_data();
	*psize = tiff_unit->data->get_compressed_size();
//printf("tiff_mmap 10\n");
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

//printf("FileTIFF::read_frame 1 %d\n", output->get_color_model());
	if(output->get_color_model() == BC_RGBA8888 ||
		output->get_color_model() == BC_RGB888)
	{
//printf("FileTIFF::read_frame 2\n");
		for(int i = 0; i < asset->height; i++)
		{
			TIFFReadScanline(stream, output->get_rows()[i], i, 0);
		}
//printf("FileTIFF::read_frame 4\n");
	}
	else
	{
		printf("FileTIFF::read_frame color model = %d\n",
			output->get_color_model());
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

//printf("FileTIFF::write_frame 1\n");
	TIFFConfigVideo::fix_codec(asset->vcodec);
//printf("FileTIFF::write_frame 1\n");
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

//printf("FileTIFF::write_frame 1\n");
	int depth, color_model;
	if(!strcmp(asset->vcodec, TIFF_RGBA))
	{
		depth = 32;
		color_model = BC_RGBA8888;
	}
	else
	{
		depth = 24;
		color_model = BC_RGB888;
	}

//printf("FileTIFF::write_frame 1\n");
	TIFFSetField(stream, TIFFTAG_IMAGEWIDTH, asset->width);
	TIFFSetField(stream, TIFFTAG_IMAGELENGTH, asset->height);
	TIFFSetField(stream, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(stream, TIFFTAG_SAMPLESPERPIXEL, depth / 8);
	TIFFSetField(stream, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(stream, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(stream, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(stream, (uint32_t)-1));
	TIFFSetField(stream, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

//printf("FileTIFF::write_frame 1\n");
	if(frame->get_color_model() == color_model)
	{
		for(int i = 0; i < asset->height; i++)
		{
//printf("FileTIFF::write_frame 2 %d\n", i);
			TIFFWriteScanline(stream, frame->get_rows()[i], i, 0);
//printf("FileTIFF::write_frame 3\n");
		}
	}
	else
	{
//printf("FileTIFF::write_frame 2\n");
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
//printf("FileTIFF::write_frame 3 %d %d\n", color_model, frame->get_color_model());
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
//printf("FileTIFF::write_frame 5\n");
		for(int i = 0; i < asset->height; i++)
		{
			TIFFWriteScanline(stream, tiff_unit->temp->get_rows()[i], i, 0);
		}
//printf("FileTIFF::write_frame 6\n");
	}
//printf("FileTIFF::write_frame 7\n");
//sleep(1);
//printf("FileTIFF::write_frame 71\n");

	TIFFClose(stream);
//printf("FileTIFF::write_frame 8\n");

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
 	parent_window->get_abs_cursor_x(),
 	parent_window->get_abs_cursor_y(),
	400,
	100)
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

	fix_codec(asset->vcodec);
	add_subwindow(new TIFFConfigAlpha(this, x, y));

	add_subwindow(new BC_OKButton(this));
	return 0;
}

int TIFFConfigVideo::close_event()
{
	set_done(0);
	return 1;
}

char* TIFFConfigVideo::alpha_to_codec(int use_alpha)
{
	if(use_alpha) 
		return TIFF_RGBA;
	else
		return TIFF_RGB;
}

int TIFFConfigVideo::codec_to_alpha(char *codec)
{
	if(!strcmp(codec, TIFF_RGBA))
		return 1;
	else
		return 0;
}

void TIFFConfigVideo::fix_codec(char *codec)
{
	if(strcmp(codec, TIFF_RGB) &&
		strcmp(codec, TIFF_RGBA))
		strcpy(codec, TIFF_RGB);
}


TIFFConfigAlpha::TIFFConfigAlpha(TIFFConfigVideo *gui, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	TIFFConfigVideo::codec_to_alpha(gui->asset->vcodec), 
 	"Use alpha")
{
	this->gui = gui;
}

int TIFFConfigAlpha::handle_event()
{
	if(TIFFConfigVideo::codec_to_alpha(gui->asset->vcodec))
	{
		strcpy(gui->asset->vcodec, TIFF_RGB);
	}
	else
		strcpy(gui->asset->vcodec, TIFF_RGBA);
	
	update(TIFFConfigVideo::codec_to_alpha(gui->asset->vcodec));
	return 1;
}

