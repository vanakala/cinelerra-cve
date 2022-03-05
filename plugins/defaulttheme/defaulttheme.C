
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

#include "bcsignals.h"
#include "bcresources.h"
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
#include "setformat.h"
#include "statusbar.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "vwindowgui.h"


PluginClient* new_plugin(PluginServer *server)
{
	return new BlondThemeMain(server);
}


BlondThemeMain::BlondThemeMain(PluginServer *server)
 : PluginTClient(server)
{
}

BlondThemeMain::~BlondThemeMain()
{
}

const char* BlondThemeMain::plugin_title()
{
	return "Blond-cv";
}

Theme* BlondThemeMain::new_theme()
{
	theme = new BlondTheme;
	extern unsigned char _binary_defaulttheme_data_start[];
	theme->set_data(_binary_defaulttheme_data_start);
	return theme;
}



BlondTheme::BlondTheme()
 : Theme()
{
}

BlondTheme::~BlondTheme()
{
}

void BlondTheme::initialize()
{
	BC_Resources *resources = BC_WindowBase::get_resources();

	resources->text_default = 0x000000;
	resources->text_background = 0xffffff;
	resources->text_border1 = 0x4a484a; // (top outer)
	resources->text_border2 = 0x000000; // (top inner)
	resources->text_border3 = 0xacaeac; // (bottom inner)
	resources->text_border4 = 0xffffff; // (bottom outer)
	resources->text_inactive_highlight = 0xacacac;

	resources->bg_color = BLOND;
	resources->default_text_color = 0x000000;
	resources->menu_title_text    = 0x000000;
	resources->popup_title_text   = 0x000000;
	resources->menu_item_text     = 0x000000;

	resources->generic_button_margin = 15;
	resources->pot_needle_color = resources->text_default;
	resources->pot_offset = 1;
	resources->progress_text = resources->text_default;
	resources->meter_font_color = RED;

	resources->menu_light = 0x00cacd;
	resources->menu_highlighted = 0x9c95ff;
	resources->menu_down = 0x007d7b;
	resources->menu_up = 0x009594;
	resources->menu_shadow = 0x004a4a;
	resources->popupmenu_margin = 10;          // ugly
	resources->popupmenu_triangle_margin = 15; // ugly

	resources->listbox_title_color = 0x000000;

	resources->listbox_title_margin = 0;
	resources->listbox_title_hotspot = 5;  // No. of pixels around the borders to allow dragging
	resources->listbox_border1 = 0x4a484a; // (top outer)
	resources->listbox_border2 = 0x000000; // (top inner)
	resources->listbox_border3 = 0xffe200; // (bottom inner)
	resources->listbox_border4 = 0xffffff; // (bottom outer)
	resources->listbox_highlighted = 0xeee6ee;
	resources->listbox_inactive = 0xffffffff; // (background)
	resources->listbox_bg = new_image("list_bg.png");
	resources->listbox_text = 0x000000;

	resources->dirbox_margin = 50;
	resources->filebox_margin = 101;
	resources->file_color = 0x000000;
	resources->directory_color = 0x0000ff;

	resources->filebox_icons_images = new_button("icons.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");

	resources->filebox_text_images = new_button("text.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");

	resources->filebox_newfolder_images = new_button("folder.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");

	resources->filebox_updir_images = new_button("updir.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");

	resources->filebox_delete_images = new_button("delete.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");

	resources->filebox_reload_images = new_button("reload.png",
		"fileboxbutton_up.png",
		"fileboxbutton_hi.png",
		"fileboxbutton_dn.png");


	resources->filebox_descend_images = new_button("openfolder.png",
		"generic_up.png", 
		"generic_hi.png", 
		"generic_dn.png");

	resources->usethis_button_images = 
		resources->ok_images = new_button("ok.png",
		"generic_up.png", 
		"generic_hi.png", 
		"generic_dn.png");

	new_button("ok.png",
		"generic_up.png", 
		"generic_hi.png", 
		"generic_dn.png",
		"new_ok_images");

	resources->cancel_images = new_button("cancel.png",
		"generic_up.png", 
		"generic_hi.png", 
		"generic_dn.png");

	new_button("cancel.png",
		"generic_up.png", 
		"generic_hi.png", 
		"generic_dn.png",
		"new_cancel_images");

	resources->bar_data = new_image("bar", "bar.png");


	resources->min_menu_w = 0;
	resources->menu_popup_bg = 0;  // if (0) use menu_light, menu_up, menu_shadow
	resources->menu_item_bg = 0;   // if (0) use menu_light, menu_highlighted, menu_down, menu_shadow
	resources->menu_bar_bg = 0;    // if (0) use menu_light, menu_shadow, and height of MEDIUMFONT + 8
	resources->menu_title_bg =  0; // if (0) use menu_light, menu_highlighted, menu_down, menu_shadow


	resources->popupmenu_images = 0; // if (0) get_resources()->use generic_button_images

	resources->toggle_highlight_bg = 0; // if (0) "Draw a plain box" as per bctoggle.C

	resources->generic_button_images = new_image_set(3, 
			"generic_up.png", 
			"generic_hi.png", 
			"generic_dn.png");
	resources->horizontal_slider_data = new_image_set(6,
			"hslider_fg_up.png",
			"hslider_fg_hi.png",
			"hslider_fg_dn.png",
			"hslider_bg_up.png",
			"hslider_bg_hi.png",
			"hslider_bg_dn.png");
	resources->vertical_slider_data = new_image_set(6,
			"vertical_slider_fg_up.png",
			"vertical_slider_fg_hi.png",
			"vertical_slider_fg_dn.png",
			"vertical_slider_bg_up.png",
			"vertical_slider_bg_hi.png",
			"vertical_slider_bg_dn.png");
	resources->progress_images = new_image_set(2,
			"progress_bg.png",
			"progress_hi.png");
	resources->tumble_data = new_image_set(4,
		"tumble_up.png",
		"tumble_hi.png",
		"tumble_bottom.png",
		"tumble_top.png");
	resources->listbox_button = new_image_set(4,
		"listbox_button_up.png",
		"listbox_button_hi.png",
		"listbox_button_dn.png",
		"listbox_button_disabled.png"); // probably need to make this for the suv theme
	resources->listbox_column = new_image_set(3,
		"column_up.png",
		"column_hi.png",
		"column_dn.png");
	resources->listbox_expand = new_image_set(5,
		"listbox_expandup.png",
		"listbox_expanduphi.png",
		"listbox_expandchecked.png",
		"listbox_expanddn.png",
		"listbox_expandcheckedhi.png");
	resources->listbox_up = new_image("listbox_up.png");
	resources->listbox_dn = new_image("listbox_dn.png");
	resources->pan_data = new_image_set(7,
			"pan_up.png", 
			"pan_hi.png", 
			"pan_popup.png", 
			"pan_channel.png", 
			"pan_stick.png", 
			"pan_channel_small.png", 
			"pan_stick_small.png");
	resources->pan_text_color = WHITE;

	resources->pot_images = new_image_set(3,
		"pot_up.png",
		"pot_hi.png",
		"pot_dn.png");

	resources->checkbox_images = new_image_set(5,
		"checkbox_up.png",
		"checkbox_hi.png",
		"checkbox_checked.png",
		"checkbox_dn.png",
		"checkbox_checkedhi.png");

	resources->radial_images = new_image_set(5,
		"radial_up.png",
		"radial_hi.png",
		"radial_checked.png",
		"radial_dn.png",
		"radial_checkedhi.png");

	resources->xmeter_images = new_image_set(6, 
		"xmeter_normal.png",
		"xmeter_green.png",
		"xmeter_red.png",
		"xmeter_yellow.png",
		"xmeter_white.png",
		"xmeter_over.png");
	resources->ymeter_images = new_image_set(6, 
		"ymeter_normal.png",
		"ymeter_green.png",
		"ymeter_red.png",
		"ymeter_yellow.png",
		"ymeter_white.png",
		"ymeter_over.png");

	resources->hscroll_data = new_image_set(10,
			"hscroll_handle_up.png",
			"hscroll_handle_hi.png",
			"hscroll_handle_dn.png",
			"hscroll_handle_bg.png",
			"hscroll_left_up.png",
			"hscroll_left_hi.png",
			"hscroll_left_dn.png",
			"hscroll_right_up.png",
			"hscroll_right_hi.png",
			"hscroll_right_dn.png");

	resources->vscroll_data = new_image_set(10,
			"vscroll_handle_up.png",
			"vscroll_handle_hi.png",
			"vscroll_handle_dn.png",
			"vscroll_handle_bg.png",
			"vscroll_left_up.png",
			"vscroll_left_hi.png",
			"vscroll_left_dn.png",
			"vscroll_right_up.png",
			"vscroll_right_hi.png",
			"vscroll_right_dn.png");

	new_button("prevtip.png", "tipbutton_up.png", "tipbutton_hi.png", "tipbutton_dn.png", "prev_tip");
	new_button("nexttip.png", "tipbutton_up.png", "tipbutton_hi.png", "tipbutton_dn.png", "next_tip");
	new_button("closetip.png", "tipbutton_up.png", "tipbutton_hi.png", "tipbutton_dn.png", "close_tip");
	new_button("swap_extents.png",
		"editpanel_up.png",
		"editpanel_hi.png",
		"editpanel_dn.png",
		"swap_extents");

// Record windows
	rgui_batch = new_image("recordgui_batch.png");
	rgui_controls = new_image("recordgui_controls.png");
	rgui_list = new_image("recordgui_list.png");
	rmonitor_panel = new_image("recordmonitor_panel.png");
	rmonitor_meters = new_image("recordmonitor_meters.png");

	preferences_category_overlap = 0;
	preferencescategory_x = 5;
	preferencescategory_y = 5;
	preferencestitle_x = 5;
	preferencestitle_y = 10;
	preferencesoptions_x = 5;
	preferencesoptions_y = 0;

// MWindow
	message_normal = resources->text_default;
	audio_color = BLACK;
	mtransport_margin = 11;
	toggle_margin = 11;

	new_image("mbutton_bg", "mbutton_bg.png");
	new_image("mbutton_blue", "mbutton_blue.png");
	new_image("timebar_bg", "timebar_bg.png");
	new_image("timebar_brender", "timebar_brender.png");
	new_image("clock_bg", "mclock.png");
	new_image("patchbay_bg", "patchbay_bg.png");
	new_image("tracks_bg","tracks_bg.png");
	new_image("zoombar_left","zoombar_left.png");
	new_image("zoombar_right","zoombar_right.png");
	new_image("statusbar_left","statusbar_left.png");
	new_image("statusbar_right","statusbar_right.png");

	new_image_set("zoombar_menu", 3, "generic_up.png", "generic_hi.png", "generic_dn.png");
	new_image_set("zoombar_tumbler", 4, "tumble_up.png", "tumble_hi.png", "tumble_bottom.png", "tumble_top.png");

	new_image_set("mode_popup", 3, "generic_up.png", "generic_hi.png", "generic_dn.png");
	new_image("mode_add", "mode_add.png");
	new_image("mode_divide", "mode_divide.png");
	new_image("mode_multiply", "mode_multiply.png");
	new_image("mode_normal", "mode_normal.png");
	new_image("mode_replace", "mode_replace.png");
	new_image("mode_subtract", "mode_subtract.png");
	new_image("mode_max", "mode_max.png");

	new_toggle("plugin_on.png", 
		"pluginbutton_hi.png", 
		"pluginbutton_hi.png", 
		"pluginbutton_select.png", 
		"pluginbutton_dn.png", 
		"pluginbutton_selecthi.png", 
		"plugin_on");

	new_toggle("plugin_show.png", 
		"plugin_show.png", 
		"pluginbutton_hi.png", 
		"pluginbutton_select.png", 
		"pluginbutton_dn.png", 
		"pluginbutton_selecthi.png", 
		"plugin_show");

// CWindow
	new_image("cpanel_bg", "cpanel_bg.png");
	new_image("cbuttons_left", "cbuttons_left.png");
	new_image("cbuttons_right", "cbuttons_right.png");
	new_image("cmeter_bg", "cmeter_bg.png");

// VWindow
	new_image("vbuttons_left", "vbuttons_left.png");
	new_image("preferences_bg", "preferences_bg.png");

	new_image("new_bg", "new_bg.png");
	new_image("setformat_bg", "setformat_bg2.png");

	timebar_view_data = new_image("timebar_view.png");

	setformat_w = get_image("setformat_bg")->get_w();
	setformat_h = get_image("setformat_bg")->get_h();
	setformat_x1 = 15;
	setformat_x2 = 100;

	setformat_x3 = 315;
	setformat_x4 = 425;
	setformat_y1 = 20;
	setformat_y2 = 85;
	setformat_y3 = 125;
	setformat_margin = 30;
	setformat_channels_x = 25;
	setformat_channels_y = 242;
	setformat_channels_w = 250;
	setformat_channels_h = 250;

	loadfile_pad = 52;
	browse_pad = 20;

	new_image_set("playpatch_data", 
		5,
		"playpatch_up.png",
		"playpatch_hi.png",
		"playpatch_checked.png",
		"playpatch_dn.png",
		"playpatch_checkedhi.png");

	new_image_set("recordpatch_data", 
		5,
		"recordpatch_up.png",
		"recordpatch_hi.png",
		"recordpatch_checked.png",
		"recordpatch_dn.png",
		"recordpatch_checkedhi.png");

	new_image_set("gangpatch_data", 
		5,
		"gangpatch_up.png",
		"gangpatch_hi.png",
		"gangpatch_checked.png",
		"gangpatch_dn.png",
		"gangpatch_checkedhi.png");

	new_image_set("drawpatch_data", 
		5,
		"drawpatch_up.png",
		"drawpatch_hi.png",
		"drawpatch_checked.png",
		"drawpatch_dn.png",
		"drawpatch_checkedhi.png");

	new_image_set("mutepatch_data", 
		5,
		"mutepatch_up.png",
		"mutepatch_hi.png",
		"mutepatch_checked.png",
		"mutepatch_dn.png",
		"mutepatch_checkedhi.png");

	new_image_set("expandpatch_data", 
		5,
		"expandpatch_up.png",
		"expandpatch_hi.png",
		"expandpatch_checked.png",
		"expandpatch_dn.png",
		"expandpatch_checkedhi.png");

	new_image_set("mastertrack_data",
		5,
		"mastertrack.png",
		"mastertrack-hi.png",
		"mastertrack-selected.png",
		"mastertrack.png",
		"mastertrack-selected-hi.png");

	build_icons();
	build_bg_data();
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

	new_image("panel_divider", "panel_divider.png");
	new_button("bottom_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "bottom_justify");
	new_button("center_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "center_justify");
	new_button("channel.png", editpanel_up, editpanel_hi, editpanel_dn, "channel");


	new_button("copy.png", editpanel_up, editpanel_hi, editpanel_dn, "copy");
	new_button("cut.png", editpanel_up, editpanel_hi, editpanel_dn, "cut");
	new_button("fit.png", editpanel_up, editpanel_hi, editpanel_dn, "fit");
	new_button("fitautos.png", editpanel_up, editpanel_hi, editpanel_dn, "fitautos");
	new_button("inpoint.png", editpanel_up, editpanel_hi, editpanel_dn, "inbutton");
	new_button("label.png", editpanel_up, editpanel_hi, editpanel_dn, "labelbutton");
	new_button("left_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "left_justify");
	new_button("magnify.png", editpanel_up, editpanel_hi, editpanel_dn, "magnify_button");
	new_button("middle_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "middle_justify");
	new_button("nextlabel.png", editpanel_up, editpanel_hi, editpanel_dn, "nextlabel");
	new_button("outpoint.png", editpanel_up, editpanel_hi, editpanel_dn, "outbutton");
	over_button = new_button("over.png", editpanel_up, editpanel_hi, editpanel_dn);
	overwrite_data = new_button("overwrite.png", editpanel_up, editpanel_hi, editpanel_dn);
	new_button("paste.png", editpanel_up, editpanel_hi, editpanel_dn, "paste");
	new_button("prevlabel.png", editpanel_up, editpanel_hi, editpanel_dn, "prevlabel");
	new_button("redo.png", editpanel_up, editpanel_hi, editpanel_dn, "redo");
	new_button("right_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "right_justify");
	splice_data = new_button("splice.png", editpanel_up, editpanel_hi, editpanel_dn);
	new_button("toclip.png", editpanel_up, editpanel_hi, editpanel_dn, "toclip");
	new_button("goto.png", editpanel_up, editpanel_hi, editpanel_dn, "goto");
	new_button("top_justify.png", editpanel_up, editpanel_hi, editpanel_dn, "top_justify");
	new_button("undo.png", editpanel_up, editpanel_hi, editpanel_dn, "undo");
	new_button("wrench.png", editpanel_up, editpanel_hi, editpanel_dn, "wrench");


#define TRANSPORT_LEFT_IMAGES  "transport_left_up.png", "transport_left_hi.png", "transport_left_dn.png"
#define TRANSPORT_CENTER_IMAGES  "transport_center_up.png", "transport_center_hi.png", "transport_center_dn.png"
#define TRANSPORT_RIGHT_IMAGES  "transport_right_up.png", "transport_right_hi.png", "transport_right_dn.png"

	new_button("end.png", TRANSPORT_RIGHT_IMAGES, "end");
	new_button("fastfwd.png",TRANSPORT_CENTER_IMAGES, "fastfwd");
	new_button("fastrev.png",TRANSPORT_CENTER_IMAGES, "fastrev");
	new_button("play.png",TRANSPORT_CENTER_IMAGES, "play");
	new_button("framefwd.png", TRANSPORT_CENTER_IMAGES, "framefwd");
	new_button("framerev.png", TRANSPORT_CENTER_IMAGES, "framerev");
	new_button("pause.png", TRANSPORT_CENTER_IMAGES, "pause");
	new_button("record.png", TRANSPORT_CENTER_IMAGES, "record");
	new_button("singleframe.png", TRANSPORT_CENTER_IMAGES, "recframe");
	new_button("reverse.png", TRANSPORT_CENTER_IMAGES, "reverse");
	new_button("rewind.png", TRANSPORT_LEFT_IMAGES, "rewind");
	new_button("stop.png", TRANSPORT_CENTER_IMAGES, "stop");
	new_button("stop.png", TRANSPORT_RIGHT_IMAGES, "stoprec");

// CWindow icons
	new_image("cwindow_inactive", "cwindow_inactive.png");
	new_image("cwindow_active", "cwindow_active.png");

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

	new_image_set("category_button",
		3,
		"preferencesbutton_dn.png",
		"preferencesbutton_dnhi.png",
		"preferencesbutton_dnlo.png");

	new_image_set("category_button_checked",
		3,
		"preferencesbutton_up.png",
		"preferencesbutton_uphi.png",
		"preferencesbutton_dnlo.png");


	new_toggle("arrow.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "arrow");
	new_toggle("autokeyframe.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "autokeyframe");
	new_toggle("ibeam.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "ibeam");
	new_toggle("show_meters.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "meters");

	VFrame *cpanel_up = new_image("editpanel_up.png");
	VFrame *cpanel_hi = new_image("editpanel_hi.png");
	VFrame *cpanel_dn = new_image("editpanel_dn.png");
	VFrame *cpanel_checked = new_image("editpanel_checked.png");
	VFrame *cpanel_checkedhi = new_image("editpanel_checkedhi.png");
	new_toggle("blank30x30.png", 
			new_image("locklabels_locked.png"),
			new_image("locklabels_lockedhi.png"),
			new_image("locklabels_unlocked.png"),
			new_image("locklabels_dn.png"), // can't have seperate down for each!!??
			new_image("locklabels_unlockedhi.png"),
			"locklabels");


	new_toggle("camera.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "camera");
	new_toggle("crop.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "crop");
	new_toggle("eyedrop.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "eyedrop");
	new_toggle("magnify.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "magnify");
	new_toggle("mask.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "mask");
	new_toggle("ruler.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "ruler");
	new_toggle("projector.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "projector");
	new_toggle("protect.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "protect");
	new_toggle("titlesafe.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "titlesafe");
	new_toggle("toolwindow.png", cpanel_up, cpanel_hi, cpanel_checked, cpanel_dn, cpanel_checkedhi, "tool");

	// toggle for tangent mode (compositor/tool window)
	new_toggle("tan_smooth.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "tan_smooth");
	new_toggle("tan_linear.png", editpanel_up, editpanel_hi, editpanel_checked, editpanel_dn, editpanel_checkedhi, "tan_linear");

	flush_images();

	title_font = MEDIUMFONT_3D;
	title_color = WHITE;

	channel_position_color = MEYELLOW;
	resources->meter_title_w = 25;

	// (asset) edit info text color
	edit_font_color = YELLOW;

	//labels
	resources->label_images = new_image_set(5,
		"radial_up.png", 
		"radial_hi.png", 
		"radial_checked.png", 
		"radial_dn.png", 
		"radial_checkedhi.png");
}

#define CWINDOW_METER_MARGIN 5
#define VWINDOW_METER_MARGIN 5

void BlondTheme::get_mwindow_sizes(MWindowGUI *gui, int w, int h)
{
	mbuttons_x = 0;
	mbuttons_y = gui->mainmenu->get_h() + 1;
	mbuttons_w = w;
	mbuttons_h = get_image("mbutton_bg")->get_h();
	mclock_x = 10;
	mclock_y = mbuttons_y - 1 + mbuttons_h + CWINDOW_METER_MARGIN;
	mclock_w = get_image("clock_bg")->get_w() - 40;
	mclock_h = get_image("clock_bg")->get_h();
	mtimebar_x = get_image("patchbay_bg")->get_w();
	mtimebar_y = mbuttons_y - 1 + mbuttons_h;
	mtimebar_w = w - mtimebar_x;
	mtimebar_h = get_image("timebar_bg")->get_h();
	mzoom_h =  get_image("zoombar_left")->get_h();
	mzoom_x = 0;
	mzoom_y = h - get_image("statusbar_left")->get_h() - mzoom_h;
	mzoom_w = w;
	mstatus_x = 0;
	mstatus_y = mzoom_y + mzoom_h;
	mstatus_w = w;
	mstatus_h = h - mstatus_y;
	mstatus_message_x = 10;
	mstatus_message_y = 5;
	mstatus_progress_w = 230;
	mstatus_progress_x = mstatus_w - statusbar_cancel_data[0]->get_w() - 2*3 - mstatus_progress_w;
	mstatus_progress_y = mstatus_h/2 - BC_WindowBase::get_resources()->progress_images[0]->get_h()/2;

	mstatus_cancel_x = mstatus_w - statusbar_cancel_data[0]->get_w() - 3;
	mstatus_cancel_y = mstatus_h/2 - statusbar_cancel_data[0]->get_h()/2;

	patchbay_x = 0;
	patchbay_y = mtimebar_y + mtimebar_h;
	patchbay_w = get_image("patchbay_bg")->get_w();
	patchbay_h = mzoom_y - patchbay_y - BC_ScrollBar::get_span(SCROLL_HORIZ);
	mcanvas_x = patchbay_x + patchbay_w;
	mcanvas_y = patchbay_y;
	mcanvas_w = w - patchbay_w - BC_ScrollBar::get_span(SCROLL_VERT);
	mcanvas_h = patchbay_h;
	mhscroll_x = 0;
	mhscroll_y = mzoom_y - BC_ScrollBar::get_span(SCROLL_HORIZ);
	mhscroll_w = w - BC_ScrollBar::get_span(SCROLL_VERT);
	mvscroll_x = mcanvas_x + mcanvas_w;
	mvscroll_y = mcanvas_y;
	mvscroll_h = mcanvas_h;
}

void BlondTheme::get_cwindow_sizes(CWindowGUI *gui, int cwindow_controls)
{
	if(cwindow_controls)
	{
		ccomposite_x = 0;
		ccomposite_y = 5;
		ccomposite_w = get_image("cpanel_bg")->get_w();
		ccomposite_h = mainsession->cwindow_h -
			get_image("cbuttons_left")->get_h();
		cslider_x = 5;
		cslider_y = ccomposite_h + 23;
		cedit_x = 10;
		cedit_y = cslider_y + 17;
		ctransport_x = 10;
		ctransport_y = mainsession->cwindow_h -
			get_image_set("autokeyframe")[0]->get_h();
		ccanvas_x = ccomposite_x + ccomposite_w;
		ccanvas_y = 0;
		ccanvas_h = ccomposite_h;
		cstatus_x = 525;
		cstatus_y = mainsession->cwindow_h - 40;
		if(edlsession->cwindow_meter)
		{
			cmeter_x = mainsession->cwindow_w -
				MeterPanel::get_meters_width(edlsession->audio_channels,
					edlsession->cwindow_meter);
			ccanvas_w = cmeter_x - ccanvas_x - 5;
		}
		else
		{
			cmeter_x = mainsession->cwindow_w;
			ccanvas_w = cmeter_x - ccanvas_x;
		}
	}
	else
	{
		ccomposite_x = -get_image("cpanel_bg")->get_w();
		ccomposite_y = 0;
		ccomposite_w = get_image("cpanel_bg")->get_w();
		ccomposite_h = mainsession->cwindow_h - get_image("cbuttons_left")->get_h();

		cslider_x = 5;
		cslider_y = mainsession->cwindow_h;
		cedit_x = 10;
		cedit_y = cslider_y + 17;
		ctransport_x = 10;
		ctransport_y = cedit_y + 40;
		ccanvas_x = 0;
		ccanvas_y = 0;
		ccanvas_w = mainsession->cwindow_w;
		ccanvas_h = mainsession->cwindow_h;
		cmeter_x = mainsession->cwindow_w;
		cstatus_x = mainsession->cwindow_w;
		cstatus_y = mainsession->cwindow_h;
	}

	czoom_x = ctransport_x + PlayTransport::get_transport_width() + 20;
	czoom_y = ctransport_y + 5;

	cmeter_y = 5;
	cmeter_h = mainsession->cwindow_h - cmeter_y;

	cslider_w = ccanvas_x + ccanvas_w - cslider_x - 5;
	ctimebar_x = ccanvas_x;
	ctimebar_y = ccanvas_y + ccanvas_h;
	ctimebar_w = ccanvas_w;
	ctimebar_h = 16;

// Not used
	ctime_x = ctransport_x + PlayTransport::get_transport_width();
	ctime_y = ctransport_y;
	cdest_x = czoom_x;
	cdest_y = czoom_y + 30;
}

void BlondTheme::get_vwindow_sizes(VWindowGUI *gui)
{
	vmeter_y = 5;
	vmeter_h = mainsession->vwindow_h - cmeter_y;
	vcanvas_x = 0;
	vcanvas_y = 0;
	vcanvas_h = mainsession->vwindow_h - get_image("vbuttons_left")->get_h();

	if(edlsession->vwindow_meter)
	{
		vmeter_x = mainsession->vwindow_w -
			VWINDOW_METER_MARGIN - 
			MeterPanel::get_meters_width(edlsession->audio_channels,
				edlsession->vwindow_meter);
		vcanvas_w = vmeter_x - vcanvas_x - VWINDOW_METER_MARGIN;
	}
	else
	{
		vmeter_x = mainsession->vwindow_w;
		vcanvas_w = mainsession->vwindow_w;
	}

	vtimebar_x = vcanvas_x;
	vtimebar_y = vcanvas_y + vcanvas_h;
	vtimebar_w = vcanvas_w;
	vtimebar_h = 16;

	vslider_x = 10;
	vslider_y = vtimebar_y + 25;
	vslider_w = vtimebar_w - vslider_x;
	vedit_x = 10;
	vedit_y = vslider_y + BC_Slider::get_span(0);
	vtransport_x = 10;
	vtransport_y = mainsession->vwindow_h -
		get_image_set("autokeyframe")[0]->get_h();
	vtime_x = 380;
	vtime_y = vedit_y + 10;
	vtime_w = 125;

	vzoom_x = vtime_x + 150;
	vzoom_y = vtime_y;
	vsource_x = vtime_x + 50;
	vsource_y = vtransport_y + 5;
}

void BlondTheme::build_icons()
{
	new_image("mwindow_icon", "heroine_icon.png");
	new_image("vwindow_icon", "heroine_icon.png");
	new_image("cwindow_icon", "heroine_icon.png");
	new_image("awindow_icon", "heroine_icon.png");
	new_image("record_icon", "heroine_icon.png");
	new_image("clip_icon", "clip_icon.png");
}


void BlondTheme::build_bg_data()
{
// Audio settings
	channel_position_data = new VFrame(get_image_data("channel_position.png"));

// Track bitmaps
	new_image("resource1024", "resource1024.png");
	new_image("resource512", "resource512.png");
	new_image("resource256", "resource256.png");
	new_image("resource128", "resource128.png");
	new_image("resource64", "resource64.png");
	new_image("resource32", "resource32.png");
	new_image("plugin_bg_data", "plugin_bg.png");
	new_image("title_bg_data", "title_bg.png");
	new_image("vtimebar_bg_data", "vwindow_timebar.png");
}


void BlondTheme::build_overlays()
{
	keyframe_data = new VFrame(get_image_data("keyframe3.png"));
	maskkeyframe_data = new VFrame(get_image_data("maskkeyframe.png"));
	modekeyframe_data = new VFrame(get_image_data("modekeyframe.png"));
	pankeyframe_data = new VFrame(get_image_data("pankeyframe.png"));
	cropkeyframe_data = new VFrame(get_image_data("cropkeyframe.png"));
}

void BlondTheme::draw_mwindow_bg(MWindowGUI *gui)
{
// Button bar
	int mbuttons_rightedge = mbuttons_x
		+ get_image("end")->get_w() + 7 * get_image("play")->get_w() + get_image("end")->get_w()
		+ mtransport_margin
		+ 2 * get_image("arrow")->get_w()
		+ toggle_margin 
		+ 2 * get_image("autokeyframe")->get_w()
		+ toggle_margin 
		+ (14 + 1) * get_image("goto")->get_w() + 5; // I don't know why + 1!!
	gui->draw_3segmenth(mbuttons_x, 
		mbuttons_y, 
		mbuttons_rightedge, 
		mbuttons_x,
		mbuttons_rightedge,
		get_image("mbutton_bg"),
		0);

	gui->draw_3segmenth(mbuttons_x + mbuttons_rightedge, 
		mbuttons_y, 
		mainsession->mwindow_w,
		get_image("mbutton_blue"));

	int pdw = get_image("panel_divider")->get_w();
	int x = mbuttons_x;
	x += get_image("end")->get_w() + 7 * get_image("play")->get_w() + get_image("end")->get_w();
	x += mtransport_margin;                                       // the control buttons

	gui->draw_vframe(get_image("panel_divider"),
		x - toggle_margin / 2 - pdw / 2 + 2,
		mbuttons_y - 1);
	x += 2 * get_image("arrow")->get_w() + toggle_margin;           // the mode buttons

	gui->draw_vframe(get_image("panel_divider"),
		x - toggle_margin / 2 - pdw / 2 + 2,
		mbuttons_y - 1);

	x += 2 * get_image("autokeyframe")->get_w() + toggle_margin;    // the state toggle buttons
	gui->draw_vframe(get_image("panel_divider"),
		x - toggle_margin / 2 - pdw / 2 + 2,
		mbuttons_y - 1);

// Clock
	gui->draw_3segmenth(0, 
		mbuttons_y + get_image("mbutton_bg")->get_h() - 1,
		get_image("patchbay_bg")->get_w(), 
		get_image("clock_bg"));

// Patchbay
	gui->draw_3segmentv(patchbay_x, 
		patchbay_y, 
		patchbay_h, 
		get_image("patchbay_bg"));

// Track canvas
	gui->draw_9segment(mcanvas_x, 
		mcanvas_y, 
		mcanvas_w, 
		mcanvas_h, 
		get_image("tracks_bg"));

// Timebar
	gui->draw_3segmenth(mtimebar_x, 
		mtimebar_y, 
		mtimebar_w, 
		get_image("timebar_bg"));

// Zoombar
#define ZOOMBAR_CENTER 888
	gui->draw_3segmenth(mzoom_x, 
		mzoom_y,
		ZOOMBAR_CENTER, 
		get_image("zoombar_left"));
	if(mzoom_w > ZOOMBAR_CENTER)
		gui->draw_3segmenth(mzoom_x + ZOOMBAR_CENTER, 
			mzoom_y, 
			mzoom_w - ZOOMBAR_CENTER, 
			get_image("zoombar_right"));

// Status
	gui->draw_3segmenth(mstatus_x, 
		mstatus_y,
		ZOOMBAR_CENTER, 
		get_image("statusbar_left"));

	if(mstatus_w > ZOOMBAR_CENTER)
		gui->draw_3segmenth(mstatus_x + ZOOMBAR_CENTER, 
				mstatus_y,
				mstatus_w - ZOOMBAR_CENTER, 
				get_image("statusbar_right"));

}

void BlondTheme::draw_cwindow_bg(CWindowGUI *gui)
{
	const int button_division = 570;
	gui->draw_3segmentv(0, 0, ccomposite_h, get_image("cpanel_bg"));
	gui->draw_3segmenth(0, ccomposite_h, button_division, get_image("cbuttons_left"));
	if(edlsession->cwindow_meter)
	{
		gui->draw_3segmenth(button_division, 
			ccomposite_h, 
			cmeter_x - CWINDOW_METER_MARGIN - button_division, 
			get_image("cbuttons_right"));
		gui->draw_9segment(cmeter_x - CWINDOW_METER_MARGIN, 
			0, 
			mainsession->cwindow_w - cmeter_x + CWINDOW_METER_MARGIN,
			mainsession->cwindow_h,
			get_image("cmeter_bg"));
	}
	else
	{
		gui->draw_3segmenth(button_division, 
			ccomposite_h, 
			cmeter_x - CWINDOW_METER_MARGIN - button_division + 100, 
			get_image("cbuttons_right"));
	}
}

void BlondTheme::draw_vwindow_bg(VWindowGUI *gui)
{
	const int button_division = 400;
	gui->draw_3segmenth(0, 
		vcanvas_h, 
		button_division, 
		get_image("vbuttons_left"));
	if(edlsession->vwindow_meter)
	{
		gui->draw_3segmenth(button_division, 
			vcanvas_h, 
			vmeter_x - VWINDOW_METER_MARGIN - button_division, 
			get_image("cbuttons_right"));
		gui->draw_9segment(vmeter_x - VWINDOW_METER_MARGIN,
			0,
			mainsession->vwindow_w - vmeter_x + VWINDOW_METER_MARGIN,
			mainsession->vwindow_h,
			get_image("cmeter_bg"));
	}
	else
	{
		gui->draw_3segmenth(button_division, 
			vcanvas_h, 
			vmeter_x - VWINDOW_METER_MARGIN - button_division + 100, 
			get_image("cbuttons_right"));
	}
}

void BlondTheme::draw_preferences_bg(PreferencesWindow *gui)
{
	gui->draw_vframe(get_image("preferences_bg"), 0, 0);
}

void BlondTheme::get_new_sizes(NewWindow *gui)
{
}

void BlondTheme::draw_new_bg(NewWindow *gui)
{
	gui->draw_vframe(get_image("new_bg"), 0, 0);
}

void BlondTheme::draw_setformat_bg(SetFormatWindow *gui)
{
	gui->draw_vframe(get_image("setformat_bg"), 0, 0);
}


// pmd: SUV (same), 1_2_2blond (nonexist)
void BlondTheme::get_plugindialog_sizes()
{
	int x = 10, y = 30;
	plugindialog_new_x = x;
	plugindialog_new_y = y;
	plugindialog_shared_x = mainsession->plugindialog_w / 3;
	plugindialog_shared_y = y;
	plugindialog_module_x = mainsession->plugindialog_w * 2 / 3;
	plugindialog_module_y = y;

	plugindialog_new_w = plugindialog_shared_x - plugindialog_new_x - 10;
	plugindialog_new_h = mainsession->plugindialog_h - 120;
	plugindialog_shared_w = plugindialog_module_x - plugindialog_shared_x - 10;
	plugindialog_shared_h = mainsession->plugindialog_h - 120;
	plugindialog_module_w = mainsession->plugindialog_w - plugindialog_module_x - 10;
	plugindialog_module_h = mainsession->plugindialog_h - 120;

	plugindialog_newattach_x = plugindialog_new_x + 20;
	plugindialog_newattach_y = plugindialog_new_y + plugindialog_new_h + 10;
	plugindialog_sharedattach_x = plugindialog_shared_x + 20;
	plugindialog_sharedattach_y = plugindialog_shared_y + plugindialog_shared_h + 10;
	plugindialog_moduleattach_x = plugindialog_module_x + 20;
	plugindialog_moduleattach_y = plugindialog_module_y + plugindialog_module_h + 10;
}
