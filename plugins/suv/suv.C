#include "clip.h"
#include "cwindowgui.h"
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
#include "suv.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "vwindowgui.h"
#include "zoombar.h"




PluginClient* new_plugin(PluginServer *server)
{
	return new SUVThemeMain(server);
}







SUVThemeMain::SUVThemeMain(PluginServer *server)
 : PluginTClient(server)
{
}

SUVThemeMain::~SUVThemeMain()
{
}

char* SUVThemeMain::plugin_title()
{
	return "SUV";
}

Theme* SUVThemeMain::new_theme()
{
	theme = new SUVTheme;
	theme->set_path(PluginClient::get_path());
	return theme;
}








SUVTheme::SUVTheme()
 : Theme()
{
}

SUVTheme::~SUVTheme()
{
}

void SUVTheme::initialize()
{
// Set default images
	BC_Resources *resources = BC_WindowBase::get_resources();
	resources->generic_button_images = new_image_set(3, 
			"generic_up.png", 
			"generic_up.png", 
			"generic_up.png");
	resources->horizontal_slider_data = new_image_set(6,
			"hslider_fg_up.png",
			"hslider_fg_up.png",
			"hslider_fg_up.png",
			"hslider_bg_up.png",
			"hslider_bg_up.png",
			"hslider_bg_up.png");
	resources->progress_images = new_image_set(2,
			"progress_bg.png",
			"progress_bg.png");
	resources->tumble_data = new_image_set(4,
		"tumble_up.png",
		"tumble_hi.png",
		"tumble_hi.png",
		"tumble_hi.png");
	resources->listbox_button = new_image_set(3,
		"listbox_button_up.png",
		"listbox_button_hi.png",
		"listbox_button_dn.png");
	resources->listbox_column = new_image_set(3,
		"listbox_column_up.png",
		"listbox_column_hi.png",
		"listbox_column_hi.png");
	resources->pan_data = new_image_set(7,
			"pan_up.png", 
			"pan_hi.png", 
			"pan_popup.png", 
			"pan_channel.png", 
			"pan_stick.png", 
			"pan_channel_small.png", 
			"pan_stick_small.png");
	resources->pan_text_color = WHITE;

	resources->hscroll_data = new_image_set(10,
			"hscroll_center_up.png",
			"hscroll_center_up.png",
			"hscroll_center_up.png",
			"hscroll_bg.png",
			"hscroll_back_up.png",
			"hscroll_back_up.png",
			"hscroll_back_up.png",
			"hscroll_fwd_up.png",
			"hscroll_fwd_up.png",
			"hscroll_fwd_up.png");

	resources->vscroll_data = new_image_set(10,
			"vscroll_center_up.png",
			"vscroll_center_up.png",
			"vscroll_center_up.png",
			"vscroll_bg.png",
			"vscroll_back_up.png",
			"vscroll_back_up.png",
			"vscroll_back_up.png",
			"vscroll_fwd_up.png",
			"vscroll_fwd_up.png",
			"vscroll_fwd_up.png");

	resources->ok_images = new_button("ok.png", 
			"generic_up.png",
			"generic_hi.png",
			"generic_dn.png");

	resources->cancel_images = new_button("cancel.png", 
			"generic_up.png",
			"generic_hi.png",
			"generic_dn.png");

	resources->popup_title_text = WHITE;
	resources->menu_item_text = WHITE;
	resources->progress_text = WHITE;

// Window backgrounds
// Record windows
	rgui_batch = new_image("recordgui_batch.png");
	rgui_controls = new_image("recordgui_controls.png");
	rgui_list = new_image("recordgui_list.png");
	rmonitor_panel = new_image("recordmonitor_panel.png");
	rmonitor_meters = new_image("recordmonitor_meters.png");


// MWindow
	mbutton_left = new_image("mbutton_left.png");
	mbutton_right = new_image("mbutton_right.png");
	timebar_bg_data = new_image("timebar_bg.png");
	timebar_brender_data = new_image("timebar_brender.png");
	clock_bg = new_image("mclock.png");
	patchbay_bg = new_image("patchbay_bg.png");
	tracks_bg = new_image("tracks_bg.png");
	zoombar_left = new_image("zoombar_left.png");
	zoombar_right = new_image("zoombar_right.png");
	statusbar_left = new_image("statusbar_left.png");
	statusbar_right = new_image("statusbar_right.png");

// CWindow
	cpanel_bg = new_image("cpanel_bg.png");
	cbuttons_left = new_image("cbuttons_left.png");
	cbuttons_right = new_image("cbuttons_right.png");
	cmeter_bg = new_image("cmeter_bg.png");

// VWindow
	vbuttons_left = new_image("vbuttons_left.png");
	vbuttons_right = new_image("vbuttons_right.png");
	vmeter_bg = new_image("vmeter_bg.png");

	preferences_bg = new_image("preferences_bg.png");


	new_bg = new_image("new_bg.png");
	setformat_bg = new_image("setformat_bg.png");


	timebar_view_data = new_image("timebar_view.png");


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




// Icons
	build_icons();
	build_bg_data();
	build_patches();
	build_overlays();



	out_point = new_image_set(5,
		"out_up.png", 
		"out_hi.png", 
		"out_checked.png", 
		"out_dn.png", 
		"out_checkedhi.png");
	in_point = new_image_set(5,
		"in_up.png", 
		"in_hi.png", 
		"in_checked.png", 
		"in_dn.png", 
		"in_checkedhi.png");

	label_toggle = new_image_set(5,
		"labeltoggle_up.png", 
		"labeltoggle_uphi.png", 
		"label_checked.png", 
		"labeltoggle_dn.png", 
		"label_checkedhi.png");


	statusbar_cancel_data = new_image_set(3,
		"statusbar_cancel_up.png",
		"statusbar_cancel_hi.png",
		"statusbar_cancel_dn.png");


	VFrame *editpanel_up = new_image("editpanel_up.png");
	VFrame *editpanel_hi = new_image("editpanel_hi.png");
	VFrame *editpanel_dn = new_image("editpanel_dn.png");
	VFrame *editpanel_checked = new_image("editpanel_checked.png");
	VFrame *editpanel_checkedhi = new_image("editpanel_checkedhi.png");

	bottom_justify = new_button("bottom_justify.png", editpanel_up, editpanel_hi, editpanel_dn);
	center_justify = new_button("center_justify.png", editpanel_up, editpanel_hi, editpanel_dn);
	channel_data = new_button("channel.png", editpanel_up, editpanel_hi, editpanel_dn);
	copy_data = new_button("copy.png", editpanel_up, editpanel_hi, editpanel_dn);
	cut_data = new_button("cut.png", editpanel_up, editpanel_hi, editpanel_dn);
	fit_data = new_button("fit.png", editpanel_up, editpanel_hi, editpanel_dn);
	in_data = new_button("inpoint.png", editpanel_up, editpanel_hi, editpanel_dn);
	indelete_data = new_button("clearinpoint.png", editpanel_up, editpanel_hi, editpanel_dn);
	labelbutton_data = new_button("label.png", editpanel_up, editpanel_hi, editpanel_dn);
	left_justify = new_button("left_justify.png", editpanel_up, editpanel_hi, editpanel_dn);
	magnify_button_data = new_button("magnify.png", editpanel_up, editpanel_hi, editpanel_dn);
	middle_justify = new_button("middle_justify.png", editpanel_up, editpanel_hi, editpanel_dn);
	nextlabel_data = new_button("nextlabel.png", editpanel_up, editpanel_hi, editpanel_dn);
	out_data = new_button("outpoint.png", editpanel_up, editpanel_hi, editpanel_dn);
	outdelete_data = new_button("clearoutpoint.png", editpanel_up, editpanel_hi, editpanel_dn);
	over_button = new_button("over.png", editpanel_up, editpanel_hi, editpanel_dn);
	overwrite_data = new_button("overwrite.png", editpanel_up, editpanel_hi, editpanel_dn);
	paste_data = new_button("paste.png", editpanel_up, editpanel_hi, editpanel_dn);
	prevlabel_data = new_button("prevlabel.png", editpanel_up, editpanel_hi, editpanel_dn);
	redo_data = new_button("redo.png", editpanel_up, editpanel_hi, editpanel_dn);
	right_justify = new_button("right_justify.png", editpanel_up, editpanel_hi, editpanel_dn);
	splice_data = new_button("splice.png", editpanel_up, editpanel_hi, editpanel_dn);
	toclip_data = new_button("toclip.png", editpanel_up, editpanel_hi, editpanel_dn);
	top_justify = new_button("top_justify.png", editpanel_up, editpanel_hi, editpanel_dn);
	undo_data = new_button("undo.png", editpanel_up, editpanel_hi, editpanel_dn);
	wrench_data = new_button("wrench.png", editpanel_up, editpanel_hi, editpanel_dn);


	new_image_set("batch_render_start",
		3,
		"batchstart_up.png",
		"batchstart_hi.png",
		"batchstart_dn.png");
	new_image_set("batch_render_stop",
		3,
		"batchstop_up.png",
		"batchstop_hi.png",
		"batchstop_dn.png");
	new_image_set("batch_render_cancel",
		3,
		"batchcancel_up.png",
		"batchcancel_hi.png",
		"batchcancel_dn.png");

	arrow_data = new_toggle("arrow.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	autokeyframe_data = new_toggle("autokeyframe.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	camera_data = new_toggle("camera.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	crop_data = new_toggle("crop.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	ibeam_data = new_toggle("ibeam.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	magnify_data = new_toggle("magnify.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	mask_data = new_toggle("mask.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	proj_data = new_toggle("projector.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	protect_data = new_toggle("protect.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	show_meters = new_toggle("show_meters.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	titlesafe_data = new_toggle("titlesafe.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);
	tool_data = new_toggle("toolwindow.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi);




	static VFrame **transport_bg = new_image_set(3,
		"transportup.png", 
		"transporthi.png", 
		"transportdn.png");
	build_transport(duplex_data, get_image_data("duplex.png"), transport_bg, 1);
	build_transport(end_data, get_image_data("end.png"), transport_bg, 2);
	build_transport(fastfwd_data, get_image_data("fastfwd.png"), transport_bg, 1);
	build_transport(fastrev_data, get_image_data("fastrev.png"), transport_bg, 1);
	build_transport(forward_data, get_image_data("play.png"), transport_bg, 1);
	build_transport(framefwd_data, get_image_data("framefwd.png"), transport_bg, 1);
	build_transport(framefwd_data, get_image_data("framefwd.png"), transport_bg, 1);
	build_transport(framerev_data, get_image_data("framerev.png"), transport_bg, 1);
	build_transport(rec_data, get_image_data("record.png"), transport_bg, 1);
	build_transport(recframe_data, get_image_data("singleframe.png"), transport_bg, 1);
	build_transport(reverse_data, get_image_data("reverse.png"), transport_bg, 1);
	build_transport(rewind_data, get_image_data("rewind.png"), transport_bg, 0);
	build_transport(stop_data, get_image_data("stop.png"), transport_bg, 1);
	build_transport(stoprec_data, get_image_data("stoprec.png"), transport_bg, 2);
	flush_images();

	title_font = MEDIUMFONT_3D;
	title_color = WHITE;
	recordgui_fixed_color = YELLOW;
	recordgui_variable_color = RED;

	channel_position_color = MEYELLOW;
	resources->meter_title_w = 25;
}

#define CWINDOW_METER_MARGIN 5
#define VWINDOW_METER_MARGIN 5

void SUVTheme::get_mwindow_sizes(MWindowGUI *gui, int w, int h)
{
	mbuttons_x = 0;
	mbuttons_y = gui->mainmenu->get_h();
	mbuttons_w = w;
	mbuttons_h = mbutton_left->get_h();
	mclock_x = 10;
	mclock_y = mbuttons_y + mbuttons_h + CWINDOW_METER_MARGIN;
	mclock_w = clock_bg->get_w() - 40;
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

void SUVTheme::get_cwindow_sizes(CWindowGUI *gui, int cwindow_controls)
{
	if(cwindow_controls)
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
	}
	else
	{
		ccomposite_x = -cpanel_bg->get_w();
		ccomposite_y = 0;
		ccomposite_w = cpanel_bg->get_w();
		ccomposite_h = mwindow->session->cwindow_h - cbuttons_left->get_h();

		cslider_x = 5;
		cslider_y = mwindow->session->cwindow_h;
		cedit_x = 10;
		cedit_y = cslider_y + 17;
		ctransport_x = 10;
		ctransport_y = cedit_y + 40;
		ccanvas_x = 0;
		ccanvas_y = 0;
		ccanvas_w = mwindow->session->cwindow_w;
		ccanvas_h = mwindow->session->cwindow_h;
		cmeter_x = mwindow->session->cwindow_w;
	}


	czoom_x = ctransport_x + PlayTransport::get_transport_width(mwindow) + 20;
	czoom_y = ctransport_y + 5;


	cmeter_y = 5;
	cmeter_h = mwindow->session->cwindow_h - cmeter_y;

	cslider_w = ccanvas_x + ccanvas_w - cslider_x;
	ctimebar_x = ccanvas_x;
	ctimebar_y = ccanvas_y + ccanvas_h;
	ctimebar_w = ccanvas_w;
	ctimebar_h = 16;


// Not used
	ctime_x = ctransport_x + PlayTransport::get_transport_width(mwindow);
	ctime_y = ctransport_y;
	cdest_x = czoom_x;
	cdest_y = czoom_y + 30;
}



void SUVTheme::get_recordgui_sizes(RecordGUI *gui, int w, int h)
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



void SUVTheme::get_vwindow_sizes(VWindowGUI *gui)
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





void SUVTheme::build_icons()
{
	mwindow_icon = new VFrame(get_image_data("heroine_icon.png"));
	vwindow_icon = new VFrame(get_image_data("heroine_icon.png"));
	cwindow_icon = new VFrame(get_image_data("heroine_icon.png"));
	awindow_icon = new VFrame(get_image_data("heroine_icon.png"));
	record_icon = new VFrame(get_image_data("heroine_icon.png"));
	clip_icon = new VFrame(get_image_data("clip_icon.png"));
}



void SUVTheme::build_bg_data()
{
// Audio settings
	channel_bg_data = new VFrame(get_image_data("channel_bg.png"));
	channel_position_data = new VFrame(get_image_data("channel_position.png"));

// Track bitmaps
	resource1024_bg_data = new VFrame(get_image_data("resource1024.png"));
	resource512_bg_data = new VFrame(get_image_data("resource512.png"));
	resource256_bg_data = new VFrame(get_image_data("resource256.png"));
	resource128_bg_data = new VFrame(get_image_data("resource128.png"));
	resource64_bg_data = new VFrame(get_image_data("resource64.png"));
	resource32_bg_data = new VFrame(get_image_data("resource32.png"));
	plugin_bg_data = new VFrame(get_image_data("plugin_bg.png"));
	title_bg_data = new VFrame(get_image_data("title_bg.png"));
	vtimebar_bg_data = new VFrame(get_image_data("vwindow_timebar.png"));
}


void SUVTheme::build_patches()
{
	static VFrame *default_drawpatch_data[] = { new VFrame(get_image_data("drawpatch_up.png")), new VFrame(get_image_data("drawpatch_hi.png")), new VFrame(get_image_data("drawpatch_checked.png")), new VFrame(get_image_data("drawpatch_dn.png")), new VFrame(get_image_data("drawpatch_checkedhi.png")) };
	static VFrame *default_expandpatch_data[] = { new VFrame(get_image_data("expandpatch_up.png")), new VFrame(get_image_data("expandpatch_hi.png")), new VFrame(get_image_data("expandpatch_checked.png")), new VFrame(get_image_data("expandpatch_dn.png")), new VFrame(get_image_data("expandpatch_checkedhi.png")) };
	static VFrame *default_gangpatch_data[] = { new VFrame(get_image_data("gangpatch_up.png")), new VFrame(get_image_data("gangpatch_hi.png")), new VFrame(get_image_data("gangpatch_checked.png")), new VFrame(get_image_data("gangpatch_dn.png")), new VFrame(get_image_data("gangpatch_checkedhi.png")) };
	static VFrame *default_mutepatch_data[] = { new VFrame(get_image_data("mutepatch_up.png")), new VFrame(get_image_data("mutepatch_hi.png")), new VFrame(get_image_data("mutepatch_checked.png")), new VFrame(get_image_data("mutepatch_dn.png")), new VFrame(get_image_data("mutepatch_checkedhi.png")) };
	static VFrame *default_patchbay_bg = new VFrame(get_image_data("patchbay_bg.png"));
	static VFrame *default_playpatch_data[] = { new VFrame(get_image_data("playpatch_up.png")), new VFrame(get_image_data("playpatch_hi.png")), new VFrame(get_image_data("playpatch_checked.png")), new VFrame(get_image_data("playpatch_dn.png")), new VFrame(get_image_data("playpatch_checkedhi.png")) };
	static VFrame *default_recordpatch_data[] = { new VFrame(get_image_data("recordpatch_up.png")), new VFrame(get_image_data("recordpatch_hi.png")), new VFrame(get_image_data("recordpatch_checked.png")), new VFrame(get_image_data("recordpatch_dn.png")), new VFrame(get_image_data("recordpatch_checkedhi.png")) };


	drawpatch_data = default_drawpatch_data;
	expandpatch_data = default_expandpatch_data;
	gangpatch_data = default_gangpatch_data;
	mutepatch_data = default_mutepatch_data;
	patchbay_bg = default_patchbay_bg;
	playpatch_data = default_playpatch_data;
	recordpatch_data = default_recordpatch_data;
}

void SUVTheme::build_overlays()
{
	keyframe_data = new VFrame(get_image_data("keyframe3.png"));
	camerakeyframe_data = new VFrame(get_image_data("camerakeyframe.png"));
	maskkeyframe_data = new VFrame(get_image_data("maskkeyframe.png"));
	modekeyframe_data = new VFrame(get_image_data("modekeyframe.png"));
	pankeyframe_data = new VFrame(get_image_data("pankeyframe.png"));
	projectorkeyframe_data = new VFrame(get_image_data("projectorkeyframe.png"));
}









void SUVTheme::draw_rwindow_bg(RecordGUI *gui)
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

void SUVTheme::draw_rmonitor_bg(RecordMonitorGUI *gui)
{
	int margin = 45;
	int panel_w = 300;
	int x = rmonitor_meter_x - margin;
	int w = mwindow->session->rmonitor_w - x;
	if(w < rmonitor_meters->get_w()) w = rmonitor_meters->get_w();
	gui->clear_box(0, 
		0, 
		mwindow->session->rmonitor_w, 
		mwindow->session->rmonitor_h);
	gui->draw_9segment(x,
		0,
		w,
		mwindow->session->rmonitor_h,
		rmonitor_meters);
}






void SUVTheme::draw_mwindow_bg(MWindowGUI *gui)
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

void SUVTheme::draw_cwindow_bg(CWindowGUI *gui)
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

void SUVTheme::draw_vwindow_bg(VWindowGUI *gui)
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

void SUVTheme::get_preferences_sizes()
{
}


void SUVTheme::draw_preferences_bg(PreferencesWindow *gui)
{
	gui->draw_9segment(0, 0, gui->get_w(), gui->get_h() - 40, preferences_bg);
}

void SUVTheme::get_new_sizes(NewWindow *gui)
{
}

void SUVTheme::draw_new_bg(NewWindow *gui)
{
	gui->draw_vframe(new_bg, 0, 0);
}

void SUVTheme::draw_setformat_bg(SetFormatWindow *gui)
{
	gui->draw_vframe(setformat_bg, 0, 0);
}






