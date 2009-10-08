
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

#include "awindowgui.h"
#include "clip.h"
#include "colors.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "levelwindowgui.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "patchbay.h"
#include "playtransport.h"
#include "recordgui.h"
#include "recordmonitor.h"
#include "resourcepixmap.h"
#include "statusbar.h"
#include "theme.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "vwindowgui.h"
#include "zoombar.h"


#include <errno.h>
#include <string.h>





Theme::Theme()
 : BC_Theme()
{
	this->mwindow = 0;
	theme_title = DEFAULT_THEME;
	data_buffer = 0;
	contents_buffer = 0;
	last_image = 0;
	mtransport_margin = 0;
	toggle_margin = 0;

	BC_WindowBase::get_resources()->bg_color = BLOND;
	BC_WindowBase::get_resources()->button_up = 0xffc000;
	BC_WindowBase::get_resources()->button_highlighted = 0xffe000;
	BC_WindowBase::get_resources()->recursive_resizing = 0;
	audio_color = BLACK;
	fade_h = 22;
	meter_h = 17;
	mode_h = 30;
	pan_h = 32;
	pan_x = 50;
	play_h = 22;
	title_h = 23;

	preferences_category_overlap = 0;

	loadmode_w = 350;

#include "data/about_png.h"
	about_bg = new VFrame(about_png);



	


}


// Need to delete everything here
Theme::~Theme()
{
	flush_images();	

	aspect_ratios.remove_all_objects();
	frame_rates.remove_all_objects();
	frame_sizes.remove_all_objects();
	sample_rates.remove_all_objects();
	zoom_values.remove_all_objects();
}

void Theme::flush_images()
{
	if(data_buffer) delete [] data_buffer;
	if(contents_buffer) delete [] contents_buffer;
	data_buffer = 0;
	contents_buffer = 0;
	contents.remove_all();
}

void Theme::initialize()
{
	message_normal = BLACK;
	message_error = RED;

// Force to use local data for images
	extern unsigned char _binary_theme_data_start[];
	set_data(_binary_theme_data_start);

// Set images which weren't set by subclass
	new_image("mode_add", "mode_add.png");
	new_image("mode_divide", "mode_divide.png");
	new_image("mode_multiply", "mode_multiply.png");
	new_image("mode_normal", "mode_normal.png");
	new_image("mode_replace", "mode_replace.png");
	new_image("mode_subtract", "mode_subtract.png");
}




void Theme::build_menus()
{


	aspect_ratios.append(new BC_ListBoxItem("3:2"));
	aspect_ratios.append(new BC_ListBoxItem("4:3"));
	aspect_ratios.append(new BC_ListBoxItem("16:9"));
	aspect_ratios.append(new BC_ListBoxItem("2.10:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.20:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.25:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.30:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.35:1"));
	aspect_ratios.append(new BC_ListBoxItem("2.66:1"));
	frame_sizes.append(new BC_ListBoxItem("160x120"));
	frame_sizes.append(new BC_ListBoxItem("240x180"));
	frame_sizes.append(new BC_ListBoxItem("320x240"));
	frame_sizes.append(new BC_ListBoxItem("360x240"));
	frame_sizes.append(new BC_ListBoxItem("400x300"));
	frame_sizes.append(new BC_ListBoxItem("512x384"));
	frame_sizes.append(new BC_ListBoxItem("640x480"));
	frame_sizes.append(new BC_ListBoxItem("720x480"));
	frame_sizes.append(new BC_ListBoxItem("720x576"));
	frame_sizes.append(new BC_ListBoxItem("1280x720"));
	frame_sizes.append(new BC_ListBoxItem("960x1080"));
	frame_sizes.append(new BC_ListBoxItem("1920x1080"));
	frame_sizes.append(new BC_ListBoxItem("1920x1088"));
	sample_rates.append(new BC_ListBoxItem("8000"));
	sample_rates.append(new BC_ListBoxItem("16000"));
	sample_rates.append(new BC_ListBoxItem("22050"));
	sample_rates.append(new BC_ListBoxItem("32000"));
	sample_rates.append(new BC_ListBoxItem("44100"));
	sample_rates.append(new BC_ListBoxItem("48000"));
	sample_rates.append(new BC_ListBoxItem("96000"));
	sample_rates.append(new BC_ListBoxItem("192000"));
	frame_rates.append(new BC_ListBoxItem("1"));
	frame_rates.append(new BC_ListBoxItem("5"));
	frame_rates.append(new BC_ListBoxItem("10"));
	frame_rates.append(new BC_ListBoxItem("12"));
	frame_rates.append(new BC_ListBoxItem("15"));
	frame_rates.append(new BC_ListBoxItem("23.97"));
	frame_rates.append(new BC_ListBoxItem("24"));
	frame_rates.append(new BC_ListBoxItem("25"));
	frame_rates.append(new BC_ListBoxItem("29.97"));
	frame_rates.append(new BC_ListBoxItem("30"));
	frame_rates.append(new BC_ListBoxItem("50"));
	frame_rates.append(new BC_ListBoxItem("59.94"));
	frame_rates.append(new BC_ListBoxItem("60"));
	char string[BCTEXTLEN];
	for(int i = 1; i < 17; i++)
	{
		sprintf(string, "%d", (int)pow(2, i));
		zoom_values.append(new BC_ListBoxItem(string));
	}
}


