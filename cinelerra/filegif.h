#ifndef FILEGIF_H
#define FILEGIF_H

#include "file.inc"
#include "filebase.h"
#include "vframe.inc"

// This header file is representative of any single frame file format.

class FileGIF : public FileBase
{
public:
	FileGIF(Asset *asset);
	~FileGIF();

// basic commands for every file interpreter
	int open_file(int rd, int wr);
	int close_file_derived();
	long get_video_length();
	long get_memory_usage();

	int read_header();
	VFrame* read_frame(int use_alpha, int use_float);

private:
	int read_raw();
	int reset_parameters_derived();

// specific to GIF
	int import_row(VPixel *output, unsigned char *row_pointer);

// routines for all image files
// frame to return through read_frame
	VFrame *data;
};


#endif
