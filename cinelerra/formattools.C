// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "bcresources.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "formattools.h"
#include "language.h"
#include "mwindow.h"
#include "paramlistwindow.h"
#include "preferences.h"
#include "selection.h"
#include "theme.h"
#include "videodevice.inc"
#include <string.h>
#include "pipe.h"

const struct container_type ContainerSelection::media_containers[] =
{
	{ N_("AC3"), FILE_AC3, "AC3", "ac3" },
	{ N_("Apple/SGI AIFF"), FILE_AIFF, "AIFF", "aif" },
	{ N_("Sun/NeXT AU"), FILE_AU, "AU", "au" },
	{ N_("JPEG"), FILE_JPEG, "JPEG", "jpg" },
	{ N_("JPEG Sequence"), FILE_JPEG_LIST, "JPEG_LIST", "list" },
	{ N_("Microsoft AVI"), FILE_AVI, "AVI", "avi" },
	{ N_("EXR"), FILE_EXR, "EXR", "exr" },
	{ N_("YUV4MPEG Stream"), FILE_YUV, "YUV", "m2v" },
	{ N_("Microsoft WAV"), FILE_WAV, "WAV", "wav" },
// MOV/MP4 group
	{ N_("QuickTime / MOV"), FILE_MOV, "MOV", "mov" },
	{ N_("3GPP"), FILE_3GP, "3GP", "3gp" },
	{ N_("MPEG-4"), FILE_MP4, "MP4", "mp4" },
	{ N_("PSP MP4"), FILE_PSP, "PSP", "psp" },
	{ N_("3GPP2"), FILE_3GPP2, "3GPP2", "3g2" },
	{ N_("iPod H.264 MP4"), FILE_IPOD, "IPOD", "m4v" },
	{ N_("ISMV/ISMA"), FILE_ISMV, "ISMV", "ismv" },
	{ N_("F4V Adobe Flash"), FILE_F4V, "F4V", "f4v" },
//
	{ N_("Raw DV"), FILE_RAWDV, "RAWDV", "dv" },
	{ N_("OGG Theora/Vorbis"), FILE_OGG, "OGG", "ogg" },
	{ N_("Raw PCM"), FILE_PCM, "PCM", "pcm" },
	{ N_("PNG"), FILE_PNG, "PNG", "png" },
	{ N_("PNG Sequence"), FILE_PNG_LIST, "PNG_LIST", "list" },
	{ N_("TGA"), FILE_TGA, "TGA", "tga" },
	{ N_("TGA Sequence"), FILE_TGA_LIST, "TGA_LIST", "list" },
	{ N_("TIFF"), FILE_TIFF, "TIFF", "tif" },
	{ N_("TIFF Sequence"), FILE_TIFF_LIST, "TIFF_LIST", "list" },
	{ N_("SVG"), FILE_SVG, "SVG", "svg" },
	{ N_("MPEG2 PS"), FILE_MPEG, "MPEG", "mpg" },
	{ N_("MPEGTS"), FILE_MPEGTS, "MPEGTS", "ts" },
	{ N_("MPEG 2/3 audio"), FILE_MP3, "MP3", "mp3" },
	{ N_("Raw Flac"), FILE_FLAC, "FLAC", "flac" },
	{ N_("Image2"), FILE_IMAGE, "IMG2", "img" },
	{ N_("Unknown sound"), FILE_SND, "SND", "snd"},
	{ N_("Material Exchange Format"), FILE_MXF, "MXF", "mxf" },
	{ N_("Matroska"), FILE_MKV, "MKV", "mkv" },
	{ N_("WebM"), FILE_WEBM, "WEBM", "webm" },
	{ 0, 0 }
};

#define NUM_MEDIA_CONTAINERS (sizeof(ContainerSelection::media_containers) / sizeof(struct container_type) - 1)

int FormatPopup::brender_menu[] = { FILE_JPEG_LIST,
	FILE_PNG_LIST, FILE_TIFF_LIST };
int FormatPopup::frender_menu1[] = { FILE_AC3 , FILE_AIFF, FILE_AU,
	FILE_JPEG,
	FILE_AVI,
	FILE_YUV, FILE_WAV,
	FILE_MOV,
	FILE_MP3,
	FILE_RAWDV,
	FILE_MXF,
	FILE_MKV,
	FILE_OGG,
	FILE_PNG,  FILE_TGA,
	FILE_TIFF,
	FILE_FLAC, FILE_MPEG,
	FILE_MP4,
	FILE_WEBM
};

