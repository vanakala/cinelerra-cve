#include "awindowgui.h"
#include "clip.h"
#include "colors.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
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
	title_h = 22;


	loadmode_w = 350;

#include "data/about_png.h"
	about_bg = new VFrame(about_png);
#include "data/microsoft_credit_png.h"
	about_microsoft = new VFrame(microsoft_credit_png);



	build_menus();


}


// Need to delete everything here
Theme::~Theme()
{
//printf("Theme::~Theme 1\n");
	flush_images();	

	aspect_ratios.remove_all_objects();
	frame_rates.remove_all_objects();
	frame_sizes.remove_all_objects();
	sample_rates.remove_all_objects();
	zoom_values.remove_all_objects();


// 
// 	delete about_bg;
// 	delete about_microsoft;
// 	delete [] appendasset_data;
// 	delete [] append_data;
// 	delete [] arrow_data;
// 	delete [] asset_append_data;
// 	delete [] asset_disk_data;
// 	delete [] asset_index_data;
// 	delete [] asset_info_data;
// 	delete [] asset_project_data;
// 	delete [] autokeyframe_data;
// 	delete awindow_icon;
// 	delete [] bottom_justify;
// 	delete [] browse_data;
// 	delete [] calibrate_data;
// 	delete [] camera_data;
// 	delete camerakeyframe_data;
// 	delete [] cancel_data;
// 	delete [] center_justify;
// 	delete [] chain_data;
// 	delete channel_bg_data;
// 	delete [] channel_data;
// 	delete channel_position_data;
// 	delete clip_icon;
// 	delete [] copy_data;
// 	delete [] crop_data;
// 	delete [] cut_data;
// 	delete cwindow_icon;
// 	delete [] delete_all_indexes_data;
// 	delete [] deletebin_data;
// 	delete [] delete_data;
// 	delete [] deletedisk_data;
// 	delete [] deleteproject_data;
// 	delete [] detach_data;
// 	delete [] dntriangle_data;
// 	delete [] drawpatch_data;
// 	delete [] duplex_data;
// 	delete [] edit_data;
// 	delete [] edithandlein_data;
// 	delete [] edithandleout_data;
// 	delete [] end_data;
// 	delete [] expandpatch_data;
// 	delete [] extract_data;
// 	delete [] fastfwd_data;
// 	delete [] fastrev_data;
// 	delete [] fit_data;
// 	delete [] forward_data;
// 	delete [] framefwd_data;
// 	delete [] framerev_data;
// 	delete [] gangpatch_data;
// 	delete [] ibeam_data;
// 	delete [] in_data;
// 	delete [] indelete_data;
// 	delete [] infoasset_data;
// 	delete [] in_point;
// 	delete [] insert_data;
// 	delete keyframe_data;
// 	delete [] labelbutton_data;
// 	delete [] label_toggle;
// 	delete [] left_justify;
// 	delete [] lift_data;
// 	delete [] magnify_button_data;
// 	delete [] magnify_data;
// 	delete [] mask_data;
// 	delete maskkeyframe_data;
// 	delete [] middle_justify;
// 	delete modekeyframe_data;
// 	delete [] movedn_data;
// 	delete [] moveup_data;
// 	delete [] mutepatch_data;
// 	delete mwindow_icon;
// 	delete [] newbin_data;
// 	delete [] nextlabel_data;
// 	delete [] no_data;
// 	delete [] options_data;
// 	delete [] out_data;
// 	delete [] outdelete_data;
// 	delete [] out_point;
// 	delete [] over_button;
// 	delete [] overwrite_data;
// 	delete pankeyframe_data;
// 	delete [] pasteasset_data;
// 	delete [] paste_data;
// 	delete patchbay_bg;
// 	delete [] pause_data;
// 	delete [] paused_data;
// 	delete [] picture_data;
// 	delete [] playpatch_data;
// 	delete plugin_bg_data;
// 	delete [] presentation_data;
// 	delete [] presentation_loop;
// 	delete [] presentation_stop;
// 	delete [] prevlabel_data;
// 	delete [] proj_data;
// 	delete projectorkeyframe_data;
// 	delete [] protect_data;
// 	delete [] rec_data;
// 	delete [] recframe_data;
// 	delete record_icon;
// 	delete [] recordpatch_data;
// 	delete [] redo_data;
// 	delete [] redrawindex_data;
// 	delete [] renamebin_data;
// 	delete [] reset_data;
// 	delete resource1024_bg_data;
// 	delete resource128_bg_data;
// 	delete resource256_bg_data;
// 	delete resource32_bg_data;
// 	delete resource512_bg_data;
// 	delete resource64_bg_data;
// 	delete [] reverse_data;
// 	delete [] rewind_data;
// 	delete [] right_justify;
// 	delete [] select_data;
// 	delete [] show_meters;
// 	delete [] splice_data;
// 	delete [] start_over_data;
// 	delete [] statusbar_cancel_data;
// 	delete [] stop_data;
// 	delete [] stoprec_data;
// 	delete timebar_bg_data;
// 	delete timebar_brender_data;
// 	delete timebar_view_data;
// 	delete title_bg_data;
// 	delete [] titlesafe_data;
// 	delete [] toclip_data;
// 	delete [] tool_data;
// 	delete [] top_justify;
// 	delete [] transition_data;
// 	delete [] undo_data;
// 	delete [] uptriangle_data;
// 	delete [] viewasset_data;
// 	delete vtimebar_bg_data;
// 	delete vwindow_icon;
// 	delete [] wrench_data;
// 	delete [] yes_data;
//printf("Theme::~Theme 2\n");
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
}


