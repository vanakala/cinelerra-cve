#include "fileyuv.h"
#include "asset.h"
#include "file.h"
#include "guicast.h"
#include "mwindow.h"
#include "defaults.h"
#include "vframe.h"
#include "edit.h"
#include "quicktime.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

FileYUV::FileYUV(Asset *asset, File *file)
	: FileBase(asset, file)
{
	if (asset->format == FILE_UNKNOWN) asset->format = FILE_YUV;
	asset->byte_order = 0; // FUTURE: is this always correct?
	temp = 0;
	ffmpeg = 0;
	stream = new YUVStream();
	pipe_latency = 0;
}

FileYUV::~FileYUV()
{
	// NOTE: close_file() is already called
	delete stream;
}
	
int FileYUV::open_file(int should_read, int should_write)
{
	int result;

	if (should_read) {

		result = stream->open_read(asset->path);
		if (result) return result;

		// NOTE: no easy way to defer setting video_length
		asset->video_length = stream->frame_count;
		
		asset->width = stream->get_width();
		asset->height = stream->get_height();
		if (asset->width * asset->height <= 0) {
			printf("illegal frame size '%d x %d'\n", 
			       asset->width, asset->height);
			return 1;
		}
		
		asset->layers = 1;
		asset->video_data = 1;
		asset->audio_data = 0;
		
		asset->frame_rate = stream->get_frame_rate();
		asset->aspect_ratio = stream->get_aspect_ratio();

		return 0;
	}

	if (should_write) {
		if (asset->use_pipe) {
			result = stream->open_write(asset->path, asset->pipe);
		}
		else {
			result = stream->open_write(asset->path, NULL);
		}
		if (result) return result;

		// not sure if we're supposed to send interlace info with each header??
		stream->set_interlace(asset->pipe_ilace_spec);
		stream->set_width(asset->width);
		stream->set_height(asset->height);
		stream->set_frame_rate(asset->frame_rate);
		stream->set_aspect_ratio(asset->aspect_ratio);

		result = stream->write_header();
		if (result) return result;
		
		return 0;
	}
	
	// no action given
	return 1;
}

int FileYUV::close_file() {
  	if (pipe_latency && ffmpeg && stream) {
		// deal with last frame still in the pipe
		ensure_temp(incoming_asset->width,
			    incoming_asset->height); 
		if (ffmpeg->decode(NULL, 0, temp) == 0) {
			uint8_t *yuv[3];
			yuv[0] = temp->get_y();
			yuv[1] = temp->get_u();
			yuv[2] = temp->get_v();
			stream->write_frame(yuv);
		}
		pipe_latency = 0;
	}
	stream->close_fd();
	if (ffmpeg) delete ffmpeg;
	ffmpeg = 0;
	return 0;
}

// NOTE: set_video_position() called every time a frame is read
int FileYUV::set_video_position(int64_t frame_number) {
	return stream->seek_frame(frame_number);
}

int FileYUV::read_frame(VFrame *frame)
{
	int result;
	VFrame *input = frame;

	// short cut for direct copy routines
	if (frame->get_color_model() == BC_COMPRESSED) {
		long frame_size = (long) // w*h + w*h/4 + w*h/4
			(stream->get_height() *	stream->get_width() * 1.5); 
		frame->allocate_compressed_data(frame_size);
		frame->set_compressed_size(frame_size);
		return stream->read_frame_raw(frame->get_data(), frame_size);
	}
	

	// process through a temp frame if necessary
	if (! cmodel_is_planar(frame->get_color_model()) ||
	    (frame->get_w() != stream->get_width()) ||
	    (frame->get_h() != stream->get_height())) {
		ensure_temp(stream->get_width(),
			    stream->get_height());
		input = temp;
	}

	uint8_t *yuv[3];
	yuv[0] = input->get_y();
	yuv[1] = input->get_u();
	yuv[2] = input->get_v();
	result = stream->read_frame(yuv);
	if (result) return result;

	// transfer from the temp frame to the real one
	if (input != frame) {
		FFMPEG::convert_cmodel(input, frame);
	}
	
	return 0;
}

