
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

#ifndef FILEAVI_H
#define FILEAVI_H

#include "asset.inc"
#include "assets.inc"
#include "file.inc"
#include "filebase.h"

#ifdef USE_AVIFILE
#include "avifile.h"
#include <vector>
#endif

//class IAviWriteFile;
//class IAviReadFile;
//class IAviReadStream;

#define MAX_STREAMS 256

class FileAVI : public FileBase
{
public:
	FileAVI(Asset *asset, File *file);
	~FileAVI();

	static int check_sig(Asset *asset);
	static void get_parameters(BC_WindowBase *parent_window, 
		Asset *asset, 
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options,
		char *locked_compressor);

	int get_best_colormodel(int driver, int colormodel);
	int open_file(int rd, int wr);
	int close_file();
	void reset();
	int set_audio_position(int64_t x);
	int set_video_position(int64_t x);
	int write_samples(double **buffer, int64_t len);
	int write_frames(VFrame ***frames, int len);
	int write_compressed_frame(VFrame *buffer);
	int read_compressed_frame(VFrame *buffer);
	int64_t compressed_frame_size();
	int read_samples(double *buffer, int64_t len);
	int read_frame(VFrame *frame);
	static char* vcodec_to_fourcc(char *input, char *output);
	static char* fourcc_to_vcodec(char *input, char *output);
	static char* acodec_to_fourcc(char *input, char *output);
	static char* fourcc_to_acodec(char *input, char *output);
	static int cmodel_bc_to_avi(int input);

	static void initialize_avifile();


private:
	static int check_sig_arne2(Asset *asset, int &score);
	static int check_sig_arne1(Asset *asset, int &score);
	static int check_sig_lavtools(Asset *asset, int &score);
	static int check_sig_avifile(Asset *asset, int &score);

	int open_avifile_in(Asset *asset);
	int open_arne2_in(Asset *asset);
	int open_arne1_in(Asset *asset);
	int open_lavtools_in(Asset *asset);


	int open_avifile_out(Asset *asset);
	int open_arne2_out(Asset *asset);
	int open_arne1_out(Asset *asset);
	int open_lavtools_out(Asset *asset);

#ifdef USE_AVIFILE
	IAviWriteFile *out_fd;
	IAviReadFile *in_fd;

	IAviReadStream *astream_in[MAX_STREAMS];
	IAviReadStream *vstream_in[MAX_STREAMS];
	IAviWriteStream *astream_out[MAX_STREAMS];
	IAviVideoWriteStream *vstream_out[MAX_STREAMS];
	int out_color_model;
    BitmapInfo *out_bitmap_info;
	static int avifile_initialized;

#endif



	



	unsigned char *temp_audio;
	int64_t temp_allocated;
	int source_cmodel;
};


class AVIConfigAudio;

class AVIACodecList : public BC_PopupTextBox
{
public:
	AVIACodecList(AVIConfigAudio *gui, int x, int y);
	~AVIACodecList();

	int handle_event();

	AVIConfigAudio *gui;
};

class AVIConfigAudio : public BC_Window
{
public:
	AVIConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~AVIConfigAudio();

	static int calculate_w(int format);
	static int calculate_h(int format);
	int create_objects();
	int close_event();
	int generate_codeclist();
	void update_codecs();

	AVIACodecList *list;

	ArrayList<BC_ListBoxItem*> codec_items;
	ArrayList<BC_ISlider*> attribute_sliders;
	BC_WindowBase *parent_window;
	Asset *asset;
	char string[BCTEXTLEN];
};

class AVIConfigVideo;

class AVIVCodecList : public BC_PopupTextBox
{
public:
	AVIVCodecList(AVIConfigVideo *gui, int x, int y);
	~AVIVCodecList();

	int handle_event();

	AVIConfigVideo *gui;
};

class AVIVAttributeList : public BC_ListBox
{
public:
	AVIVAttributeList(AVIConfigVideo *gui, int x, int y);
	int handle_event();
	int selection_changed();
	AVIConfigVideo *gui;
};

class AVIVAttribute : public BC_TextBox
{
public:
	AVIVAttribute(AVIConfigVideo *gui, int x, int y);
	int handle_event();
	AVIConfigVideo *gui;
};

class AVIConfigVideo : public BC_Window
{
public:
	AVIConfigVideo(BC_WindowBase *parent_window, 
		Asset *asset, 
		char *locked_compressor);
	~AVIConfigVideo();

	static int calculate_w(int format);
	static int calculate_h(int format);
	int create_objects();
	int close_event();
	int generate_codeclist();
	void generate_attributelist();
	void update_attribute(int recursive);
// Get text associated with current attribute
	char* get_current_attribute_text();
	char* get_current_attribute_value();
	void set_current_attribute(char *text);
	void reset();

	AVIVCodecList *list;
	AVIVAttributeList *attributes;
	AVIVAttribute *attribute;
	ArrayList<BC_ListBoxItem*> codec_items;
	ArrayList<BC_ListBoxItem*> attribute_items[2];
	BC_WindowBase *parent_window;
	Asset *asset;
	char *locked_compressor;
	char string[BCTEXTLEN];
};



#endif