int FormatPopup::frender_menu2[] = {
	FILE_JPEG_LIST,
	FILE_PNG_LIST,
	FILE_TGA_LIST,
	FILE_TIFF_LIST,
	FILE_MPEGTS,
	FILE_PSP,
	FILE_3GP,
	FILE_3GPP2,
	FILE_IPOD,
	FILE_ISMV,
	FILE_F4V
};

FormatTools::FormatTools(BC_WindowBase *window,
	Asset *asset,
	int &init_x,
	int &init_y,
	int support,
	int checkbox,
	int details,
	int *strategy,
	int brender,
	int horizontal_layout)
{
	int x = init_x;
	int y = init_y;
	int ylev = init_y;
	int text_x = init_x;

	this->window = window;
	this->asset = asset;
	aparams_button = 0;
	vparams_button = 0;
	fparams_button = 0;
	aparams_thread = 0;
	vparams_thread = 0;
	fparams_thread = 0;
	audio_switch = 0;
	video_switch = 0;
	path_textbox = 0;
	path_button = 0;
	path_recent = 0;
	w = 0;
	this->use_brender = brender;
	this->do_audio = support & SUPPORTS_AUDIO;
	this->do_video = support & SUPPORTS_VIDEO;
	this->support = support;
	this->checkbox = checkbox;
	this->details = details;
	this->strategy = strategy;

	window->add_subwindow(path_textbox = new FormatPathText(x, y, this));
	x += 305;
	path_recent = new BC_RecentList("PATH", mwindow_global->defaults,
		path_textbox, 10, x, y, 300, 100);
	window->add_subwindow(path_recent);
	path_recent->load_items(ContainerSelection::container_prefix(asset->format));

	x += 18;
	window->add_subwindow(path_button = new BrowseButton(
		mwindow_global,
		window,
		path_textbox,
		x,
		y,
		asset->path,
		_("Output to file"),
		_("Select a file to write to:"),
		0));

// Set w for user.
	w = x + path_button->get_w() + 5;
	x -= 305;

	y += 35;

	window->add_subwindow(format_title = new BC_Title(x, y, _("File Format:")));
	x += 90;

	format_popup = new FormatPopup(window, x, y, &asset->format, this, use_brender);
	x = init_x;
	y += format_popup->get_h() + 10;
	if(do_audio)
	{
		window->add_subwindow(audio_title = new BC_Title(x, y, _("Audio:"), LARGEFONT,  BC_WindowBase::get_resources()->audiovideo_color));
		x += 80;
		window->add_subwindow(aparams_button = new FormatAParams(this, x, y));
		x += aparams_button->get_w() + 10;
		if(checkbox & SUPPORTS_AUDIO)
		{
			window->add_subwindow(audio_switch = new FormatAudio(x, y,
				this, asset->stream_count(STRDSC_AUDIO)));
			text_x = x + audio_switch->text_x;
		}
		x = init_x;
		ylev = y;
		y += aparams_button->get_h() + 5;

		aparams_thread = new FormatAThread(this);
	}

	if(do_video)
	{
		if(horizontal_layout && do_audio){
			x += 370;
			y = ylev;
		}
		window->add_subwindow(video_title = new BC_Title(x, y, _("Video:"), LARGEFONT,  BC_WindowBase::get_resources()->audiovideo_color));
		x += 80;
		if(details & SUPPORTS_VIDEO)
		{
			window->add_subwindow(vparams_button = new FormatVParams(this, x, y));
			x += vparams_button->get_w() + 10;
		}

		if(checkbox & SUPPORTS_VIDEO)
		{
			window->add_subwindow(video_switch = new FormatVideo(x, y,
				this, asset->stream_count(STRDSC_VIDEO)));
			text_x = x + video_switch->text_x;
			y += video_switch->get_h();
		}
		else
		{
			y += vparams_button->get_h();
		}

		y += 10;
		vparams_thread = new FormatVThread(this);
	}

	if(support & SUPPORTS_LIBPARA)
	{
		window->add_subwindow(fparams_button = new FormatFParams(this,
			init_x + 80, y));
		window->add_subwindow(new BC_Title(text_x, y, _("Format options")));
		y += fparams_button->get_h() + 10;
		asset->get_format_params(support);
		fparams_thread = new ParamlistThread(&asset->encoder_parameters[ASSET_FMT_IX],
			_("Format options"), mwindow_global->get_window_icon());
	}
	enable_supported();
	x = init_x;
	if(strategy)
	{
		window->add_subwindow(multiple_files = new FormatMultiple(x, y, strategy));
		y += multiple_files->get_h() + 10;
	}

	init_y = y;
}

