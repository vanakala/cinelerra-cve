#include "asset.h"
#include "autoconf.h"
#include "automation.h"
#include "bcsignals.h"
#include "bezierauto.h"
#include "bezierautos.h"
#include "clip.h"
#include "colors.h"
#include "cplayback.h"
#include "cursors.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edithandles.h"
#include "editpopup.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "intauto.h"
#include "keyframe.h"
#include "keyframes.h"
#include "keys.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainundo.h"
#include "maskautos.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "tracking.h"
#include "panautos.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "plugin.h"
#include "pluginpopup.h"
#include "pluginset.h"
#include "pluginserver.h"
#include "preferences.h"
#include "resourcepixmap.h"
#include "mainsession.h"
#include "transitionhandles.h"
#include "transitionpopup.h"
#include "transportque.h"
#include "zoombar.h"
#include "theme.h"
#include "intautos.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "vframe.h"

#include <string.h>

TrackCanvas::TrackCanvas(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mcanvas_x,
 	mwindow->theme->mcanvas_y,
	gui->view_w,
	gui->view_h)
{
	this->mwindow = mwindow;
	this->gui = gui;
	current_end = 0;
	selection_midpoint1 = selection_midpoint2 = 0;
	selection_type = 0;
	region_selected = 0;
	handle_selected = 0;
	auto_selected = 0;
	translate_selected = 0;
	which_handle = 0;
	handle_pixel = 0;
	drag_scroll = 0;
	drag_pixmap = 0;
	drag_popup = 0;
	active = 0;
}

TrackCanvas::~TrackCanvas()
{
	for(int i = 0; i < resource_pixmaps.total; i++)
		delete resource_pixmaps.values[i];
//	delete transition_handles;
	delete edit_handles;
	delete keyframe_pixmap;
	delete camerakeyframe_pixmap;
	delete modekeyframe_pixmap;
	delete pankeyframe_pixmap;
	delete projectorkeyframe_pixmap;
	delete maskkeyframe_pixmap;
	delete background_pixmap;
}

int TrackCanvas::create_objects()
{
	background_pixmap = new BC_Pixmap(this, get_w(), get_h());
//	transition_handles = new TransitionHandles(mwindow, this);
	edit_handles = new EditHandles(mwindow, this);
	keyframe_pixmap = new BC_Pixmap(this, mwindow->theme->keyframe_data, PIXMAP_ALPHA);
	camerakeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->camerakeyframe_data, PIXMAP_ALPHA);
	modekeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->modekeyframe_data, PIXMAP_ALPHA);
	pankeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->pankeyframe_data, PIXMAP_ALPHA);
	projectorkeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->projectorkeyframe_data, PIXMAP_ALPHA);
	maskkeyframe_pixmap = new BC_Pixmap(this, mwindow->theme->maskkeyframe_data, PIXMAP_ALPHA);
	draw();
	update_cursor();
	flash();
	return 0;
}

void TrackCanvas::resize_event()
{
//printf("TrackCanvas::resize_event 1\n");
	draw(0, 0);
	flash();
//printf("TrackCanvas::resize_event 2\n");
}

int TrackCanvas::keypress_event()
{
	int result = 0;

	switch(get_keypress())
	{
		case LEFT:
			if(!ctrl_down()) 
			{ 
				mwindow->move_left(); 
				result = 1; 
			}
			break;
		case RIGHT:
			if(!ctrl_down()) 
			{ 
				mwindow->move_right(); 
				result = 1; 
			}
			break;

		case UP:
			if(!ctrl_down())
			{
				mwindow->expand_sample();
				result = 1;
			}
			else
			{
				mwindow->expand_y();
				result = 1;
			}
			break;

		case DOWN:
			if(!ctrl_down())
			{
				mwindow->zoom_in_sample();
				result = 1;
			}
			else
			{
				mwindow->zoom_in_y();
				result = 1;
			}
			break;

		case PGUP:
			if(!ctrl_down())
			{
				mwindow->move_up();
				result = 1;
			}
			else
			{
				mwindow->expand_t();
				result = 1;
			}
			break;

		case PGDN:
			if(!ctrl_down())
			{
				mwindow->move_down();
				result = 1;
			}
			else
			{
				mwindow->zoom_in_t();
				result = 1;
			}
			break;
	}

	return result;
}

int TrackCanvas::drag_motion()
{

	int cursor_x = get_relative_cursor_x();
	int cursor_y = get_relative_cursor_y();
	Track *over_track = 0;
	Edit *over_edit = 0;
	PluginSet *over_pluginset = 0;
	Plugin *over_plugin = 0;
	int redraw = 0;


	if(drag_popup)
	{
		drag_popup->cursor_motion_event();
	}




	if(get_cursor_over_window() &&
		cursor_x >= 0 && 
		cursor_y >= 0 && 
		cursor_x < get_w() && 
		cursor_y < get_h())
	{
// Find the edit and track the cursor is over
		for(Track *track = mwindow->edl->tracks->first; track; track = track->next)
		{
			int64_t track_x, track_y, track_w, track_h;
			track_dimensions(track, track_x, track_y, track_w, track_h);

			if(cursor_y >= track_y && 
				cursor_y < track_y + track_h)
			{
				over_track = track;
				for(Edit *edit = track->edits->first; edit; edit = edit->next)
				{
					int64_t edit_x, edit_y, edit_w, edit_h;
					edit_dimensions(edit, edit_x, edit_y, edit_w, edit_h);

					if(cursor_x >= edit_x && 
						cursor_y >= edit_y && 
						cursor_x < edit_x + edit_w && 
						cursor_y < edit_y + edit_h)
					{
						over_edit = edit;
						break;
					}
				}

				for(int i = 0; i < track->plugin_set.total; i++)
				{
					PluginSet *pluginset = track->plugin_set.values[i];
					


					for(Plugin *plugin = (Plugin*)pluginset->first;
						plugin;
						plugin = (Plugin*)plugin->next)
					{
						int64_t plugin_x, plugin_y, plugin_w, plugin_h;
						plugin_dimensions(plugin, plugin_x, plugin_y, plugin_w, plugin_h);
						
						if(cursor_y >= plugin_y &&
							cursor_y < plugin_y + plugin_h)
						{
							over_pluginset = plugin->plugin_set;
						
							if(cursor_x >= plugin_x &&
								cursor_x < plugin_x + plugin_w)
							{
								over_plugin = plugin;
								break;
							}
						}
					}
				}
				break;
			}
		}
	}

	if(mwindow->session->track_highlighted != over_track) 
	{
		mwindow->session->track_highlighted = over_track;
		redraw = 1;
	}

	if(mwindow->session->edit_highlighted != over_edit)
	{
		mwindow->session->edit_highlighted = over_edit;
		redraw = 1;
	}

	if(mwindow->session->pluginset_highlighted != over_pluginset)
	{
		mwindow->session->pluginset_highlighted = over_pluginset;
		redraw = 1;
	}

	if(mwindow->session->plugin_highlighted != over_plugin)
	{
		mwindow->session->plugin_highlighted = over_plugin;
		redraw = 1;
	}

//printf("TrackCanvas::drag_motion 2 %p\n", mwindow->session->track_highlighted);
	if(redraw)
	{
		lock_window("TrackCanvas::drag_motion");
		draw_overlays();
		flash();
		unlock_window();
	}

	return 0;
}

int TrackCanvas::drag_start_event()
{
	int result = 0;
	int redraw = 0;
	int rerender = 0;
	int new_cursor, update_cursor;

	if(mwindow->session->current_operation != NO_OPERATION) return 0;

	if(is_event_win())
	{
		if(test_plugins(get_drag_x(), 
			get_drag_y(), 
			1,
			0))
		{
			result = 1;
		}
		else
		if(test_edits(get_drag_x(),
			get_drag_y(),
			0,
			1,
			redraw,
			rerender,
			new_cursor,
			update_cursor))
		{
			result = 1;
		}
	}

	return result;
}

int TrackCanvas::drag_motion_event()
{
	return drag_motion();
}

int TrackCanvas::drag_stop_event()
{
	int result = drag_stop();

	if(drag_popup)
	{
//printf("TrackCanvas::drag_stop_event 1 %p\n", drag_popup);
		delete drag_popup;
		delete drag_pixmap;
		drag_popup = 0;
		drag_pixmap = 0;
	}
	return result;
}


