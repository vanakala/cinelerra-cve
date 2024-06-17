// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILETGA_H
#define FILETGA_H

#include "filetga.inc"
#include "filelist.h"

class FileTGA : public FileList
{
public:
	FileTGA(Asset *asset, File *file);

	static int check_sig(Asset *asset);
	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int options);
	int read_frame_header(const char *path);

	size_t get_memory_usage();
	static void write_tga(Asset *asset, VFrame *frame, VFrame *data, VFrame* &temp);
	int read_frame(VFrame *frame, VFrame *data);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);
	static void save_render_options(Asset *asset);
	static void load_render_options(Asset *asset);
	static void dump_TGA(unsigned const char *tga, int indent = 0);
	static const char *tga_image_type(int val);

	static const struct selection_int tga_compression[];
	static struct paramlist_defaults encoder_params[];

private:
	static void write_data(unsigned char *buffer, VFrame *data,
		int &file_offset, int len);
	static void read_line(unsigned char *row, unsigned char *data,
		int &file_offset, int image_type, int bpp,
		int image_compression, int bytes, int width,
		int fliphoriz, int alphabits, int data_size);
	static void flip_line(unsigned char *row, int bytes, int width);
	static void rle_read(unsigned char *row, unsigned char *data,
		int &file_offset, int bytes, int width);
	static void rle_write(unsigned char *buffer, int width, int bytes,
		VFrame *frame, int &file_offset);
	static void bgr2rgb(unsigned char *dest, unsigned char *src,
		int width, int bytes, int alpha);
	static void upsample(unsigned char *dest, unsigned char *src,
		int width, int bytes);
};

class TGAUnit : public FrameWriterUnit
{
public:
	TGAUnit(FileTGA *file, FrameWriter *writer);
	~TGAUnit();

	VFrame *temp;
	FileTGA *file;
};

#endif
