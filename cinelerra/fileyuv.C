
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
#include "bcsignals.h"
#include "edit.h"
#include "file.h"
#include "formatpresets.h"
#include "guicast.h"
#include "interlacemodes.h"
#include "language.h"
#include "theme.h"
#include "mainerror.h"
#include "mwindow.h"
#include "theme.h"
#include "vframe.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

extern Theme *theme_global;

FileYUV::FileYUV(Asset *asset, File *file)
 : FileBase(asset, file)
{
	if (asset->format == FILE_UNKNOWN) asset->format = FILE_YUV;
	asset->byte_order = 0; // FUTURE: is this always correct?
	temp = 0;
	stream = new YUVStream();
	current_frame = -1;
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
			errormsg("Illegal frame size '%d x %d'\n", asset->width, asset->height);
			return 1;
		}

		asset->layers = 1;
		asset->video_data = 1;
		asset->audio_data = 0;

		asset->frame_rate = stream->get_frame_rate();
		asset->aspect_ratio = stream->get_aspect_ratio();
		asset->interlace_mode = stream->get_interlace();
		asset->file_length = stream->get_file_length();
		return 0;
	}

	if (should_write)
	{
		if (asset->use_pipe)
			result = stream->open_write(asset->path, asset->pipe);
		else
			result = stream->open_write(asset->path, NULL);

		if (result) return result;

		// not sure if we're supposed to send interlace info with each set of frames, (wouldn't know howto!)??
		stream->set_interlace(asset->interlace_mode);
		stream->set_width(asset->width);
		stream->set_height(asset->height);
		stream->set_frame_rate(asset->frame_rate);
		stream->set_aspect_ratio(asset->aspect_ratio);

		result = stream->write_header();
		return result;
	}

	// no action given
	return 1;
}

void FileYUV::close_file()
{
	stream->close_fd();
}

int FileYUV::read_frame(VFrame *frame)
{
	int result;
	uint8_t *yuv[3];
	framenum pos = (frame->get_source_pts() + FRAME_OVERLAP) * asset->frame_rate;

	if(pos != current_frame)
		stream->seek_frame(pos);
	current_frame = pos;
	yuv[0] = frame->get_y();
	yuv[1] = frame->get_u();
	yuv[2] = frame->get_v();
	result = stream->read_frame(yuv);

	frame->set_source_pts((ptstime)current_frame / asset->frame_rate);
	frame->set_duration((ptstime)1 / asset->frame_rate);
	frame->set_frame_number(current_frame);
	current_frame++;

	return result;
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

		// process through a temp frame only if necessary
		if (!ColorModels::is_planar(frame->get_color_model()) ||
			(frame->get_w() != stream->get_width()) ||
			(frame->get_h() != stream->get_height())) 
		{
			ensure_temp(asset->width, asset->height);
			temp->transfer_from(frame);
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
	int options,
	FormatTools *format)
{
	if (!(options & SUPPORTS_VIDEO)) return;

	YUVConfigVideo *config = new YUVConfigVideo(parent_window, asset, format);
	format_window = config;

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
		const char *prefix = ContainerSelection::container_prefix(asset->format);
		config->path_recent->add_item(prefix, asset->path);
		config->pipe_recent->add_item(prefix, asset->pipe);
	}
	delete config;
}

int FileYUV::check_sig(Asset *asset)
{
	char temp[16];
	int l;
	FILE *f = fopen(asset->path, "rb");

	// check for starting with "YUV4MPEG2"
	l = fread(&temp, 9, 1, f);
	fclose(f);
	if (l && strncmp(temp, "YUV4MPEG2", 9) == 0) return 1;

	return 0;
}

int FileYUV::supports(int format)
{
	if(format == FILE_YUV)
		return SUPPORTS_VIDEO;
	return 0;
}

int FileYUV::get_best_colormodel(Asset *asset, int driver) 
{
	// FUTURE: is there a reason to try to accept anything else?
	return BC_YUV420P;
}

int FileYUV::colormodel_supported(int color_model) 
{
	return BC_YUV420P;
}

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
	: BC_Window(MWindow::create_title(N_("YUV4MPEG Stream")),
		parent_window->get_abs_cursor_x(1),
		parent_window->get_abs_cursor_y(1),
		500,
		240)
{
	BC_Title *bt;
	int init_x = 10;
	int init_y = 10;

	int x = init_x;
	int y = init_y;

	set_icon(theme_global->get_image("mwindow_icon"));
	this->asset = asset;
	this->format = format;
	this->defaults = format->mwindow->defaults;
	add_subwindow(new BC_Title(init_x, y, _("Output Path:")));
	add_subwindow(path_textbox = new BC_TextBox(init_x + 100, y, 350, 1, asset->path));
	add_subwindow(path_recent = new BC_RecentList("PATH", defaults, path_textbox, 10, init_x + 450, y, path_textbox->get_w(), 100));
	path_recent->load_items(ContainerSelection::container_prefix(asset->format));

	x = init_x;
	y += 30;

	add_subwindow(bt = new BC_Title(init_x, y, _("Use Pipe:")));
	add_subwindow(pipe_checkbox = new PipeCheckBox(init_x + bt->get_w(), y, asset->use_pipe));
	add_subwindow(pipe_textbox = new BC_TextBox(init_x + 100, y, 350, 1, asset->pipe));
	add_subwindow(pipe_recent = new BC_RecentList("PIPE", defaults, pipe_textbox, 10, init_x + 450, y, pipe_textbox->get_w(), 100));
	pipe_recent->load_items(ContainerSelection::container_prefix(asset->format));

	pipe_checkbox->textbox = pipe_textbox;
	if (!asset->use_pipe) pipe_textbox->disable();

	x = init_x;
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Stream Header:"), MEDIUMFONT, RED));

	x = init_x + 20;
	y += 30;
	add_subwindow(bt = new BC_Title(x, y, _("Interlacing:")));
	add_subwindow(new BC_Title(x + bt->get_w() + 5, y,
		AInterlaceModeSelection::name(asset->interlace_mode),
		MEDIUMFONT, YELLOW));

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
}

void YUVConfigVideo::close_event()
{
	set_done(0);
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
	this->pipe_checkbox = checkbox;
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