int TrackCanvas::drag_stop()
{
// In most cases the editing routine redraws and not the drag_stop
	int result = 0, redraw = 0;

	switch(mwindow->session->current_operation)
	{
		case DRAG_VTRANSITION:
		case DRAG_ATRANSITION:
			if(mwindow->session->edit_highlighted)
			{
				if((mwindow->session->current_operation == DRAG_ATRANSITION &&
					mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
					(mwindow->session->current_operation == DRAG_VTRANSITION &&
					mwindow->session->track_highlighted->data_type == TRACK_VIDEO))
				{
					mwindow->session->current_operation = NO_OPERATION;
					mwindow->paste_transition();
 					result = 1;
				}
			}
			redraw = 1;
			break;




// Behavior for dragged plugins is limited by the fact that a shared plugin
// can only refer to a standalone plugin that exists in the same position in
// time.  Dragging a plugin from one point in time to another can't produce
// a shared plugin to the original plugin.  In this case we relocate the
// plugin instead of sharing it.
		case DRAG_AEFFECT_COPY:
		case DRAG_VEFFECT_COPY:
			if(mwindow->session->track_highlighted &&
				((mwindow->session->current_operation == DRAG_AEFFECT_COPY &&
					mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
					(mwindow->session->current_operation == DRAG_VEFFECT_COPY &&
					mwindow->session->track_highlighted->data_type == TRACK_VIDEO)))
			{
				mwindow->session->current_operation = NO_OPERATION;

// Insert shared plugin in source
				if(mwindow->session->track_highlighted != mwindow->session->drag_plugin->track &&
					!mwindow->session->plugin_highlighted &&
					!mwindow->session->pluginset_highlighted)
				{
// Move plugin if different startproject
					mwindow->move_effect(mwindow->session->drag_plugin,
						0,
						mwindow->session->track_highlighted,
						0);
					result = 1;
				}
				else
// Move source to different location
				if(mwindow->session->pluginset_highlighted)
				{
//printf("TrackCanvas::drag_stop 6\n");
					if(mwindow->session->plugin_highlighted)
					{
						mwindow->move_effect(mwindow->session->drag_plugin,
							mwindow->session->plugin_highlighted->plugin_set,
							0,
							mwindow->session->plugin_highlighted->startproject);
						result = 1;
					}
					else
					{
						mwindow->move_effect(mwindow->session->drag_plugin,
							mwindow->session->pluginset_highlighted,
							0,
							mwindow->session->pluginset_highlighted->length());
						result = 1;
					}
				}
				else
// Move to a new plugin set between two edits
				if(mwindow->session->edit_highlighted)
				{
					mwindow->move_effect(mwindow->session->drag_plugin,
						0,
						mwindow->session->track_highlighted,
						mwindow->session->edit_highlighted->startproject);
					result = 1;
				}
				else
// Move to a new plugin set
				if(mwindow->session->track_highlighted)
				{
					mwindow->move_effect(mwindow->session->drag_plugin,
						0,
						mwindow->session->track_highlighted,
						0);
					result = 1;
				}
			}
			break;

		case DRAG_AEFFECT:
		case DRAG_VEFFECT:
			if(mwindow->session->track_highlighted && 
				((mwindow->session->current_operation == DRAG_AEFFECT &&
				mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
				(mwindow->session->current_operation == DRAG_VEFFECT &&
				mwindow->session->track_highlighted->data_type == TRACK_VIDEO)))
			{
// Drop all the effects
				PluginSet *plugin_set = mwindow->session->pluginset_highlighted;
				Track *track = mwindow->session->track_highlighted;
				double start = 0;
				double length = track->get_length();

				if(mwindow->session->plugin_highlighted)
				{
					start = track->from_units(mwindow->session->plugin_highlighted->startproject);
					length = track->from_units(mwindow->session->plugin_highlighted->length);
					if(length <= 0) length = track->get_length();
				}
				else
				if(mwindow->session->pluginset_highlighted)
				{
					start = track->from_units(plugin_set->length());
					length = track->get_length() - start;
					if(length <= 0) length = track->get_length();
				}
				else
				if(mwindow->edl->local_session->get_selectionend() > 
					mwindow->edl->local_session->get_selectionstart())
				{
					start = mwindow->edl->local_session->get_selectionstart();
					length = mwindow->edl->local_session->get_selectionend() - 
						mwindow->edl->local_session->get_selectionstart();
				}
// Move to a point between two edits
// 				else
// 				if(mwindow->session->edit_highlighted)
// 				{
// 					start = mwindow->session->track_highlighted->from_units(
// 						mwindow->session->edit_highlighted->startproject);
// 					length = mwindow->session->track_highlighted->from_units(
// 						mwindow->session->edit_highlighted->length);
// 				}

				mwindow->insert_effects_canvas(start, length);
				result = 1;
				redraw = 1;
			}
			break;

		case DRAG_ASSET:
			if(mwindow->session->track_highlighted)
			{
				int64_t position = mwindow->session->edit_highlighted ?
					mwindow->session->edit_highlighted->startproject :
					mwindow->session->track_highlighted->edits->length();
				double position_f = mwindow->session->track_highlighted->from_units(position);
				Track *track = mwindow->session->track_highlighted;
				mwindow->session->track_highlighted = 0;
				result = mwindow->paste_assets(position_f, track);
			}
			break;

		case DRAG_EDIT:
			mwindow->session->current_operation = NO_OPERATION;
			if(mwindow->session->track_highlighted)
			{
				if(mwindow->session->track_highlighted->data_type == mwindow->session->drag_edit->track->data_type)
				{
					int64_t position = mwindow->session->edit_highlighted ?
						mwindow->session->edit_highlighted->startproject :
						mwindow->session->track_highlighted->edits->length();
					double position_f = mwindow->session->track_highlighted->from_units(position);
					Track *track = mwindow->session->track_highlighted;
					mwindow->session->track_highlighted = 0;
					mwindow->move_edits(mwindow->session->drag_edits,
						track,
						position_f);
				}

				result = 1;
			}
			break;
	}

	if(result)
	{
		mwindow->session->track_highlighted = 0;
		mwindow->session->edit_highlighted = 0;
		mwindow->session->plugin_highlighted = 0;
		mwindow->session->current_operation = NO_OPERATION;
	}


//printf("TrackCanvas::drag_stop %d %d\n", redraw, mwindow->session->current_operation);
	if(redraw)
	{
		mwindow->edl->tracks->update_y_pixels(mwindow->theme);
		gui->get_scrollbars();
		draw();
		gui->patchbay->update();
		gui->cursor->update();
		flash();
		flush();
	}

	return result;
}


void TrackCanvas::draw(int force, int hide_cursor)
{
// Swap pixmap layers
TRACE("TrackCanvas::draw 1")
 	if(get_w() != background_pixmap->get_w() ||
 		get_h() != background_pixmap->get_h())
 	{
 		delete background_pixmap;
 		background_pixmap = new BC_Pixmap(this, get_w(), get_h());
 	}

TRACE("TrackCanvas::draw 10")
// Cursor disappears after resize when this is called.
// Cursor doesn't redraw after editing when this isn't called.
	if(gui->cursor && hide_cursor) gui->cursor->hide();
//printf("TrackCanvas::draw 3\n");
	draw_top_background(get_parent(), 0, 0, get_w(), get_h(), background_pixmap);
//printf("TrackCanvas::draw 4\n");
	draw_resources(force);
//printf("TrackCanvas::draw 5\n");
	draw_overlays();
UNTRACE
}

void TrackCanvas::update_cursor()
{
	switch(mwindow->edl->session->editing_mode)
	{
		case EDITING_ARROW: set_cursor(ARROW_CURSOR); break;
		case EDITING_IBEAM: set_cursor(IBEAM_CURSOR); break;
	}
}


void TrackCanvas::draw_indexes(Asset *asset)
{
// Don't redraw raw samples
	if(asset->index_zoom > mwindow->edl->local_session->zoom_sample)
		return;

	draw_resources(0, 1, asset);

	draw_overlays();
	draw_automation();
	flash();
	flush();
}

void TrackCanvas::draw_resources(int force, 
	int indexes_only, 
	Asset *index_asset)
{
// Age resource pixmaps for deletion
	if(!indexes_only)
		for(int i = 0; i < resource_pixmaps.total; i++)
			resource_pixmaps.values[i]->visible--;

	if(force)
		resource_pixmaps.remove_all_objects();

//printf("TrackCanvas::draw_resources 1 %d %d\n", force, indexes_only);

// Search every edit
	for(Track *current = mwindow->edl->tracks->first;
		current;
		current = NEXT)
	{
//printf("TrackCanvas::draw_resources 2\n");
		for(Edit *edit = current->edits->first; edit; edit = edit->next)
		{
//printf("TrackCanvas::draw_resources 3\n");
			if(!edit->asset) continue;
//printf("TrackCanvas::draw_resources 4\n");
			if(indexes_only)
			{
				if(edit->track->data_type != TRACK_AUDIO) continue;
				if(!edit->asset->test_path(index_asset->path)) continue;
			}

			int64_t edit_x, edit_y, edit_w, edit_h;
			edit_dimensions(edit, edit_x, edit_y, edit_w, edit_h);
//printf("TrackCanvas::draw_resources 10\n");

// Edit is visible
			if(MWindowGUI::visible(edit_x, edit_x + edit_w, 0, get_w()) &&
				MWindowGUI::visible(edit_y, edit_y + edit_h, 0, get_h()))
			{
				int64_t pixmap_x, pixmap_w, pixmap_h;

//printf("TrackCanvas::draw_resources 20\n");
// Search for existing pixmap containing edit
				for(int i = 0; i < resource_pixmaps.total; i++)
				{
					ResourcePixmap* pixmap = resource_pixmaps.values[i];
//printf("TrackCanvas::draw_resources 30\n");
// Same pointer can be different edit if editing took place
					if(pixmap->edit_id == edit->id)
					{
//printf("TrackCanvas::draw_resources 40\n");
						pixmap->visible = 1;
						break;
					}
//printf("TrackCanvas::draw_resources 50\n");
				}

// Get new size, offset of pixmap needed
				get_pixmap_size(edit, 
					edit_x, 
					edit_w, 
					pixmap_x, 
					pixmap_w, 
					pixmap_h);
//printf("TrackCanvas::draw_resources 60\n");

// Draw new data
				if(pixmap_w && pixmap_h)
				{
// Create pixmap if it doesn't exist
					ResourcePixmap* pixmap = create_pixmap(edit, 
						edit_x, 
						pixmap_x, 
						pixmap_w, 
						pixmap_h);
//printf("TrackCanvas::draw_resources 70\n");
// Resize it if it's bigger
					if(pixmap_w > pixmap->pixmap_w ||
						pixmap_h > pixmap->pixmap_h)
						pixmap->resize(pixmap_w, pixmap_h);
//printf("TrackCanvas::draw_resources 80\n");
					pixmap->draw_data(edit,
						edit_x, 
						edit_w, 
						pixmap_x, 
						pixmap_w, 
						pixmap_h, 
						force,
						indexes_only);
//printf("TrackCanvas::draw_resources 90\n");
// Resize it if it's smaller
					if(pixmap_w < pixmap->pixmap_w ||
						pixmap_h < pixmap->pixmap_h)
						pixmap->resize(pixmap_w, pixmap_h);
//printf("TrackCanvas::draw_resources 100\n");
// Copy pixmap to background canvas
					background_pixmap->draw_pixmap(pixmap, 
						pixmap->pixmap_x, 
						current->y_pixel,
						pixmap->pixmap_w,
						edit_h);
//printf("TrackCanvas::draw_resources 110\n");
				}
			}
		}
	}
//printf("TrackCanvas::draw_resources 200\n");

// Delete unused pixmaps
	if(!indexes_only)
		for(int i = resource_pixmaps.total - 1; i >= 0; i--)
			if(resource_pixmaps.values[i]->visible < -5)
			{
				delete resource_pixmaps.values[i];
				resource_pixmaps.remove(resource_pixmaps.values[i]);
			}
}

ResourcePixmap* TrackCanvas::create_pixmap(Edit *edit, 
	int64_t edit_x, 
	int64_t pixmap_x, 
	int64_t pixmap_w, 
	int64_t pixmap_h)
{
	ResourcePixmap *result = 0;

	for(int i = 0; i < resource_pixmaps.total; i++)
	{
//printf("TrackCanvas::create_pixmap 1 %d %d\n", edit->id, resource_pixmaps.values[i]->edit->id);
		if(resource_pixmaps.values[i]->edit_id == edit->id) 
		{
			result = resource_pixmaps.values[i];
			break;
		}
	}

	if(!result)
	{
//printf("TrackCanvas::create_pixmap 2\n");
		result = new ResourcePixmap(mwindow, 
			this, 
			edit, 
			pixmap_w, 
			pixmap_h);
		resource_pixmaps.append(result);
	}

//	result->resize(pixmap_w, pixmap_h);
	return result;
}

void TrackCanvas::get_pixmap_size(Edit *edit, 
	int64_t edit_x, 
	int64_t edit_w, 
	int64_t &pixmap_x, 
	int64_t &pixmap_w,
	int64_t &pixmap_h)
{

// Align x on frame boundaries


// 	switch(edit->edits->track->data_type)
// 	{
// 		case TRACK_AUDIO:

			pixmap_x = edit_x;
			pixmap_w = edit_w;
			if(pixmap_x < 0)
			{
				pixmap_w -= -edit_x;
				pixmap_x = 0;
			}

			if(pixmap_x + pixmap_w > get_w())
			{
				pixmap_w = get_w() - pixmap_x;
			}

// 			break;
// 
// 		case TRACK_VIDEO:
// 		{
// 			int64_t picon_w = (int64_t)(edit->picon_w() + 0.5);
// 			int64_t frame_w = (int64_t)(edit->frame_w() + 0.5);
// 			int64_t pixel_increment = MAX(picon_w, frame_w);
// 			int64_t pixmap_x1 = edit_x;
// 			int64_t pixmap_x2 = edit_x + edit_w;
// 
// 			if(pixmap_x1 < 0)
// 			{
// 				pixmap_x1 = (int64_t)((double)-edit_x / pixel_increment) * 
// 					pixel_increment + 
// 					edit_x;
// 			}
// 
// 			if(pixmap_x2 > get_w())
// 			{
// 				pixmap_x2 = (int64_t)((double)(get_w() - edit_x) / pixel_increment + 1) * 
// 					pixel_increment + 
// 					edit_x;
// 			}
// 			pixmap_x = pixmap_x1;
// 			pixmap_w = pixmap_x2 - pixmap_x1;
// 			break;
// 		}
// 	}

	pixmap_h = mwindow->edl->local_session->zoom_track;
	if(mwindow->edl->session->show_titles) pixmap_h += mwindow->theme->title_bg_data->get_h();
//printf("get_pixmap_size %d %d %d %d\n", edit_x, edit_w, pixmap_x, pixmap_w);
}

void TrackCanvas::edit_dimensions(Edit *edit, 
	int64_t &x, 
	int64_t &y, 
	int64_t &w, 
	int64_t &h)
{
//printf("TrackCanvas::edit_dimensions 1 %p\n", edit->track);
	w = Units::round(edit->track->from_units(edit->length) * 
		mwindow->edl->session->sample_rate / 
		mwindow->edl->local_session->zoom_sample);
//printf("TrackCanvas::edit_dimensions 1\n");

	h = resource_h();

//printf("TrackCanvas::edit_dimensions 1\n");
	x = Units::round(edit->track->from_units(edit->startproject) * 
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start);

//printf("TrackCanvas::edit_dimensions 1\n");
	y = edit->edits->track->y_pixel;
//printf("TrackCanvas::edit_dimensions 1\n");

	if(mwindow->edl->session->show_titles) 
		h += mwindow->theme->title_bg_data->get_h();
//printf("TrackCanvas::edit_dimensions 2\n");
}

void TrackCanvas::track_dimensions(Track *track, int64_t &x, int64_t &y, int64_t &w, int64_t &h)
{
	x = 0;
	w = get_w();
	y = track->y_pixel;
	h = track->vertical_span(mwindow->theme);
}


void TrackCanvas::draw_paste_destination()
{
	int cursor_x = get_cursor_x();
	int cursor_y = get_cursor_y();
	int current_atrack = 0;
	int current_vtrack = 0;
	int current_aedit = 0;
	int current_vedit = 0;
	int64_t w = 0;
	int64_t x;
	double position;

//printf("TrackCanvas::draw_paste_destination 1\n");
	if((mwindow->session->current_operation == DRAG_ASSET &&
			(mwindow->session->drag_assets->total ||
			mwindow->session->drag_clips->total)) ||
		(mwindow->session->current_operation == DRAG_EDIT &&
			mwindow->session->drag_edits->total))
	{
//printf("TrackCanvas::draw_paste_destination 1\n");
		Asset *asset = 0;
		EDL *clip = 0;
		int draw_box = 0;

		if(mwindow->session->current_operation == DRAG_ASSET &&
			mwindow->session->drag_assets->total)
			asset = mwindow->session->drag_assets->values[0];

		if(mwindow->session->current_operation == DRAG_ASSET &&
			mwindow->session->drag_clips->total)
			clip = mwindow->session->drag_clips->values[0];

// Get destination track
		for(Track *dest = mwindow->session->track_highlighted; 
			dest; 
			dest = dest->next)
		{
			if(dest->record)
			{
// Get source width in pixels
				w = 0;


// Use start of highlighted edit
				if(mwindow->session->edit_highlighted)
					position = mwindow->session->track_highlighted->from_units(
						mwindow->session->edit_highlighted->startproject);
				else
// Use end of highlighted track, disregarding effects
					position = mwindow->session->track_highlighted->from_units(
						mwindow->session->track_highlighted->edits->length());

// Get the x coordinate
				x = Units::to_int64(position * 
					mwindow->edl->session->sample_rate /
					mwindow->edl->local_session->zoom_sample) - 
					mwindow->edl->local_session->view_start;

				if(dest->data_type == TRACK_AUDIO)
				{
					if(asset && current_atrack < asset->channels)
					{
						w = Units::to_int64((double)asset->audio_length /
							asset->sample_rate *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample);
						current_atrack++;
						draw_box = 1;
					}
					else
					if(clip && current_atrack < clip->tracks->total_audio_tracks())
					{
						w = Units::to_int64((double)clip->tracks->total_length() *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample);
//printf("draw_paste_destination %d\n", x);
						current_atrack++;
						draw_box = 1;
					}
					else
					if(mwindow->session->current_operation == DRAG_EDIT &&
						current_aedit < mwindow->session->drag_edits->total)
					{
						Edit *edit;
						while(current_aedit < mwindow->session->drag_edits->total &&
							mwindow->session->drag_edits->values[current_aedit]->track->data_type != TRACK_AUDIO)
							current_aedit++;

						if(current_aedit < mwindow->session->drag_edits->total)
						{
							edit = mwindow->session->drag_edits->values[current_aedit];
							w = Units::to_int64(edit->length / mwindow->edl->local_session->zoom_sample);

							current_aedit++;
							draw_box = 1;
						}
					}
				}
				else
				if(dest->data_type == TRACK_VIDEO)
				{
//printf("draw_paste_destination 1\n");
					if(asset && current_vtrack < asset->layers)
					{
						w = Units::to_int64((double)asset->video_length / 
							asset->frame_rate *
							mwindow->edl->session->sample_rate /
							mwindow->edl->local_session->zoom_sample);
						current_vtrack++;
						draw_box = 1;
					}
					else
					if(clip && current_vtrack < clip->tracks->total_video_tracks())
					{
						w = Units::to_int64(clip->tracks->total_length() *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample);
						current_vtrack++;
						draw_box = 1;
					}
					else
					if(mwindow->session->current_operation == DRAG_EDIT &&
						current_vedit < mwindow->session->drag_edits->total)
					{
//printf("draw_paste_destination 2\n");
						Edit *edit;
						while(current_vedit < mwindow->session->drag_edits->total &&
							mwindow->session->drag_edits->values[current_vedit]->track->data_type != TRACK_VIDEO)
							current_vedit++;

						if(current_vedit < mwindow->session->drag_edits->total)
						{
//printf("draw_paste_destination 3\n");
							edit = mwindow->session->drag_edits->values[current_vedit];
							w = Units::to_int64(edit->track->from_units(edit->length) *
								mwindow->edl->session->sample_rate / 
								mwindow->edl->local_session->zoom_sample);

							current_vedit++;
							draw_box = 1;
						}
					}
				}

				if(w)
				{
					int y = dest->y_pixel;
					int h = dest->vertical_span(mwindow->theme);


//printf("TrackCanvas::draw_paste_destination 2 %d %d %d %d\n", x, y, w, h);
					if(x < -BC_INFINITY)
					{
						w -= -BC_INFINITY - x;
						x += -BC_INFINITY - x;
					}
					w = MIN(65535, w);
					draw_highlight_rectangle(x, y, w, h);
				}
			}
		}
	}
}

void TrackCanvas::plugin_dimensions(Plugin *plugin, int64_t &x, int64_t &y, int64_t &w, int64_t &h)
{
	x = Units::round(plugin->track->from_units(plugin->startproject) *
		mwindow->edl->session->sample_rate / 
		mwindow->edl->local_session->zoom_sample - 
		mwindow->edl->local_session->view_start);
	w = Units::round(plugin->track->from_units(plugin->length) *
		mwindow->edl->session->sample_rate / 
		mwindow->edl->local_session->zoom_sample);
	y = plugin->track->y_pixel + 
			mwindow->edl->local_session->zoom_track +
			plugin->plugin_set->get_number() * 
			mwindow->theme->plugin_bg_data->get_h();
	if(mwindow->edl->session->show_titles)
		y += mwindow->theme->title_bg_data->get_h();
	h = mwindow->theme->plugin_bg_data->get_h();
}

int TrackCanvas::resource_h()
{
	return mwindow->edl->local_session->zoom_track;
}

void TrackCanvas::draw_highlight_rectangle(int x, int y, int w, int h)
{
	if(x < -10)
	{
		w += x - -10;
		x = -10;
	}
	if(y < -10)
	{
		h += y - -10;
		y = -10;
	}
	w = MIN(w, get_w() + 20);
	h = MIN(h, get_h() + 20);
	set_color(WHITE);
	set_inverse();
	draw_rectangle(x, y, w, h);
	draw_rectangle(x + 1, y + 1, w - 2, h - 2);
	set_opaque();
//printf("TrackCanvas::draw_highlight_rectangle %d %d %d %d\n", x, y, w, h);
}

void TrackCanvas::draw_playback_cursor()
{
// Called before playback_cursor exists
// 	if(mwindow->playback_cursor && mwindow->playback_cursor->visible)
// 	{
// 		mwindow->playback_cursor->visible = 0;
// 		mwindow->playback_cursor->draw();
// 	}
}

void TrackCanvas::get_handle_coords(Edit *edit, int64_t &x, int64_t &y, int64_t &w, int64_t &h, int side)
{
	int handle_w = mwindow->theme->edithandlein_data[0]->get_w();
	int handle_h = mwindow->theme->edithandlein_data[0]->get_h();

	edit_dimensions(edit, x, y, w, h);

	if(mwindow->edl->session->show_titles)
	{
		y += mwindow->theme->title_bg_data->get_h();
	}
	else
	{
		y = 0;
	}

	if(side == EDIT_OUT)
	{
		x += w - handle_w;
	}

	h = handle_h;
	w = handle_w;
}

void TrackCanvas::get_transition_coords(int64_t &x, int64_t &y, int64_t &w, int64_t &h)
{
//printf("TrackCanvas::get_transition_coords 1\n");
// 	int transition_w = mwindow->theme->transitionhandle_data[0]->get_w();
// 	int transition_h = mwindow->theme->transitionhandle_data[0]->get_h();
	int transition_w = 30;
	int transition_h = 30;
//printf("TrackCanvas::get_transition_coords 1\n");

	if(mwindow->edl->session->show_titles)
		y += mwindow->theme->title_bg_data->get_h();
//printf("TrackCanvas::get_transition_coords 2\n");

	y += (h - mwindow->theme->title_bg_data->get_h()) / 2 - transition_h / 2;
	x -= transition_w / 2;

	h = transition_h;
	w = transition_w;
}

void TrackCanvas::draw_highlighting()
{
	int64_t x, y, w, h;
	int draw_box = 0;
//printf("TrackCanvas::draw_highlighting 1 %p %d\n", mwindow->session->track_highlighted, mwindow->session->current_operation);



	switch(mwindow->session->current_operation)
	{
		case DRAG_ATRANSITION:
		case DRAG_VTRANSITION:
//printf("TrackCanvas::draw_highlighting 1 %p %p\n", 
//	mwindow->session->track_highlighted, mwindow->session->edit_highlighted);
			if(mwindow->session->edit_highlighted)
			{
//printf("TrackCanvas::draw_highlighting 2\n");
				if((mwindow->session->current_operation == DRAG_ATRANSITION && 
					mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
					(mwindow->session->current_operation == DRAG_VTRANSITION && 
					mwindow->session->track_highlighted->data_type == TRACK_VIDEO))
				{
//printf("TrackCanvas::draw_highlighting 2\n");
					edit_dimensions(mwindow->session->edit_highlighted, x, y, w, h);
//printf("TrackCanvas::draw_highlighting 2\n");

					if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
						MWindowGUI::visible(y, y + h, 0, get_h()))
					{
						draw_box = 1;
						get_transition_coords(x, y, w, h);
					}
//printf("TrackCanvas::draw_highlighting 3\n");
				}
			}
			break;



// Dragging a new effect from the Resource window
		case DRAG_AEFFECT:
		case DRAG_VEFFECT:
			if(mwindow->session->track_highlighted &&
				((mwindow->session->current_operation == DRAG_AEFFECT && mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
					(mwindow->session->current_operation == DRAG_VEFFECT && mwindow->session->track_highlighted->data_type == TRACK_VIDEO)))
			{
// Put it before another plugin
				if(mwindow->session->plugin_highlighted)
				{
					plugin_dimensions(mwindow->session->plugin_highlighted, 
						x, 
						y, 
						w, 
						h);
//printf("TrackCanvas::draw_highlighting 1 %d %d\n", x, w);
				}
				else
// Put it after a plugin set
				if(mwindow->session->pluginset_highlighted &&
					mwindow->session->pluginset_highlighted->last)
				{
					plugin_dimensions((Plugin*)mwindow->session->pluginset_highlighted->last, 
						x, 
						y, 
						w, 
						h);
//printf("TrackCanvas::draw_highlighting 1 %d %d\n", x, w);
					int64_t track_x, track_y, track_w, track_h;
					track_dimensions(mwindow->session->track_highlighted, 
						track_x, 
						track_y, 
						track_w, 
						track_h);

					x += w;
					w = Units::round(
							mwindow->session->track_highlighted->get_length() *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample - 
							mwindow->edl->local_session->view_start) -
						x;
//printf("TrackCanvas::draw_highlighting 2 %d\n", w);
					if(w <= 0) w = track_w;
				}
				else
				{
					track_dimensions(mwindow->session->track_highlighted, 
						x, 
						y, 
						w, 
						h);

//printf("TrackCanvas::draw_highlighting 1 %d %d %d %d\n", x, y, w, h);
// Put it in a new plugin set determined by the selected range
					if(mwindow->edl->local_session->get_selectionend() > 
						mwindow->edl->local_session->get_selectionstart())
					{
						x = Units::to_int64(mwindow->edl->local_session->get_selectionstart() *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample -
							mwindow->edl->local_session->view_start);
						w = Units::to_int64((mwindow->edl->local_session->get_selectionend() - 
							mwindow->edl->local_session->get_selectionstart()) *
							mwindow->edl->session->sample_rate / 
							mwindow->edl->local_session->zoom_sample);
					}
// Put it in a new plugin set determined by an edit boundary
//					else
// 					if(mwindow->session->edit_highlighted)
// 					{
// 						int64_t temp_y, temp_h;
// 						edit_dimensions(mwindow->session->edit_highlighted, 
// 							x, 
// 							temp_y, 
// 							w, 
// 							temp_h);
// 					}
// Put it at the beginning of the track in a new plugin set
				}

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
//printf("TrackCanvas::draw_highlighting 1\n");
					draw_box = 1;
				}
			}
			break;
		
		case DRAG_ASSET:
			if(mwindow->session->track_highlighted)
			{
				track_dimensions(mwindow->session->track_highlighted, x, y, w, h);

				if(MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					draw_paste_destination();
				}
			}
			break;

// Dragging an effect from the timeline
		case DRAG_AEFFECT_COPY:
		case DRAG_VEFFECT_COPY:
			if((mwindow->session->plugin_highlighted || mwindow->session->track_highlighted) &&
				((mwindow->session->current_operation == DRAG_AEFFECT_COPY && mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
				(mwindow->session->current_operation == DRAG_VEFFECT_COPY && mwindow->session->track_highlighted->data_type == TRACK_VIDEO)))
			{
// Put it before another plugin
				if(mwindow->session->plugin_highlighted)
					plugin_dimensions(mwindow->session->plugin_highlighted, x, y, w, h);
				else
// Put it after a plugin set
				if(mwindow->session->pluginset_highlighted &&
					mwindow->session->pluginset_highlighted->last)
				{
					plugin_dimensions((Plugin*)mwindow->session->pluginset_highlighted->last, x, y, w, h);
					x += w;
				}
				else
				if(mwindow->session->track_highlighted)
				{
					track_dimensions(mwindow->session->track_highlighted, x, y, w, h);

// Put it in a new plugin set determined by an edit boundary
					if(mwindow->session->edit_highlighted)
					{
						int64_t temp_y, temp_h;
						edit_dimensions(mwindow->session->edit_highlighted, 
							x, 
							temp_y, 
							w, 
							temp_h);
					}
// Put it in a new plugin set at the start of the track
				}

// Calculate length of plugin based on data type of track and units
				if(mwindow->session->track_highlighted->data_type == TRACK_VIDEO)
				{
					w = (int64_t)((double)mwindow->session->drag_plugin->length / 
						mwindow->edl->session->frame_rate *
						mwindow->edl->session->sample_rate /
						mwindow->edl->local_session->zoom_sample);
				}
				else
				{
					w = (int64_t)mwindow->session->drag_plugin->length /
						mwindow->edl->local_session->zoom_sample;
				}

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					draw_box = 1;
				}

			}
			break;

		case DRAG_EDIT:
//printf("TrackCanvas::draw_highlighting 1\n");
			if(mwindow->session->track_highlighted)
			{
				track_dimensions(mwindow->session->track_highlighted, x, y, w, h);

				if(MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					draw_paste_destination();
				}
			}


			break;
	}


	if(draw_box)
	{
		draw_highlight_rectangle(x, y, w, h);
	}
}

void TrackCanvas::draw_plugins()
{
	char string[BCTEXTLEN];

	for(Track *track = mwindow->edl->tracks->first;
		track;
		track = track->next)
	{
		if(track->expand_view)
		{
			for(int i = 0; i < track->plugin_set.total; i++)
			{
				PluginSet *pluginset = track->plugin_set.values[i];

				for(Plugin *plugin = (Plugin*)pluginset->first; plugin; plugin = (Plugin*)plugin->next)
				{
					int64_t total_x, y, total_w, h;
					plugin_dimensions(plugin, total_x, y, total_w, h);
					
					if(MWindowGUI::visible(total_x, total_x + total_w, 0, get_w()) &&
						MWindowGUI::visible(y, y + h, 0, get_h()) &&
						plugin->plugin_type != PLUGIN_NONE)
					{
						int x = total_x, w = total_w, left_margin = 5;
						if(x < 0)
						{
							w -= -x;
							x = 0;
						}
						if(w + x > get_w()) w -= (w + x) - get_w();

						draw_3segmenth(x, 
							y, 
							w, 
							total_x,
							total_w,
							mwindow->theme->plugin_bg_data,
							0);
						set_color(WHITE);
						set_font(MEDIUMFONT_3D);
						plugin->calculate_title(string, 0);

// Truncate string to int64_test visible in background
						int len = strlen(string), j;
						for(j = len; j >= 0; j--)
						{
							if(left_margin + get_text_width(MEDIUMFONT_3D, string) > w)
							{
								string[j] = 0;
							}
							else
								break;
						}

// Justify the text on the left boundary of the edit if it is visible.
// Otherwise justify it on the left side of the screen.
						int text_x = total_x + left_margin;
						text_x = MAX(left_margin, text_x);
						draw_text(text_x, 
							y + get_text_ascent(MEDIUMFONT_3D) + 2, 
							string,
							strlen(string),
							0);
					}
				}
			}
		}
	}
}


void TrackCanvas::draw_inout_points()
{
}


void TrackCanvas::draw_drag_handle()
{
	if(mwindow->session->current_operation == DRAG_EDITHANDLE2 ||
		mwindow->session->current_operation == DRAG_PLUGINHANDLE2)
	{
//printf("TrackCanvas::draw_drag_handle 1 %ld %ld\n", mwindow->session->drag_sample, mwindow->edl->local_session->view_start);
		int64_t pixel1 = Units::round(mwindow->session->drag_position * 
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start);
//printf("TrackCanvas::draw_drag_handle 2 %d\n", pixel1);
		set_color(GREEN);
		set_inverse();
//printf("TrackCanvas::draw_drag_handle 3\n");
		draw_line(pixel1, 0, pixel1, get_h());
		set_opaque();
//printf("TrackCanvas::draw_drag_handle 4\n");
	}
}


void TrackCanvas::draw_transitions()
{
	int64_t x, y, w, h;

	for(Track *track = mwindow->edl->tracks->first;
		track;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit;
			edit = edit->next)
		{
			if(edit->transition)
			{
				edit_dimensions(edit, x, y, w, h);
				get_transition_coords(x, y, w, h);

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					PluginServer *server = mwindow->scan_plugindb(edit->transition->title);
					draw_vframe(server->picon, 
						x, 
						y, 
						w, 
						h, 
						0, 
						0, 
						server->picon->get_w(), 
						server->picon->get_h());
				}
			}
		}
	}
}

void TrackCanvas::draw_loop_points()
{
//printf("TrackCanvas::draw_loop_points 1\n");
	if(mwindow->edl->local_session->loop_playback)
	{
//printf("TrackCanvas::draw_loop_points 2\n");
		int64_t x = Units::round(mwindow->edl->local_session->loop_start *
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start);
//printf("TrackCanvas::draw_loop_points 3\n");

		if(MWindowGUI::visible(x, x + 1, 0, get_w()))
		{
			set_color(GREEN);
			draw_line(x, 0, x, get_h());
		}
//printf("TrackCanvas::draw_loop_points 4\n");

		x = Units::round(mwindow->edl->local_session->loop_end *
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start);
//printf("TrackCanvas::draw_loop_points 5\n");

		if(MWindowGUI::visible(x, x + 1, 0, get_w()))
		{
			set_color(GREEN);
			draw_line(x, 0, x, get_h());
		}
//printf("TrackCanvas::draw_loop_points 6\n");
	}
//printf("TrackCanvas::draw_loop_points 7\n");
}

void TrackCanvas::draw_brender_start()
{
	if(mwindow->preferences->use_brender)
	{
		int64_t x = Units::round(mwindow->edl->session->brender_start *
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start);

		if(MWindowGUI::visible(x, x + 1, 0, get_w()))
		{
			set_color(RED);
			draw_line(x, 0, x, get_h());
		}
	}
}

int TrackCanvas::do_keyframes(int cursor_x, 
	int cursor_y, 
	int draw, 
	int buttonpress, 
	int &new_cursor,
	int &update_cursor,
	int &rerender)
{
	int current_tool = 0;
	int result = 0;
	EDLSession *session = mwindow->edl->session;

	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		Automation *automation = track->automation;

//printf("TrackCanvas::draw_keyframes 1\n");
		if(!result && session->auto_conf->fade)
		{
			result = do_float_autos(track, 
				automation->fade_autos,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress, 
				WHITE);
			if(result && mwindow->session->current_operation == DRAG_FADE)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_FADE;
				update_drag_caption();
			}
		}

//printf("TrackCanvas::draw_keyframes 1 %d\n", result);

		if(!result && session->auto_conf->czoom && automation->czoom_autos)
		{
			result = do_float_autos(track, 
				automation->czoom_autos,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				MEPURPLE);
			if(result && mwindow->session->current_operation == DRAG_CZOOM)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_CZOOM;
				update_drag_caption();
			}
		}

