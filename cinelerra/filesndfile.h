#ifndef FILESNDFILE_H
#define FILESNDFILE_H

#include "bitspopup.inc"
#include "filebase.h"
#include "filesndfile.h"
#include "sndfile.h"

class FileSndFile : public FileBase
{
public:
	FileSndFile(Asset *asset, File *file);
	~FileSndFile();

	static int check_sig(Asset *asset);
	int open_file(int rd, int wr);
	int close_file();
	int set_audio_position(long sample);
	int read_samples(double *buffer, long len);
	int write_samples(double **buffer, long len);
	void format_to_asset();
	void asset_to_format();

	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);

	SNDFILE *fd;
	SF_INFO fd_config;
// Temp for interleaved channels
	double *temp_double;
	long temp_allocated;
};

class SndFileConfig;

class SndFileHILO : public BC_Radial
{
public:
	SndFileHILO(SndFileConfig *gui, int x, int y);
	int handle_event();
	SndFileConfig *gui;
};

class SndFileLOHI : public BC_Radial
{
public:
	SndFileLOHI(SndFileConfig *gui, int x, int y);
	int handle_event();
	SndFileConfig *gui;
};


class SndFileConfig : public BC_Window
{
public:
	SndFileConfig(BC_WindowBase *parent_window, Asset *asset);
	~SndFileConfig();

	int create_objects();
	int close_event();

	BC_WindowBase *parent_window;
	BitsPopup *bits_popup;
	SndFileHILO *hilo;
	SndFileLOHI *lohi;
	Asset *asset;
};

#endif
