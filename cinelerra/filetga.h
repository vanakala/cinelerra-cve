#ifndef FILETGA_H
#define FILETGA_H

#include "filelist.h"
#include "guicast.h"

class FileTGA : public FileList
{
public:
	FileTGA(Asset *asset, File *file);
	~FileTGA();

	static int check_sig(Asset *asset);
	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);
	int read_frame_header(char *path);
	static char* compression_to_str(char *compression);
	static char* str_to_compression(char *string);

// To be used in single frame FileTGA
	static void read_tga(Asset *asset, VFrame *frame, VFrame *data, VFrame* &temp);
	static void write_tga(Asset *asset, VFrame *frame, VFrame *data, VFrame* &temp);

	int can_copy_from(Edit *edit, long position);
	static int get_best_colormodel(Asset *asset, int driver);
	int colormodel_supported(int colormodel);
	int read_frame(VFrame *frame, VFrame *data);
	int write_frame(VFrame *frame, VFrame *data, FrameWriterUnit *unit);
	FrameWriterUnit* new_writer_unit(FrameWriter *writer);

// For decoding only
	VFrame *temp;

private:
	static void write_data(unsigned char *buffer, 
		VFrame *data, 
		long &file_offset,
		long len);
	static void read_line(unsigned char *row,
		unsigned char *data,
		long &file_offset,
		int image_type,
		int bpp,
		int image_compression,
		int bytes,
		int width,
		int fliphoriz,
		int alphabits,
		int data_size);
	static void flip_line(unsigned char *row, int bytes, int width);
	static void rle_read(unsigned char *row,
		unsigned char *data,
		long &file_offset,
		int bytes,
		int width);
	static void rle_write(unsigned char *buffer, 
		int width, 
		int bytes, 
		VFrame *frame, 
		long &file_offset);
	static void bgr2rgb(unsigned char *dest,
		 unsigned char *src,
		 int width,
		 int bytes,
		 int alpha);
	static void upsample(unsigned char *dest,
		  unsigned char *src,
		  int width,
		  int bytes);
};

class TGAUnit : public FrameWriterUnit
{
public:
	TGAUnit(FileTGA *file, FrameWriter *writer);
	~TGAUnit();
	VFrame *temp;
	FileTGA *file;
};

class TGAConfigVideo : public BC_Window
{
public:
	TGAConfigVideo(BC_WindowBase *gui, Asset *asset);
	~TGAConfigVideo();
	int create_objects();
	int close_event();

	ArrayList<BC_ListBoxItem*> compression_items;
	BC_WindowBase *gui;
	Asset *asset;
};

class TGACompression : public BC_PopupTextBox
{
public:
	TGACompression(TGAConfigVideo *gui,
		int x, 
		int y, 
		Asset *asset, 
		ArrayList<BC_ListBoxItem*> *compression_items);
	int handle_event();
	Asset *asset;
};

#endif