//printf("TrackCanvas::draw_keyframes 2 %d\n", result);
		if(!result && session->auto_conf->pzoom && automation->pzoom_autos)
		{
			result = do_float_autos(track, 
				automation->pzoom_autos,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				PINK);
			if(result && mwindow->session->current_operation == DRAG_PZOOM)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_PZOOM;
				update_drag_caption();
			}
		}

//printf("TrackCanvas::draw_keyframes 1 %d\n", result);
		if(!result && session->auto_conf->mute)
		{
			result = do_toggle_autos(track, 
				automation->mute_autos,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				BLUE);
			if(result && mwindow->session->current_operation == DRAG_MUTE)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_MUTE;
				update_drag_caption();
			}
		}

//printf("TrackCanvas::draw_keyframes 1 %d\n", result);
		if(!result && session->auto_conf->camera && automation->camera_autos)
		{
			result = do_autos(track, 
				automation->camera_autos,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				camerakeyframe_pixmap);
			if(result && mwindow->session->current_operation == DRAG_CAMERA)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_CAMERA_PRE;
				update_drag_caption();
				rerender = 1;
			}
		}

		if(!result && session->auto_conf->mode && automation->mode_autos)
		{
			result = do_autos(track, 
				automation->mode_autos,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				modekeyframe_pixmap);
			if(result && mwindow->session->current_operation == DRAG_MODE)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_MODE_PRE;
				update_drag_caption();
				rerender = 1;
			}
		}

