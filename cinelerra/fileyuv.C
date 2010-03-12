
/*
 * CINELERRA
 * Copyright (C) 2004 Nathan Kurz
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

#include "fileyuv.h"
#include "asset.h"
#include "bchash.h"
#include "edit.h"
#include "file.h"
#include "guicast.h"
#include "guicast.h"
#include "interlacemodes.h"
#include "quicktime.h"
#include "mainerror.h"
#include "mwindow.h"
#include "vframe.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

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

	if (should_read) 
	{
		result = stream->open_read(asset->path);
		if (result) return result;

		// NOTE: no easy way to defer setting video_length
		asset->video_length = stream->frame_count;
		
		asset->width = stream->get_width();
		asset->height = stream->get_height();
		if (asset->width * asset->height <= 0) 
		{
			eprintf("illegal frame size '%d x %d'\n", asset->width, asset->height);
			return 1;
		}
		
		asset->layers = 1;
		asset->video_data = 1;
		asset->audio_data = 0;
		
		asset->frame_rate = stream->get_frame_rate();
		asset->aspect_ratio = stream->get_aspect_ratio();
		asset->interlace_mode = stream->get_interlace();
		
		return 0;
	}

	if (should_write) {
		if (asset->use_pipe) {
			result = stream->open_write(asset->path, asset->pipe);
		} else {
			result = stream->open_write(asset->path, NULL);
		}
		if (result) return result;

		// not sure if we're supposed to send interlace info with each set of frames, (wouldn't know howto!)??
		stream->set_interlace(asset->interlace_mode);
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
		ensure_temp(incoming_asset->width, incoming_asset->height); 
		if (ffmpeg->decode(NULL, 0, temp) == 0) 
		{
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
	    (frame->get_h() != stream->get_height())) 
	{
		ensure_temp(stream->get_width(), stream->get_height());
		input = temp;
	}

	uint8_t *yuv[3];
	yuv[0] = input->get_y();
	yuv[1] = input->get_u();
	yuv[2] = input->get_v();
	result = stream->read_frame(yuv);
	if (result) return result;

	// transfer from the temp frame to the real one
	if (input != frame) 
	{
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

	for (int n = 0; n < len; n++) 
	{
		frame = frames[n];

		// short cut for direct copy routines
		if (frame->get_color_model() == BC_COMPRESSED) 
		{
			long frame_size = frame->get_compressed_size();
			if (incoming_asset->format == FILE_YUV) 
				return stream->write_frame_raw(frame->get_data(), frame_size);

			// decode and write an encoded frame
			if (FFMPEG::codec_id(incoming_asset->vcodec) != CODEC_ID_NONE) 
			{
				if (! ffmpeg) 
				{
					ffmpeg = new FFMPEG(incoming_asset);
					ffmpeg->init(incoming_asset->vcodec);
				}
				
				ensure_temp(incoming_asset->width, incoming_asset->height); 
				int result = ffmpeg->decode(frame->get_data(), frame_size, temp);

				// some formats are decoded one frame later
				if (result == FFMPEG_LATENCY) 
				{
					// remember to write the last frame
					pipe_latency++;
					return 0;
				}

				if (result) 
				{
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
		    (frame->get_h() != stream->get_height())) 
		{
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

	YUVConfigVideo *config = new YUVConfigVideo(parent_window, asset, format);
	format_window = config;
	config->create_objects();
	if (config->run_window() == 0) 
	{
		// save the new path and pipe to the asset
		strcpy(asset->path, config->path_textbox->get_text());
		strcpy(asset->pipe, config->pipe_textbox->get_text());
		// are we using the pipe (if there is one)
		asset->use_pipe = config->pipe_checkbox->get_value();
		// update the path textbox in the render window
		format->path_textbox->update(asset->path);
		// add the new path and pipe to the defaults list
		const char *prefix = FILE_FORMAT_PREFIX(asset->format);
		config->path_recent->add_item(prefix, asset->path);
		config->pipe_recent->add_item(prefix, asset->pipe);
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
{
	// NOTE: width and height already checked in file.C

	// FUTURE: is the incoming asset already available somewhere?
	incoming_asset = edit->asset;

	if (edit->asset->format == FILE_YUV) return 1;

	// if FFMPEG can decode it, we'll accept it
	if (FFMPEG::codec_id(edit->asset->vcodec) != CODEC_ID_NONE) return 1;

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
	


void FileYUV::ensure_temp(int width, int height) 
{
	// make sure the temp is correct size and type
	if (temp && (temp->get_w() != width ||
		     temp->get_h() != height ||
		     temp->get_color_model() != BC_YUV420P)) 
	{
		delete temp;
		temp = 0;
	}
	
	// create a correct temp frame if we don't have one
	if (temp == 0) 
	{
		temp = new VFrame(0, width, height, BC_YUV420P);
	}
}


YUVConfigVideo::YUVConfigVideo(BC_WindowBase *parent_window, Asset *asset, FormatTools *format)
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
	delete pipe_checkbox;
	delete pipe_textbox;
	delete pipe_recent;
	delete mpeg2enc;
	delete ffmpeg;
}

int YUVConfigVideo::create_objects()
{
	BC_Title *bt;
	int init_x = 10;
	int init_y = 10;
	
	int x = init_x;
	int y = init_y;

	add_subwindow(new BC_Title(init_x, y, _("Output Path:")));
	add_subwindow(path_textbox = new BC_TextBox(init_x + 100, y, 350, 1, asset->path));
	add_subwindow(path_recent = new BC_RecentList("PATH", defaults, path_textbox, 10, init_x + 450, y, path_textbox->get_w(), 100));
	path_recent->load_items(FILE_FORMAT_PREFIX(asset->format));

	x = init_x;
	y += 30;

	add_subwindow(bt = new BC_Title(init_x, y, _("Use Pipe:")));
	add_subwindow(pipe_checkbox = new PipeCheckBox(init_x + bt->get_w(), y, asset->use_pipe));
	add_subwindow(pipe_textbox = new BC_TextBox(init_x + 100, y, 350, 1, asset->pipe));
	add_subwindow(pipe_recent = new BC_RecentList("PIPE", defaults, pipe_textbox, 10, init_x + 450, y, pipe_textbox->get_w(), 100));
	pipe_recent->load_items(FILE_FORMAT_PREFIX(asset->format));

	pipe_checkbox->textbox = pipe_textbox;
	if (!asset->use_pipe) pipe_textbox->disable();

	x = init_x;
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Stream Header:"), MEDIUMFONT, RED));

	x = init_x + 20;
	y += 30;
	add_subwindow(bt = new BC_Title(x, y, _("Interlacing:")));
	char string[BCTEXTLEN];
	ilacemode_to_text(string,asset->interlace_mode);
	add_subwindow(new BC_Title(x + bt->get_w() + 5, y, string, MEDIUMFONT, YELLOW));

	x = init_x;
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Pipe Presets:")));

	x += 130;
	add_subwindow(mpeg2enc = new PipePreset(x, y, "mpeg2enc", pipe_textbox, pipe_checkbox));
	// NOTE: the '%' character will be replaced by the current path
	// NOTE: to insert a real '%' double it up: '%%' -> '%'
	// NOTE: preset items must have a '|' before the actual command
	mpeg2enc->add_item(new BC_MenuItem ("(DVD) | mpeg2enc -f 8 -o %"));
	mpeg2enc->add_item(new BC_MenuItem ("(VCD) | mpeg2enc -f 2 -o %"));

	x += 180;
	add_subwindow(ffmpeg = new PipePreset(x, y, "ffmpeg", pipe_textbox, pipe_checkbox));
	ffmpeg->add_item(new BC_MenuItem("(DVD) | ffmpeg -f yuv4mpegpipe -i - -y -target dvd -ilme -ildct -hq -f mpeg2video %"));
	ffmpeg->add_item(new BC_MenuItem("(VCD) | ffmpeg -f yuv4mpegpipe -i - -y -target vcd -hq -f mpeg2video %"));

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


PipeCheckBox::PipeCheckBox(int x, int y, int value)
	: BC_CheckBox(x, y, value)
{
	this->textbox = 0;
}

int PipeCheckBox::handle_event() 
{
	if (textbox)
		if (get_value()) 
			textbox->enable();
		else 
			textbox->disable();
}


PipePreset::PipePreset(int x, int y, const char *title, BC_TextBox *textbox, BC_CheckBox *checkbox)
	: BC_PopupMenu(x, y, 150, title)
{
	this->pipe_textbox = textbox;
	this->pipe_checkbox =checkbox;
	this->title = title;
}

int PipePreset::handle_event()
{
	char *text = get_text();
	// NOTE: preset items must have a '|' before the actual command
	char *pipe = strchr(text, '|');
	// pipe + 1 to skip over the '|'
	if (pipe) pipe_textbox->update(pipe + 1);

	pipe_textbox->enable();
	pipe_checkbox->set_value(1, 1);
	
	// menuitem sets the title after selection but we reset it
	set_text(title);
}