unsigned char* Theme::get_image(char *title)
{
// Read contents
	if(!data_buffer)
	{
		FILE *fd = fopen(path, "r");

		if(!fd)
		{
			fprintf(stderr, "Theme::get_image: %s when opening %s\n", strerror(errno), path);
		}
		int data_offset, contents_offset;
		int total_bytes;
		int data_size;
		int contents_size;

		fseek(fd, -8, SEEK_END);
		total_bytes = ftell(fd);
		fread(&data_offset, 1, 4, fd);
		fread(&contents_offset, 1, 4, fd);


		fseek(fd, data_offset, SEEK_SET);
		data_size = contents_offset - data_offset;
		data_buffer = new char[data_size];
		fread(data_buffer, 1, data_size, fd);

		fseek(fd, contents_offset, SEEK_SET);
		contents_size = total_bytes - contents_offset;
		contents_buffer = new char[contents_size];
		fread(contents_buffer, 1, contents_size, fd);

		char *start_of_title = contents_buffer;
		for(int i = 0; i < contents_size; )
		{
			if(contents_buffer[i] == 0)
			{
				contents.append(start_of_title);
				i++;
				offsets.append(*(int*)(contents_buffer + i));
				i += 4;
				start_of_title = contents_buffer + i;
			}
			else
				i++;
		}
		fclose(fd);
	}

	if(last_image && !strcasecmp(last_image, title))
	{
		return (unsigned char*)(data_buffer + last_offset);
	}
	else
	for(int i = 0; i < contents.total; i++)
	{
		if(!strcasecmp(contents.values[i], title))
		{
			last_offset = offsets.values[i];
			last_image = contents.values[i];
			return (unsigned char*)(data_buffer + offsets.values[i]);
		}
	}

	fprintf(stderr, "Theme::get_image: %s not found.\n", title);
	return 0;
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

void Theme::build_transport(VFrame** &data,
	unsigned char *png_overlay,
	VFrame **bg_data,
	int third)
{
	if(!png_overlay) return;
	VFrame default_data(png_overlay);
	data = new VFrame*[3];
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
printf("Theme::draw_rwindow_bg 1\n");
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
		case 1024: image = resource1024_bg_data;  break;
		case 512: image = resource512_bg_data;  break;
		case 256: image = resource256_bg_data;  break;
		case 128: image = resource128_bg_data;  break;
		case 64:  image = resource64_bg_data;   break;
		default:
		case 32:  image = resource32_bg_data;   break;
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

void Theme::get_cwindow_sizes(CWindowGUI *gui)
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

void Theme::get_recordmonitor_sizes(int do_audio, 
	int do_video)
{
	rmonitor_tx_x = 10;
	rmonitor_tx_y = 3;
	rmonitor_source_x = 150;
	rmonitor_source_y = 10;
	rmonitor_interlace_x = 455;
	rmonitor_interlace_y = 5;
	rmonitor_channel_x = 0;
	rmonitor_channel_y = 0;

	if(do_audio)
	{
		rmonitor_meter_x = mwindow->session->rmonitor_w - MeterPanel::get_meters_width(mwindow->edl->session->audio_channels, 1);
		rmonitor_meter_y = 40;
		rmonitor_meter_h = mwindow->session->rmonitor_h - 10 - rmonitor_meter_y;
	}
	else
	{
		rmonitor_meter_x = mwindow->session->rmonitor_w;
	}

// 	if(do_video)
// 	{
		rmonitor_canvas_x = 0;
		rmonitor_canvas_y = 35;
		rmonitor_canvas_w = rmonitor_meter_x - rmonitor_canvas_x;
		if(do_audio) rmonitor_canvas_w -= 10;
		rmonitor_canvas_h = mwindow->session->rmonitor_h - rmonitor_canvas_y;
		rmonitor_channel_x = 220;
		rmonitor_channel_y = 5;
//	}
}

void Theme::get_recordgui_sizes(RecordGUI *gui, int w, int h)
{
	recordgui_status_x = 10;
	recordgui_status_y = 10;
	recordgui_status_x2 = 150;
	recordgui_batch_x = 310;
	recordgui_batch_y = 10;
	recordgui_batchcaption_x = recordgui_batch_x + 110;


	recordgui_transport_x = recordgui_batch_x;
	recordgui_transport_y = recordgui_batch_y + 150;

	recordgui_buttons_x = recordgui_batch_x - 50;
	recordgui_buttons_y = recordgui_transport_y + 40;
	recordgui_options_x = recordgui_buttons_x;
	recordgui_options_y = recordgui_buttons_y + 40;

	recordgui_batches_x = 10;
	recordgui_batches_y = 270;
	recordgui_batches_w = w - 20;
	recordgui_batches_h = h - recordgui_batches_y - 90;
	recordgui_loadmode_x = w / 2 - loadmode_w / 2;
	recordgui_loadmode_y = h - 85;

	recordgui_controls_x = 10;
	recordgui_controls_y = h - 40;
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
	plugindialog_new_h = mwindow->session->plugindialog_h - 110;
	plugindialog_shared_w = plugindialog_module_x - plugindialog_shared_x - 10;
	plugindialog_shared_h = mwindow->session->plugindialog_h - 110;
	plugindialog_module_w = mwindow->session->plugindialog_w - plugindialog_module_x - 10;
	plugindialog_module_h = mwindow->session->plugindialog_h - 110;

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
		menueffect_list_h = mwindow->session->menueffect_h - 70;
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