void Theme::overlay(VFrame *dst, VFrame *src, int in_x1, int in_x2)
{
	int w;
	int h;
	unsigned char **in_rows;
	unsigned char **out_rows;

	if(in_x1 < 0)
	{
		w = MIN(src->get_w(), dst->get_w());
		h = MIN(dst->get_h(), src->get_h());
		in_x1 = 0;
		in_x2 = w;
	}
	else
	{
		w = in_x2 - in_x1;
		h = MIN(dst->get_h(), src->get_h());
	}
	in_rows = src->get_rows();
	out_rows = dst->get_rows();

	switch(src->get_color_model())
	{
		case BC_RGBA8888:
			switch(dst->get_color_model())
			{
				case BC_RGBA8888:
					for(int i = 0; i < h; i++)
					{
						unsigned char *in_row = in_rows[i] + in_x1 * 4;
						unsigned char *out_row = out_rows[i];
						for(int j = 0; j < w; j++)
						{
							int opacity = in_row[3];
							int transparency = 0xff - opacity;
							out_row[0] = (in_row[0] * opacity + out_row[0] * transparency) / 0xff;
							out_row[1] = (in_row[1] * opacity + out_row[1] * transparency) / 0xff;
							out_row[2] = (in_row[2] * opacity + out_row[2] * transparency) / 0xff;
							out_row[3] = MAX(in_row[3], out_row[3]);
							out_row += 4;
							in_row += 4;
						}
					}
					break;
				case BC_RGB888:
					for(int i = 0; i < h; i++)
					{
						unsigned char *in_row = in_rows[i] + in_x1 * 4;
						unsigned char *out_row = out_rows[i];
						for(int j = 0; j < w; j++)
						{
							int opacity = in_row[3];
							int transparency = 0xff - opacity;
							out_row[0] = (in_row[0] * opacity + out_row[0] * transparency) / 0xff;
							out_row[1] = (in_row[1] * opacity + out_row[1] * transparency) / 0xff;
							out_row[2] = (in_row[2] * opacity + out_row[2] * transparency) / 0xff;
							out_row += 3;
							in_row += 4;
						}
					}
					break;
			}
			break;
	}
}

void Theme::build_transport(char *title,
	unsigned char *png_overlay,
	VFrame **bg_data,
	int third)
{
	if(!png_overlay) return;
	VFrame default_data(png_overlay);
	VFrame *data[3];
	data[0] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[1] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[2] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[0]->clear_frame();
	data[1]->clear_frame();
	data[2]->clear_frame();

	for(int i = 0; i < 3; i++)
	{
		int in_x1;
		int in_x2;
		if(!bg_data[i]) break;

		switch(third)
		{
			case 0:
				in_x1 = 0;
				in_x2 = default_data.get_w();
				break;

			case 1:
				in_x1 = (int)(bg_data[i]->get_w() * 0.33);
				in_x2 = in_x1 + default_data.get_w();
				break;

			case 2:
				in_x1 = bg_data[i]->get_w() - default_data.get_w();
				in_x2 = in_x1 + default_data.get_w();
				break;
		}

		overlay(data[i], 
			bg_data[i],
			in_x1,
			in_x2);
		overlay(data[i], 
			&default_data);
	}

	new_image_set_images(title, 3, data[0], data[1], data[2]);
}









void Theme::build_patches(VFrame** &data,
	unsigned char *png_overlay,
	VFrame **bg_data,
	int region)
{
	if(!png_overlay || !bg_data) return;
	VFrame default_data(png_overlay);
	data = new VFrame*[5];
	data[0] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[1] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[2] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[3] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[4] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);

	for(int i = 0; i < 5; i++)
	{
		int in_x1;
		int in_x2;

		switch(region)
		{
			case 0:
				in_x1 = 0;
				in_x2 = default_data.get_w();
				break;

			case 1:
				in_x1 = (int)(bg_data[i]->get_w() * 0.33);
				in_x2 = in_x1 + default_data.get_w();
				break;

			case 2:
				in_x1 = bg_data[i]->get_w() - default_data.get_w();
				in_x2 = in_x1 + default_data.get_w();
				break;
		}

		overlay(data[i], 
			bg_data[i]);
		overlay(data[i], 
			&default_data);
	}
}








