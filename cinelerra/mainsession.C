#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "guicast.h"
#include "mainsession.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "auto.h"

MainSession::MainSession(MWindow *mwindow)
{
	this->mwindow = mwindow;
	changes_made = 0;
	filename[0] = 0;
//	playback_cursor_visible = 0;
//	is_playing_back = 0;
	track_highlighted = 0;
	plugin_highlighted = 0;
	pluginset_highlighted = 0;
	vcanvas_highlighted = 0;
	ccanvas_highlighted = 0;
	edit_highlighted = 0;
	current_operation = NO_OPERATION;
	drag_pluginservers = new ArrayList<PluginServer*>;
	drag_plugin = 0;
	drag_assets = new ArrayList<Asset*>;
	drag_auto_gang = new ArrayList<Auto*>;
	drag_clips = new ArrayList<EDL*>;
	drag_edits = new ArrayList<Edit*>;
	drag_edit = 0;
	clip_number = 1;
	brender_end = 0;
	cwindow_controls = 1;
	trim_edits = 0;
}

MainSession::~MainSession()
{
	delete drag_pluginservers;
	delete drag_assets;
	delete drag_auto_gang;
	delete drag_clips;
	delete drag_edits;
}

void MainSession::boundaries()
{
	lwindow_x = MAX(0, lwindow_x);
	lwindow_y = MAX(0, lwindow_y);
	mwindow_x = MAX(0, mwindow_x);
	mwindow_y = MAX(0, mwindow_y);
	cwindow_x = MAX(0, cwindow_x);
	cwindow_y = MAX(0, cwindow_y);
	vwindow_x = MAX(0, vwindow_x);
	vwindow_y = MAX(0, vwindow_y);
	awindow_x = MAX(0, awindow_x);
	awindow_y = MAX(0, awindow_y);
	rwindow_x = MAX(0, rwindow_x);
	rwindow_y = MAX(0, rwindow_y);
	rmonitor_x = MAX(0, rmonitor_x);
	rmonitor_y = MAX(0, rmonitor_y);
	cwindow_controls = CLIP(cwindow_controls, 0, 1);
}

void MainSession::default_window_positions()
{
// Get defaults based on root window size
	BC_DisplayInfo display_info;

	int root_x = 0;
	int root_y = 0;
	int root_w = display_info.get_root_w();
	int root_h = display_info.get_root_h();
	int border_left = 0;
	int border_right = 0;
	int border_top = 0;
	int border_bottom = 0;

	border_left = display_info.get_left_border();
	border_top = display_info.get_top_border();
	border_right = display_info.get_right_border();
	border_bottom = display_info.get_bottom_border();

// Wider than 16:9, narrower than dual head
	if((float)root_w / root_h > 1.8) root_w /= 2;



	vwindow_x = root_x;
	vwindow_y = root_y;
	vwindow_w = root_w / 2 - border_left - border_right;
	vwindow_h = root_h * 6 / 10 - border_top - border_bottom;

	cwindow_x = root_x + root_w / 2;
	cwindow_y = root_y;
	cwindow_w = vwindow_w;
	cwindow_h = vwindow_h;

	ctool_x = cwindow_x + cwindow_w / 2;
	ctool_y = cwindow_y + cwindow_h / 2;

	mwindow_x = root_x;
	mwindow_y = vwindow_y + vwindow_h + border_top + border_bottom;
	mwindow_w = root_w * 2 / 3 - border_left - border_right;
	mwindow_h = root_h - mwindow_y - border_top - border_bottom;

	awindow_x = mwindow_x + border_left + border_right + mwindow_w;
	awindow_y = mwindow_y;
	awindow_w = root_x + root_w - awindow_x - border_left - border_right;
	awindow_h = mwindow_h;

	if(mwindow->edl)
		lwindow_w = MeterPanel::get_meters_width(mwindow->edl->session->audio_channels, 1);
	else
		lwindow_w = 100;

	lwindow_y = 0;
	lwindow_x = root_w - lwindow_w;
	lwindow_h = mwindow_y;

	rwindow_x = 0;
	rwindow_y = 0;
	rwindow_h = 500;
	rwindow_w = 650;

	rmonitor_x = rwindow_x + rwindow_w + 10;
	rmonitor_y = rwindow_y;
	rmonitor_w = root_w - rmonitor_x;
	rmonitor_h = rwindow_h;

	batchrender_w = 540;
	batchrender_h = 340;
	batchrender_x = root_w / 2 - batchrender_w / 2;
	batchrender_y = root_h / 2 - batchrender_h / 2;
}

