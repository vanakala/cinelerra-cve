
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

#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "loadmode.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "microtheme.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "statusbar.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "vframe.h"

PluginClient* new_plugin(PluginServer *server)
{
	return new MicroThemeMain(server);
}


MicroThemeMain::MicroThemeMain(PluginServer *server)
 : PluginTClient(server)
{
}

MicroThemeMain::~MicroThemeMain()
{
}


char* MicroThemeMain::plugin_title() 
{
	return "Microscopic"; 
}

Theme* MicroThemeMain::new_theme()
{
	return theme = new MicroTheme;
}





MicroTheme::MicroTheme()
 : Theme()
{
}
MicroTheme::~MicroTheme()
{
}

void MicroTheme::initialize()
{
//printf("MicroTheme::initialize 1\n");
	mwindow_icon = new VFrame(get_image("mwindow_icon.png"));
	vwindow_icon = new VFrame(get_image("mwindow_icon.png"));
	cwindow_icon = new VFrame(get_image("mwindow_icon.png"));
	awindow_icon = new VFrame(get_image("mwindow_icon.png"));
	record_icon = new VFrame(get_image("mwindow_icon.png"));
	clip_icon = new VFrame(get_image("clip_icon.png"));


	static VFrame *default_patchbay_bg = new VFrame(get_image("patchbay_bg.png"));

	BC_WindowBase::get_resources()->bg_color = WHITE;
	BC_WindowBase::get_resources()->menu_light = WHITE;
	BC_WindowBase::get_resources()->menu_highlighted = WHITE;
	BC_WindowBase::get_resources()->menu_down = LTGREY;
	BC_WindowBase::get_resources()->menu_up = WHITE;
	BC_WindowBase::get_resources()->menu_shadow = MEGREY;
	BC_WindowBase::get_resources()->medium_font = "-*-helvetica-medium-r-normal-*-10-*";
	
	static VFrame* default_listbox_bg = new VFrame(get_image("patchbay_bg.png"));
	BC_WindowBase::get_resources()->listbox_bg = default_listbox_bg;
	BC_WindowBase::get_resources()->button_light = WHITE;
	BC_WindowBase::get_resources()->button_up = WHITE;

	
	static VFrame *default_cancel_images[] = 
	{
		new VFrame(get_image("cancel_up.png")), new VFrame(get_image("cancel_hi.png")), new VFrame(get_image("cancel_dn.png"))
	};
	BC_WindowBase::get_resources()->cancel_images = default_cancel_images;

	static VFrame *default_ok_images[] = 
	{
		new VFrame(get_image("ok_up.png")), new VFrame(get_image("ok_hi.png")), new VFrame(get_image("ok_dn.png"))
	};
	BC_WindowBase::get_resources()->ok_images = default_ok_images;

	static VFrame *default_button_images[] = 
	{
		new VFrame(get_image("generic_up.png")), new VFrame(get_image("generic_hi.png")), new VFrame(get_image("generic_dn.png"))
	};
	BC_WindowBase::get_resources()->generic_button_images = default_button_images;

	static VFrame *default_tumble_images[] = 
	{
		new VFrame(get_image("tumble_up.png")), new VFrame(get_image("tumble_hi.png")), new VFrame(get_image("tumble_bottomdn.png")), new VFrame(get_image("tumble_topdn.png"))
	};
	BC_WindowBase::get_resources()->tumble_data = default_tumble_images;

	static VFrame *default_checkbox_images[] = 
	{
		new VFrame(get_image("checkbox_up.png")), new VFrame(get_image("checkbox_hi.png")), new VFrame(get_image("checkbox_checked.png")), new VFrame(get_image("checkbox_dn.png")), new VFrame(get_image("checkbox_checkedhi.png"))
	};
	BC_WindowBase::get_resources()->checkbox_images = default_checkbox_images;
	
	static VFrame *default_radial_images[] = 
	{
		new VFrame(get_image("radial_up.png")), new VFrame(get_image("radial_hi.png")), new VFrame(get_image("radial_checked.png")), new VFrame(get_image("radial_dn.png")), new VFrame(get_image("radial_checkedhi.png"))
	};
	BC_WindowBase::get_resources()->radial_images = default_radial_images;

	static VFrame* default_xmeter_data[] =
	{
		new VFrame(get_image("xmeter_normal.png")),
		new VFrame(get_image("xmeter_green.png")),
		new VFrame(get_image("xmeter_red.png")),
		new VFrame(get_image("xmeter_yellow.png")),
		new VFrame(get_image("over_horiz.png"))
	};

	static VFrame* default_ymeter_data[] =
	{
		new VFrame(get_image("ymeter_normal.png")),
		new VFrame(get_image("ymeter_green.png")),
		new VFrame(get_image("ymeter_red.png")),
		new VFrame(get_image("ymeter_yellow.png")),
		new VFrame(get_image("over_vert.png"))
	};
	BC_WindowBase::get_resources()->xmeter_images = default_xmeter_data;
	BC_WindowBase::get_resources()->ymeter_images = default_ymeter_data;
	BC_WindowBase::get_resources()->meter_font = SMALLFONT;
	BC_WindowBase::get_resources()->meter_font_color = BLACK;
	BC_WindowBase::get_resources()->meter_title_w = 25;
	BC_WindowBase::get_resources()->meter_3d = 0;

	static VFrame* default_pan_data[] = 
	{
		new VFrame(get_image("pan_up.png")), 
		new VFrame(get_image("pan_hi.png")), 
		new VFrame(get_image("pan_popup.png")), 
		new VFrame(get_image("pan_channel.png")), 
		new VFrame(get_image("pan_stick.png")), 
		new VFrame(get_image("pan_channel_small.png")), 
		new VFrame(get_image("pan_stick_small.png"))
	};
	BC_WindowBase::get_resources()->pan_data = default_pan_data;
	BC_WindowBase::get_resources()->pan_text_color = BLACK;

	static VFrame *default_hscroll_data[] = 
	{
		new VFrame(get_image("hscroll_handle_up.png")), 
		new VFrame(get_image("hscroll_handle_hi.png")), 
		new VFrame(get_image("hscroll_handle_dn.png")), 
		new VFrame(get_image("hscroll_handle_bg.png")), 
		new VFrame(get_image("hscroll_left_up.png")), 
		new VFrame(get_image("hscroll_left_hi.png")), 
		new VFrame(get_image("hscroll_left_dn.png")), 
		new VFrame(get_image("hscroll_right_up.png")), 
		new VFrame(get_image("hscroll_right_hi.png")), 
		new VFrame(get_image("hscroll_right_dn.png"))
	};
	static VFrame *default_vscroll_data[] = 
	{
		new VFrame(get_image("vscroll_handle_up.png")), 
		new VFrame(get_image("vscroll_handle_hi.png")), 
		new VFrame(get_image("vscroll_handle_dn.png")), 
		new VFrame(get_image("vscroll_handle_bg.png")), 
		new VFrame(get_image("vscroll_left_up.png")), 
		new VFrame(get_image("vscroll_left_hi.png")), 
		new VFrame(get_image("vscroll_left_dn.png")), 
		new VFrame(get_image("vscroll_right_up.png")), 
		new VFrame(get_image("vscroll_right_hi.png")), 
		new VFrame(get_image("vscroll_right_dn.png"))
	};
	BC_WindowBase::get_resources()->hscroll_data = default_hscroll_data;
	BC_WindowBase::get_resources()->vscroll_data = default_vscroll_data;

	channel_bg_data = new VFrame(get_image("channel_bg.png"));
	channel_position_data = new VFrame(get_image("channel_position.png"));
	channel_position_color = BLACK;
	recordgui_fixed_color = BLACK;
	recordgui_variable_color = RED;

	patchbay_bg = default_patchbay_bg;
	resource1024_bg_data = new VFrame(get_image("resource1024.png"));
	resource512_bg_data = new VFrame(get_image("resource512.png"));
	resource256_bg_data = new VFrame(get_image("resource256.png"));
	resource128_bg_data = new VFrame(get_image("resource128.png"));
	resource64_bg_data = new VFrame(get_image("resource64.png"));
	resource32_bg_data = new VFrame(get_image("resource32.png"));
	plugin_bg_data = new VFrame(get_image("plugin_bg.png"));
	title_bg_data = new VFrame(get_image("title_bg.png"));
	timebar_bg_data = new VFrame(get_image("timebar_bg.png"));
	vtimebar_bg_data = new VFrame(get_image("vwindow_timebar.png"));

	keyframe_data = new VFrame(get_image("keyframe3.png"));
	camerakeyframe_data = new VFrame(get_image("camerakeyframe.png"));
	maskkeyframe_data = new VFrame(get_image("maskkeyframe.png"));
	modekeyframe_data = new VFrame(get_image("modekeyframe.png"));
	pankeyframe_data = new VFrame(get_image("pankeyframe.png"));
	projectorkeyframe_data = new VFrame(get_image("projectorkeyframe.png"));

	VFrame editpanel_up(get_image("editpanel_up.png"));
	VFrame editpanel_hi(get_image("editpanel_hi.png"));
	VFrame editpanel_dn(get_image("editpanel_dn.png"));
	VFrame editpanel_checked(get_image("editpanel_checked.png"));
	VFrame editpanel_checkedhi(get_image("editpanel_checkedhi.png"));

	static VFrame *default_inpoint[] = { new VFrame(get_image("out_up.png")), new VFrame(get_image("out_hi.png")), new VFrame(get_image("out_checked.png")), new VFrame(get_image("out_dn.png")), new VFrame(get_image("out_checkedhi.png")) };
	static VFrame *default_labeltoggle[] = { new VFrame(get_image("labeltoggle_up.png")), new VFrame(get_image("labeltoggle_uphi.png")), new VFrame(get_image("label_checked.png")), new VFrame(get_image("labeltoggle_dn.png")), new VFrame(get_image("label_checkedhi.png")) };
	static VFrame *default_outpoint[] = { new VFrame(get_image("in_up.png")), new VFrame(get_image("in_hi.png")), new VFrame(get_image("in_checked.png")), new VFrame(get_image("in_dn.png")), new VFrame(get_image("in_checkedhi.png")) };
	static VFrame *transport_bg[] = { new VFrame(get_image("transportup.png")), new VFrame(get_image("transporthi.png")), new VFrame(get_image("transportdn.png")) };
	static VFrame *patches_bg[] = { new VFrame(get_image("patches_up.png")), new VFrame(get_image("patches_hi.png")), new VFrame(get_image("patches_checked.png")), new VFrame(get_image("patches_dn.png")), new VFrame(get_image("patches_checkedhi.png")) };

	build_button(BC_WindowBase::get_resources()->filebox_updir_images, get_image("filebox_updir.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(BC_WindowBase::get_resources()->filebox_newfolder_images, get_image("filebox_newfolder.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(BC_WindowBase::get_resources()->filebox_icons_images, get_image("filebox_icons.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(BC_WindowBase::get_resources()->filebox_text_images, get_image("filebox_text.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);

	build_button(BC_WindowBase::get_resources()->listbox_button, get_image("listbox_button.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(bottom_justify, get_image("bottom_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(center_justify, get_image("center_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(copy_data, get_image("copy.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(cut_data, get_image("cut.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(fit_data, get_image("fit.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(in_data, get_image("outpoint.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(indelete_data, get_image("clearinpoint.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(labelbutton_data, get_image("label.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(left_justify, get_image("left_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(magnify_button_data, get_image("magnify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(middle_justify, get_image("middle_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(nextlabel_data, get_image("nextlabel.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(out_data, get_image("inpoint.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(outdelete_data, get_image("clearoutpoint.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(over_button, get_image("over.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(overwrite_data, get_image("overwrite.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(paste_data, get_image("paste.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(prevlabel_data, get_image("prevlabel.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(redo_data, get_image("redo.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(right_justify, get_image("right_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(splice_data, get_image("splice.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(statusbar_cancel_data, get_image("cancel_small.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(toclip_data, get_image("toclip.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(top_justify, get_image("top_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(undo_data, get_image("undo.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_toggle(arrow_data, get_image("arrow.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(autokeyframe_data, get_image("autokeyframe.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(camera_data, get_image("camera.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(crop_data, get_image("crop.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(ibeam_data, get_image("ibeam.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(magnify_data, get_image("magnify.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(mask_data, get_image("mask.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(proj_data, get_image("projector.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(protect_data, get_image("protect.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(show_meters, get_image("show_meters.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(titlesafe_data, get_image("titlesafe.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_toggle(tool_data, get_image("toolwindow.png"), &editpanel_up, &editpanel_hi, &editpanel_checked, &editpanel_dn, &editpanel_checkedhi);
	build_transport(duplex_data, get_image("duplex.png"), transport_bg, 1);
	build_transport(end_data, get_image("end.png"), transport_bg, 2);
	build_transport(fastfwd_data, get_image("fastfwd.png"), transport_bg, 1);
	build_transport(fastrev_data, get_image("fastrev.png"), transport_bg, 1);
	build_transport(forward_data, get_image("play.png"), transport_bg, 1);
	build_transport(framefwd_data, get_image("framefwd.png"), transport_bg, 1);
	build_transport(framefwd_data, get_image("framefwd.png"), transport_bg, 1);
	build_transport(framerev_data, get_image("framerev.png"), transport_bg, 1);
	build_transport(rec_data, get_image("record.png"), transport_bg, 1);
	build_transport(recframe_data, get_image("singleframe.png"), transport_bg, 1);
	build_transport(reverse_data, get_image("reverse.png"), transport_bg, 1);
	build_transport(rewind_data, get_image("rewind.png"), transport_bg, 0);
	build_transport(stop_data, get_image("stop.png"), transport_bg, 1);
	build_transport(stoprec_data, get_image("stoprec.png"), transport_bg, 2);

	build_patches(playpatch_data, get_image("playpatch.png"), patches_bg, 0);
	build_patches(recordpatch_data, get_image("recordpatch.png"), patches_bg, 1);
	build_patches(gangpatch_data, get_image("gangpatch.png"), patches_bg, 1);
	build_patches(drawpatch_data, get_image("drawpatch.png"), patches_bg, 1);
	build_patches(mutepatch_data, get_image("mutepatch.png"), patches_bg, 2);

	static VFrame *default_expandpatch_data[] = 
	{
		new VFrame(get_image("expandpatch_up.png")), 
		new VFrame(get_image("expandpatch_hi.png")), 
		new VFrame(get_image("expandpatch_checked.png")), 
		new VFrame(get_image("expandpatch_dn.png")), 
		new VFrame(get_image("expandpatch_checkedhi.png"))
	};
	expandpatch_data = default_expandpatch_data;

	build_button(channel_data, get_image("channel.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(wrench_data, get_image("wrench.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	in_point = default_inpoint;
	label_toggle = default_labeltoggle;
	out_point = default_outpoint;

	fade_h = BC_WindowBase::get_resources()->horizontal_slider_data[0]->get_h();
	mode_h = BC_WindowBase::get_resources()->generic_button_images[0]->get_h();
	meter_h = BC_WindowBase::get_resources()->xmeter_images[0]->get_h();
	pan_h = BC_WindowBase::get_resources()->pan_data[PAN_UP]->get_h();
	play_h = playpatch_data[0]->get_h();
	title_h = 18;
	pan_x = 25;

	title_font = MEDIUMFONT;
	title_color = BLACK;

	loadmode_w = 250;
	flush_images();
//printf("MicroTheme::initialize 2\n");
}

void MicroTheme::draw_mwindow_bg(MWindowGUI *gui)
{
	gui->clear_box(0, 0, gui->get_w(), gui->get_h());
}

void MicroTheme::get_cwindow_sizes(CWindowGUI *gui)
{
//printf("Theme::get_cwindow_sizes 1 %p\n", mwindow);
	cauto_x = 0;
	cauto_y = 0; 
	cauto_w = 0;
	cauto_h = 0;
	ccomposite_x = 0;
	ccomposite_y = 5;
	ccomposite_w = protect_data[0]->get_w();
	ccomposite_h = mwindow->session->cwindow_h - ccomposite_y;
	ccanvas_x = ccomposite_x + ccomposite_w;
	ccanvas_y = 0;
//printf("Theme::get_cwindow_sizes 1\n");


	if(mwindow->edl->session->cwindow_meter)
	{
		cmeter_x = mwindow->session->cwindow_w - MeterPanel::get_meters_width(mwindow->edl->session->audio_channels, 
			mwindow->edl->session->cwindow_meter);
		ccanvas_w = cmeter_x - ccanvas_x - 5;
	}
	else
	{
		cmeter_x = mwindow->session->cwindow_w;
		ccanvas_w = cmeter_x - ccanvas_x;
	}
//printf("Theme::get_cwindow_sizes 1\n");

	cmeter_y = 10;
	cmeter_h = mwindow->session->cwindow_h - cmeter_y;
	cedit_x = 10;
	cedit_y = mwindow->session->cwindow_h - autokeyframe_data[0]->get_h();
//printf("Theme::get_cwindow_sizes 1\n");
	ctransport_x = 10;
	ctransport_y = cedit_y - forward_data[0]->get_h();
	cslider_x = 5;
	cslider_y = ctransport_y - 
		BC_WindowBase::get_resources()->horizontal_slider_data[0]->get_h();
	cslider_w = ccanvas_x + ccanvas_w - cslider_x - cslider_x;
	ccanvas_h = cslider_y - 5;
//printf("Theme::get_cwindow_sizes 1\n");
	ctime_x = ctransport_x + PlayTransport::get_transport_width(mwindow);
	ctime_y = ctransport_y;
//	czoom_x = ctime_x + 150;
	czoom_x = ctransport_x + PlayTransport::get_transport_width(mwindow) + 20;
//printf("Theme::get_cwindow_sizes 1\n");
	czoom_y = ctime_y;
	cdest_x = czoom_x;
	cdest_y = czoom_y + 30;
//printf("Theme::get_cwindow_sizes 2\n");
}


void MicroTheme::get_vwindow_sizes(VWindowGUI *gui)
{
	vcanvas_x = 0;
	vcanvas_y = 0;
	if(mwindow->edl->session->vwindow_meter)
	{
		vmeter_x = mwindow->session->vwindow_w - 
			MeterPanel::get_meters_width(mwindow->edl->session->audio_channels, 
				mwindow->edl->session->vwindow_meter);
		vcanvas_w = vmeter_x - vcanvas_x - 5;
	}
	else
	{
		vmeter_x = mwindow->session->vwindow_w;
		vcanvas_w = mwindow->session->vwindow_w;
	}

	vedit_x = 5;
	vedit_y = mwindow->session->vwindow_h - autokeyframe_data[0]->get_h();
	vtransport_x = 5;
	vtransport_y = vedit_y - forward_data[0]->get_h();
	vslider_x = 5;
	vslider_y = vtransport_y - 
		BC_WindowBase::get_resources()->horizontal_slider_data[0]->get_h();
	vslider_w = vcanvas_w - vslider_x - 5;
	vtimebar_x = 0;
	vtimebar_y = vslider_y - 20;
	vtimebar_w = vcanvas_w - vcanvas_x;


	vcanvas_h = vtimebar_y;

	vmeter_y = 5;
	vmeter_h = mwindow->session->vwindow_h - cmeter_y;



	vtime_x = vtransport_x + PlayTransport::get_transport_width(mwindow) + 5;
	vtime_y = vtransport_y;
	vtime_w = 150;
	vzoom_x = vtime_x + 150;
	vzoom_y = vtime_y;
	vsource_x = vtime_x;
	vsource_y = vedit_y;
}


#define PATCHBAY_W 100
#define ZOOM_H 20
void MicroTheme::get_mwindow_sizes(MWindowGUI *gui, int w, int h)
{
	mbuttons_x = 0;
	mbuttons_y = gui->mainmenu->get_h();
	mbuttons_w = w;
	mbuttons_h = arrow_data[0]->get_h() + 4;
	mclock_x = 0;
	mclock_y = mbuttons_y + mbuttons_h;
	mclock_w = PATCHBAY_W - 10;
	mclock_h = timebar_bg_data->get_h();
	mtimebar_x = PATCHBAY_W;
	mtimebar_y = mclock_y;
	mtimebar_w = w - mtimebar_x;
	mtimebar_h = timebar_bg_data->get_h();
	mstatus_x = 0;
	mstatus_y = h - statusbar_cancel_data[0]->get_h();
	mstatus_w = w;
	mstatus_h = statusbar_cancel_data[0]->get_h();
	mstatus_progress_x = mstatus_w - statusbar_cancel_data[0]->get_w() - 240;
	mstatus_progress_y = mstatus_h - BC_WindowBase::get_resources()->progress_images[0]->get_h();
	mstatus_progress_w = 230;
	mstatus_cancel_x = mstatus_w - statusbar_cancel_data[0]->get_w();
	mstatus_cancel_y = mstatus_h - statusbar_cancel_data[0]->get_h();
	mzoom_x = 0;
	mzoom_y = mstatus_y - ZOOM_H;
	mzoom_h = ZOOM_H;
	mzoom_w = w;
	patchbay_x = 0;
	patchbay_y = mtimebar_y + mtimebar_h;
	patchbay_w = PATCHBAY_W;
	patchbay_h = mzoom_y - patchbay_y;
	mcanvas_x = patchbay_x + patchbay_w;
	mcanvas_y = patchbay_y;
	mcanvas_w = w - patchbay_w;
	mcanvas_h = patchbay_h;
}

void MicroTheme::get_recordgui_sizes(RecordGUI *gui, int w, int h)
{
	recordgui_status_x = 5;
	recordgui_status_y = 5;
	recordgui_status_x2 = 100;
	recordgui_batch_x = 220;
	recordgui_batch_y = 5;
	recordgui_batchcaption_x = recordgui_batch_x + 70;


	recordgui_transport_x = recordgui_batch_x;
	recordgui_transport_y = recordgui_batch_y + 150;

	recordgui_buttons_x = 220;
	recordgui_buttons_y = recordgui_transport_y + 30;
	recordgui_options_x = 220;
	recordgui_options_y = recordgui_buttons_y + 30;

	recordgui_batches_x = 10;
	recordgui_batches_y = 250;
	recordgui_batches_w = w - 20;
	recordgui_batches_h = h - recordgui_batches_y - 70;
	recordgui_loadmode_x = w / 2 - loadmode_w / 2;
	recordgui_loadmode_y = h - 50;

	recordgui_controls_x = 10;
	recordgui_controls_y = h - 30;
}