void Theme::build_button(VFrame** &data,
	unsigned char *png_overlay,
	VFrame *up_vframe,
	VFrame *hi_vframe,
	VFrame *dn_vframe)
{
	if(!png_overlay) return;
	VFrame default_data(png_overlay);

	if(!up_vframe || !hi_vframe || !dn_vframe) return;
	data = new VFrame*[3];
	data[0] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[1] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[2] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[0]->copy_from(up_vframe);
	data[1]->copy_from(hi_vframe);
	data[2]->copy_from(dn_vframe);
	for(int i = 0; i < 3; i++)
		overlay(data[i], 
			&default_data);
}

void Theme::build_toggle(VFrame** &data,
	unsigned char *png_overlay,
	VFrame *up_vframe,
	VFrame *hi_vframe,
	VFrame *checked_vframe,
	VFrame *dn_vframe,
	VFrame *checkedhi_vframe)
{
	if(!png_overlay || 
		!up_vframe || 
		!hi_vframe || 
		!checked_vframe || 
		!dn_vframe || 
		!checkedhi_vframe) return;
	VFrame default_data(png_overlay);
	data = new VFrame*[5];
	data[0] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[1] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[2] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[3] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[4] = new VFrame(0, default_data.get_w(), default_data.get_h(), BC_RGBA8888);
	data[0]->copy_from(up_vframe);
	data[1]->copy_from(hi_vframe);
	data[2]->copy_from(checked_vframe);
	data[3]->copy_from(dn_vframe);
	data[4]->copy_from(checkedhi_vframe);
	for(int i = 0; i < 5; i++)
		overlay(data[i], 
			&default_data);
}

#define TIMEBAR_HEIGHT 10
#define PATCHBAY_W 145
#define STATUS_H 20
#define ZOOM_H 30

void Theme::get_mwindow_sizes(MWindowGUI *gui, int w, int h)
{
}

void Theme::draw_mwindow_bg(MWindowGUI *gui)
{
}




void Theme::draw_awindow_bg(AWindowGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->awindow_w, mwindow->session->awindow_h);
	gui->flash();
}

void Theme::draw_vwindow_bg(VWindowGUI *gui)
{
// 	gui->clear_box(0, 
// 		0, 
// 		mwindow->session->vwindow_w, 
// 		mwindow->session->vwindow_h);
// // Timebar
// 	gui->draw_3segmenth(vtimebar_x, 
// 		vtimebar_y, 
// 		vtimebar_w, 
// 		vtimebar_bg_data,
// 		0);
// 	gui->flash();
}


void Theme::draw_cwindow_bg(CWindowGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->cwindow_w, mwindow->session->cwindow_h);
	gui->flash();
}

void Theme::draw_lwindow_bg(LevelWindowGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->lwindow_w, mwindow->session->lwindow_h);
	gui->flash();
}


void Theme::draw_rmonitor_bg(RecordMonitorGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->rmonitor_w, mwindow->session->rmonitor_h);
	gui->flash();
}


void Theme::draw_rwindow_bg(RecordGUI *gui)
{
	gui->clear_box(0, 0, mwindow->session->rwindow_w, mwindow->session->rwindow_h);
}


void Theme::draw_resource_bg(TrackCanvas *canvas,
	ResourcePixmap *pixmap, 
	int edit_x,
	int edit_w,
	int pixmap_x,
	int x1, 
	int y1, 
	int x2,
	int y2)
{
	VFrame *image;

	switch(mwindow->edl->local_session->zoom_track)
	{
		case 1024: image = get_image("resource1024");  break;
		case 512: image = get_image("resource512");  break;
		case 256: image = get_image("resource256");  break;
		case 128: image = get_image("resource128");  break;
		case 64:  image = get_image("resource64");   break;
		default:
		case 32:  image = get_image("resource32");   break;
	}

	canvas->draw_3segmenth(x1, 
		y1, 
		x2 - x1, 
		edit_x - pixmap_x,
		edit_w,
		image,
		pixmap);
}

void Theme::get_vwindow_sizes(VWindowGUI *gui)
{
}

void Theme::get_cwindow_sizes(CWindowGUI *gui, int cwindow_controls)
{
}

void Theme::get_awindow_sizes(AWindowGUI *gui)
{
	abuttons_x = 0; 
	abuttons_y = 0;
	afolders_x = 0;
//	afolders_y = deletedisk_data[0]->get_h();
	afolders_y = 0;
	afolders_w = mwindow->session->afolders_w;
	afolders_h = mwindow->session->awindow_h - afolders_y;
	adivider_x = afolders_x + afolders_w;
	adivider_y = 0;
	adivider_w = 5;
	adivider_h = afolders_h;
	alist_x = afolders_x + afolders_w + 5;
	alist_y = afolders_y;
	alist_w = mwindow->session->awindow_w - alist_x;
	alist_h = afolders_h;
}