//printf("TrackCanvas::draw_keyframes 1 %d\n", result);
		if(!result && session->auto_conf->projector && automation->projector_autos)
		{
			result = do_autos(track, 
				automation->projector_autos,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				projectorkeyframe_pixmap);
			if(result && mwindow->session->current_operation == DRAG_PROJECTOR)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_PROJECTOR_PRE;
				update_drag_caption();
				rerender = 1;
			}
		}

		if(!result && session->auto_conf->mask && automation->mask_autos)
		{
			result = do_autos(track, 
				automation->mask_autos,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				maskkeyframe_pixmap);
			if(result && mwindow->session->current_operation == DRAG_MASK)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_MASK_PRE;
				update_drag_caption();
				rerender = 1;
			}
		}

//printf("TrackCanvas::draw_keyframes 1 %d\n", result);
		if(!result && session->auto_conf->pan && automation->pan_autos)
		{
			result = do_autos(track, 
				automation->pan_autos,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				pankeyframe_pixmap);
			if(result && mwindow->session->current_operation == DRAG_PAN)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_PAN_PRE;
				update_drag_caption();
				rerender = 1;
			}
		}

//printf("TrackCanvas::draw_keyframes 1 %d\n", result);
		if(!result && session->auto_conf->plugins)
		{
			result = do_plugin_autos(track,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress);
			if(result && mwindow->session->current_operation == DRAG_PLUGINKEY)
			{
				rerender = 1;
			}
			if(result && buttonpress)
			{
				mwindow->session->current_operation = DRAG_PLUGINKEY_PRE;
				update_drag_caption();
				rerender = 1;
			}
		}
//printf("TrackCanvas::draw_keyframes 1 %d\n", result);
	}

	if(mwindow->session->current_operation == DRAG_FADE ||
		mwindow->session->current_operation == DRAG_CZOOM ||
		mwindow->session->current_operation == DRAG_PZOOM ||
		mwindow->session->current_operation == DRAG_PLAY ||
		mwindow->session->current_operation == DRAG_MUTE ||
		mwindow->session->current_operation == DRAG_CAMERA ||
		mwindow->session->current_operation == DRAG_CAMERA_PRE ||
		mwindow->session->current_operation == DRAG_MASK ||
		mwindow->session->current_operation == DRAG_MASK_PRE ||
		mwindow->session->current_operation == DRAG_MODE ||
		mwindow->session->current_operation == DRAG_MODE_PRE ||
		mwindow->session->current_operation == DRAG_PAN ||
		mwindow->session->current_operation == DRAG_PAN_PRE ||
		mwindow->session->current_operation == DRAG_PLUGINKEY ||
		mwindow->session->current_operation == DRAG_PLUGINKEY_PRE ||
		mwindow->session->current_operation == DRAG_PROJECTOR ||
		mwindow->session->current_operation == DRAG_PROJECTOR_PRE)
	{
	 	result = 1;
	}

//printf("TrackCanvas::draw_keyframes 2 %d\n", result);
	update_cursor = 1;
	if(result)
	{
		new_cursor = UPRIGHT_ARROW_CURSOR;
//		rerender = 1;
	}

//printf("TrackCanvas::do_keyframes 3 %d\n", result);
	return result;
}

void TrackCanvas::draw_auto(Auto *current, 
	int x, 
	int y, 
	int center_pixel, 
	int zoom_track,
	int color)
{
	int x1, y1, x2, y2;
	char string[BCTEXTLEN];

	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	if(y1 < center_pixel + -zoom_track / 2) y1 = center_pixel + -zoom_track / 2;
	if(y2 > center_pixel + zoom_track / 2) y2 = center_pixel + zoom_track / 2;

	set_color(BLACK);
	draw_box(x1 + 1, y1 + 1, x2 - x1, y2 - y1);
	set_color(color);
	draw_box(x1, y1, x2 - x1, y2 - y1);
}

