#ifndef FILEDV_H
#define FILEDV_H

#include "filebase.h"
#include "file.inc"

#include <libdv/dv.h>

// TODO: Do we need a thread for audio/video encoding?

class FileDV : public FileBase
{
public:
	FileDV(Asset *asset, File *file);
	~FileDV();

	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset,
		BC_WindowBase* &format_window,
		int audio_options,
		int video_options);
	
	int reset_parameters_derived();
	int open_file(int rd, int wr);
	static int check_sig(Asset *asset);
	int close_file();
	int close_file_derived();
	int64_t get_video_position();
	int64_t get_audio_position();
	int set_video_position(int64_t x);
	int set_audio_position(int64_t x);
	int write_samples(double **buffer, int64_t len);
	int write_frames(VFrame ***frames, int len);
	int read_compressed_frame(VFrame *buffer);
	int write_compressed_frame(VFrame *buffers);
	int64_t compressed_frame_size();
	int read_samples(double *buffer, int64_t len);
	int read_frame(VFrame *frame);
	int colormodel_supported(int colormodel);
	int can_copy_from(Edit *edit, int64_t position);
	static int get_best_colormodel(Asset *asset, int driver);

private:
	int fd;
	dv_decoder_t *decoder;
	dv_encoder_t *encoder;
	int64_t audio_position;
	int64_t video_position;
	unsigned char *output;
	unsigned char *input;
	int output_size;
	int64_t audio_offset;
	int64_t video_offset;
	int frames_written;
	int16_t **audio_buffer;
	int samples_in_buffer;
};


class DVConfigAudio: public BC_Window
{
public:
	DVConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~DVConfigAudio();

	int create_objects();
	int close_event();

private:
	Asset *asset;
	BC_WindowBase *parent_window;
};



class DVConfigVideo: public BC_Window
{
public:
	DVConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~DVConfigVideo();

	int create_objects();
	int close_event();

private:
	Asset *asset;
	BC_WindowBase *parent_window;
};


#endif