int FileYUV::write_frames(VFrame ***layers, int len)
{
	int result;

	// only one layer supported
	VFrame **frames = layers[0];
	VFrame *frame;

	for (int n = 0; n < len; n++) {

		frame = frames[n];

		// short cut for direct copy routines
		if (frame->get_color_model() == BC_COMPRESSED) {
			long frame_size = frame->get_compressed_size();
			if (incoming_asset->format == FILE_YUV) {
				return stream->write_frame_raw
					(frame->get_data(), frame_size);
			}

			// decode and write an encoded frame
			if (FFMPEG::codec_id(incoming_asset->vcodec) != CODEC_ID_NONE) {
				if (! ffmpeg) {
					ffmpeg = new FFMPEG(incoming_asset);
					ffmpeg->init(incoming_asset->vcodec);
				}
				
				ensure_temp(incoming_asset->width,
					    incoming_asset->height); 
				int result = ffmpeg->decode(frame->get_data(),
							    frame_size, temp);

				// some formats are decoded one frame later
				if (result == FFMPEG_LATENCY) {
					// remember to write the last frame
					pipe_latency++;
					return 0;
				}

				if (result) {
					delete ffmpeg;
					ffmpeg = 0;
					return 1;
				}


				uint8_t *yuv[3];
				yuv[0] = temp->get_y();
				yuv[1] = temp->get_u();
				yuv[2] = temp->get_v();
				return stream->write_frame(yuv);
			}

		}

		// process through a temp frame only if necessary
		if (! cmodel_is_planar(frame->get_color_model()) ||
		    (frame->get_w() != stream->get_width()) ||
		    (frame->get_h() != stream->get_height())) {
			ensure_temp(asset->width, asset->height);
			FFMPEG::convert_cmodel(frame, temp);
			frame = temp;
		}

		uint8_t *yuv[3];
		yuv[0] = frame->get_y();
		yuv[1] = frame->get_u();
		yuv[2] = frame->get_v();
		result = stream->write_frame(yuv);
		if (result) return result;


	}

	return 0;
}


void FileYUV::get_parameters(BC_WindowBase *parent_window, 
			     Asset *asset, 
			     BC_WindowBase* &format_window,
			     int video_options,
			     FormatTools *format)
{
	if (! video_options) return;

	YUVConfigVideo *config =  
		new YUVConfigVideo(parent_window, asset, format);
	format_window = config;
	config->create_objects();
	if (config->run_window() == 0) {
		// save the new path and pipe to the asset
		strcpy(asset->path, config->path_textbox->get_text());
		strcpy(asset->pipe, config->pipe_config->textbox->get_text());
		// are we using the pipe (if there is one)
		asset->use_pipe = config->pipe_config->checkbox->get_value();
		// update the path textbox in the render window
		format->path_textbox->update(asset->path);
		// set the pipe status in the render window
		format->pipe_status->set_status(asset);
		// and add the new path and pipe to the defaults list
		const char *prefix = FILE_FORMAT_PREFIX(asset->format);
		config->path_recent->add_item(prefix, asset->path);
		config->pipe_config->recent->add_item(prefix, asset->pipe);
	}
	delete config;
}

int FileYUV::check_sig(Asset *asset)
{
	char temp[9];
        FILE *f = fopen(asset->path, "rb");

        // check for starting with "YUV4MPEG2"
        fread(&temp, 9, 1, f);
        fclose(f);
	if (strncmp(temp, "YUV4MPEG2", 9) == 0) return 1;

        return 0;
}

// NOTE: this is called on the write stream, not the read stream!
//       as such, I have no idea what one is supposed to do with position.
int FileYUV::can_copy_from(Edit *edit, int64_t position)
{	// NOTE: width and height already checked in file.C

	// FUTURE: is the incoming asset already available somewhere?
	incoming_asset = edit->asset;

	if (edit->asset->format == FILE_YUV) return 1;

	// if FFMPEG can decode it, we'll accept it
	if (FFMPEG::codec_id(edit->asset->vcodec) != CODEC_ID_NONE) {
		return 1;
	}

	incoming_asset = 0;

	return 0;
}