void TrackCanvas::draw_floatauto(Auto *current, 
	int x, 
	int y, 
	int in_x, 
	int in_y, 
	int out_x, 
	int out_y, 
	int center_pixel, 
	int zoom_track,
	int color)
{
	int x1, y1, x2, y2;
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	char string[BCTEXTLEN];

// Center
	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	CLAMP(y1, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(y2, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);

	set_color(BLACK);
	draw_box(x1 + 1, y1 + 1, x2 - x1, y2 - y1);
	set_color(color);
	draw_box(x1, y1, x2 - x1, y2 - y1);

// In handle
	in_x1 = in_x - HANDLE_W / 2;
	in_x2 = in_x + HANDLE_W / 2;
	in_y1 = center_pixel + in_y - HANDLE_W / 2;
	in_y2 = center_pixel + in_y + HANDLE_W / 2;

	CLAMP(in_y1, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(in_y2, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(in_y, -zoom_track / 2, zoom_track / 2);

	set_color(BLACK);
	draw_line(x + 1, center_pixel + y + 1, in_x + 1, center_pixel + in_y + 1);
	draw_box(in_x1 + 1, in_y1 + 1, in_x2 - in_x1, in_y2 - in_y1);
	set_color(color);
	draw_line(x, center_pixel + y, in_x, center_pixel + in_y);
	draw_box(in_x1, in_y1, in_x2 - in_x1, in_y2 - in_y1);



// Out handle
	out_x1 = out_x - HANDLE_W / 2;
	out_x2 = out_x + HANDLE_W / 2;
	out_y1 = center_pixel + out_y - HANDLE_W / 2;
	out_y2 = center_pixel + out_y + HANDLE_W / 2;

	CLAMP(out_y1, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(out_y2, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(out_y, -zoom_track / 2, zoom_track / 2);


	set_color(BLACK);
	draw_line(x + 1, center_pixel + y + 1, out_x + 1, center_pixel + out_y + 1);
	draw_box(out_x1 + 1, out_y1 + 1, out_x2 - out_x1, out_y2 - out_y1);
	set_color(color);
	draw_line(x, center_pixel + y, out_x, center_pixel + out_y);
	draw_box(out_x1, out_y1, out_x2 - out_x1, out_y2 - out_y1);

}

int TrackCanvas::test_auto(Auto *current, 
	int x, 
	int y, 
	int center_pixel, 
	int zoom_track, 
	int cursor_x, 
	int cursor_y, 
	int buttonpress)
{
	int x1, y1, x2, y2;
	char string[BCTEXTLEN];
	int result = 0;

	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	if(y1 < center_pixel + -zoom_track / 2) y1 = center_pixel + -zoom_track / 2;
	if(y2 > center_pixel + zoom_track / 2) y2 = center_pixel + zoom_track / 2;
//printf("test_auto 1 %d %d %d %d %d %d\n", cursor_x, cursor_y, x1, x2, y1, y2);

	if(cursor_x >= x1 && cursor_x < x2 && cursor_y >= y1 && cursor_y < y2)
	{
//printf("test_auto 2 %d\n", buttonpress);
		if(buttonpress)
		{
//printf("test_auto 3\n");
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = current->value_to_percentage();
			mwindow->session->drag_start_position = current->position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
		}
		result = 1;
	}

	return result;
}

int TrackCanvas::test_floatauto(Auto *current, 
	int x, 
	int y, 
	int in_x,
	int in_y,
	int out_x,
	int out_y,
	int center_pixel, 
	int zoom_track, 
	int cursor_x, 
	int cursor_y, 
	int buttonpress)
{
	int x1, y1, x2, y2;
	int in_x1, in_y1, in_x2, in_y2;
	int out_x1, out_y1, out_x2, out_y2;
	char string[BCTEXTLEN];
	int result = 0;

	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	if(y1 < center_pixel + -zoom_track / 2) y1 = center_pixel + -zoom_track / 2;
	if(y2 > center_pixel + zoom_track / 2) y2 = center_pixel + zoom_track / 2;

	in_x1 = in_x - HANDLE_W / 2;
	in_x2 = in_x + HANDLE_W / 2;
	in_y1 = center_pixel + in_y - HANDLE_W / 2;
	in_y2 = center_pixel + in_y + HANDLE_W / 2;

	if(in_y1 < center_pixel + -zoom_track / 2) in_y1 = center_pixel + -zoom_track / 2;
	if(in_y2 > center_pixel + zoom_track / 2) in_y2 = center_pixel + zoom_track / 2;

	out_x1 = out_x - HANDLE_W / 2;
	out_x2 = out_x + HANDLE_W / 2;
	out_y1 = center_pixel + out_y - HANDLE_W / 2;
	out_y2 = center_pixel + out_y + HANDLE_W / 2;

	if(out_y1 < center_pixel + -zoom_track / 2) out_y1 = center_pixel + -zoom_track / 2;
	if(out_y2 > center_pixel + zoom_track / 2) out_y2 = center_pixel + zoom_track / 2;



//printf("TrackCanvas::test_floatauto %d %d %d %d %d %d\n", cursor_x, cursor_y, x1, x2, y1, y2);
// Test value
	if(!ctrl_down() &&
		cursor_x >= x1 && 
		cursor_x < x2 && 
		cursor_y >= y1 && 
		cursor_y < y2)
	{
		if(buttonpress)
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = current->value_to_percentage();
			mwindow->session->drag_start_position = current->position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
			mwindow->session->drag_handle = 0;
		}
		result = 1;
	}
	else
// Test in control
	if(ctrl_down() &&
		cursor_x >= in_x1 && 
		cursor_x < in_x2 && 
		cursor_y >= in_y1 && 
		cursor_y < in_y2 &&
		current->position > 0)
	{
		if(buttonpress)
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = 
				current->invalue_to_percentage();
			mwindow->session->drag_start_position = 
				((FloatAuto*)current)->control_in_position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
			mwindow->session->drag_handle = 1;
		}
		result = 1;
	}
	else
// Test out control
	if(ctrl_down() &&
		cursor_x >= out_x1 && 
		cursor_x < out_x2 && 
		cursor_y >= out_y1 && 
		cursor_y < out_y2)
	{
		if(buttonpress)
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = 
				current->outvalue_to_percentage();
			mwindow->session->drag_start_position = 
				((FloatAuto*)current)->control_out_position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
			mwindow->session->drag_handle = 2;
		}
		result = 1;
	}

// if(buttonpress) 
// printf("TrackCanvas::test_floatauto 2 drag_handle=%d ctrl_down=%d cursor_x=%d cursor_y=%d x1=%d x2=%d y1=%d y2=%d\n", 
// mwindow->session->drag_handle,
// ctrl_down(),
// cursor_x,
// cursor_y,
// x1, x2, y1, y2);

	return result;
}

void TrackCanvas::draw_floatline(int center_pixel, 
	FloatAuto *previous,
	FloatAuto *next,
	FloatAutos *autos,
	int64_t unit_start,
	double zoom_units,
	double yscale,
	int x1,
	int y1,
	int x2,
	int y2,
	int color)
{
// Solve bezier equation for either every pixel or a certain large number of
// points.



// Not using slope intercept
	x1 = MAX(0, x1);




	int prev_y;
// Call by reference fails for some reason here
	FloatAuto *previous1 = previous, *next1 = next;
	for(int x = x1; x < x2; x++)
	{
		int64_t position = (int64_t)(unit_start + x * zoom_units);
		float value = autos->get_value(position, PLAY_FORWARD, previous1, next1);

		int y = (int)(center_pixel + 
			(autos->value_to_percentage(value) - 0.5) * -yscale);

		if(x > x1)
		{
 			set_color(BLACK);
 			draw_line(x - 1, prev_y + 1, x, y + 1);
 			set_color(color);
 			draw_line(x - 1, prev_y, x, y);
		}
		prev_y = y;
	}



// 	set_color(BLACK);
// 	draw_line(x1, center_pixel + y1 + 1, x2, center_pixel + y2 + 1);
// 	set_color(WHITE);
// 	draw_line(x1, center_pixel + y1, x2, center_pixel + y2);





}

int TrackCanvas::test_floatline(int center_pixel, 
		FloatAutos *autos,
		int64_t unit_start,
		double zoom_units,
		double yscale,
		int x1,
		int x2,
		int cursor_x, 
		int cursor_y, 
		int buttonpress)
{
	int result = 0;


	int64_t position = (int64_t)(unit_start + cursor_x * zoom_units);
// Call by reference fails for some reason here
	FloatAuto *previous = 0, *next = 0;
	float value = autos->get_value(position, PLAY_FORWARD, previous, next);
	int y = (int)(center_pixel + 
		(autos->value_to_percentage(value) - 0.5) * -yscale);

	if(cursor_x >= x1 && 
		cursor_x < x2 &&
		cursor_y >= y - HANDLE_W / 2 && 
		cursor_y < y + HANDLE_W / 2 &&
		!ctrl_down())
	{
		result = 1;


		if(buttonpress)
		{
			mwindow->undo->update_undo_before(_("keyframe"), LOAD_AUTOMATION);


			Auto *current;
			current = mwindow->session->drag_auto = autos->insert_auto(position);
			((FloatAuto*)current)->value = value;
			mwindow->session->drag_start_percentage = current->value_to_percentage();
			mwindow->session->drag_start_position = current->position;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
			mwindow->session->drag_handle = 0;

		}
	}


	return result;
}

void TrackCanvas::draw_toggleline(int center_pixel, 
	int x1,
	int y1,
	int x2,
	int y2,
	int color)
{
	set_color(BLACK);
	draw_line(x1, center_pixel + y1 + 1, x2, center_pixel + y1 + 1);
	set_color(color);
	draw_line(x1, center_pixel + y1, x2, center_pixel + y1);

	if(y2 != y1)
	{
		set_color(BLACK);
		draw_line(x2 + 1, center_pixel + y1, x2 + 1, center_pixel + y2);
		set_color(color);
		draw_line(x2, center_pixel + y1, x2, center_pixel + y2);
	}
}

int TrackCanvas::test_toggleline(Autos *autos,
	int center_pixel, 
	int x1,
	int y1,
	int x2,
	int y2, 
	int cursor_x, 
	int cursor_y, 
	int buttonpress)
{
	int result = 0;
	if(cursor_x >= x1 && cursor_x < x2)
	{
		int miny = center_pixel + y1 - HANDLE_W / 2;
		int maxy = center_pixel + y1 + HANDLE_W / 2;
		if(cursor_y >= miny && cursor_y < maxy) 
		{
			result = 1;

			if(buttonpress)
			{
				mwindow->undo->update_undo_before(_("keyframe"), LOAD_AUTOMATION);


				Auto *current;
				double position = (double)(cursor_x +
						mwindow->edl->local_session->view_start) * 
					mwindow->edl->local_session->zoom_sample / 
					mwindow->edl->session->sample_rate;
				int64_t unit_position = autos->track->to_units(position, 0);
				int new_value = (int)((IntAutos*)autos)->get_automation_constant(unit_position, unit_position);

				current = mwindow->session->drag_auto = autos->insert_auto(unit_position);
				((IntAuto*)current)->value = new_value;
				mwindow->session->drag_start_percentage = current->value_to_percentage();
				mwindow->session->drag_start_position = current->position;
				mwindow->session->drag_origin_x = cursor_x;
				mwindow->session->drag_origin_y = cursor_y;

			}
		}
	}
	return result;
}

void TrackCanvas::calculate_viewport(Track *track, 
	double &view_start,   // Seconds
	int64_t &unit_start,
	double &view_end,     // Seconds
	int64_t &unit_end,
	double &yscale,
	int &center_pixel,
	double &zoom_sample,
	double &zoom_units)
{
	view_start = (double)mwindow->edl->local_session->view_start * 
		mwindow->edl->local_session->zoom_sample /
		mwindow->edl->session->sample_rate;
	unit_start = track->to_units(view_start, 0);
	view_end = (double)(mwindow->edl->local_session->view_start + 
		get_w()) * 
		mwindow->edl->local_session->zoom_sample / 
		mwindow->edl->session->sample_rate;
	unit_end = (int64_t)(track->to_units(view_end, 1));
	yscale = mwindow->edl->local_session->zoom_track;
	center_pixel = (int)(track->y_pixel + yscale / 2) + 
		(mwindow->edl->session->show_titles ? 
			mwindow->theme->title_bg_data->get_h() : 
			0);
	zoom_sample = mwindow->edl->local_session->zoom_sample;

	zoom_units = track->to_doubleunits(zoom_sample / mwindow->edl->session->sample_rate);
}

int TrackCanvas::do_float_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		int color)
{
	int result = 0;

	double view_start;
	int64_t unit_start;
	double view_end;
	int64_t unit_end;
	double yscale;
	int center_pixel;
	double zoom_sample;
	double zoom_units;
	double ax, ay, ax2, ay2;
	double in_x2, in_y2, out_x2, out_y2;
	int draw_auto;
	double slope;
	int skip = 0;

	calculate_viewport(track, 
		view_start,
		unit_start,
		view_end,
		unit_end,
		yscale,
		center_pixel,
		zoom_sample,
		zoom_units);



// Get first auto before start
	Auto *current = 0;
	Auto *previous = 0;
	for(current = autos->last; 
		current && current->position >= unit_start; 
		current = PREVIOUS)
		;

	if(current)
	{
		ax = (double)(current->position - unit_start) / zoom_units;
		ay = (((FloatAuto*)current)->value_to_percentage() - 0.5) * -yscale;
		current = NEXT;
	}
	else
	{
		current = autos->first ? autos->first : autos->default_auto;
		if(current)
		{		
			ax = 0;
			ay = (((FloatAuto*)current)->value_to_percentage() - 0.5) * -yscale;
		}
		else
		{
			ax = 0;
			ay = 0;
		}
	}


//printf("TrackCanvas::do_float_autos 1\n");



	do
	{
		skip = 0;
		draw_auto = 1;

		if(current)
		{
			ax2 = (double)(current->position - unit_start) / zoom_units;
			ay2 = (((FloatAuto*)current)->value_to_percentage() - 0.5) * -yscale;
			in_x2 = (double)(current->position + 
				((FloatAuto*)current)->control_in_position - 
				unit_start) / 
				zoom_units;
			in_y2 = (((FloatAuto*)current)->invalue_to_percentage() - 0.5) * -yscale;
			out_x2 = (double)(current->position + 
				((FloatAuto*)current)->control_out_position - 
				unit_start) / 
				zoom_units;
			out_y2 = (((FloatAuto*)current)->outvalue_to_percentage() - 0.5) * -yscale;
		}
		else
		{
			ax2 = get_w();
			ay2 = ay;
			skip = 1;
		}

		slope = (ay2 - ay) / (ax2 - ax);

		if(ax2 > get_w())
		{
			draw_auto = 0;
			ax2 = get_w();
			ay2 = ay + slope * (get_w() - ax);
		}
		
		if(ax < 0)
		{
			ay = ay + slope * (0 - ax);
			ax = 0;
		}














// Draw handle
		if(current && !result)
		{
			if(current != autos->default_auto)
			{
				if(!draw)
				{
					if(track->record)
						result = test_floatauto(current, 
							(int)ax2, 
							(int)ay2, 
							(int)in_x2,
							(int)in_y2,
							(int)out_x2,
							(int)out_y2,
							(int)center_pixel, 
							(int)yscale, 
							cursor_x, 
							cursor_y, 
							buttonpress);
				}
				else
				if(draw_auto)
					draw_floatauto(current, 
						(int)ax2, 
						(int)ay2, 
						(int)in_x2,
						(int)in_y2,
						(int)out_x2,
						(int)out_y2,
						(int)center_pixel, 
						(int)yscale,
						color);
			}
		}





// Draw joining line
		if(!draw)
		{
			if(!result)
			{
				if(track->record)
				{
					result = test_floatline(center_pixel, 
						(FloatAutos*)autos,
						unit_start,
						zoom_units,
						yscale,
						(int)ax,
// Exclude auto coverage from the end of the line.  The auto overlaps
						(int)ax2 - HANDLE_W / 2,
						cursor_x, 
						cursor_y, 
						buttonpress);
				}
			}
		}
		else
			draw_floatline(center_pixel,
				(FloatAuto*)previous,
				(FloatAuto*)current,
				(FloatAutos*)autos,
				unit_start,
				zoom_units,
				yscale,
				(int)ax, 
				(int)ay, 
				(int)ax2, 
				(int)ay2,
				color);







		if(current)
		{
			previous = current;
			current = NEXT;
		}



		ax = ax2;
		ay = ay2;
	}while(current && 
		current->position <= unit_end && 
		!result);

//printf("TrackCanvas::do_float_autos 100\n");








	if(ax < get_w() && !result)
	{
		ax2 = get_w();
		ay2 = ay;
		if(!draw)
		{
			if(track->record)
			{
				result = test_floatline(center_pixel, 
					(FloatAutos*)autos,
					unit_start,
					zoom_units,
					yscale,
					(int)ax,
					(int)ax2,
					cursor_x, 
					cursor_y, 
					buttonpress);
			}
		}
		else
			draw_floatline(center_pixel, 
				(FloatAuto*)previous,
				(FloatAuto*)current,
				(FloatAutos*)autos,
				unit_start,
				zoom_units,
				yscale,
				(int)ax, 
				(int)ay, 
				(int)ax2, 
				(int)ay2,
				color);
	}








	return result;
}


int TrackCanvas::do_toggle_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		int color)
{
	int result = 0;
	double view_start;
	int64_t unit_start;
	double view_end;
	int64_t unit_end;
	double yscale;
	int center_pixel;
	double zoom_sample;
	double zoom_units;
	double ax, ay, ax2, ay2;

	calculate_viewport(track, 
		view_start,
		unit_start,
		view_end,
		unit_end,
		yscale,
		center_pixel,
		zoom_sample,
		zoom_units);


	double high = -yscale * 0.8 / 2;
	double low = yscale * 0.8 / 2;

// Get first auto before start
	Auto *current;
	for(current = autos->last; current && current->position >= unit_start; current = PREVIOUS)
		;

	if(current)
	{
		ax = 0;
		ay = ((IntAuto*)current)->value > 0 ? high : low;
		current = NEXT;
	}
	else
	{
		current = autos->first ? autos->first : autos->default_auto;
		if(current)
		{
			ax = 0;
			ay = ((IntAuto*)current)->value > 0 ? high : low;
		}
		else
		{
			ax = 0;
			ay = yscale;
		}
	}

	do
	{
		if(current)
		{
			ax2 = (double)(current->position - unit_start) / zoom_units;
			ay2 = ((IntAuto*)current)->value > 0 ? high : low;
		}
		else
		{
			ax2 = get_w();
			ay2 = ay;
		}

		if(ax2 > get_w()) ax2 = get_w();

	    if(current && !result) 
		{
			if(current != autos->default_auto)
			{
				if(!draw)
				{
					if(track->record)
					{
						result = test_auto(current, 
							(int)ax2, 
							(int)ay2, 
							(int)center_pixel, 
							(int)yscale, 
							cursor_x, 
							cursor_y, 
							buttonpress);
					}
				}
				else
					draw_auto(current, 
						(int)ax2, 
						(int)ay2, 
						(int)center_pixel, 
						(int)yscale,
						color);
			}

			current = NEXT;
		}

		if(!draw)
		{
			if(!result)
			{
				if(track->record)
				{
					result = test_toggleline(autos, 
						center_pixel, 
						(int)ax, 
						(int)ay, 
						(int)ax2, 
						(int)ay2,
						cursor_x, 
						cursor_y, 
						buttonpress);
				}
			}
		}
		else
			draw_toggleline(center_pixel, 
				(int)ax, 
				(int)ay, 
				(int)ax2, 
				(int)ay2,
				color);

		ax = ax2;
		ay = ay2;
	}while(current && current->position <= unit_end && !result);

	if(ax < get_w() && !result)
	{
		ax2 = get_w();
		ay2 = ay;
		if(!draw)
		{
			if(track->record)
			{
				result = test_toggleline(autos,
					center_pixel, 
					(int)ax, 
					(int)ay, 
					(int)ax2, 
					(int)ay2,
					cursor_x, 
					cursor_y, 
					buttonpress);
			}
		}
		else
			draw_toggleline(center_pixel, 
				(int)ax, 
				(int)ay, 
				(int)ax2, 
				(int)ay2,
				color);
	}
	return result;
}

int TrackCanvas::do_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		BC_Pixmap *pixmap)
{
	int result = 0;

	double view_start;
	int64_t unit_start;
	double view_end;
	int64_t unit_end;
	double yscale;
	int center_pixel;
	double zoom_sample;
	double zoom_units;

	calculate_viewport(track, 
		view_start,
		unit_start,
		view_end,
		unit_end,
		yscale,
		center_pixel,
		zoom_sample,
		zoom_units);

	Auto *current;

	for(current = autos->first; current && !result; current = NEXT)
	{
		if(current->position >= unit_start && current->position < unit_end)
		{
			int64_t x, y;
			x = (int64_t)((double)(current->position - unit_start) / 
				zoom_units - (pixmap->get_w() / 2 + 0.5));
			y = center_pixel - pixmap->get_h() / 2;

			if(!draw)
			{
				if(cursor_x >= x && cursor_y >= y &&
					cursor_x < x + pixmap->get_w() &&
					cursor_y < y + pixmap->get_h())
				{
					result = 1;

					if(buttonpress)
					{
						mwindow->session->drag_auto = current;
						mwindow->session->drag_start_position = current->position;
						mwindow->session->drag_origin_x = cursor_x;
						mwindow->session->drag_origin_y = cursor_y;

						double position = autos->track->from_units(current->position);
						double center = (mwindow->edl->local_session->selectionstart +
							mwindow->edl->local_session->selectionend) / 
							2;

						if(!shift_down())
						{
							mwindow->edl->local_session->selectionstart = position;
							mwindow->edl->local_session->selectionend = position;
						}
						else
						if(position < center)
						{
							mwindow->edl->local_session->selectionstart = position;
						}
						else
							mwindow->edl->local_session->selectionend = position;
					}
				}
			}
			else
				draw_pixmap(pixmap, x, y);
		}
	}
	return result;
}