FormatTools::~FormatTools()
{
	delete path_recent;
	delete path_button;
	delete path_textbox;
	delete format_popup;

	delete aparams_thread;
	delete vparams_thread;
	if(fparams_thread)
	{
		fparams_thread->cancel_window();
		if(!fparams_thread->win_result)
			asset->set_format_params();
		delete fparams_thread;
	}
}

void FormatTools::enable_supported()
{
	int filesup =  File::supports(asset->format);

	if(aparams_button)
	{
		if((filesup & SUPPORTS_AUDIO) == 0)
		{
			aparams_button->disable();
			asset->remove_stream_type(STRDSC_AUDIO);

			if(audio_switch)
			{
				audio_switch->set_value(0);
				audio_switch->disable();
			}
		}
		else
		{
			aparams_button->enable();
			if(audio_switch)
			{
				audio_switch->enable();
				if((filesup & ~SUPPORTS_AUDIO) == 0)
				{
					asset->create_render_stream(STRDSC_AUDIO);
					audio_switch->set_value(1);
				}
			}
		}
	}
	if(vparams_button)
	{
		if((filesup & SUPPORTS_VIDEO) == 0)
		{
			vparams_button->disable();
			asset->remove_stream_type(STRDSC_VIDEO);
			if(video_switch)
			{
				video_switch->set_value(0);
				video_switch->disable();
				asset->single_image = 0;
			}
		}
		else
		{
			vparams_button->enable();
			if(video_switch)
			{
				video_switch->enable();
				if((filesup & ~(SUPPORTS_VIDEO|SUPPORTS_STILL)) == 0)
				{
					asset->create_render_stream(STRDSC_VIDEO);
					video_switch->set_value(1);
				}
			}
			asset->single_image = filesup & SUPPORTS_STILL;
		}
	}
	if(fparams_button)
	{
		if((filesup & SUPPORTS_LIBPARA) && asset->encoder_parameters[ASSET_FMT_IX])
			fparams_button->enable();
		else
			fparams_button->disable();
	}
}

Asset* FormatTools::get_asset()
{
	return asset;
}

void FormatTools::update_extension()
{
	const char *extension = ContainerSelection::container_extension(asset->format);
	if(extension)
	{
		char *ptr = strrchr(asset->path, '.');
		if(!ptr)
		{
			ptr = asset->path + strlen(asset->path);
			*ptr = '.';
		}
		ptr++;
		strcpy(ptr, extension);

		int character1 = ptr - asset->path;
		int character2 = ptr - asset->path + strlen(extension);
		*(asset->path + character2) = 0;
		if(path_textbox) 
		{
			path_textbox->update(asset->path);
			path_textbox->set_selection(character1, character2, character2);
		}
	}
}

void FormatTools::update(Asset *asset, int *strategy)
{
	this->asset = asset;
	this->strategy = strategy;

	if(path_textbox) 
		path_textbox->update(asset->path);

	format_popup->update(asset->format);

	if(do_audio && audio_switch)
		audio_switch->update(asset->stream_count(STRDSC_AUDIO));

	if(do_video && video_switch)
		video_switch->update(asset->stream_count(STRDSC_VIDEO));

	if(strategy)
		multiple_files->update(strategy);
	close_format_windows();
}

void FormatTools::close_format_windows()
{
	if(aparams_thread)
		aparams_thread->file->close_window();
	if(vparams_thread)
		vparams_thread->file->close_window();
}

int FormatTools::get_w()
{
	return w;
}

