#include "fileyuv.h"
#include "asset.h"
#include "file.h"
#include "guicast.h"
#include "mwindow.h"
#include "defaults.h"
#include "vframe.h"
#include "edit.h"

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
}

FileYUV::~FileYUV()
{
	close_file();
}
	
int FileYUV::open_file(int should_read, int should_write)
{
	int result;

	if (should_read) {

		result = stream.open_read(asset->path);
		if (result) return result;
		
		// NOTE: no easy way to defer setting video_length
		asset->video_length = stream.frame_count;
		
		asset->width = stream.get_width();
		asset->height = stream.get_height();
		if (asset->width * asset->height <= 0) {
			printf("illegal frame size '%d x %d'\n", 
			       asset->width, asset->height);
			return 1;
		}
		
		asset->layers = 1;
		asset->video_data = 1;
		asset->audio_data = 0;
		
		asset->frame_rate = stream.get_frame_rate();
		asset->aspect_ratio = stream.get_aspect_ratio();

		return 0;
	}

	if (should_write) {
		result = stream.open_write(asset->path);
		if (result) return result;

		stream.set_width(asset->width);
		stream.set_height(asset->height);
		stream.set_frame_rate(asset->frame_rate);
		stream.set_aspect_ratio(asset->aspect_ratio);

		result = stream.write_header();
		if (result) return result;
		
		return 0;
	}
	
	// no action given
	return 1;
}

int FileYUV::close_file() {
	stream.close_fd();
	return 0;
}

// NOTE: set_video_position() called every time a frame is read
int FileYUV::set_video_position(int64_t frame_number) {
	return stream.seek_frame(frame_number);
}
	
int FileYUV::read_frame(VFrame *frame)
{
	int result;
	VFrame *input = frame;

	// short cut for direct copy routines
	if (frame->get_color_model() == BC_COMPRESSED) {
		long frame_size = (long) // w*h + w*h/4 + w*h/4
			(stream.get_height() * 	stream.get_width() * 1.5); 
		frame->allocate_compressed_data(frame_size);
		frame->set_compressed_size(frame_size);
		return stream.read_frame_raw(frame->get_data(), frame_size);
	}
	

	// process through a temp frame if necessary
	if (! cmodel_is_planar(frame->get_color_model()) ||
	    (frame->get_w() != stream.get_width()) ||
	    (frame->get_h() != stream.get_height())) {
		
		// make sure the temp is correct size and type
		if (temp && (temp->get_w() != asset->width ||
			     temp->get_h() != asset->height ||
			     temp->get_color_model() != BC_YUV420P)) {
			delete temp;
			temp = 0;
		}
		
		// create a correct temp frame if we don't have one
		if (temp == 0) {
			temp = new VFrame(0, stream.get_width(), 
					  stream.get_height(), 
					  BC_YUV420P);
		}
		
		input = temp;
	}

	uint8_t *yuv[3];
	yuv[0] = input->get_y();
	yuv[1] = input->get_u();
	yuv[2] = input->get_v();
	result = stream.read_frame(yuv);
	if (result) return result;

	// transfer from the temp frame to the real one
	if (input != frame) {
		cmodel_transfer(frame->get_rows(), 
				input->get_rows(),
				frame->get_y(),
				frame->get_u(),
				frame->get_v(),
				input->get_y(),
				input->get_u(),
				input->get_v(),
				0,
				0,
				asset->width,
				asset->height,
				0,
				0,
				frame->get_w(),
				frame->get_h(),
				input->get_color_model(), 
				frame->get_color_model(),
				0, 
				asset->width,
				frame->get_w());
	}

	
	return 0;
}

