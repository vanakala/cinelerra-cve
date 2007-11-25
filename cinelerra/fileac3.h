#ifndef FILEAC3_H
#define FILEAC3_H

extern "C" {
#include <avcodec.h>
};

#include "filebase.h"
#include <stdio.h>


class FileAC3 : public FileBase
{
public:
	FileAC3(Asset *asset, File *file);
	~FileAC3();

	int reset_parameters_derived();
	static void get_parameters(BC_WindowBase *parent_window, 
			Asset *asset, 
			BC_WindowBase* &format_window,
			int audio_options,
			int video_options);
	static int check_sig();
	int open_file(int rd, int wr);
	int close_file();
	int write_samples(double **buffer, int64_t len);

private:
	AVCodec *codec;
	AVCodecContext *codec_context;
	FILE *fd;
	int16_t *temp_raw;
	int temp_raw_allocated;
	int temp_raw_size;
	unsigned char *temp_compressed;
	int compressed_allocated;
};



class AC3ConfigAudio : public BC_Window
{
public:
	AC3ConfigAudio(BC_WindowBase *parent_window,
		Asset *asset);

	void create_objects();
	int close_event();

	Asset *asset;
	BC_WindowBase *parent_window;
	char string[BCTEXTLEN];
};


class AC3ConfigAudioBitrate : public BC_PopupMenu
{
public:
	AC3ConfigAudioBitrate(AC3ConfigAudio *gui, int x, int y);

	void create_objects();
	int handle_event();
	static char* bitrate_to_string(char *string, int bitrate);

	AC3ConfigAudio *gui;
};





#endif