void FormatTools::reposition_window(int &init_x, int &init_y)
{
	int x = init_x;
	int y = init_y;

	if(path_textbox) 
	{
		path_textbox->reposition_window(x, y);
		x += 305;
		path_button->reposition_window(x, y);
		x -= 305;
		y += 35;
	}

	format_title->reposition_window(x, y);
	x += 90;
	format_popup->reposition_window(x, y);

	x = init_x;
	y += format_popup->get_h() + 10;

	if(do_audio)
	{
		audio_title->reposition_window(x, y);
		x += 80;
		aparams_button->reposition_window(x, y);
		x += aparams_button->get_w() + 10;
		if(checkbox & SUPPORTS_AUDIO) audio_switch->reposition_window(x, y);

		x = init_x;
		y += aparams_button->get_h() + 5;
	}

	if(do_video)
	{
		video_title->reposition_window(x, y);
		x += 80;
		if(details & SUPPORTS_VIDEO)
		{
			vparams_button->reposition_window(x, y);
			x += vparams_button->get_w() + 10;
		}

		if(checkbox & SUPPORTS_VIDEO)
		{
			video_switch->reposition_window(x, y);
			y += video_switch->get_h();
		}
		else
		{
			y += vparams_button->get_h();
		}

		y += 10;
		x = init_x;
	}

	if(strategy)
	{
		multiple_files->reposition_window(x, y);
		y += multiple_files->get_h() + 10;
	}

	init_y = y;
}

void FormatTools::set_audio_options()
{
	if(!aparams_thread->running())
		aparams_thread->start();
	else
		aparams_thread->file->raise_window();
}

void FormatTools::set_video_options()
{
	if(!vparams_thread->running())
		vparams_thread->start();
	else
		vparams_thread->file->raise_window();
}

void FormatTools::set_format_options()
{
	fparams_thread->show_window();
}

void FormatTools::format_changed()
{
	if(!use_brender)
		update_extension();
	close_format_windows();
	if(path_recent)
		path_recent->load_items(ContainerSelection::container_prefix(asset->format));
	asset->format_changed();
	asset->get_format_params(support);
	enable_supported();
}

FormatAParams::FormatAParams(FormatTools *format, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("wrench"))
{
	this->format = format;
	set_tooltip(_("Configure audio compression"));
}

int FormatAParams::handle_event() 
{
	format->set_audio_options();
	return 1;
}


FormatVParams::FormatVParams(FormatTools *format, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("wrench"))
{ 
	this->format = format; 
	set_tooltip(_("Configure video compression"));
}

int FormatVParams::handle_event()
{ 
	format->set_video_options();
	return 1;
}

FormatFParams::FormatFParams(FormatTools *format, int x, int y)
 : BC_Button(x, y, theme_global->get_image_set("wrench"))
{
	this->format = format;
	set_tooltip(_("Configure format"));
}

int FormatFParams::handle_event()
{
	format->set_format_options();
	return 1;
}

FormatAThread::FormatAThread(FormatTools *format)
 : Thread()
{ 
	this->format = format;
	file = new File;
}

FormatAThread::~FormatAThread()
{
	delete file;
}

void FormatAThread::run()
{
	file->get_options(format, SUPPORTS_AUDIO);
}


FormatVThread::FormatVThread(FormatTools *format)
 : Thread()
{
	this->format = format;
	file = new File;
}

FormatVThread::~FormatVThread() 
{
	delete file;
}

void FormatVThread::run()
{
	file->get_options(format, SUPPORTS_VIDEO);
}


FormatPathText::FormatPathText(int x, int y, FormatTools *format)
 : BC_TextBox(x, y, 300, 1, format->asset->path) 
{
	this->format = format; 
}

int FormatPathText::handle_event() 
{
	strcpy(format->asset->path, get_text());
	return format->handle_event();
}


FormatAudio::FormatAudio(int x, int y, FormatTools *format, int value)
 : BC_CheckBox(x, y, value, _("Render audio tracks"))
{
	this->format = format;
}

int FormatAudio::handle_event()
{
	if(get_value())
		format->asset->create_render_stream(STRDSC_AUDIO);
	else
		format->asset->remove_stream_type(STRDSC_AUDIO);
	return 1;
}