int TrackCanvas::do_plugin_autos(Track *track, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress)
{
	int result = 0;

	double view_start;
	int64_t unit_start;
	double view_end;
	int64_t unit_end;
	double yscale;
	int center_pixel;
	double zoom_sample;
	double zoom_units;

	calculate_viewport(track, 
		view_start,
		unit_start,
		view_end,
		unit_end,
		yscale,
		center_pixel,
		zoom_sample,
		zoom_units);

//printf("TrackCanvas::draw_plugin_autos 1 %d\n", track->plugin_set.total);
	for(int i = 0; i < track->plugin_set.total && !result; i++)
	{
		PluginSet *plugin_set = track->plugin_set.values[i];
		int center_pixel = (int)(track->y_pixel + 
			mwindow->edl->local_session->zoom_track +
			(i + 0.5) * mwindow->theme->plugin_bg_data->get_h() + 
			(mwindow->edl->session->show_titles ? mwindow->theme->title_bg_data->get_h() : 0));
		
//printf("TrackCanvas::draw_plugin_autos 2\n");
		for(Plugin *plugin = (Plugin*)plugin_set->first; 
			plugin && !result; 
			plugin = (Plugin*)plugin->next)
		{
			for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first; 
				keyframe && !result; 
				keyframe = (KeyFrame*)keyframe->next)
			{
//printf("TrackCanvas::draw_plugin_autos 3 %d\n", keyframe->position);
				if(keyframe->position >= unit_start && keyframe->position < unit_end)
				{
					int64_t x = (int64_t)((keyframe->position - unit_start) / zoom_units);
					int y = center_pixel - keyframe_pixmap->get_h() / 2;

//printf("TrackCanvas::draw_plugin_autos 4 %d %d\n", x, center_pixel);
					if(!draw)
					{
						if(cursor_x >= x && cursor_y >= y &&
							cursor_x < x + keyframe_pixmap->get_w() &&
							cursor_y < y + keyframe_pixmap->get_h())
						{
							result = 1;

							if(buttonpress)
							{
								mwindow->session->drag_auto = keyframe;
								mwindow->session->drag_start_position = keyframe->position;
								mwindow->session->drag_origin_x = cursor_x;
								mwindow->session->drag_origin_y = cursor_y;

								double position = track->from_units(keyframe->position);
								double center = (mwindow->edl->local_session->selectionstart +
									mwindow->edl->local_session->selectionend) / 
									2;

								if(!shift_down())
								{
									mwindow->edl->local_session->selectionstart = position;
									mwindow->edl->local_session->selectionend = position;
								}
								else
								if(position < center)
								{
									mwindow->edl->local_session->selectionstart = position;
								}
								else
									mwindow->edl->local_session->selectionend = position;
							}
						}
					}
					else
						draw_pixmap(keyframe_pixmap, 
							x, 
							y);
				}
			}
		}
	}
//printf("TrackCanvas::draw_plugin_autos 5\n");
	return result;
}

void TrackCanvas::draw_overlays()
{
	int new_cursor, update_cursor, rerender;
TRACE("TrackCanvas::draw_overlays 1")

// Move background pixmap to foreground pixmap
	draw_pixmap(background_pixmap, 
		0, 
		0,
		get_w(),
		get_h(),
		0,
		0);
TRACE("TrackCanvas::draw_overlays 10")

//printf("TrackCanvas::draw_overlays 4\n");
// In/Out points
	draw_inout_points();

//printf("TrackCanvas::draw_overlays 5\n");
// Transitions
	if(mwindow->edl->session->auto_conf->transitions) draw_transitions();

//printf("TrackCanvas::draw_overlays 6\n");
// Plugins
	draw_plugins();

//printf("TrackCanvas::draw_overlays 2\n");
// Loop points
	draw_loop_points();
	draw_brender_start();

//printf("TrackCanvas::draw_overlays 7\n");
// Highlighted areas
	draw_highlighting();

//printf("TrackCanvas::draw_overlays 8\n");
// Automation
	do_keyframes(0, 
		0, 
		1, 
		0, 
		new_cursor, 
		update_cursor,
		rerender);

//printf("TrackCanvas::draw_overlays 9\n");
// Selection cursor
	if(gui->cursor) gui->cursor->restore();

//printf("TrackCanvas::draw_overlays 10\n");
// Handle dragging
	draw_drag_handle();

//printf("TrackCanvas::draw_overlays 11\n");
// Playback cursor
	draw_playback_cursor();

}

int TrackCanvas::activate()
{
	if(!active)
	{
		get_top_level()->deactivate();
		active = 1;
		set_active_subwindow(this);
		gui->cursor->activate();
	}
	return 0;
}

int TrackCanvas::deactivate()
{
	if(active)
	{
		active = 0;
		gui->cursor->deactivate();
	}
	return 0;
}


void TrackCanvas::update_drag_handle()
{
	double new_position;

	new_position = 
		(double)(get_cursor_x() + mwindow->edl->local_session->view_start) *
		mwindow->edl->local_session->zoom_sample /
		mwindow->edl->session->sample_rate;
	new_position = 
		mwindow->edl->align_to_frame(new_position, 0);


	if(new_position != mwindow->session->drag_position)
	{
		mwindow->session->drag_position = new_position;
		gui->mainclock->update(new_position);
// Que the CWindow.  Doesn't do anything if selectionstart and selection end 
// aren't changed.
//		mwindow->cwindow->update(1, 0, 0);
	}
}

int TrackCanvas::update_drag_edit()
{
	int result = 0;
	
	
	
	return result;
}

#define UPDATE_DRAG_HEAD(do_clamp) \
	int result = 0; \
	int x = cursor_x - mwindow->session->drag_origin_x; \
	int y = cursor_y - mwindow->session->drag_origin_y; \
 \
	if(!current->autos->track->record) return 0; \
	double view_start; \
	int64_t unit_start; \
	double view_end; \
	int64_t unit_end; \
	double yscale; \
	int center_pixel; \
	double zoom_sample; \
	double zoom_units; \
 \
 	mwindow->undo->update_undo_before(_("tweek"), LOAD_AUTOMATION); \
	calculate_viewport(current->autos->track,  \
		view_start, \
		unit_start, \
		view_end, \
		unit_end, \
		yscale, \
		center_pixel, \
		zoom_sample, \
		zoom_units); \
 \
	float percentage = (float)(mwindow->session->drag_origin_y - cursor_y) / \
		MAX(128, yscale) +  \
		mwindow->session->drag_start_percentage; \
	if(do_clamp) CLAMP(percentage, 0, 1); \
 \
	int64_t position = Units::to_int64(zoom_units * \
		(cursor_x - mwindow->session->drag_origin_x) + \
		mwindow->session->drag_start_position); \
 \
	if((do_clamp) && position < 0) position = 0;









int TrackCanvas::update_drag_floatauto(int cursor_x, int cursor_y)
{
	FloatAuto *current = (FloatAuto*)mwindow->session->drag_auto;

	UPDATE_DRAG_HEAD(mwindow->session->drag_handle == 0);

	float value;
//printf("TrackCanvas::update_drag_floatauto %ld %d\n", 
//position, 
//mwindow->session->drag_handle);

	switch(mwindow->session->drag_handle)
	{
// Center
		case 0:
// Snap to nearby values
			if(shift_down())
			{
				if(current->previous)
					value = ((FloatAuto*)current->previous)->value;
				else
				if(current->next)
					value = ((FloatAuto*)current->next)->value;
				else
					value = ((FloatAutos*)current->autos)->default_;
			}
			else
				value = ((FloatAuto*)current)->percentage_to_value(percentage);

//printf("TrackCanvas::update_drag_floatauto 1 %f\n", value);
			if(value != current->value || position != current->position)
			{
				result = 1;
				current->value = value;
				current->position = position;

				char string[BCTEXTLEN], string2[BCTEXTLEN];
				Units::totext(string2, 
					current->autos->track->from_units(current->position),
					mwindow->edl->session->time_format,
					mwindow->edl->session->sample_rate,
					mwindow->edl->session->frame_rate,
					mwindow->edl->session->frames_per_foot);
				sprintf(string, "%s, %.2f", string2, current->value);
				gui->show_message(string);
			}
			break;

// In control
		case 1:
			value = ((FloatAuto*)current)->percentage_to_invalue(percentage);
			position = MIN(0, position);
			if(value != current->control_in_value || 
				position != current->control_in_position)
			{
				result = 1;
				current->control_in_value = value;
				current->control_in_position = position;

				char string[BCTEXTLEN], string2[BCTEXTLEN];
				Units::totext(string2, 
					current->autos->track->from_units(current->control_in_position),
					mwindow->edl->session->time_format,
					mwindow->edl->session->sample_rate,
					mwindow->edl->session->frame_rate,
					mwindow->edl->session->frames_per_foot);
				sprintf(string, "%s, %.2f", string2, current->control_in_value);
				gui->show_message(string);
			}
			break;

// Out control
		case 2:
			value = ((FloatAuto*)current)->percentage_to_outvalue(percentage);
			position = MAX(0, position);
			if(value != current->control_out_value || 
				position != current->control_out_position)
			{
				result = 1;
				((FloatAuto*)current)->control_out_value = value;
				((FloatAuto*)current)->control_out_position = position;

				char string[BCTEXTLEN], string2[BCTEXTLEN];
				Units::totext(string2, 
					current->autos->track->from_units(
						((FloatAuto*)current)->control_out_position),
					mwindow->edl->session->time_format,
					mwindow->edl->session->sample_rate,
					mwindow->edl->session->frame_rate,
					mwindow->edl->session->frames_per_foot);
				sprintf(string, "%s, %.2f", 
					string2, 
					((FloatAuto*)current)->control_out_value);
				gui->show_message(string);
			}
			break;
	}

	return result;
}

