
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

#include "asset.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "channel.h"
#include "channeldb.h"
#include "clip.h"
#include "bchash.h"
#include "edlsession.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "libdv.h"
#include "libmjpeg.h"
#include "mwindow.h"
#include "picon_png.h"
#include "picture.h"
#include "pluginvclient.h"
#include "recordconfig.h"
#include "transportque.inc"
#include "vframe.h"
#include "videodevice.h"
#include "videodevice.inc"

#include <string.h>
#include <stdint.h>

#define HISTORY_FRAMES 30
class LiveVideo;
class LiveVideoWindow;


class LiveVideoConfig
{
public:
	LiveVideoConfig();
	void copy_from(LiveVideoConfig &src);
	int equivalent(LiveVideoConfig &src);
	void interpolate(LiveVideoConfig &prev, 
		LiveVideoConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	int channel;
};


// Without access to the video device, the ChannelPicker can't
// do any of the things it was designed to.  Instead, just provide
// a list of channels.
class LiveChannelList : public BC_ListBox
{
public:
	LiveChannelList(LiveVideo *plugin, 
		LiveVideoWindow *gui, 
		int x, 
		int y,
		int w,
		int h);
	int handle_event();
	LiveVideo *plugin;
	LiveVideoWindow *gui;
};

class LiveChannelSelect : public BC_Button
{
public:
	LiveChannelSelect(LiveVideo *plugin, 
		LiveVideoWindow *gui, 
		int x, 
		int y);
	int handle_event();
	LiveVideo *plugin;
	LiveVideoWindow *gui;
};


class LiveVideoWindow : public BC_Window
{
public:
	LiveVideoWindow(LiveVideo *plugin, int x, int y);
	~LiveVideoWindow();

	void create_objects();
	int close_event();
	int resize_event(int w, int h);

	ArrayList<BC_ListBoxItem*> channel_list;
	BC_Title *title;
	LiveChannelList *list;
	LiveChannelSelect *select;
	LiveVideo *plugin;
};


PLUGIN_THREAD_HEADER(LiveVideo, LiveVideoThread, LiveVideoWindow)



class LiveVideo : public PluginVClient
{
public:
	LiveVideo(PluginServer *server);
	~LiveVideo();


	PLUGIN_CLASS_MEMBERS(LiveVideoConfig, LiveVideoThread);

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int is_multichannel();
	int is_synthesis();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_stop();