FormatVideo::FormatVideo(int x, int y, FormatTools *format, int value)
 : BC_CheckBox(x, y, value, _("Render video tracks"))
{
	this->format = format; 
}

int FormatVideo::handle_event()
{
	if(get_value())
		format->asset->create_render_stream(STRDSC_VIDEO);
	else
		format->asset->remove_stream_type(STRDSC_VIDEO);
	return 1;
}


FormatToTracks::FormatToTracks(int x, int y, int *output)
 : BC_CheckBox(x, y, *output, _("Overwrite project with output"))
{ 
	this->output = output; 
}

int FormatToTracks::handle_event()
{
	*output = get_value();
	return 1;
}


FormatMultiple::FormatMultiple(int x, int y, int *output)
 : BC_CheckBox(x, y, *output & RENDER_FILE_PER_LABEL,
	_("Create new file at each label"))
{ 
	this->output = output;
}

int FormatMultiple::handle_event()
{
	if(get_value())
		*output = RENDER_FILE_PER_LABEL;
	else
		*output = 0;
	return 1;
}

void FormatMultiple::update(int *output)
{
	this->output = output;
	set_value(*output & RENDER_FILE_PER_LABEL);
}


FormatPopup::FormatPopup(BC_WindowBase *parent,
	int x,
	int y,
	int *output,
	FormatTools *tools,
	int use_brender)
{
	int *menu;
	int length;

	if(use_brender)
	{
		menu = brender_menu;
		length = sizeof(brender_menu) / sizeof(int);
	}
	else
	{
		if(edlsession->encoders_menu)
		{
			menu = frender_menu2;
			length = sizeof(frender_menu2) / sizeof(int);
		}
		else
		{
			menu = frender_menu1;
			length = sizeof(frender_menu1) / sizeof(int);
		}
	}
	current_menu = new struct selection_int[length + 1];

	for(int i = 0; i < length; i++)
	{
		const struct container_type *ct = ContainerSelection::get_item(menu[i]);
		current_menu[i].text = ct->text;
		current_menu[i].value = ct->value;
	}
	current_menu[length].text = 0;

	parent->add_subwindow(selection = new ContainerSelection(x, y, parent,
		current_menu, output, tools));
	selection->update(*output);
}

FormatPopup::~FormatPopup()
{
	delete [] current_menu;
}

int FormatPopup::get_h()
{
	return selection->get_h();
}

void FormatPopup::update(int value)
{
	selection->update(value);
}

void FormatPopup::reposition_window(int x, int y)
{
	selection->reposition_window(x, y);
}


ContainerSelection::ContainerSelection(int x, int y, BC_WindowBase *base,
	selection_int *menu, int *value, FormatTools *tools)
 : Selection(x, y , base, menu, value, SELECTION_VARWIDTH)
{
	disable(1);
	this->tools = tools;
}

void ContainerSelection::update(int value)
{
	BC_TextBox::update(_(container_to_text(value)));
}

int ContainerSelection::handle_event()
{
	if(current_int && current_int->value != *intvalue)
	{
		*intvalue = current_int->value;
		tools->format_changed();
	}
	return 1;
}

const char *ContainerSelection::container_to_text(int format)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(media_containers[i].value == format)
			return media_containers[i].text;
	}
	return N_("Unknown");
}

int ContainerSelection::text_to_container(const char *string)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(!strcmp(media_containers[i].text, string))
			return media_containers[i].value;
	}
// Backward compatibility
	if(!strcmp(string, "Quicktime for Linux"))
		return FILE_MOV;
	return FILE_UNKNOWN;
}

const struct container_type *ContainerSelection::get_item(int format)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(media_containers[i].value == format)
			return &media_containers[i];
	}
	return 0;
}

const char *ContainerSelection::container_prefix(int format)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(media_containers[i].value == format)
			return media_containers[i].prefix;
	}
	return "UNKNOWN";
}

int ContainerSelection::prefix_to_container(const char *string)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(!strcmp(media_containers[i].prefix, string))
			return media_containers[i].value;
	}
	return FILE_UNKNOWN;
}

const char *ContainerSelection::container_extension(int format)
{
	for(int i = 0; i < NUM_MEDIA_CONTAINERS; i++)
	{
		if(media_containers[i].value == format)
			return media_containers[i].extension;
	}
	return 0;
}