void Theme::get_rmonitor_sizes(int do_audio, 
	int do_video,
	int do_channel,
	int do_interlace,
	int do_avc,
	int audio_channels)
{
	int x = 10;
	int y = 3;


	if(do_avc)
	{
		rmonitor_canvas_y = 30;
		rmonitor_tx_x = 10;
		rmonitor_tx_y = 0;
	}
	else
	{
		rmonitor_canvas_y = 0;
		rmonitor_tx_x = 0;
		rmonitor_tx_y = 0;
	}


	if(do_channel)
	{
		y = 5;
		rmonitor_channel_x = x;
		rmonitor_channel_y = 5;
		x += 235;
		rmonitor_canvas_y = 35;
	}

	if(do_interlace)
	{
		y = 4;
		rmonitor_interlace_x = x;
		rmonitor_interlace_y = y;
	}


	if(do_audio)
	{
		rmonitor_meter_x = mwindow->session->rmonitor_w - MeterPanel::get_meters_width(audio_channels, 1);
		rmonitor_meter_y = 40;
		rmonitor_meter_h = mwindow->session->rmonitor_h - 10 - rmonitor_meter_y;
	}
	else
	{
		rmonitor_meter_x = mwindow->session->rmonitor_w;
	}

	rmonitor_canvas_x = 0;
	rmonitor_canvas_w = rmonitor_meter_x - rmonitor_canvas_x;
	if(do_audio) rmonitor_canvas_w -= 10;
	rmonitor_canvas_h = mwindow->session->rmonitor_h - rmonitor_canvas_y;
}

void Theme::get_recordgui_sizes(RecordGUI *gui, int w, int h)
{
}

void Theme::get_batchrender_sizes(BatchRenderGUI *gui,
	int w, 
	int h)
{
	batchrender_x1 = 5;
	batchrender_x2 = 300;
	batchrender_x3 = 400;
}

void Theme::get_plugindialog_sizes()
{
	int x = 10, y = 30;
	plugindialog_new_x = x;
	plugindialog_new_y = y;
	plugindialog_shared_x = mwindow->session->plugindialog_w / 3;
	plugindialog_shared_y = y;
	plugindialog_module_x = mwindow->session->plugindialog_w * 2 / 3;
	plugindialog_module_y = y;

	plugindialog_new_w = plugindialog_shared_x - plugindialog_new_x - 10;
	plugindialog_new_h = mwindow->session->plugindialog_h - 100;
	plugindialog_shared_w = plugindialog_module_x - plugindialog_shared_x - 10;
	plugindialog_shared_h = mwindow->session->plugindialog_h - 100;
	plugindialog_module_w = mwindow->session->plugindialog_w - plugindialog_module_x - 10;
	plugindialog_module_h = mwindow->session->plugindialog_h - 100;

	plugindialog_newattach_x = plugindialog_new_x + 20;
	plugindialog_newattach_y = plugindialog_new_y + plugindialog_new_h + 10;
	plugindialog_sharedattach_x = plugindialog_shared_x + 20;
	plugindialog_sharedattach_y = plugindialog_shared_y + plugindialog_shared_h + 10;
	plugindialog_moduleattach_x = plugindialog_module_x + 20;
	plugindialog_moduleattach_y = plugindialog_module_y + plugindialog_module_h + 10;
}

void Theme::get_menueffect_sizes(int use_list)
{
	if(use_list)
	{
		menueffect_list_x = 10;
		menueffect_list_y = 10;
		menueffect_list_w = mwindow->session->menueffect_w - 400;
		menueffect_list_h = mwindow->session->menueffect_h - 
			menueffect_list_y -
			BC_OKButton::calculate_h() - 10;
	}
	else
	{
		menueffect_list_x = 0;
		menueffect_list_y = 10;
		menueffect_list_w = 0;
		menueffect_list_h = 0;
	}

	menueffect_file_x = menueffect_list_x + menueffect_list_w + 10;
	menueffect_file_y = 10;

	menueffect_tools_x = menueffect_file_x;
	menueffect_tools_y = menueffect_file_y + 20;
}

void Theme::get_preferences_sizes()
{
}

void Theme::draw_preferences_bg(PreferencesWindow *gui)
{
}

void Theme::get_new_sizes(NewWindow *gui)
{
}

void Theme::draw_new_bg(NewWindow *gui)
{
}

void Theme::draw_setformat_bg(SetFormatWindow *window)
{
}