	ChannelDB *channeldb;
	VideoDevice *vdevice;
// Colormodel the device generates
	int input_cmodel;
// Temporary for colormodel conversion
	VFrame *temp;
// What configuration parameters the device supports
	Channel master_channel;
	PictureConfig *picture;
	BC_Hash *picture_defaults;
	int prev_channel;
	int w, h;
// Decompressors for different video drivers
	dv_t *dv;
	mjpeg_t *mjpeg;
};












LiveVideoConfig::LiveVideoConfig()
{
	channel = 0;
}

void LiveVideoConfig::copy_from(LiveVideoConfig &src)
{
	this->channel = src.channel;
}

int LiveVideoConfig::equivalent(LiveVideoConfig &src)
{
	return (this->channel == src.channel);
}

void LiveVideoConfig::interpolate(LiveVideoConfig &prev, 
	LiveVideoConfig &next, 
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	this->channel = prev.channel;
}





LiveVideoWindow::LiveVideoWindow(LiveVideo *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	plugin->w, 
	plugin->h, 
	100, 
	100, 
	1, 
	0,
	1)
{
	this->plugin = plugin;
}

LiveVideoWindow::~LiveVideoWindow()
{
	channel_list.remove_all_objects();
}

void LiveVideoWindow::create_objects()
{
	int x = 10, y = 10;

	for(int i = 0; i < plugin->channeldb->size(); i++)
	{
		BC_ListBoxItem *current;
		channel_list.append(current = 
			new BC_ListBoxItem(plugin->channeldb->get(i)->title));
		if(i == plugin->config.channel) current->set_selected(1);
	}

	add_subwindow(title = new BC_Title(x, y, _("Channels:")));
	y += title->get_h() + 5;
	add_subwindow(list = new LiveChannelList(plugin, 
		this, 
		x, 
		y,
		get_w() - x - 10,
		get_h() - y - BC_OKButton::calculate_h() - 10 - 10));
	y += list->get_h() + 10;
	add_subwindow(select = new LiveChannelSelect(plugin, 
		this, 
		x, 
		y));
	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(LiveVideoWindow)

int LiveVideoWindow::resize_event(int w, int h)
{
	int list_bottom = get_h() - list->get_y() - list->get_h();
	int list_side = get_w() - list->get_x() - list->get_w();
	int select_top = get_h() - select->get_y();

	title->reposition_window(title->get_x(), title->get_y());

	list->reposition_window(list->get_x(),
		list->get_y(),
		w - list->get_x() - list_side,
		h - list->get_y() - list_bottom);
	select->reposition_window(select->get_x(),
		h - select_top);
	plugin->w = w;
	plugin->h = h;
	return 1;
}




LiveChannelList::LiveChannelList(LiveVideo *plugin, 
	LiveVideoWindow *gui, 
	int x, 
	int y,
	int w,
	int h)
 : BC_ListBox(x, 
	y, 
	w, 
	h,
	LISTBOX_TEXT,                   // Display text list or icons
	&gui->channel_list) // Each column has an ArrayList of BC_ListBoxItems.
{
	this->plugin = plugin;
	this->gui = gui;
}

int LiveChannelList::handle_event()
{
	plugin->config.channel = get_selection_number(0, 0);
	plugin->send_configure_change();
	return 1;
}


LiveChannelSelect::LiveChannelSelect(LiveVideo *plugin, 
	LiveVideoWindow *gui, 
	int x, 
	int y)
 :  BC_Button(x, y, 
 	BC_WindowBase::get_resources()->ok_images)
{
	this->plugin = plugin;
	this->gui = gui;
}

int LiveChannelSelect::handle_event()
{
	plugin->config.channel = gui->list->get_selection_number(0, 0);
	plugin->send_configure_change();
	return 1;
}















PLUGIN_THREAD_OBJECT(LiveVideo, LiveVideoThread, LiveVideoWindow)










REGISTER_PLUGIN(LiveVideo)






LiveVideo::LiveVideo(PluginServer *server)
 : PluginVClient(server)
{
	vdevice = 0;
	temp = 0;
	channeldb = new ChannelDB;
	w = 320;
	h = 640;
	prev_channel = 0;
	dv = 0;
	mjpeg = 0;
	picture = 0;
	picture_defaults = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}


LiveVideo::~LiveVideo()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(vdevice)
	{
		vdevice->interrupt_crash();
		vdevice->close_all();
		delete vdevice;
	}

	delete channeldb;
	delete temp;
	if(dv) dv_delete(dv);
	if(mjpeg) mjpeg_delete(mjpeg);
	delete picture;
	delete picture_defaults;
}



int LiveVideo::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();
//printf("LiveVideo::process_buffer 10 start_position=%lld buffer_size=%d size=%d\n", 
//start_position, get_buffer_size(), size);

	EDLSession *session = PluginClient::get_edlsession();
	if(!vdevice)
	{
		if(session)
		{
			vdevice = new VideoDevice;
			vdevice->open_input(session->vconfig_in, 
				0, 
				0,
				1.0,
				frame_rate);

// The color model depends on the asset configured by the user for recording.
// Unfortunately, get_best_colormodel returns the best colormodel for displaying
// on the record monitor, not the colormodel supported by the device.
// Some devices can read directly to the best colormodel and some can't.
			switch(session->vconfig_in->driver)
			{
				case CAPTURE_FIREWIRE:
				case CAPTURE_IEC61883:
				case CAPTURE_BUZ:
				case VIDEO4LINUX2JPEG:
					input_cmodel = BC_COMPRESSED;
					break;
				default:
					input_cmodel = vdevice->get_best_colormodel(session->recording_format);
					break;
			}


// Load the picture config from the main defaults file.
			if(!picture_defaults)
			{
				char path[BCTEXTLEN];
				MWindow::create_defaults_path(path);
				picture_defaults = new BC_Hash(path);
				picture_defaults->load();
			}

			if(!picture)
			{
				picture = new PictureConfig(picture_defaults);
			}

// Picture must have usage from driver before it can load defaults.
			master_channel.copy_usage(vdevice->channel);
			picture->copy_usage(vdevice->picture);
			picture->load_defaults();

// Need to load picture defaults but this requires MWindow.
			vdevice->set_picture(picture);
			vdevice->set_channel(channeldb->get(config.channel));
		}
		prev_channel = config.channel;
	}