int TrackCanvas::update_drag_toggleauto(int cursor_x, int cursor_y)
{
	IntAuto *current = (IntAuto*)mwindow->session->drag_auto;

	UPDATE_DRAG_HEAD(1);

	int value = ((IntAuto*)current)->percentage_to_value(percentage);

	if(value != current->value || position != current->position)
	{
		result = 1;
		current->value = value;
		current->position = position;

		char string[BCTEXTLEN], string2[BCTEXTLEN];
		Units::totext(string2, 
			current->autos->track->from_units(current->position),
			mwindow->edl->session->time_format,
			mwindow->edl->session->sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
		sprintf(string, "%s, %d", string2, current->value);
		gui->show_message(string);
	}

	return result;
}

// Autos which can't change value through dragging.

int TrackCanvas::update_drag_auto(int cursor_x, int cursor_y)
{
	Auto *current = (Auto*)mwindow->session->drag_auto;

	UPDATE_DRAG_HEAD(1)

	if(position != current->position)
	{
		result = 1;
		current->position = position;

		char string[BCTEXTLEN];
		Units::totext(string, 
			current->autos->track->from_units(current->position),
			mwindow->edl->session->time_format,
			mwindow->edl->session->sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
		gui->show_message(string);

		double position_f = current->autos->track->from_units(current->position);
		double center_f = (mwindow->edl->local_session->selectionstart +
			mwindow->edl->local_session->selectionend) / 
			2;
		if(!shift_down())
		{
			mwindow->edl->local_session->selectionstart = position_f;
			mwindow->edl->local_session->selectionend = position_f;
		}
		else
		if(position_f < center_f)
		{
			mwindow->edl->local_session->selectionstart = position_f;
		}
		else
			mwindow->edl->local_session->selectionend = position_f;
	}


	return result;
}

void TrackCanvas::update_drag_caption()
{
	switch(mwindow->session->current_operation)
	{
		case DRAG_FADE:
			
			break;
	}
}



int TrackCanvas::cursor_motion_event()
{
	int result, cursor_x, cursor_y;
	int update_clock = 0;
	int update_zoom = 0;
	int update_scroll = 0;
	int update_overlay = 0;
	int update_cursor = 0;
	int new_cursor = 0;
	int rerender = 0;
	double position = 0;
//printf("TrackCanvas::cursor_motion_event 1\n");
	result = 0;

// Default cursor
	switch(mwindow->edl->session->editing_mode)
	{
		case EDITING_ARROW: new_cursor = ARROW_CURSOR; break;
		case EDITING_IBEAM: new_cursor = IBEAM_CURSOR; break;
	}

	switch(mwindow->session->current_operation)
	{
		case DRAG_EDITHANDLE1:
// Outside threshold.  Upgrade status
//printf("TrackCanvas::cursor_motion_event 1\n");
			if(labs(get_cursor_x() - mwindow->session->drag_origin_x) > HANDLE_W)
			{
//printf("TrackCanvas::cursor_motion_event 2\n");
				mwindow->session->current_operation = DRAG_EDITHANDLE2;
				update_overlay = 1;
			}
			break;

		case DRAG_EDITHANDLE2:
			update_drag_handle();
			update_overlay = 1;
			break;

		case DRAG_PLUGINHANDLE1:
			if(labs(get_cursor_x() - mwindow->session->drag_origin_x) > HANDLE_W)
			{
//printf("TrackCanvas::cursor_motion_event 2\n");
				mwindow->session->current_operation = DRAG_PLUGINHANDLE2;
				update_overlay = 1;
			}
			break;

		case DRAG_PLUGINHANDLE2:
			update_drag_handle();
			update_overlay = 1;
			break;

// Rubber band curves
		case DRAG_FADE:
			rerender = update_overlay = update_drag_floatauto(get_cursor_x(), get_cursor_y());
			break;

		case DRAG_CZOOM:
			rerender = update_overlay = update_drag_floatauto(get_cursor_x(), get_cursor_y());
			break;

		case DRAG_PZOOM:
			rerender = update_overlay = update_drag_floatauto(get_cursor_x(), get_cursor_y());
			break;

		case DRAG_PLAY:
			rerender = update_overlay = update_drag_toggleauto(get_cursor_x(), get_cursor_y());
			break;

		case DRAG_MUTE:
			rerender = update_overlay = update_drag_toggleauto(get_cursor_x(), get_cursor_y());
			break;

// Keyframe icons are sticky
		case DRAG_PAN_PRE:
		case DRAG_CAMERA_PRE:
		case DRAG_MASK_PRE:
		case DRAG_MODE_PRE:
		case DRAG_PROJECTOR_PRE:
		case DRAG_PLUGINKEY_PRE:
			if(labs(get_cursor_x() - mwindow->session->drag_origin_x) > HANDLE_W)
			{
//printf("TrackCanvas::cursor_motion_event 2\n");
				mwindow->session->current_operation++;
				update_overlay = 1;
			}
			break;

		case DRAG_PAN:
		case DRAG_CAMERA:
		case DRAG_MASK:
		case DRAG_MODE:
		case DRAG_PROJECTOR:
		case DRAG_PLUGINKEY:
			rerender = update_overlay = 
				update_drag_auto(get_cursor_x(), get_cursor_y());
			break;

		case SELECT_REGION:
		{
			cursor_x = get_cursor_x();
			cursor_y = get_cursor_y();
			position = (double)(cursor_x + mwindow->edl->local_session->view_start) * 
				mwindow->edl->local_session->zoom_sample /
				mwindow->edl->session->sample_rate;

			position = mwindow->edl->align_to_frame(position, 0);
			position = MAX(position, 0);

			if(position < selection_midpoint1)
			{
				mwindow->edl->local_session->selectionend = selection_midpoint1;
				mwindow->edl->local_session->selectionstart = position;
// Que the CWindow
				mwindow->cwindow->update(1, 0, 0, 0, 1);
// Update the faders
				mwindow->update_plugin_guis();
				gui->patchbay->update();
			}
			else
			{
				mwindow->edl->local_session->selectionstart = selection_midpoint1;
				mwindow->edl->local_session->selectionend = position;
// Don't que the CWindow
			}
//printf("TrackCanvas::cursor_motion_event 1 %f %f %f\n", position, mwindow->edl->local_session->selectionstart, mwindow->edl->local_session->selectionend);

			gui->cursor->hide();
			gui->cursor->draw();
			flash();
			result = 1;
			update_clock = 1;
			update_zoom = 1;
			update_scroll = 1;
			break;
		}

		default:
			if(is_event_win() && cursor_inside())
			{
// Update clocks
				cursor_x = get_cursor_x();
				position = (double)cursor_x * 
					(double)mwindow->edl->local_session->zoom_sample / 
					(double)mwindow->edl->session->sample_rate + 
					(double)mwindow->edl->local_session->view_start * 
					(double)mwindow->edl->local_session->zoom_sample / 
					(double)mwindow->edl->session->sample_rate;
				position = mwindow->edl->align_to_frame(position, 0);
				update_clock = 1;

// Update cursor
				if(mwindow->edl->session->auto_conf->transitions && 
					test_transitions(get_cursor_x(), 
						get_cursor_y(), 
						0, 
						new_cursor, 
						update_cursor))
				{
					break;
				}
				else
// Update cursor
				if(do_keyframes(get_cursor_x(), 
					get_cursor_y(), 
					0, 
					0, 
					new_cursor,
					update_cursor,
					rerender))
				{
					break;
				}
				else
// Edit boundaries
				if(test_edit_handles(get_cursor_x(), 
					get_cursor_y(), 
					0, 
					new_cursor,
					update_cursor))
				{
					break;
				}
				else
// Plugin boundaries
				if(test_plugin_handles(get_cursor_x(), 
					get_cursor_y(), 
					0, 
					new_cursor,
					update_cursor))
				{
					break;
				}
				else
				if(test_edits(get_cursor_x(), 
					get_cursor_y(), 
					0, 
					0, 
					update_overlay, 
					rerender,
					new_cursor,
					update_cursor))
				{
					break;
				}
			}
			break;
	}

//printf("TrackCanvas::cursor_motion_event 1\n");
	if(update_cursor && new_cursor != get_cursor())
	{
		set_cursor(new_cursor);
	}

//printf("TrackCanvas::cursor_motion_event 1 %d\n", rerender);
	if(rerender)
	{
		mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_PARAMS);
		mwindow->update_plugin_guis();
		mwindow->cwindow->update(1, 0, 0, 0, 1);
// Update faders
		gui->patchbay->update();
	}


	if(update_clock)
	{
		if(!mwindow->cwindow->playback_engine->is_playing_back)
			gui->mainclock->update(position);
	}

	if(update_zoom)
	{
		gui->zoombar->update();
	}

	if(update_scroll)
	{
		if(!drag_scroll && 
			(cursor_x >= get_w() || cursor_x < 0 || cursor_y >= get_h() || cursor_y < 0))
			start_dragscroll();
		else
		if(drag_scroll &&
			(cursor_x < get_w() && cursor_x >= 0 && cursor_y < get_h() && cursor_y >= 0))
			stop_dragscroll();
	}

	if(update_overlay)
	{
		draw_overlays();
		flash();
	}


//printf("TrackCanvas::cursor_motion_event 100\n");
	return result;
}

void TrackCanvas::start_dragscroll()
{
	if(!drag_scroll)
	{
		drag_scroll = 1;
		set_repeat(BC_WindowBase::get_resources()->scroll_repeat);
//printf("TrackCanvas::start_dragscroll 1\n");
	}
}

void TrackCanvas::stop_dragscroll()
{
	if(drag_scroll)
	{
		drag_scroll = 0;
		unset_repeat(BC_WindowBase::get_resources()->scroll_repeat);
//printf("TrackCanvas::stop_dragscroll 1\n");
	}
}

int TrackCanvas::repeat_event(int64_t duration)
{
	if(!drag_scroll) return 0;
	if(duration != BC_WindowBase::get_resources()->scroll_repeat) return 0;

	int sample_movement = 0;
	int track_movement = 0;
	int64_t x_distance = 0;
	int64_t y_distance = 0;
	double position = 0;
	int result = 0;

	switch(mwindow->session->current_operation)
	{
		case SELECT_REGION:
//printf("TrackCanvas::repeat_event 1 %d\n", mwindow->edl->local_session->view_start);
			if(get_cursor_x() > get_w())
			{
				x_distance = get_cursor_x() - get_w();
				sample_movement = 1;
			}
			else
			if(get_cursor_x() < 0)
			{
				x_distance = get_cursor_x();
				sample_movement = 1;
			}

			if(get_cursor_y() > get_h())
			{
				y_distance = get_cursor_y() - get_h();
				track_movement = 1;
			}
			else
			if(get_cursor_y() < 0)
			{
				y_distance = get_cursor_y();
				track_movement = 1;
			}
			result = 1;
			break;
	}


	if(sample_movement)
	{
		position = (double)(get_cursor_x() + 
			mwindow->edl->local_session->view_start + 
			x_distance) * 
			mwindow->edl->local_session->zoom_sample /
			mwindow->edl->session->sample_rate;
		position = mwindow->edl->align_to_frame(position, 0);
		position = MAX(position, 0);

//printf("TrackCanvas::repeat_event 1 %f\n", position);
		switch(mwindow->session->current_operation)
		{
			case SELECT_REGION:
				if(position < selection_midpoint1)
				{
					mwindow->edl->local_session->selectionend = selection_midpoint1;
					mwindow->edl->local_session->selectionstart = position;
// Que the CWindow
					mwindow->cwindow->update(1, 0, 0);
// Update the faders
					mwindow->update_plugin_guis();
					gui->patchbay->update();
				}
				else
				{
					mwindow->edl->local_session->selectionstart = selection_midpoint1;
					mwindow->edl->local_session->selectionend = position;
// Don't que the CWindow
				}
				break;
		}

		mwindow->samplemovement(mwindow->edl->local_session->view_start + 
			x_distance);
	}

	if(track_movement)
	{
		mwindow->trackmovement(mwindow->edl->local_session->track_start + 
			y_distance);
	}

	return result;
}

int TrackCanvas::button_release_event()
{
	int redraw = 0, update_overlay = 0, result = 0;

	switch(mwindow->session->current_operation)
	{
		case DRAG_EDITHANDLE2:
			mwindow->session->current_operation = NO_OPERATION;
			drag_scroll = 0;
			result = 1;
			
			end_edithandle_selection();
			break;

		case DRAG_EDITHANDLE1:
			mwindow->session->current_operation = NO_OPERATION;
			drag_scroll = 0;
			result = 1;
			break;

		case DRAG_PLUGINHANDLE2:
			mwindow->session->current_operation = NO_OPERATION;
			drag_scroll = 0;
			result = 1;
			
			end_pluginhandle_selection();
			break;

		case DRAG_PLUGINHANDLE1:
			mwindow->session->current_operation = NO_OPERATION;
			drag_scroll = 0;
			result = 1;
			break;

		case DRAG_FADE:
		case DRAG_CZOOM:
		case DRAG_PZOOM:
		case DRAG_PLAY:
		case DRAG_MUTE:
		case DRAG_MASK:
		case DRAG_MODE:
		case DRAG_PAN:
		case DRAG_CAMERA:
		case DRAG_PROJECTOR:
		case DRAG_PLUGINKEY:
			mwindow->session->current_operation = NO_OPERATION;
			mwindow->session->drag_handle = 0;
// Remove any out-of-order keyframe
			if(mwindow->session->drag_auto)
			{
				mwindow->session->drag_auto->autos->remove_nonsequential(
					mwindow->session->drag_auto);
//				mwindow->session->drag_auto->autos->optimize();
				update_overlay = 1;
			}
			mwindow->undo->update_undo_after();
			result = 1;
			break;

		case DRAG_EDIT:
		case DRAG_AEFFECT_COPY:
		case DRAG_VEFFECT_COPY:
// Trap in drag stop

			break;


		default:
			if(mwindow->session->current_operation)
			{
				mwindow->session->current_operation = NO_OPERATION;
				drag_scroll = 0;
// Traps button release events
//				result = 1;
			}
			break;
	}

	if(update_overlay)
	{
		draw_overlays();
		flash();
	}
	if(redraw)
	{
		draw();
		flash();
	}
	return result;
}

int TrackCanvas::test_edit_handles(int cursor_x, 
	int cursor_y, 
	int button_press, 
	int &new_cursor,
	int &update_cursor)
{
	Edit *edit_result = 0;
	int handle_result = 0;
	int result = 0;

	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit && !result;
			edit = edit->next)
		{
			int64_t edit_x, edit_y, edit_w, edit_h;
			edit_dimensions(edit, edit_x, edit_y, edit_w, edit_h);

			if(cursor_x >= edit_x && cursor_x < edit_x + edit_w &&
				cursor_y >= edit_y && cursor_y < edit_y + edit_h)
			{
				if(cursor_x < edit_x + HANDLE_W)
				{
					edit_result = edit;
					handle_result = 0;
					result = 1;
				}
				else
				if(cursor_x >= edit_x + edit_w - HANDLE_W)
				{
					edit_result = edit;
					handle_result = 1;
					result = 1;
				}
				else
				{
					result = 0;
				}
			}
		}
	}

	update_cursor = 1;
	if(result)
	{
		double position;
		if(handle_result == 0)
		{
			position = edit_result->track->from_units(edit_result->startproject);
			new_cursor = LEFT_CURSOR;
		}
		else
		if(handle_result == 1)
		{
			position = edit_result->track->from_units(edit_result->startproject + edit_result->length);
			new_cursor = RIGHT_CURSOR;
		}

// Reposition cursor
		if(button_press)
		{
			mwindow->session->drag_edit = edit_result;
			mwindow->session->drag_handle = handle_result;
			mwindow->session->drag_button = get_buttonpress() - 1;
			mwindow->session->drag_position = position;
			mwindow->session->current_operation = DRAG_EDITHANDLE1;
			mwindow->session->drag_origin_x = get_cursor_x();
			mwindow->session->drag_origin_y = get_cursor_y();
			mwindow->session->drag_start = position;

			int rerender = start_selection(position);
			if(rerender)
				mwindow->cwindow->update(1, 0, 0);
			gui->timebar->update_highlights();
			gui->zoombar->update();
			gui->cursor->hide();
			gui->cursor->draw();
			draw_overlays();
			flash();
			flush();
		}
	}

	return result;
}

int TrackCanvas::test_plugin_handles(int cursor_x, 
	int cursor_y, 
	int button_press,
	int &new_cursor,
	int &update_cursor)
{
	Plugin *plugin_result = 0;
	int handle_result = 0;
	int result = 0;
	
	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(int i = 0; i < track->plugin_set.total && !result; i++)
		{
			PluginSet *plugin_set = track->plugin_set.values[i];
			for(Plugin *plugin = (Plugin*)plugin_set->first; 
				plugin && !result; 
				plugin = (Plugin*)plugin->next)
			{
				int64_t plugin_x, plugin_y, plugin_w, plugin_h;
				plugin_dimensions(plugin, plugin_x, plugin_y, plugin_w, plugin_h);
				
				if(cursor_x >= plugin_x && cursor_x < plugin_x + plugin_w &&
					cursor_y >= plugin_y && cursor_y < plugin_y + plugin_h)
				{
					if(cursor_x < plugin_x + HANDLE_W)
					{
						plugin_result = plugin;
						handle_result = 0;
						result = 1;
					}
					else
					if(cursor_x >= plugin_x + plugin_w - HANDLE_W)
					{
						plugin_result = plugin;
						handle_result = 1;
						result = 1;
					}
				}
			}
		}
	}

//printf("TrackCanvas::test_plugin_handles %d %d %d\n", button_press, handle_result, result);
	update_cursor = 1;
	if(result)
	{
		double position;
		if(handle_result == 0)
		{
			position = plugin_result->track->from_units(plugin_result->startproject);
			new_cursor = LEFT_CURSOR;
		}
		else
		if(handle_result == 1)
		{
			position = plugin_result->track->from_units(plugin_result->startproject + plugin_result->length);
			new_cursor = RIGHT_CURSOR;
		}
		
		if(button_press)
		{
			mwindow->session->drag_plugin = plugin_result;
			mwindow->session->drag_handle = handle_result;
			mwindow->session->drag_button = get_buttonpress() - 1;
			mwindow->session->drag_position = position;
			mwindow->session->current_operation = DRAG_PLUGINHANDLE1;
			mwindow->session->drag_origin_x = get_cursor_x();
			mwindow->session->drag_origin_y = get_cursor_y();
			mwindow->session->drag_start = position;

			int rerender = start_selection(position);
			if(rerender) mwindow->cwindow->update(1, 0, 0);
			gui->timebar->update_highlights();
			gui->zoombar->update();
			gui->cursor->hide();
			gui->cursor->draw();
			draw_overlays();
			flash();
			flush();
		}
	}
	
	return result;
}


int TrackCanvas::test_tracks(int cursor_x, 
		int cursor_y,
		int button_press)
{
	int result = 0;
	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		int64_t track_x, track_y, track_w, track_h;
		track_dimensions(track, track_x, track_y, track_w, track_h);

		if(button_press && 
			get_buttonpress() == 3 &&
			cursor_y >= track_y && 
			cursor_y < track_y + track_h)
		{
			gui->edit_menu->update(track, 0);
			gui->edit_menu->activate_menu();
			result = 1;
		}
	}
	return result;
}