int FileYUV::get_best_colormodel(Asset *asset, int driver) 
{
	// FUTURE: is there a reason to try to accept anything else?  
	return BC_YUV420P;
}


int FileYUV::colormodel_supported(int color_model) 
{
	// we convert internally to any color model proposed
	return color_model;
	// NOTE: file.C does not convert from YUV, so we have to do it.  
}


/*  
    Other member functions used in other file* modules:

    write_compressed_frame(): used for record, so probably not needed
    read_compressed_frame(): perhaps never used?
    get_video_position: used by record only
    reset_parameters(): not sure when used or needed
    reset_parameters_derived(): not sure when used or needed
    *_audio_*: yuv4mpeg doesn't handle audio
    
*/
	


void FileYUV::ensure_temp(int width, int height) {
	
	// make sure the temp is correct size and type
	if (temp && (temp->get_w() != width ||
		     temp->get_h() != height ||
		     temp->get_color_model() != BC_YUV420P)) {
		delete temp;
		temp = 0;
	}
	
	// create a correct temp frame if we don't have one
	if (temp == 0) {
		temp = new VFrame(0, width, height, BC_YUV420P);
	}
}


YUVConfigVideo::YUVConfigVideo(BC_WindowBase *parent_window, Asset *asset, 
			       FormatTools *format)
	: BC_Window(PROGRAM_NAME ": YUV4MPEG Stream",
		    parent_window->get_abs_cursor_x(1),
		    parent_window->get_abs_cursor_y(1),
		    500,
		    240)
{
	this->parent_window = parent_window;
	this->asset = asset;
	this->format = format;
	this->defaults = format->mwindow->defaults;
}

YUVConfigVideo::~YUVConfigVideo()
{
	delete path_textbox;
	delete path_recent;
	delete pipe_config;
	delete mpeg2enc;
	delete ffmpeg;
}

int YUVConfigVideo::create_objects()
{
	int init_x = 10;
	int init_y = 10;
	
	int x = init_x;
	int y = init_y;

	add_subwindow(new BC_Title(x, y, _("Output Path:")));
	x += 120;
	path_textbox = new BC_TextBox(x, y, 350, 1, asset->path);
	add_subwindow(path_textbox);

	x += 350;
	path_recent = new BC_RecentList("PATH", defaults, path_textbox, 
					10, x, y, 350, 100);
	add_subwindow(path_recent);
	path_recent->load_items(FILE_FORMAT_PREFIX(asset->format));

	x = init_x;
	y += 30;

	pipe_config = new PipeConfig(this, defaults, asset);
	pipe_config->create_objects(x, y, 350, asset->format);
	

	x = init_x;
	y += 120;

	add_subwindow(new BC_Title(x, y, _("Pipe Presets:")));
	x += 130;
	mpeg2enc = new PipePreset(x, y, "mpeg2enc", pipe_config);
	add_subwindow(mpeg2enc);
	// NOTE: the '%' character will be replaced by the current path
	// NOTE: to insert a real '%' double it up: '%%' -> '%'
	// NOTE: preset items must have a '|' before the actual command
	mpeg2enc->add_item(new BC_MenuItem ("(DVD) | mpeg2enc -f 8 -o %"));
	mpeg2enc->add_item(new BC_MenuItem ("(VCD) | mpeg2enc -f 2 -o %"));

	x += 160;
	ffmpeg = new PipePreset(x, y, "ffmpeg", pipe_config);
	add_subwindow(ffmpeg);
	ffmpeg->add_item(new BC_MenuItem("(DVD) | ffmpeg -f yuv4mpegpipe -i - -y -target dvd %"));
	ffmpeg->add_item(new BC_MenuItem("(VCD) | ffmpeg -f yuv4mpegpipe -i - -y -target vcd %"));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	return 0;
}

int YUVConfigVideo::close_event()
{
	set_done(0);
	return 1;
}