int FileYUV::write_frames(VFrame ***layers, int len)
{
	int result;
	uint8_t *yuv[3];

	// only one layer supported
	VFrame **frames = layers[0];
	VFrame *frame;

	for (int n = 0; n < len; n++) {

		frame = frames[n];

		// short cut for direct copy routines
		if (frame->get_color_model() == BC_COMPRESSED) {
			long frame_size = frame->get_compressed_size();
			return stream.write_frame_raw(frame->get_data(), 
						      frame_size);
		}

		// process through a temp frame only if necessary
		if (! cmodel_is_planar(frame->get_color_model()) ||
		    (frame->get_w() != stream.get_width()) ||
		    (frame->get_h() != stream.get_height())) {


			// make sure the temp is correct size and type
			if (temp && (temp->get_w() != asset->width ||
				     temp->get_h() != asset->height ||
				     temp->get_color_model() != BC_YUV420P)) {
				delete temp;
				temp = 0;
			}
			
			// create a correct temp frame if we don't have one
			if (temp == 0) {
				temp = new VFrame(0, stream.get_width(), 
						  stream.get_height(), 
						  BC_YUV420P);
			}
			
			cmodel_transfer(temp->get_rows(), 
					frame->get_rows(),
					temp->get_y(),
					temp->get_u(),
					temp->get_v(),
					frame->get_y(),
					frame->get_u(),
					frame->get_v(),
					0,
					0,
					asset->width,
					asset->height,
					0,
					0,
					asset->width,
					asset->height,
					frame->get_color_model(), 
					temp->get_color_model(),
					0, 
					frame->get_w(),
					temp->get_w());
			
			frame = temp;
		}

		uint8_t *yuv[3];
		yuv[0] = frame->get_y();
		yuv[1] = frame->get_u();
		yuv[2] = frame->get_v();
		result = stream.write_frame(yuv);
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
		// save the new path/pipe and update render window
		strcpy(asset->path, config->textbox->get_text());
		// update the textbox in the render window
		format->path_textbox->update(asset->path);
		// and add the new path/pipe to the defaults list
		config->recent->add_item(asset->path);
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

int FileYUV::can_copy_from(Edit *edit, int64_t position)
{
	if (position > stream.frame_count || position < 0) return 0;
	// NOTE: width and height already checked in file.C
	if (edit->asset->format == FILE_YUV) return 1;
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
	

YUVConfigVideo::YUVConfigVideo(BC_WindowBase *parent_window, Asset *asset, 
			       FormatTools *format)
	: BC_Window(PROGRAM_NAME ": YUV4MPEG Stream",
		    parent_window->get_abs_cursor_x(1),
		    parent_window->get_abs_cursor_y(1),
		    500,
		    100)
{
	this->parent_window = parent_window;
	this->asset = asset;
	this->format = format;
}

YUVConfigVideo::~YUVConfigVideo()
{
	delete textbox;
	delete mpeg2enc;
	delete recent;
}

int YUVConfigVideo::create_objects()
{
	int init_x = 10;
	int init_y = 10;
	
	int x = init_x;
	int y = init_y;

	add_subwindow(new BC_Title(x, y, _("Path or Pipe:")));
	x += 90;
	textbox = new YUVPathText(x, y, this);
	add_subwindow(textbox);

	x += 350;
	recent = new RecentList("YUV4MPEG_STREAM", format->mwindow->defaults,
				textbox, 10, x, y, 350, 100);
	add_subwindow(recent);
	recent->load_items();
	recent->create_objects();

	x = init_x;
	y += 25;

	add_subwindow(new BC_Title(x, y, _("Presets:")));
	x += 90;
	mpeg2enc = new YUVMpeg2Enc(x, y, this);
	add_subwindow(mpeg2enc);
	mpeg2enc->create_objects();	
	
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


YUVPathText::YUVPathText(int x, int y, YUVConfigVideo *config)
 : BC_TextBox(x, y, 350, 1, config->asset->path) 
{
	this->config = config;
}


YUVMpeg2Enc::YUVMpeg2Enc(int x, int y, YUVConfigVideo *config)
	: BC_PopupMenu(x, y, 150, "mpeg2enc")
{
	this->config = config;
	set_tooltip(_("Preset pipes to mpeg2enc encoder"));
}

int YUVMpeg2Enc::handle_event() {
	char *text = get_text();
	char *pipe = strchr(text, '|');
	if (pipe) config->textbox->update(pipe);
	// menuitem sets the title after selection but we reset it
	set_text("mpeg2enc");
}

int YUVMpeg2Enc::create_objects() {
	add_item(new BC_MenuItem("(DVD) | mpeg2enc -f 8 -o dvd.m2v"));
	add_item(new BC_MenuItem("(VCD) | mpeg2enc -f 2 -o vcd.m2v"));
}

	