int TrackCanvas::test_edits(int cursor_x, 
	int cursor_y, 
	int button_press,
	int drag_start,
	int &redraw,
	int &rerender,
	int &new_cursor,
	int &update_cursor)
{
	int result = 0;
	int over_edit_handle = 0;

//printf("TrackCanvas::test_edits 1\n");
	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit && !result;
			edit = edit->next)
		{
			int64_t edit_x, edit_y, edit_w, edit_h;
//printf("TrackCanvas::test_edits 1\n");
			edit_dimensions(edit, edit_x, edit_y, edit_w, edit_h);

// Cursor inside a track
// Cursor inside an edit
			if(cursor_x >= edit_x && cursor_x < edit_x + edit_w &&
				cursor_y >= edit_y && cursor_y < edit_y + edit_h)
			{
// Select duration of edit
//printf("TrackCanvas::test_edits 2\n");
				if(button_press)
				{
					if(get_double_click() && !drag_start)
					{
//printf("TrackCanvas::test_edits 3\n");
						mwindow->edl->local_session->selectionstart = 
							edit->track->from_units(edit->startproject);
						mwindow->edl->local_session->selectionend = 
							edit->track->from_units(edit->startproject) + 
							edit->track->from_units(edit->length);
						if(mwindow->edl->session->cursor_on_frames) 
						{
							mwindow->edl->local_session->selectionstart = 
								mwindow->edl->align_to_frame(mwindow->edl->local_session->selectionstart, 0);
							mwindow->edl->local_session->selectionend = 
								mwindow->edl->align_to_frame(mwindow->edl->local_session->selectionend, 1);
						}
						redraw = 1;
						rerender = 1;
						result = 1;
					}
				}
				else
				if(drag_start)
				{
					if(mwindow->edl->session->editing_mode == EDITING_ARROW)
					{
// Need to create drag window
						mwindow->session->current_operation = DRAG_EDIT;
						mwindow->session->drag_edit = edit;
//printf("TrackCanvas::test_edits 2\n");

// Drag only one edit
						if(ctrl_down())
						{
							mwindow->session->drag_edits->remove_all();
							mwindow->session->drag_edits->append(edit);
						}
						else
// Construct list of all affected edits
						{
							mwindow->edl->tracks->get_affected_edits(
								mwindow->session->drag_edits, 
								edit->track->from_units(edit->startproject),
								edit->track);
						}
						mwindow->session->drag_origin_x = cursor_x;
						mwindow->session->drag_origin_y = cursor_y;

						drag_pixmap = new BC_Pixmap(gui,
							mwindow->theme->clip_icon,
							PIXMAP_ALPHA);
						drag_popup = new BC_DragWindow(gui, 
							drag_pixmap, 
							get_abs_cursor_x() - drag_pixmap->get_w() / 2,
							get_abs_cursor_y() - drag_pixmap->get_h() / 2);
//printf("TrackCanvas::test_edits 3 %p\n", drag_popup);

						result = 1;
					}
				}
			}
		}
	}
	return result;
}


int TrackCanvas::test_resources(int cursor_x, int cursor_y)
{
	return 0;
}

int TrackCanvas::test_plugins(int cursor_x, 
	int cursor_y, 
	int drag_start,
	int button_press)
{
	Plugin *plugin = 0;
	int result = 0;
	int done = 0;
	int64_t x, y, w, h;

//printf("TrackCanvas::test_plugins 1\n");
	for(Track *track = mwindow->edl->tracks->first;
		track && !done;
		track = track->next)
	{
		for(int i = 0; i < track->plugin_set.total && !done; i++)
		{
			PluginSet *plugin_set = track->plugin_set.values[i];
			for(plugin = (Plugin*)plugin_set->first;
				plugin && !done;
				plugin = (Plugin*)plugin->next)
			{
				plugin_dimensions(plugin, x, y, w, h);
				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					if(cursor_x >= x && cursor_x < x + w &&
						cursor_y >= y && cursor_y < y + h)
					{
						done = 1;
						break;
					}
				}
			}
		}
	}

//printf("TrackCanvas::test_plugins 1\n");
	if(plugin)
	{
// Start plugin popup
		if(button_press)
		{
//printf("TrackCanvas::test_plugins 2\n");
			if(get_buttonpress() == 3)
			{
				gui->plugin_menu->update(plugin);
				gui->plugin_menu->activate_menu();
				result = 1;
			}
//printf("TrackCanvas::test_plugins 3\n");
		}
		else
// Move plugin
		if(drag_start)
		{
//printf("TrackCanvas::test_plugins 4\n");
			if(mwindow->edl->session->editing_mode == EDITING_ARROW)
			{
//printf("TrackCanvas::test_plugins 5\n");
				if(plugin->track->data_type == TRACK_AUDIO)
					mwindow->session->current_operation = DRAG_AEFFECT_COPY;
				else
				if(plugin->track->data_type == TRACK_VIDEO)
					mwindow->session->current_operation = DRAG_VEFFECT_COPY;

				mwindow->session->drag_plugin = plugin;
//printf("TrackCanvas::test_plugins 6\n");





// Create picon
				switch(plugin->plugin_type)
				{
					case PLUGIN_STANDALONE:
					{
						PluginServer *server = mwindow->scan_plugindb(plugin->title);
						VFrame *frame = server->picon;
//printf("TrackCanvas::test_plugins 7\n");
						if(frame->get_color_model() == BC_RGB888)
						{
							drag_pixmap = new BC_Pixmap(gui, frame->get_w(), frame->get_h());
							drag_pixmap->draw_vframe(frame,
								0,
								0,
								frame->get_w(),
								frame->get_h(),
								0,
								0);
						}
						else
						{
							drag_pixmap = new BC_Pixmap(gui,
								frame,
								PIXMAP_ALPHA);
						}
//printf("TrackCanvas::test_plugins 8\n");
						break;
					}
					
					case PLUGIN_SHAREDPLUGIN:
						drag_pixmap = new BC_Pixmap(gui,
							mwindow->theme->clip_icon,
							PIXMAP_ALPHA);
						break;

					case PLUGIN_SHAREDMODULE:
						drag_pixmap = new BC_Pixmap(gui,
							mwindow->theme->clip_icon,
							PIXMAP_ALPHA);
						break;
//printf("test plugins %d %p\n", mwindow->edl->session->editing_mode, mwindow->session->drag_plugin);
				}

//printf("TrackCanvas::test_plugins 9 %p\n");
				drag_popup = new BC_DragWindow(gui, 
					drag_pixmap, 
					get_abs_cursor_x() - drag_pixmap->get_w() / 2,
					get_abs_cursor_y() - drag_pixmap->get_h() / 2);
//printf("TrackCanvas::test_plugins 10\n");
				result = 1;
			}
		}
	}

//printf("TrackCanvas::test_plugins 11\n");
	return result;
}

int TrackCanvas::test_transitions(int cursor_x, 
	int cursor_y, 
	int button_press,
	int &new_cursor,
	int &update_cursor)
{
	Transition *transition = 0;
	int result = 0;
	int64_t x, y, w, h;
	
	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit;
			edit = edit->next)
		{
			if(edit->transition)
			{
				edit_dimensions(edit, x, y, w, h);
				get_transition_coords(x, y, w, h);

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					if(cursor_x >= x && cursor_x < x + w &&
						cursor_y >= y && cursor_y < y + h)
					{
						transition = edit->transition;
						result = 1;
						break;
					}
				}
			}
		}
	}
	
	update_cursor = 1;
	if(transition)
	{
		if(!button_press)
		{
			new_cursor = UPRIGHT_ARROW_CURSOR;
		}
		else
		if(get_buttonpress() == 3)
		{
			gui->transition_menu->update(transition);
			gui->transition_menu->activate_menu();
		}
	}

	return result;
}

int TrackCanvas::button_press_event()
{
	int result = 0;
	int cursor_x, cursor_y;
	int new_cursor, update_cursor;

//printf("TrackCanvas::button_press_event 1\n");
	cursor_x = get_cursor_x();
	cursor_y = get_cursor_y();

	if(is_event_win() && cursor_inside())
	{
		if(!active)
		{
			activate();
		}

		if(get_buttonpress() == 1)
		{
			gui->unlock_window();
			gui->mbuttons->transport->handle_transport(STOP, 1);
			gui->lock_window("TrackCanvas::button_press_event");
		}

		int update_overlay = 0, update_cursor = 0, rerender = 0;

		if(get_buttonpress() == 4)
		{
//printf("TrackCanvas::button_press_event 1\n");
			mwindow->move_up(get_h() / 10);
			result = 1;
		}
		else
		if(get_buttonpress() == 5)
		{
//printf("TrackCanvas::button_press_event 2\n");
			mwindow->move_down(get_h() / 10);
			result = 1;
		}
		else
		switch(mwindow->edl->session->editing_mode)
		{
// Test handles and resource boundaries and highlight a track
			case EDITING_ARROW:
			{
				Edit *edit;
				int handle;
				if(mwindow->edl->session->auto_conf->transitions && 
					test_transitions(cursor_x, 
						cursor_y, 
						1, 
						new_cursor, 
						update_cursor))
				{
					break;
				}
				else
				if(do_keyframes(cursor_x, 
					cursor_y, 
					0, 
					1, 
					new_cursor, 
					update_cursor,
					rerender))
				{
					break;
				}
				else
// Test edit boundaries
				if(test_edit_handles(cursor_x, cursor_y, 1, new_cursor, update_cursor))
				{
					break;
				}
				else
// Test plugin boundaries
				if(test_plugin_handles(cursor_x, cursor_y, 1, new_cursor, update_cursor))
				{
					break;
				}
				else
				if(test_edits(cursor_x, cursor_y, 1, 0, update_cursor, rerender, new_cursor, update_cursor))
				{
					break;
				}
				else
				if(test_plugins(cursor_x, cursor_y, 0, 1))
				{
					break;
				}
				else
				if(test_resources(cursor_x, cursor_y))
				{
					break;
				}
				else
				if(test_tracks(cursor_x, cursor_y, 1))
				{
					break;
				}
				break;
			}

// Test handles only and select a region
			case EDITING_IBEAM:
			{
				double position = (double)cursor_x * 
					mwindow->edl->local_session->zoom_sample /
					mwindow->edl->session->sample_rate + 
					(double)mwindow->edl->local_session->view_start * 
					mwindow->edl->local_session->zoom_sample /
					mwindow->edl->session->sample_rate;
//printf("TrackCanvas::button_press_event %d\n", position);

				if(mwindow->edl->session->auto_conf->transitions && 
					test_transitions(cursor_x, 
						cursor_y, 
						1, 
						new_cursor, 
						update_cursor))
				{
					break;
				}
				else
				if(do_keyframes(cursor_x, 
					cursor_y, 
					0, 
					1, 
					new_cursor, 
					update_cursor,
					rerender))
				{
					update_overlay = 1;
					break;
				}
				else
// Test edit boundaries
				if(test_edit_handles(cursor_x, cursor_y, 1, new_cursor, update_cursor))
				{
					break;
				}
				else
// Test plugin boundaries
				if(test_plugin_handles(cursor_x, cursor_y, 1, new_cursor, update_cursor))
				{
					break;
				}
				else
				if(test_edits(cursor_x, cursor_y, 1, 0, update_cursor, rerender, new_cursor, update_cursor))
				{
					break;
				}
				else
				if(test_plugins(cursor_x, cursor_y, 0, 1))
				{
					break;
				}
				else
				if(test_tracks(cursor_x, cursor_y, 1))
				{
					break;
				}
// Highlight selection
				else
				{
					rerender = start_selection(position);
					mwindow->session->current_operation = SELECT_REGION;
					update_cursor = 1;
				}

				break;
			}
		}


		if(rerender)
		{
			mwindow->cwindow->update(1, 0, 0, 0, 1);
// Update faders
			mwindow->update_plugin_guis();
			gui->patchbay->update();
		}

		if(update_overlay)
		{
			draw_overlays();
			flash();
		}

		if(update_cursor)
		{
			gui->timebar->update_highlights();
			gui->cursor->hide();
			gui->cursor->show();
			gui->zoombar->update();
			flash();
			result = 1;
		}



	}
	return result;
}

int TrackCanvas::start_selection(double position)
{
	int rerender = 0;
	position = mwindow->edl->align_to_frame(position, 0);

// Extend a border
	if(shift_down())
	{
		double midpoint = (mwindow->edl->local_session->selectionstart + 
			mwindow->edl->local_session->selectionend) / 2;

		if(position < midpoint)
		{
			mwindow->edl->local_session->selectionstart = position;
			selection_midpoint1 = mwindow->edl->local_session->selectionend;
// Que the CWindow
			rerender = 1;
		}
		else
		{
			mwindow->edl->local_session->selectionend = position;
			selection_midpoint1 = mwindow->edl->local_session->selectionstart;
// Don't que the CWindow for the end
		}
	}
	else
// Start a new selection
	{
//printf("TrackCanvas::start_selection %f\n", position);
		mwindow->edl->local_session->selectionstart = 
			mwindow->edl->local_session->selectionend = 
			position;
		selection_midpoint1 = position;
// Que the CWindow
		rerender = 1;
	}
	
	return rerender;
}

void TrackCanvas::end_edithandle_selection()
{
	mwindow->modify_edithandles();
}

void TrackCanvas::end_pluginhandle_selection()
{
	mwindow->modify_pluginhandles();
}


double TrackCanvas::time_visible()
{
	return (double)get_w() * 
		mwindow->edl->local_session->zoom_sample / 
		mwindow->edl->session->sample_rate;
}









































void TrackCanvas::draw_automation()
{
}


int TrackCanvas::set_index_file(int flash, Asset *asset)
{
	return 0;
}


int TrackCanvas::button_release()
{
	return 0;
}


int TrackCanvas::auto_reposition(int &cursor_x, int &cursor_y, int64_t cursor_position)
{
	return 0;
}


int TrackCanvas::draw_floating_handle(int flash)
{
	return 0;
}

int TrackCanvas::draw_loop_point(int64_t position, int flash)
{
	return 0;
}

int TrackCanvas::draw_playback_cursor(int pixel, int flash)
{
	return 0;
}


int TrackCanvas::update_handle_selection(int64_t cursor_position)
{
	return 0;
}

int TrackCanvas::end_translation()
{
	return 0;
}