int MainSession::load_defaults(Defaults *defaults)
{
// Setup main windows
	default_window_positions();
	vwindow_x = defaults->get("VWINDOW_X", vwindow_x);
	vwindow_y = defaults->get("VWINDOW_Y", vwindow_y);
	vwindow_w = defaults->get("VWINDOW_W", vwindow_w);
	vwindow_h = defaults->get("VWINDOW_H", vwindow_h);


	cwindow_x = defaults->get("CWINDOW_X", cwindow_x);
	cwindow_y = defaults->get("CWINDOW_Y", cwindow_y);
	cwindow_w = defaults->get("CWINDOW_W", cwindow_w);
	cwindow_h = defaults->get("CWINDOW_H", cwindow_h);

	ctool_x = defaults->get("CTOOL_X", ctool_x);
	ctool_y = defaults->get("CTOOL_Y", ctool_y);


	mwindow_x = defaults->get("MWINDOW_X", mwindow_x);
	mwindow_y = defaults->get("MWINDOW_Y", mwindow_y);
	mwindow_w = defaults->get("MWINDOW_W", mwindow_w);
	mwindow_h = defaults->get("MWINDOW_H", mwindow_h);

	lwindow_x = defaults->get("LWINDOW_X", lwindow_x);
	lwindow_y = defaults->get("LWINDOW_Y", lwindow_y);
	lwindow_w = defaults->get("LWINDOW_W", lwindow_w);
	lwindow_h = defaults->get("LWINDOW_H", lwindow_h);


	awindow_x = defaults->get("AWINDOW_X", awindow_x);
	awindow_y = defaults->get("AWINDOW_Y", awindow_y);
	awindow_w = defaults->get("AWINDOW_W", awindow_w);
	awindow_h = defaults->get("AWINDOW_H", awindow_h);

//printf("MainSession::load_defaults 1\n");

// Other windows
	afolders_w = defaults->get("ABINS_W", 100);
	rwindow_x = defaults->get("RWINDOW_X", rwindow_x);
	rwindow_y = defaults->get("RWINDOW_Y", rwindow_y);
	rwindow_w = defaults->get("RWINDOW_W", rwindow_w);
	rwindow_h = defaults->get("RWINDOW_H", rwindow_h);

	rmonitor_x = defaults->get("RMONITOR_X", rmonitor_x);
	rmonitor_y = defaults->get("RMONITOR_Y", rmonitor_y);
	rmonitor_w = defaults->get("RMONITOR_W", rmonitor_w);
	rmonitor_h = defaults->get("RMONITOR_H", rmonitor_h);

	batchrender_x = defaults->get("BATCHRENDER_X", batchrender_x);
	batchrender_y = defaults->get("BATCHRENDER_Y", batchrender_y);
	batchrender_w = defaults->get("BATCHRENDER_W", batchrender_w);
	batchrender_h = defaults->get("BATCHRENDER_H", batchrender_h);

	show_vwindow = defaults->get("SHOW_VWINDOW", 1);
	show_awindow = defaults->get("SHOW_AWINDOW", 1);
	show_cwindow = defaults->get("SHOW_CWINDOW", 1);
	show_lwindow = defaults->get("SHOW_LWINDOW", 0);

	cwindow_controls = defaults->get("CWINDOW_CONTROLS", cwindow_controls);

	plugindialog_w = defaults->get("PLUGINDIALOG_W", 510);
	plugindialog_h = defaults->get("PLUGINDIALOG_H", 415);
	menueffect_w = defaults->get("MENUEFFECT_W", 580);
	menueffect_h = defaults->get("MENUEFFECT_H", 350);

	boundaries();
	return 0;
}

int MainSession::save_defaults(Defaults *defaults)
{

// Window positions
	defaults->update("MWINDOW_X", mwindow_x);
	defaults->update("MWINDOW_Y", mwindow_y);
	defaults->update("MWINDOW_W", mwindow_w);
	defaults->update("MWINDOW_H", mwindow_h);

	defaults->update("LWINDOW_X", lwindow_x);
	defaults->update("LWINDOW_Y", lwindow_y);
	defaults->update("LWINDOW_W", lwindow_w);
	defaults->update("LWINDOW_H", lwindow_h);

	defaults->update("VWINDOW_X", vwindow_x);
	defaults->update("VWINDOW_Y", vwindow_y);
	defaults->update("VWINDOW_W", vwindow_w);
	defaults->update("VWINDOW_H", vwindow_h);

	defaults->update("CWINDOW_X", cwindow_x);
	defaults->update("CWINDOW_Y", cwindow_y);
	defaults->update("CWINDOW_W", cwindow_w);
	defaults->update("CWINDOW_H", cwindow_h);

	defaults->update("CTOOL_X", ctool_x);
	defaults->update("CTOOL_Y", ctool_y);

	defaults->update("AWINDOW_X", awindow_x);
	defaults->update("AWINDOW_Y", awindow_y);
	defaults->update("AWINDOW_W", awindow_w);
	defaults->update("AWINDOW_H", awindow_h);

 	defaults->update("ABINS_W", afolders_w);

	defaults->update("RMONITOR_X", rmonitor_x);
	defaults->update("RMONITOR_Y", rmonitor_y);
	defaults->update("RMONITOR_W", rmonitor_w);
	defaults->update("RMONITOR_H", rmonitor_h);

	defaults->update("RWINDOW_X", rwindow_x);
	defaults->update("RWINDOW_Y", rwindow_y);
	defaults->update("RWINDOW_W", rwindow_w);
	defaults->update("RWINDOW_H", rwindow_h);

	defaults->update("BATCHRENDER_X", batchrender_x);
	defaults->update("BATCHRENDER_Y", batchrender_y);
	defaults->update("BATCHRENDER_W", batchrender_w);
	defaults->update("BATCHRENDER_H", batchrender_h);

	defaults->update("SHOW_VWINDOW", show_vwindow);
	defaults->update("SHOW_AWINDOW", show_awindow);
	defaults->update("SHOW_CWINDOW", show_cwindow);
	defaults->update("SHOW_LWINDOW", show_lwindow);

	defaults->update("CWINDOW_CONTROLS", cwindow_controls);

	defaults->update("PLUGINDIALOG_W", plugindialog_w);
	defaults->update("PLUGINDIALOG_H", plugindialog_h);

	defaults->update("MENUEFFECT_W", menueffect_w);
	defaults->update("MENUEFFECT_H", menueffect_h);



	return 0;
}