	if(session && vdevice)
	{
// Update channel
		if(prev_channel != config.channel)
		{
			prev_channel = config.channel;
			vdevice->set_picture(picture);
			vdevice->set_channel(channeldb->get(config.channel));
		}

	
		VFrame *input = frame;
		if(input_cmodel != frame->get_color_model() ||
			session->vconfig_in->w != frame->get_w() ||
			session->vconfig_in->h != frame->get_h())
		{
			if(!temp)
			{
				temp = new VFrame(0, 
					session->vconfig_in->w,
					session->vconfig_in->h,
					input_cmodel);
			}
			input = temp;
		}
		vdevice->read_buffer(input);
		if(input != frame)
		{
			if(input->get_color_model() != BC_COMPRESSED)
			{
SET_TRACE
				int w = MIN(session->vconfig_in->w, frame->get_w());
				int h = MIN(session->vconfig_in->h, frame->get_h());
				cmodel_transfer(frame->get_rows(), /* Leave NULL if non existent */
					input->get_rows(),
					frame->get_y(), /* Leave NULL if non existent */
					frame->get_u(),
					frame->get_v(),
					input->get_y(), /* Leave NULL if non existent */
					input->get_u(),
					input->get_v(),
					0,        /* Dimensions to capture from input frame */
					0, 
					w, 
					h,
					0,       /* Dimensions to project on output frame */
					0, 
					w, 
					h,
					input->get_color_model(), 
					frame->get_color_model(),
					0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
					input->get_bytes_per_line(),       /* For planar use the luma rowspan */
					frame->get_bytes_per_line());     /* For planar use the luma rowspan */
				frame->set_opengl_state(VFrame::RAM);
SET_TRACE
			}
			else
			{
				switch(session->vconfig_in->driver)
				{
					case CAPTURE_FIREWIRE:
					case CAPTURE_IEC61883:
// Decompress a DV frame from the driver
						if(!dv)
							dv = dv_new();
						dv_read_video(((dv_t*)dv), 
							frame->get_rows(), 
							input->get_data(), 
							input->get_compressed_size(),
							frame->get_color_model());
						frame->set_opengl_state(VFrame::RAM);
SET_TRACE
						break;

					case CAPTURE_BUZ:
					case VIDEO4LINUX2JPEG:
						if(!mjpeg)
							mjpeg = mjpeg_new(w, 
								h, 
								2);  // fields
						mjpeg_decompress(mjpeg, 
							input->get_data(), 
							input->get_compressed_size(), 
							input->get_field2_offset(), 
							frame->get_rows(), 
							frame->get_y(), 
							frame->get_u(), 
							frame->get_v(),
							frame->get_color_model(),
							get_project_smp() + 1);
						break;
				}
			}
		}
	}

	return 0;
}

void LiveVideo::render_stop()
{
	if(vdevice)
	{
		vdevice->interrupt_crash();
		vdevice->close_all();
		delete vdevice;
		vdevice = 0;
	}
	delete picture_defaults;
	picture_defaults = 0;
	delete picture;
	picture = 0;
}


char* LiveVideo::plugin_title() { return N_("Live Video"); }
int LiveVideo::is_realtime() { return 1; }
int LiveVideo::is_multichannel() { return 0; }
int LiveVideo::is_synthesis() { return 1; }


NEW_PICON_MACRO(LiveVideo) 

SHOW_GUI_MACRO(LiveVideo, LiveVideoThread)

RAISE_WINDOW_MACRO(LiveVideo)

SET_STRING_MACRO(LiveVideo);

LOAD_CONFIGURATION_MACRO(LiveVideo, LiveVideoConfig)

int LiveVideo::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%slivevideo.rc", BCASTDIR);
// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

// Load channel table
	EDLSession *session = PluginClient::get_edlsession();
	if(session)
		VideoDevice::load_channeldb(channeldb, session->vconfig_in);
	config.channel = defaults->get("CHANNEL", 0);
	w = defaults->get("W", w);
	h = defaults->get("H", h);
	return 0;
}

int LiveVideo::save_defaults()
{
	defaults->update("CHANNEL", config.channel);
	defaults->update("W", w);
	defaults->update("H", h);
	defaults->save();
	return 0;
}

void LiveVideo::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("LIVEVIDEO");
	output.tag.set_property("CHANNEL", config.channel);
	output.append_tag();
	output.tag.set_title("/LIVEVIDEO");
	output.append_tag();
	output.terminate_string();
}

void LiveVideo::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("LIVEVIDEO"))
			{
				config.channel = input.tag.get_property("CHANNEL", config.channel);
			}
		}
	}
}

void LiveVideo::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("LiveVideo::update_gui");
			thread->window->list->set_selected(&thread->window->channel_list, 
				config.channel, 
				1);
			thread->window->list->draw_items(1);
			thread->window->unlock_window();
		}
	}
}





