#include "clip.h"
#include "cwindowgui.h"
#include "defaulttheme.h"
#include "edl.h"
#include "edlsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "patchbay.h"
#include "preferencesthread.h"
#include "recordgui.h"
#include "recordmonitor.h"
#include "setformat.h"
#include "statusbar.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "vwindowgui.h"
#include "zoombar.h"




PluginClient* new_plugin(PluginServer *server)
{
	return new DefaultThemeMain(server);
}







DefaultThemeMain::DefaultThemeMain(PluginServer *server)
 : PluginTClient(server)
{
}

DefaultThemeMain::~DefaultThemeMain()
{
}

char* DefaultThemeMain::plugin_title()
{
	return "Blond";
}

Theme* DefaultThemeMain::new_theme()
{
	return theme = new DefaultTheme;
}








DefaultTheme::DefaultTheme()
 : Theme()
{
}

DefaultTheme::~DefaultTheme()
{
}

void DefaultTheme::initialize()
{

	static VFrame *default_button_images[] = 
	{
		new VFrame(get_image("generic_up.png")), new VFrame(get_image("generic_hi.png")), new VFrame(get_image("generic_dn.png"))
	};
	BC_WindowBase::get_resources()->generic_button_images = default_button_images;

	static VFrame *default_hslider_data[] = 
	{
		new VFrame(get_image("hslider_fg_up.png")),
		new VFrame(get_image("hslider_fg_hi.png")),
		new VFrame(get_image("hslider_fg_dn.png")),
		new VFrame(get_image("hslider_bg_up.png")),
		new VFrame(get_image("hslider_bg_hi.png")),
		new VFrame(get_image("hslider_bg_dn.png")),
	};
	BC_WindowBase::get_resources()->horizontal_slider_data = default_hslider_data;


	static VFrame* default_progress_images[] = 
	{
		new VFrame(get_image("progress_bg.png")),
		new VFrame(get_image("progress_hi.png"))
	};
	BC_WindowBase::get_resources()->progress_images = default_progress_images;

	static VFrame* default_tumbler_data[] = 
	{
		new VFrame(get_image("tumble_up.png")),
		new VFrame(get_image("tumble_hi.png")),
		new VFrame(get_image("tumble_botdn.png")),
		new VFrame(get_image("tumble_topdn.png"))
	};
	BC_WindowBase::get_resources()->tumble_data = default_tumbler_data;

	static VFrame* default_listbox_data[] =
	{
		new VFrame(get_image("listbox_button_up.png")),
		new VFrame(get_image("listbox_button_hi.png")),
		new VFrame(get_image("listbox_button_dn.png"))
	};
	BC_WindowBase::get_resources()->listbox_button = default_listbox_data;


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
	BC_WindowBase::get_resources()->pan_text_color = WHITE;

	static VFrame* default_hscroll_data[] = 
	{
		new VFrame(get_image("hscroll_center_up.png")),
		new VFrame(get_image("hscroll_center_hi.png")),
		new VFrame(get_image("hscroll_center_dn.png")),
		new VFrame(get_image("hscroll_bg.png")),
		new VFrame(get_image("hscroll_back_up.png")),
		new VFrame(get_image("hscroll_back_hi.png")),
		new VFrame(get_image("hscroll_back_dn.png")),
		new VFrame(get_image("hscroll_fwd_up.png")),
		new VFrame(get_image("hscroll_fwd_hi.png")),
		new VFrame(get_image("hscroll_fwd_dn.png")),
	};
	BC_WindowBase::get_resources()->hscroll_data = default_hscroll_data;

	static VFrame* default_vscroll_data[] = 
	{
		new VFrame(get_image("vscroll_center_up.png")),
		new VFrame(get_image("vscroll_center_hi.png")),
		new VFrame(get_image("vscroll_center_dn.png")),
		new VFrame(get_image("vscroll_bg.png")),
		new VFrame(get_image("vscroll_back_up.png")),
		new VFrame(get_image("vscroll_back_hi.png")),
		new VFrame(get_image("vscroll_back_dn.png")),
		new VFrame(get_image("vscroll_fwd_up.png")),
		new VFrame(get_image("vscroll_fwd_hi.png")),
		new VFrame(get_image("vscroll_fwd_dn.png")),
	};
	BC_WindowBase::get_resources()->vscroll_data = default_vscroll_data;

	build_button(BC_WindowBase::get_resources()->ok_images,
		get_image("ok.png"), 
		default_button_images[0],
		default_button_images[1],
		default_button_images[2]);

	build_button(BC_WindowBase::get_resources()->cancel_images,
		get_image("cancel.png"), 
		default_button_images[0],
		default_button_images[1],
		default_button_images[2]);


// Record windows
	rgui_batch = new VFrame(get_image("recordgui_batch.png"));
	rgui_controls = new VFrame(get_image("recordgui_controls.png"));
	rgui_list = new VFrame(get_image("recordgui_list.png"));
	rmonitor_panel = new VFrame(get_image("recordmonitor_panel.png"));
	rmonitor_meters = new VFrame(get_image("recordmonitor_meters.png"));


// MWindow
	mbutton_left = new VFrame(get_image("mbutton_left.png"));
	mbutton_right = new VFrame(get_image("mbutton_right.png"));
	timebar_bg_data = new VFrame(get_image("timebar_bg.png"));
	timebar_brender_data = new VFrame(get_image("timebar_brender.png"));
	clock_bg = new VFrame(get_image("mclock.png"));
	patchbay_bg = new VFrame(get_image("patchbay_bg.png"));
	tracks_bg = new VFrame(get_image("tracks_bg.png"));
	zoombar_left = new VFrame(get_image("zoombar_left.png"));
	zoombar_right = new VFrame(get_image("zoombar_right.png"));
	statusbar_left = new VFrame(get_image("statusbar_left.png"));
	statusbar_right = new VFrame(get_image("statusbar_right.png"));

// CWindow
	cpanel_bg = new VFrame(get_image("cpanel_bg.png"));
	cbuttons_left = new VFrame(get_image("cbuttons_left.png"));
	cbuttons_right = new VFrame(get_image("cbuttons_right.png"));
	cmeter_bg = new VFrame(get_image("cmeter_bg.png"));

// VWindow
	vbuttons_left = new VFrame(get_image("vbuttons_left.png"));
	vbuttons_right = new VFrame(get_image("vbuttons_right.png"));
	vmeter_bg = new VFrame(get_image("vmeter_bg.png"));

	preferences_bg = new VFrame(get_image("preferences_bg.png"));


	new_bg = new VFrame(get_image("new_bg.png"));
	setformat_bg = new VFrame(get_image("setformat_bg.png"));


	timebar_view_data = new VFrame(get_image("timebar_view.png"));

	setformat_x1 = 15;
	setformat_x2 = 100;

	setformat_x3 = 315;
	setformat_x4 = 415;
	setformat_y1 = 20;
	setformat_margin = 30;
	setformat_w = 600;
	setformat_h = 480;
	setformat_channels_x = 25;
	setformat_channels_y = 173;
	setformat_channels_w = 250;
	setformat_channels_h = 250;

	loadfile_pad = 70;
	browse_pad = 20;





	build_icons();
	build_bg_data();
	build_patches();
	build_overlays();



	VFrame editpanel_up(get_image("editpanel_up.png"));
	VFrame editpanel_hi(get_image("editpanel_hi.png"));
	VFrame editpanel_dn(get_image("editpanel_dn.png"));
	VFrame editpanel_checked(get_image("editpanel_checked.png"));
	VFrame editpanel_checkedhi(get_image("editpanel_checkedhi.png"));

	static VFrame *default_outpoint[] = { new VFrame(get_image("out_up.png")), new VFrame(get_image("out_hi.png")), new VFrame(get_image("out_checked.png")), new VFrame(get_image("out_dn.png")), new VFrame(get_image("out_checkedhi.png")) };
	static VFrame *default_labeltoggle[] = { new VFrame(get_image("labeltoggle_up.png")), new VFrame(get_image("labeltoggle_uphi.png")), new VFrame(get_image("label_checked.png")), new VFrame(get_image("labeltoggle_dn.png")), new VFrame(get_image("label_checkedhi.png")) };
	static VFrame *default_inpoint[] = { new VFrame(get_image("in_up.png")), new VFrame(get_image("in_hi.png")), new VFrame(get_image("in_checked.png")), new VFrame(get_image("in_dn.png")), new VFrame(get_image("in_checkedhi.png")) };
	static VFrame *default_wrench_data[] = { new VFrame(get_image("wrench_up.png")), new VFrame(get_image("wrench_hi.png")), new VFrame(get_image("wrench_dn.png")) };
	static VFrame *transport_bg[] = { new VFrame(get_image("transportup.png")), new VFrame(get_image("transporthi.png")), new VFrame(get_image("transportdn.png")) };

	static VFrame *default_statusbar_cancel[] = 
	{
		new VFrame(get_image("statusbar_cancel_up.png")),
		new VFrame(get_image("statusbar_cancel_hi.png")),
		new VFrame(get_image("statusbar_cancel_dn.png"))
	};
	statusbar_cancel_data = default_statusbar_cancel;


	build_button(bottom_justify, get_image("bottom_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(center_justify, get_image("center_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(channel_data, get_image("channel.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(copy_data, get_image("copy.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(cut_data, get_image("cut.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(fit_data, get_image("fit.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(in_data, get_image("inpoint.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(indelete_data, get_image("clearinpoint.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(labelbutton_data, get_image("label.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(left_justify, get_image("left_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(magnify_button_data, get_image("magnify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(middle_justify, get_image("middle_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(nextlabel_data, get_image("nextlabel.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(out_data, get_image("outpoint.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(outdelete_data, get_image("clearoutpoint.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(over_button, get_image("over.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(overwrite_data, get_image("overwrite.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(paste_data, get_image("paste.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(prevlabel_data, get_image("prevlabel.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(redo_data, get_image("redo.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(right_justify, get_image("right_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(splice_data, get_image("splice.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(toclip_data, get_image("toclip.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(top_justify, get_image("top_justify.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(undo_data, get_image("undo.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
	build_button(wrench_data, get_image("wrench.png"), &editpanel_up, &editpanel_hi, &editpanel_dn);
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
	in_point = default_inpoint;
	label_toggle = default_labeltoggle;
	out_point = default_outpoint;
	flush_images();

	title_font = MEDIUMFONT_3D;
	title_color = WHITE;
	recordgui_fixed_color = YELLOW;
	recordgui_variable_color = RED;

	channel_position_color = MEYELLOW;
	BC_WindowBase::get_resources()->meter_title_w = 25;
}

#define CWINDOW_METER_MARGIN 5
#define VWINDOW_METER_MARGIN 5

void DefaultTheme::get_mwindow_sizes(MWindowGUI *gui, int w, int h)
{
	mbuttons_x = 0;
	mbuttons_y = gui->mainmenu->get_h();
	mbuttons_w = w;
	mbuttons_h = mbutton_left->get_h();
	mclock_x = 0;
	mclock_y = mbuttons_y + mbuttons_h + CWINDOW_METER_MARGIN;
	mclock_w = clock_bg->get_w() - 30;
	mclock_h = clock_bg->get_h();
	mtimebar_x = patchbay_bg->get_w();
	mtimebar_y = mbuttons_y + mbuttons_h;
	mtimebar_w = w - mtimebar_x;
	mtimebar_h = timebar_bg_data->get_h();
	mstatus_x = 0;
	mstatus_y = h - statusbar_left->get_h();
	mstatus_w = w;
	mstatus_h = statusbar_left->get_h();
	mstatus_message_x = 10;
	mstatus_message_y = 5;
	mstatus_progress_x = mstatus_w - statusbar_cancel_data[0]->get_w() - 240;
	mstatus_progress_y = mstatus_h - BC_WindowBase::get_resources()->progress_images[0]->get_h();
	mstatus_progress_w = 230;
	mstatus_cancel_x = mstatus_w - statusbar_cancel_data[0]->get_w();
	mstatus_cancel_y = mstatus_h - statusbar_cancel_data[0]->get_h();
	mzoom_x = 0;
	mzoom_y = mstatus_y - zoombar_left->get_h();
	mzoom_h = zoombar_left->get_h();
	mzoom_w = w;
	patchbay_x = 0;
	patchbay_y = mtimebar_y + mtimebar_h;
	patchbay_w = patchbay_bg->get_w();
	patchbay_h = mzoom_y - patchbay_y;
	mcanvas_x = patchbay_x + patchbay_w;
	mcanvas_y = mtimebar_y + mtimebar_h;
	mcanvas_w = w - patchbay_w;
	mcanvas_h = patchbay_h;
}

void DefaultTheme::get_cwindow_sizes(CWindowGUI *gui)
{
	ccomposite_x = 0;
	ccomposite_y = 5;
	ccomposite_w = cpanel_bg->get_w();
	ccomposite_h = mwindow->session->cwindow_h - cbuttons_left->get_h();
	cslider_x = 5;
	cslider_y = ccomposite_h + 23;
	cedit_x = 10;
	cedit_y = cslider_y + 17;
	ctransport_x = 10;
	ctransport_y = mwindow->session->cwindow_h - autokeyframe_data[0]->get_h();
	czoom_x = ctransport_x + PlayTransport::get_transport_width(mwindow) + 20;
	czoom_y = ctransport_y + 5;
	ccanvas_x = ccomposite_x + ccomposite_w;
	ccanvas_y = 0;
	ccanvas_h = ccomposite_h;


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
	cmeter_y = 5;
	cmeter_h = mwindow->session->cwindow_h - cmeter_y;

	cslider_w = ccanvas_x + ccanvas_w - cslider_x;
	ctimebar_x = ccanvas_x;
	ctimebar_y = ccanvas_y + ccanvas_h;
	ctimebar_w = ccanvas_w;
	ctimebar_h = 16;

	ctime_x = ctransport_x + PlayTransport::get_transport_width(mwindow);
	ctime_y = ctransport_y;
	cdest_x = czoom_x;
	cdest_y = czoom_y + 30;
}



void DefaultTheme::get_recordgui_sizes(RecordGUI *gui, int w, int h)
{
	recordgui_status_x = 10;
	recordgui_status_y = 10;
	recordgui_status_x2 = 160;
	recordgui_batch_x = 310;
	recordgui_batch_y = 10;
	recordgui_batchcaption_x = recordgui_batch_x + 110;


	recordgui_transport_x = recordgui_batch_x;
	recordgui_transport_y = recordgui_batch_y + 150;

	recordgui_buttons_x = recordgui_batch_x - 50;
	recordgui_buttons_y = recordgui_transport_y + 40;
	recordgui_options_x = recordgui_buttons_x;
	recordgui_options_y = recordgui_buttons_y + 35;

	recordgui_batches_x = 10;
	recordgui_batches_y = 270;
	recordgui_batches_w = w - 20;
	recordgui_batches_h = h - recordgui_batches_y - 70;
	recordgui_loadmode_x = w / 2 - loadmode_w / 2;
	recordgui_loadmode_y = h - 60;

	recordgui_controls_x = 10;
	recordgui_controls_y = h - 40;
}



void DefaultTheme::get_vwindow_sizes(VWindowGUI *gui)
{
	vmeter_y = 5;
	vmeter_h = mwindow->session->vwindow_h - cmeter_y;
	vcanvas_x = 0;
	vcanvas_y = 0;
	vcanvas_h = mwindow->session->vwindow_h - vbuttons_left->get_h();

	if(mwindow->edl->session->vwindow_meter)
	{
		vmeter_x = mwindow->session->vwindow_w - 
			VWINDOW_METER_MARGIN - 
			MeterPanel::get_meters_width(mwindow->edl->session->audio_channels, 
			mwindow->edl->session->vwindow_meter);
		vcanvas_w = vmeter_x - vcanvas_x - VWINDOW_METER_MARGIN;
	}
	else
	{
		vmeter_x = mwindow->session->vwindow_w;
		vcanvas_w = mwindow->session->vwindow_w;
	}

	vtimebar_x = vcanvas_x;
	vtimebar_y = vcanvas_y + vcanvas_h;
	vtimebar_w = vcanvas_w;
	vtimebar_h = 16;

	vslider_x = 10;
	vslider_y = vtimebar_y + 25;
	vslider_w = vtimebar_w - vslider_x;
	vedit_x = 10;
	vedit_y = vslider_y + 17;
	vtransport_x = 10;
	vtransport_y = mwindow->session->vwindow_h - autokeyframe_data[0]->get_h();
	vtime_x = 370;
	vtime_y = vedit_y + 10;
	vtime_w = 150;




	vzoom_x = vtime_x + 150;
	vzoom_y = vtime_y;
	vsource_x = vtime_x + 50;
	vsource_y = vtransport_y + 5;
}





void DefaultTheme::build_icons()
{
	mwindow_icon = new VFrame(get_image("heroine_icon.png"));
	vwindow_icon = new VFrame(get_image("heroine_icon.png"));
	cwindow_icon = new VFrame(get_image("heroine_icon.png"));
	awindow_icon = new VFrame(get_image("heroine_icon.png"));
	record_icon = new VFrame(get_image("heroine_icon.png"));
	clip_icon = new VFrame(get_image("clip_icon.png"));
}



void DefaultTheme::build_bg_data()
{
// Audio settings
	channel_bg_data = new VFrame(get_image("channel_bg.png"));
	channel_position_data = new VFrame(get_image("channel_position.png"));

// Track bitmaps
	resource1024_bg_data = new VFrame(get_image("resource1024.png"));
	resource512_bg_data = new VFrame(get_image("resource512.png"));
	resource256_bg_data = new VFrame(get_image("resource256.png"));
	resource128_bg_data = new VFrame(get_image("resource128.png"));
	resource64_bg_data = new VFrame(get_image("resource64.png"));
	resource32_bg_data = new VFrame(get_image("resource32.png"));
	plugin_bg_data = new VFrame(get_image("plugin_bg.png"));
	title_bg_data = new VFrame(get_image("title_bg.png"));
	vtimebar_bg_data = new VFrame(get_image("vwindow_timebar.png"));
}


void DefaultTheme::build_patches()
{
	static VFrame *default_drawpatch_data[] = { new VFrame(get_image("drawpatch_up.png")), new VFrame(get_image("drawpatch_hi.png")), new VFrame(get_image("drawpatch_checked.png")), new VFrame(get_image("drawpatch_dn.png")), new VFrame(get_image("drawpatch_checkedhi.png")) };
	static VFrame *default_expandpatch_data[] = { new VFrame(get_image("expandpatch_up.png")), new VFrame(get_image("expandpatch_hi.png")), new VFrame(get_image("expandpatch_checked.png")), new VFrame(get_image("expandpatch_dn.png")), new VFrame(get_image("expandpatch_checkedhi.png")) };
	static VFrame *default_gangpatch_data[] = { new VFrame(get_image("gangpatch_up.png")), new VFrame(get_image("gangpatch_hi.png")), new VFrame(get_image("gangpatch_checked.png")), new VFrame(get_image("gangpatch_dn.png")), new VFrame(get_image("gangpatch_checkedhi.png")) };
	static VFrame *default_mutepatch_data[] = { new VFrame(get_image("mutepatch_up.png")), new VFrame(get_image("mutepatch_hi.png")), new VFrame(get_image("mutepatch_checked.png")), new VFrame(get_image("mutepatch_dn.png")), new VFrame(get_image("mutepatch_checkedhi.png")) };
	static VFrame *default_patchbay_bg = new VFrame(get_image("patchbay_bg.png"));
	static VFrame *default_playpatch_data[] = { new VFrame(get_image("playpatch_up.png")), new VFrame(get_image("playpatch_hi.png")), new VFrame(get_image("playpatch_checked.png")), new VFrame(get_image("playpatch_dn.png")), new VFrame(get_image("playpatch_checkedhi.png")) };
	static VFrame *default_recordpatch_data[] = { new VFrame(get_image("recordpatch_up.png")), new VFrame(get_image("recordpatch_hi.png")), new VFrame(get_image("recordpatch_checked.png")), new VFrame(get_image("recordpatch_dn.png")), new VFrame(get_image("recordpatch_checkedhi.png")) };


	drawpatch_data = default_drawpatch_data;
	expandpatch_data = default_expandpatch_data;
	gangpatch_data = default_gangpatch_data;
	mutepatch_data = default_mutepatch_data;
	patchbay_bg = default_patchbay_bg;
	playpatch_data = default_playpatch_data;
	recordpatch_data = default_recordpatch_data;
}

void DefaultTheme::build_overlays()
{
	keyframe_data = new VFrame(get_image("keyframe3.png"));
	camerakeyframe_data = new VFrame(get_image("camerakeyframe.png"));
	maskkeyframe_data = new VFrame(get_image("maskkeyframe.png"));
	modekeyframe_data = new VFrame(get_image("modekeyframe.png"));
	pankeyframe_data = new VFrame(get_image("pankeyframe.png"));
	projectorkeyframe_data = new VFrame(get_image("projectorkeyframe.png"));
}









void DefaultTheme::draw_rwindow_bg(RecordGUI *gui)
{
	int y;
	int margin = 50;
	int margin2 = 80;
	gui->draw_9segment(recordgui_batch_x - margin,
		0,
		mwindow->session->rwindow_w - recordgui_status_x + margin,
		recordgui_buttons_y,
		rgui_batch);
	gui->draw_3segmenth(recordgui_options_x - margin2,
		recordgui_buttons_y - 5,
		mwindow->session->rwindow_w - recordgui_options_x + margin2,
		rgui_controls);
	y = recordgui_buttons_y - 5 + rgui_controls->get_h();
	gui->draw_9segment(0,
		y,
		mwindow->session->rwindow_w,
		mwindow->session->rwindow_h - y,
		rgui_list);
}

void DefaultTheme::draw_rmonitor_bg(RecordMonitorGUI *gui)
{
	int margin = 35;
	int panel_w = 300;
	int x = rmonitor_canvas_w - margin;
	gui->draw_9segment(x,
		0,
		mwindow->session->rmonitor_w - x,
		mwindow->session->rmonitor_h,
		rmonitor_meters);
}






void DefaultTheme::draw_mwindow_bg(MWindowGUI *gui)
{
// Button bar
	gui->draw_3segmenth(mbuttons_x, 
		mbuttons_y, 
		750, 
		mbutton_left);
	gui->draw_3segmenth(mbuttons_x + 750, 
		mbuttons_y, 
		mbuttons_w - 500, 
		mbutton_right);

// Clock
	gui->draw_3segmenth(0, 
		mbuttons_y + mbutton_left->get_h(),
		patchbay_bg->get_w(), 
		clock_bg);

// Patchbay
	gui->draw_3segmentv(patchbay_x, 
		patchbay_y, 
		patchbay_h + 10, 
		patchbay_bg);

// Track canvas
	gui->draw_9segment(mcanvas_x, 
		mcanvas_y, 
		mcanvas_w, 
		patchbay_h + 10, 
		tracks_bg);

// Timebar
	gui->draw_3segmenth(mtimebar_x, 
		mtimebar_y, 
		mtimebar_w, 
		timebar_bg_data);

// Zoombar
	int zoombar_center = 710;
	gui->draw_3segmenth(mzoom_x, 
		mzoom_y,
		zoombar_center, 
		zoombar_left);
	if(mzoom_w > zoombar_center)
		gui->draw_3segmenth(mzoom_x + zoombar_center, 
			mzoom_y, 
			mzoom_w - zoombar_center, 
			zoombar_right);

// Status
	gui->draw_3segmenth(mstatus_x, 
		mstatus_y,
		zoombar_center, 
		statusbar_left);

	if(mstatus_w > zoombar_center)
		gui->draw_3segmenth(mstatus_x + zoombar_center, 
			mstatus_y,
			mstatus_w - zoombar_center, 
			statusbar_right);
}

void DefaultTheme::draw_cwindow_bg(CWindowGUI *gui)
{
	const int button_division = 530;
	gui->draw_3segmentv(0, 0, ccomposite_h, cpanel_bg);
	gui->draw_3segmenth(0, ccomposite_h, button_division, cbuttons_left);
	if(mwindow->edl->session->cwindow_meter)
	{
		gui->draw_3segmenth(button_division, 
			ccomposite_h, 
			cmeter_x - CWINDOW_METER_MARGIN - button_division, 
			cbuttons_right);
		gui->draw_9segment(cmeter_x - CWINDOW_METER_MARGIN, 
			0, 
			mwindow->session->cwindow_w - cmeter_x + CWINDOW_METER_MARGIN, 
			mwindow->session->cwindow_h, 
			cmeter_bg);
	}
	else
	{
		gui->draw_3segmenth(button_division, 
			ccomposite_h, 
			cmeter_x - CWINDOW_METER_MARGIN - button_division + 100, 
			cbuttons_right);
	}
}

void DefaultTheme::draw_vwindow_bg(VWindowGUI *gui)
{
	const int button_division = 400;
	gui->draw_3segmenth(0, vcanvas_h, button_division, vbuttons_left);
	if(mwindow->edl->session->vwindow_meter)
	{
		gui->draw_3segmenth(button_division, 
			vcanvas_h, 
			vmeter_x - VWINDOW_METER_MARGIN - button_division, 
			vbuttons_right);
		gui->draw_9segment(vmeter_x - VWINDOW_METER_MARGIN,
			0,
			mwindow->session->vwindow_w - vmeter_x + VWINDOW_METER_MARGIN, 
			mwindow->session->vwindow_h, 
			vmeter_bg);
	}
	else
	{
		gui->draw_3segmenth(button_division, 
			vcanvas_h, 
			vmeter_x - VWINDOW_METER_MARGIN - button_division + 100, 
			vbuttons_right);
	}
}

void DefaultTheme::get_preferences_sizes()
{
}


void DefaultTheme::draw_preferences_bg(PreferencesWindow *gui)
{
	gui->draw_9segment(0, 0, gui->get_w(), gui->get_h() - 40, preferences_bg);
}

void DefaultTheme::get_new_sizes(NewWindow *gui)
{
}

void DefaultTheme::draw_new_bg(NewWindow *gui)
{
	gui->draw_vframe(new_bg, 0, 0);
}

void DefaultTheme::draw_setformat_bg(SetFormatWindow *gui)
{
	gui->draw_vframe(setformat_bg, 0, 0);
}






