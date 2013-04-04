
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
#include "autoconf.h"
#include "automation.h"
#include "bcsignals.h"
#include "bctimer.h"
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
#include "keyframepopup.h"
#include "keyframes.h"
#include "keys.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mainundo.h"
#include "maskautos.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "tracking.h"
#include "panautos.h"
#include "resourcethread.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "plugin.h"
#include "pluginpopup.h"
#include "pluginset.h"
#include "pluginserver.h"
#include "plugintoggles.h"
#include "preferences.h"
#include "resourcepixmap.h"
#include "mainsession.h"
#include "transitionpopup.h"
#include "zoombar.h"
#include "theme.h"
#include "intautos.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "vframe.h"
#include "apatchgui.inc"
#include "vpatchgui.inc"

#include <string.h>

TrackCanvas::TrackCanvas(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mcanvas_x,
	mwindow->theme->mcanvas_y,
	gui->view_w,
	gui->view_h)
{
	this->mwindow = mwindow;
	this->gui = gui;
	selection_midpoint1 = selection_midpoint2 = 0;
	handle_selected = 0;
	auto_selected = 0;
	translate_selected = 0;
	which_handle = 0;
	drag_scroll = 0;
	drag_popup = 0;
	active = 0;
	temp_picon = 0;
	resource_timer = new Timer;
	hourglass_enabled = 0;
	pixmaps_lock = new Mutex("TrackCanvas::pixm_lock");
	resource_thread = new ResourceThread(mwindow);
}

TrackCanvas::~TrackCanvas()
{
	for(int i = 0; i < resource_pixmaps.total; i++)
		delete resource_pixmaps.values[i];
	delete pixmaps_lock;
	delete edit_handles;
	delete keyframe_pixmap;
	delete camerakeyframe_pixmap;
	delete modekeyframe_pixmap;
	delete pankeyframe_pixmap;
	delete projectorkeyframe_pixmap;
	delete maskkeyframe_pixmap;
	delete background_pixmap;
	if(temp_picon) delete temp_picon;
	delete resource_timer;
}

int TrackCanvas::create_objects()
{
	background_pixmap = new BC_Pixmap(this, get_w(), get_h());
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
	draw(0, 0);
	flash();
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


// there's no point in drawing highlights has until drag operation has been set
	if (!mwindow->session->current_operation)
		return 0;

	if(get_cursor_over_window() &&
		cursor_x >= 0 && 
		cursor_y >= 0 && 
		cursor_x < get_w() && 
		cursor_y < get_h())
	{
// Find the edit and track the cursor is over
		for(Track *track = mwindow->edl->tracks->first; track; track = track->next)
		{
			int track_x, track_y, track_w, track_h;
			track_dimensions(track, track_x, track_y, track_w, track_h);

			if(cursor_y >= track_y && 
				cursor_y < track_y + track_h)
			{
				over_track = track;
				for(Edit *edit = track->edits->first; edit; edit = edit->next)
				{
					if (mwindow->session->current_operation != DRAG_ATRANSITION && 
						mwindow->session->current_operation != DRAG_VTRANSITION &&
						edit == track->edits->last) 
						break;
					int edit_x, edit_y, edit_w, edit_h;
					edit_dimensions(track, edit->project_pts, edit->end_pts(),
						edit_x, edit_y, edit_w, edit_h);

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
						plugin != pluginset->last;
						plugin = (Plugin*)plugin->next)
					{
						int plugin_x, plugin_y, plugin_w, plugin_h;
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

	if (!over_track) 	// check for pastes from patchbay
		over_track = mwindow->gui->patchbay->is_over_track();

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

	if (mwindow->session->current_operation == DRAG_ASSET ||
			mwindow->session->current_operation == DRAG_EDIT)
	{
		redraw = 1;
	}

	if(redraw)
	{
		draw_overlays();
		flash();
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
		if(do_plugins(get_drag_x(), 
			get_drag_y(), 
			1,
			0,
			redraw,
			rerender))
		{
			result = 1;
		}
		else
		if(do_edits(get_drag_x(),
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

void TrackCanvas::drag_motion_event()
{
	drag_motion();
}

void TrackCanvas::drag_stop_event()
{
	drag_stop();

	if(drag_popup)
	{
		delete drag_popup;
		drag_popup = 0;
	}
}


int TrackCanvas::drag_stop()
{
// In most cases the editing routine redraws and not the drag_stop
	int result = 0, redraw = 0;

	int insertion = 0;           // used in drag and drop mode
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
// Move source to different location
			if(mwindow->session->pluginset_highlighted)
			{
				if(mwindow->session->plugin_highlighted)
				{
					mwindow->move_effect(mwindow->session->drag_plugin,
						mwindow->session->plugin_highlighted->plugin_set,
						0,
						mwindow->session->plugin_highlighted->project_pts);
				}
				else
				{
					mwindow->move_effect(mwindow->session->drag_plugin,
						mwindow->session->pluginset_highlighted,
						0,
						mwindow->session->pluginset_highlighted->last->project_pts);
				}
				result = 1;
			}
			else
// Move to a new plugin set between two edits
			if(mwindow->session->edit_highlighted)
			{
				mwindow->move_effect(mwindow->session->drag_plugin,
					0,
					mwindow->session->track_highlighted,
					mwindow->session->edit_highlighted->project_pts);
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
				start = mwindow->session->plugin_highlighted->project_pts;
				length = mwindow->session->plugin_highlighted->length();
				if(length <= 0) length = track->get_length();
			}
			else
			if(mwindow->session->pluginset_highlighted)
			{
				start = plugin_set->last->project_pts;
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
			else
			if(mwindow->session->edit_highlighted)
			{
				start = mwindow->session->edit_highlighted->project_pts;
				length = mwindow->session->edit_highlighted->length();
			}

			mwindow->insert_effects_canvas(start, length);
			redraw = 1;
		}
		if (mwindow->session->track_highlighted)
			result = 1;  // we have to cleanup
		break;

	case DRAG_ASSET:
		if(mwindow->session->track_highlighted)
		{
			ptstime asset_length_float;
			ptstime position = 0;

			if(mwindow->session->current_operation == DRAG_ASSET &&
				mwindow->session->drag_assets->total)
			{
				Asset *asset = mwindow->session->drag_assets->values[0];
				// we use video if we are over video and audio if we are over audio
				if (asset->video_data && mwindow->session->track_highlighted->data_type == TRACK_VIDEO)
				{
					// Images have length -1
					double video_length = asset->video_length;
					if (video_length < 0)
					{
						if(mwindow->edl->session->si_useduration)
							video_length = mwindow->edl->session->si_duration;
						else
							video_length = 1.0 / mwindow->edl->session->frame_rate ; 
					}
					asset_length_float = video_length / asset->frame_rate;
				}
				else if (asset->audio_data && mwindow->session->track_highlighted->data_type == TRACK_AUDIO)
					asset_length_float = asset->audio_length / asset->sample_rate;
				else
				{
					result = 1;
					break;	// Do not do anything
				}
			} else
			if(mwindow->session->current_operation == DRAG_ASSET &&
				mwindow->session->drag_clips->total)
			{
				EDL *clip = mwindow->session->drag_clips->values[0];
				asset_length_float = clip->tracks->total_length();
			}
			else
			{
				printf("DRAG_ASSET error: Asset dropped, but both drag_clips and drag_assets total is zero\n");
			}
			position = get_drop_position (&insertion, NULL, asset_length_float);
			if (position == -1)
			{
				result = 1;
				break;		// Do not do anything
			}
			Track *track = mwindow->session->track_highlighted;

			mwindow->paste_assets(mwindow->session->track_highlighted->to_units(position), track, !insertion);
			result = 1;    // need to be one no matter what, since we have track highlited so we have to cleanup....
		}
		break;

	case DRAG_EDIT:
		mwindow->session->current_operation = NO_OPERATION;
		if(mwindow->session->track_highlighted)
		{
			if(mwindow->session->track_highlighted->data_type == mwindow->session->drag_edit->track->data_type)
			{
				ptstime position = get_drop_position (&insertion, mwindow->session->drag_edit, mwindow->session->drag_edit->length());

				if (position < -0.5)
				{
					result = 1;
					break;		// Do not do anything
				}
				Track *track = mwindow->session->track_highlighted;
				mwindow->move_edits(mwindow->session->drag_edits,
					track,
					position,
					!insertion);
			}

			result = 1;
		}
		break;
	}

// since we don't have subwindows we have to terminate any drag operation
	if(result)
	{
		if (mwindow->session->track_highlighted
			|| mwindow->session->edit_highlighted
			|| mwindow->session->plugin_highlighted
			|| mwindow->session->pluginset_highlighted) 
			redraw = 1;
		mwindow->session->track_highlighted = 0;
		mwindow->session->edit_highlighted = 0;
		mwindow->session->plugin_highlighted = 0;
		mwindow->session->pluginset_highlighted = 0;
		mwindow->session->current_operation = NO_OPERATION;
	}

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


ptstime TrackCanvas::get_drop_position(int *is_insertion,
	Edit *moved_edit, ptstime moved_edit_length)
{
	*is_insertion = 0;
// get the canvas/track position
	int cursor_x = get_relative_cursor_x();
	ptstime pos = round(cursor_x * mwindow->edl->local_session->zoom_time) +
			mwindow->edl->local_session->view_start_pts;

	Track *track = mwindow->session->track_highlighted;

// cursor relative position - depending on where we started the drag inside the edit
	ptstime cursor_position;
	if (moved_edit)  // relative cursor position depends upon grab point
		cursor_position = pos - (mwindow->session->drag_position - moved_edit->project_pts);
	else             // for clips and assets acts as they were grabbed in the middle
		cursor_position = pos - moved_edit_length / 2;

// we use real cursor position for affinity calculations
	ptstime real_cursor_position = pos;
	if (cursor_position < 0) cursor_position = 0;
	if (real_cursor_position < 0) real_cursor_position = 0;
	ptstime position = -1;
	ptstime span_start = 0;
	ptstime span_length = 0;
	int span_asset = 0;
	int last_ignore = 0; // used to make sure we can ignore the last edit if that is what we are dragging

	if (!track->edits->last)
	{
		// No edits -> no problems!
		position = cursor_position;
	}
	else
	{
		for (Edit *edit = track->edits->first; edit; edit = edit->next)
		{
// See edit liikus ja on eelmine asset
// Või puudub asset ja eelmine edit liikus
			if (edit && 
				((moved_edit && edit == moved_edit && edit->previous && !edit->previous->asset) ||
				(moved_edit && edit->previous == moved_edit  && !edit->asset)))
			{
// our fake edit spans over the edit we are moving
				span_length += edit->length();
				last_ignore = 1;
			} else
			{
				int edit_x, edit_y, edit_w, edit_h;

				edit_dimensions(track, span_start, span_start + span_length,
					edit_x, edit_y, edit_w, edit_h);
				if (abs(edit_x - cursor_x) < HANDLE_W)
				{
// cursor is close to the beginning of an edit -> insertion
					*is_insertion = 1;
					position = span_start;
				} else
				if (abs(edit_x + edit_w - cursor_x) < HANDLE_W)
				{
// cursor is close to the end of an edit -> insertion
					*is_insertion = 1;
					position = span_start + span_length;
				}  else
				if (!span_asset &&
					span_start <= cursor_position &&
					span_start + span_length >= cursor_position + moved_edit_length)
				{
// we have enough empty space to position the edit where user wants 
					position = cursor_position; 
				} else
				if (!span_asset &
					real_cursor_position >= span_start && 
					real_cursor_position < span_start + span_length && 
					span_length >= moved_edit_length)
				{
// we are inside an empty edit, but cannot push the edit as far as user wants, so 'resist moving it further'
					if (fabs(real_cursor_position - span_start) < fabs(real_cursor_position - span_start - span_length))
						position = span_start;
					else
						position = span_start + span_length - moved_edit_length;
				} else
				if (cursor_x > edit_x && cursor_x <= edit_x + edit_w / 2)
				{
// we are inside an nonempty edit, - snap to left
					*is_insertion = 1;
					position = span_start;
				} else
				if (cursor_x > edit_x + edit_w / 2 && cursor_x <= edit_x + edit_w)
				{
// we are inside an nonempty edit, - snap to right
					*is_insertion = 1;
					position = span_start + span_length;
				}

				if (position != -1) 
					break;

				if (edit)
				{
					span_length = edit->length();
					span_start = edit->project_pts;
					last_ignore = 0;
					if (!edit->asset || (!moved_edit || moved_edit == edit)) 
					{
// empty edit, missing moved edit or not current
						if (moved_edit && moved_edit == edit)
							last_ignore = 1;
							span_asset = 0;
					} else
						span_asset = 1;
				}
			}
		}

	}
	if (PTSEQU(real_cursor_position, 0))
	{
		position = 0;
		*is_insertion = 1;
	}
	return position;
}

void TrackCanvas::draw(int mode, int hide_cursor)
{
// Swap pixmap layers
	if(get_w() != background_pixmap->get_w() ||
		get_h() != background_pixmap->get_h())
	{
		delete background_pixmap;
		background_pixmap = new BC_Pixmap(this, get_w(), get_h());
	}

// Cursor disappears after resize when this is called.
// Cursor doesn't redraw after editing when this isn't called.
	if(gui->cursor && hide_cursor) gui->cursor->hide();
	draw_top_background(get_parent(), 0, 0, get_w(), get_h(), background_pixmap);
	draw_resources(mode);
	draw_overlays();
}

void TrackCanvas::update_cursor()
{
	switch(mwindow->edl->session->editing_mode)
	{
	case EDITING_ARROW:
		set_cursor(ARROW_CURSOR);
		break;
	case EDITING_IBEAM: 
		set_cursor(IBEAM_CURSOR);
		break;
	}
}


void TrackCanvas::test_timer()
{
	if(resource_timer->get_difference() > 1000 && 
		!hourglass_enabled)
	{
		start_hourglass();
		hourglass_enabled = 1;
	}
}


void TrackCanvas::draw_indexes(Asset *asset)
{
// Don't redraw raw samples
	if(asset->index_zoom > mwindow->edl->local_session->zoom_time *
			mwindow->edl->session->sample_rate)
		return;

	draw_resources(0, 1, asset);
	draw_overlays();
	flash();
	flush();
}

void TrackCanvas::draw_resources(int mode, 
	int indexes_only, 
	Asset *index_asset)
{
	if(!mwindow->edl->session->show_assets) return;

// Drawing is already in progress, do nothing
	if(pixmaps_lock->is_locked())
		return;

	pixmaps_lock->lock("TrackCanvas::draw_resources");

	if(mode != 3 && !indexes_only)
		resource_thread->stop_draw(!indexes_only);

	resource_timer->update();

// Age resource pixmaps for deletion
	if(!indexes_only)
		for(int i = 0; i < resource_pixmaps.total; i++)
			resource_pixmaps.values[i]->visible--;

	if(mode == 2)
		resource_pixmaps.remove_all_objects();

// Search every edit
	for(Track *current = mwindow->edl->tracks->first;
		current;
		current = NEXT)
	{
		for(Edit *edit = current->edits->first; edit; edit = edit->next)
		{
			if(!edit->asset) continue;
			if(indexes_only)
			{
				if(edit->track->data_type != TRACK_AUDIO) continue;
				if(!edit->asset->test_path(index_asset->path)) continue;
			}

			int edit_x, edit_y, edit_w, edit_h;
			edit_dimensions(current, edit->project_pts, edit->end_pts(),
				edit_x, edit_y, edit_w, edit_h);

// Edit is visible
			if(MWindowGUI::visible(edit_x, edit_x + edit_w, 0, get_w()) &&
				MWindowGUI::visible(edit_y, edit_y + edit_h, 0, get_h()))
			{
				int pixmap_x, pixmap_w, pixmap_h;

// Search for existing pixmap containing edit
				for(int i = 0; i < resource_pixmaps.total; i++)
				{
					ResourcePixmap* pixmap = resource_pixmaps.values[i];
// Same pointer can be different edit if editing took place
					if(pixmap->edit_id == edit->id)
					{
						pixmap->visible = 1;
						break;
					}
				}

// Get new size, offset of pixmap needed
				get_pixmap_size(edit, 
					edit_x, 
					edit_w, 
					pixmap_x, 
					pixmap_w, 
					pixmap_h);

// Draw new data
				if(pixmap_w && pixmap_h)
				{
// Create pixmap if it doesn't exist
					ResourcePixmap* pixmap = create_pixmap(edit, 
						pixmap_w, 
						pixmap_h);
// Resize it if it's bigger
					if(pixmap_w > pixmap->pixmap_w ||
						pixmap_h > pixmap->pixmap_h)
						pixmap->resize(pixmap_w, pixmap_h);
					pixmap->draw_data(edit,
						edit_x, 
						edit_w, 
						pixmap_x, 
						pixmap_w, 
						pixmap_h, 
						mode,
						indexes_only);
// Resize it if it's smaller
					if(pixmap_w < pixmap->pixmap_w ||
						pixmap_h < pixmap->pixmap_h)
						pixmap->resize(pixmap_w, pixmap_h);

// Copy pixmap to background canvas
					background_pixmap->draw_pixmap(pixmap, 
						pixmap->pixmap_x, 
						current->y_pixel,
						pixmap->pixmap_w,
						edit_h);
				}
			}
		}
	}

// Delete unused pixmaps
	if(!indexes_only)
		for(int i = resource_pixmaps.total - 1; i >= 0; i--)
			if(resource_pixmaps.values[i]->visible < -5)
			{
				delete resource_pixmaps.values[i];
				resource_pixmaps.remove(resource_pixmaps.values[i]);
			}
	pixmaps_lock->unlock();

	if(hourglass_enabled) 
	{
		stop_hourglass();
		hourglass_enabled = 0;
	}

	if(mode != 3 && !indexes_only)
		resource_thread->start_draw();
}

ResourcePixmap* TrackCanvas::create_pixmap(Edit *edit,
	int pixmap_w, 
	int pixmap_h)
{
	ResourcePixmap *result = 0;

	for(int i = 0; i < resource_pixmaps.total; i++)
	{
		if(resource_pixmaps.values[i]->edit_id == edit->id) 
		{
			result = resource_pixmaps.values[i];
			break;
		}
	}

	if(!result)
	{
		result = new ResourcePixmap(mwindow, 
			this, 
			edit, 
			pixmap_w, 
			pixmap_h);
		resource_pixmaps.append(result);
	}

	return result;
}

void TrackCanvas::get_pixmap_size(Edit *edit, 
	int edit_x,
	int edit_w, 
	int &pixmap_x, 
	int &pixmap_w,
	int &pixmap_h)
{

// Align x on frame boundaries

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

	pixmap_h = mwindow->edl->local_session->zoom_track;
	if(mwindow->edl->session->show_titles) 
		pixmap_h += mwindow->theme->get_image("title_bg_data")->get_h();
}

void TrackCanvas::edit_dimensions(Track *track, ptstime start, ptstime end,
	int &x,
	int &y,
	int &w,
	int &h)
{
	h = resource_h();

	x = round((start - mwindow->edl->local_session->view_start_pts) /
			mwindow->edl->local_session->zoom_time);

// Method for calculating w so when edits are together we never get off by one error due to rounding
	w = (int)(round((end - mwindow->edl->local_session->view_start_pts) /
			mwindow->edl->local_session->zoom_time)) - x;

	y = track->y_pixel;

	if(mwindow->edl->session->show_titles) 
		h += mwindow->theme->get_image("title_bg_data")->get_h();
}

void TrackCanvas::track_dimensions(Track *track, 
	int &x, int &y, int &w, int &h)
{
	x = 0;
	w = get_w();
	y = track->y_pixel;
	h = track->vertical_span(mwindow->theme);
}


void TrackCanvas::draw_paste_destination()
{
	int current_atrack = 0;
	int current_vtrack = 0;
	int current_aedit = 0;
	int current_vedit = 0;
	int w = 0;
	int x;
	ptstime position;
	int insertion  = 0;

	if((mwindow->session->current_operation == DRAG_ASSET &&
		(mwindow->session->drag_assets->total ||
			mwindow->session->drag_clips->total)) ||
		(mwindow->session->current_operation == DRAG_EDIT &&
			mwindow->session->drag_edits->total))
	{
		Asset *asset = 0;
		EDL *clip = 0;
		int draw_box = 0;

		if(mwindow->session->current_operation == DRAG_ASSET &&
			mwindow->session->drag_assets->total)
			asset = mwindow->session->drag_assets->values[0];

		if(mwindow->session->current_operation == DRAG_ASSET &&
			mwindow->session->drag_clips->total)
			clip = mwindow->session->drag_clips->values[0];

// 'Align cursor of frame' lengths calculations
		ptstime paste_audio_length, paste_video_length;
		ptstime asset_length;
		ptstime desta_position = 0;
		ptstime destv_position = 0;

		if (asset)
		{
			double asset_length_ac = asset->total_length_framealigned(mwindow->edl->session->frame_rate);
			if (mwindow->edl->session->cursor_on_frames)
			{
				paste_video_length = paste_audio_length = asset_length_ac;
			} 
			else 
			{
				paste_audio_length = (double)asset->audio_length / asset->sample_rate;
				paste_video_length = (double)asset->video_length / asset->frame_rate;
			}

			// Images have length -1 (they are a single image!!)
			if (asset->video_data && asset->video_length < 0)
			{
				if(mwindow->edl->session->si_useduration)
					paste_video_length = mwindow->edl->session->si_duration / asset->frame_rate;
				else
					paste_video_length = 1.0 / asset->frame_rate;  // bit confused!! (this is 1 frame)
			}

			ptstime asset_length = 0;

			if(asset->audio_data)
			{
				// we use video if we are over video and audio if we are over audio
				if(asset->video_data && mwindow->session->track_highlighted->data_type == TRACK_VIDEO)
					asset_length = paste_video_length;
				else
					asset_length = paste_audio_length;
				desta_position = get_drop_position(&insertion, NULL, asset_length);
			}

			if(asset->video_data)
			{
				asset_length = paste_video_length;
				destv_position = get_drop_position(&insertion, NULL, asset_length);
			}
		}

		if(clip)
		{
			if(mwindow->edl->session->cursor_on_frames)
				paste_audio_length = paste_video_length = clip->tracks->total_length_framealigned(mwindow->edl->session->frame_rate);
			else
				paste_audio_length = paste_video_length = clip->tracks->total_length();

			desta_position = get_drop_position(&insertion, NULL, clip->tracks->total_length());
		}

// Get destination track
		for(Track *dest = mwindow->session->track_highlighted; 
			dest; 
			dest = dest->next)
		{
			if(dest->record)
			{
// Get source width in pixels
				w = -1;

// Use start of highlighted edit
				if(mwindow->session->edit_highlighted)
					position = mwindow->session->edit_highlighted->project_pts;
				else
// Use end of highlighted track, disregarding effects
					position = mwindow->session->track_highlighted->edits->last->project_pts;


				if(dest->data_type == TRACK_AUDIO)
				{
					if( (asset && current_atrack < asset->channels)
						|| (clip  && current_atrack < clip->tracks->total_audio_tracks()) )
					{
						w = Units::to_int64(paste_audio_length *
							mwindow->edl->local_session->zoom_time *
							mwindow->edl->session->sample_rate);

						position = desta_position;
						if (position < 0) 
							w = -1;
						else
						{
							current_atrack++;
							draw_box = 1;
						}
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
							w = Units::to_int64(edit->length() / mwindow->edl->local_session->zoom_time);
							position = get_drop_position(&insertion, mwindow->session->drag_edit, mwindow->session->drag_edit->length());
							if (position < 0) 
								w = -1;
							else
							{
								current_aedit++;
								draw_box = 1;
							}
						}
					}
				}
				else
				if(dest->data_type == TRACK_VIDEO)
				{
					if( (asset && current_vtrack < asset->layers)
						|| (clip && current_vtrack < clip->tracks->total_video_tracks()) )
					{
						// Images have length -1
						w = Units::to_int64((double)paste_video_length /
							mwindow->edl->local_session->zoom_time);

						position = destv_position;
						if (position < 0) 
							w = -1;
						else
						{
							current_vtrack++;
							draw_box = 1;
						}
					}
					else
					if(mwindow->session->current_operation == DRAG_EDIT &&
						current_vedit < mwindow->session->drag_edits->total)
					{
						Edit *edit;
						while(current_vedit < mwindow->session->drag_edits->total &&
							mwindow->session->drag_edits->values[current_vedit]->track->data_type != TRACK_VIDEO)
							current_vedit++;

						if(current_vedit < mwindow->session->drag_edits->total)
						{
							edit = mwindow->session->drag_edits->values[current_vedit];
							w = Units::to_int64(edit->length() /
								mwindow->edl->local_session->zoom_time);
							position = get_drop_position(&insertion, mwindow->session->drag_edit, mwindow->session->drag_edit->length());
							if (position < 0) 
								w = -1;
							else
							{
								current_vedit++;
								draw_box = 1;
							}
						}
					}
				}

				if(w >= 0)
				{
// Get the x coordinate
					x = round((position - mwindow->edl->local_session->view_start_pts) /
						mwindow->edl->local_session->zoom_time);

					int y = dest->y_pixel;
					int h = dest->vertical_span(mwindow->theme);

					if (insertion)
						draw_highlight_insertion(x, y, w, h);
					else
						draw_highlight_rectangle(x, y, w, h);
				}
			}
		}
	}
}

void TrackCanvas::plugin_dimensions(Plugin *plugin, 
	int &x, int &y, int &w, int &h)
{
	x = round((plugin->project_pts - mwindow->edl->local_session->view_start_pts) /
		mwindow->edl->local_session->zoom_time);
	w = round(plugin->length() /
		mwindow->edl->local_session->zoom_time);
	y = plugin->track->y_pixel + 
			mwindow->edl->local_session->zoom_track +
			plugin->plugin_set->get_number() * 
			mwindow->theme->get_image("plugin_bg_data")->get_h();
	if(mwindow->edl->session->show_titles)
		y += mwindow->theme->get_image("title_bg_data")->get_h();
	h = mwindow->theme->get_image("plugin_bg_data")->get_h();
}

int TrackCanvas::resource_h()
{
	return mwindow->edl->local_session->zoom_track;
}

void TrackCanvas::draw_highlight_rectangle(int x, int y, int w, int h)
{

// if we have to draw a highlighted rectangle completely on the left or completely on the right of the viewport, 
// just draw arrows, so user has indication that something is there
// FIXME: get better colors

	if (x + w <= 0)
	{
		draw_triangle_left(0, y + h /6, h * 2/3, h * 2/3, BLACK, GREEN, YELLOW, RED, BLUE);
		return;
	} else
	if (x >= get_w())
	{
		draw_triangle_right(get_w() - h * 2/3, y + h /6, h * 2/3, h * 2/3, BLACK, GREEN, YELLOW, RED, BLUE);
		return;
	}

// Fix bug in heroines & cvs version as of 22.8.2005:
// If we grab when zoomed in and zoom out while dragging, when edit gets really narrow strange things start happening
	if (w >= 0 && w < 3)
	{
		x -= w /2;
		w = 3;
	}
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
}

void TrackCanvas::draw_highlight_insertion(int x, int y, int w, int h)
{

// if we have to draw a highlighted rectangle completely on the left or completely on the right of the viewport, 
// just draw arrows, so user has indication that something is there
// FIXME: get better colors

	int h1 = h / 8;
	int h2 = h / 4;

	set_inverse();

	draw_triangle_right(x - h2, y + h1, h2, h2, BLACK, GREEN, YELLOW, RED, BLUE);
	draw_triangle_right(x - h2, y + h1*5, h2, h2, BLACK, GREEN, YELLOW, RED, BLUE);

	draw_triangle_left(x, y + h1, h2, h2, BLACK, GREEN, YELLOW, RED, BLUE);
	draw_triangle_left(x, y + h1*5, h2, h2, BLACK, GREEN, YELLOW, RED, BLUE);

// draw the box centred around x
	x -= w / 2;
// Fix bug in heroines & cvs version as of 22.8.2005:
// If we grab when zoomed in and zoom out while dragging, when edit gets really narrow strange things start happening
	if (w >= 0 && w < 3) 
	{
		x -= w /2;
		w = 3;
	}
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
}

void TrackCanvas::get_handle_coords(Edit *edit, 
		int &x, int &y, int &w, int &h, int side)
{
	int handle_w = mwindow->theme->edithandlein_data[0]->get_w();
	int handle_h = mwindow->theme->edithandlein_data[0]->get_h();

	edit_dimensions(edit->track, edit->project_pts, edit->end_pts(), x, y, w, h);

	if(mwindow->edl->session->show_titles)
	{
		y += mwindow->theme->get_image("title_bg_data")->get_h();
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

void TrackCanvas::get_transition_coords(int &x, int &y, int &w, int &h)
{
	int transition_w = 30;
	int transition_h = 30;

	if(mwindow->edl->session->show_titles)
		y += mwindow->theme->get_image("title_bg_data")->get_h();

	y += (h - mwindow->theme->get_image("title_bg_data")->get_h()) / 2 - transition_h / 2;
	x -= transition_w / 2;

	h = transition_h;
	w = transition_w;
}

void TrackCanvas::draw_highlighting()
{
	int x, y, w, h;
	int draw_box = 0;

	switch(mwindow->session->current_operation)
	{
	case DRAG_ATRANSITION:
	case DRAG_VTRANSITION:
		if(mwindow->session->edit_highlighted)
		{
			if((mwindow->session->current_operation == DRAG_ATRANSITION && 
				mwindow->session->track_highlighted->data_type == TRACK_AUDIO) ||
				(mwindow->session->current_operation == DRAG_VTRANSITION && 
				mwindow->session->track_highlighted->data_type == TRACK_VIDEO))
			{
				edit_dimensions(mwindow->session->edit_highlighted->track,
				mwindow->session->edit_highlighted->project_pts,
					mwindow->session->edit_highlighted->end_pts(),
					x, y, w, h);

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					draw_box = 1;
					get_transition_coords(x, y, w, h);
				}
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
				int track_x, track_y, track_w, track_h;
				track_dimensions(mwindow->session->track_highlighted, 
					track_x, 
					track_y, 
					track_w, 
					track_h);

				x += w;
				w = (int)(round((mwindow->session->track_highlighted->get_length() -
					mwindow->edl->local_session->view_start_pts) /
					mwindow->edl->local_session->zoom_time)) - x;
				if(w <= 0) w = track_w;
			}
			else
			{
				track_dimensions(mwindow->session->track_highlighted,
					x,
					y,
					w,
					h);

// Put it in a new plugin set determined by the selected range
				if(mwindow->edl->local_session->get_selectionend() > 
					mwindow->edl->local_session->get_selectionstart())
				{
					x = round((mwindow->edl->local_session->get_selectionstart() - mwindow->edl->local_session->view_start_pts) /
						mwindow->edl->local_session->zoom_time);
					w = round((mwindow->edl->local_session->get_selectionend() -
						mwindow->edl->local_session->get_selectionstart()) /
						mwindow->edl->local_session->zoom_time);
				}
// Put it in a new plugin set determined by an edit boundary
				else
				if(mwindow->session->edit_highlighted)
				{
					int temp_y, temp_h;
					edit_dimensions(mwindow->session->edit_highlighted->track,
						mwindow->session->edit_highlighted->project_pts,
						mwindow->session->edit_highlighted->end_pts(),
						x,
						temp_y,
						w,
						temp_h);
				}
// Put it at the beginning of the track in a new plugin set
			}

			if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
				MWindowGUI::visible(y, y + h, 0, get_h()))
			{
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
					int temp_y, temp_h;
					edit_dimensions(mwindow->session->edit_highlighted->track,
						mwindow->session->edit_highlighted->project_pts,
						mwindow->session->edit_highlighted->end_pts(),
						x,
						temp_y,
						w,
						temp_h);
				}
// Put it in a new plugin set at the start of the track
			}

			w = (int)(mwindow->session->drag_plugin->length() /
				mwindow->edl->local_session->zoom_time);

			if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
				MWindowGUI::visible(y, y + h, 0, get_h()))
			{
					draw_box = 1;
			}
		}
		break;

	case DRAG_PLUGINKEY:
		if(mwindow->session->plugin_highlighted && 
			mwindow->session->current_operation == DRAG_PLUGINKEY)
		{
// Just highlight the plugin
			plugin_dimensions(mwindow->session->plugin_highlighted, x, y, w, h);

			if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
				MWindowGUI::visible(y, y + h, 0, get_h()))
			{
				draw_box = 1;
			}
		}
		break;

	case DRAG_EDIT:
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
	int current_toggle = 0;

	if(!mwindow->edl->session->show_assets) goto done;

	for(int i = 0; i < plugin_on_toggles.total; i++)
		plugin_on_toggles.values[i]->in_use = 0;
	for(int i = 0; i < plugin_show_toggles.total; i++)
		plugin_show_toggles.values[i]->in_use = 0;


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
					int total_x, y, total_w, h;
					plugin_dimensions(plugin, total_x, y, total_w, h);

					if(MWindowGUI::visible(total_x, total_x + total_w, 0, get_w()) &&
						MWindowGUI::visible(y, y + h, 0, get_h()) &&
						plugin->plugin_type != PLUGIN_NONE)
					{
						int x = total_x, w = total_w, left_margin = 5;
						int right_margin = 5;
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
							mwindow->theme->get_image("plugin_bg_data"),
							0);
						set_color(get_resources()->default_text_color);
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


// Update plugin toggles
						int toggle_x = total_x + total_w;
						toggle_x = MIN(get_w() - right_margin, toggle_x);
						toggle_x -= PluginOn::calculate_w(mwindow) + 10;
						int toggle_y = y;
						if(current_toggle >= plugin_on_toggles.total)
						{
							PluginOn *plugin_on = new PluginOn(mwindow, toggle_x, toggle_y, plugin);
							add_subwindow(plugin_on);
							plugin_on_toggles.append(plugin_on);
						}
						else
						{
							plugin_on_toggles.values[current_toggle]->update(toggle_x, toggle_y, plugin);
						}

						toggle_x -= PluginShow::calculate_w(mwindow) + 10;
						if(current_toggle >= plugin_show_toggles.total)
						{
							PluginShow *plugin_off = new PluginShow(mwindow, toggle_x, toggle_y, plugin);
							add_subwindow(plugin_off);
							plugin_show_toggles.append(plugin_off);
						}
						else
						{
							plugin_show_toggles.values[current_toggle]->update(toggle_x, toggle_y, plugin);
						}
						current_toggle++;
					}
				}
			}
		}
	}


done:
	int i = current_toggle;
	while(i < plugin_on_toggles.total &&
		i < plugin_show_toggles.total)
	{
		plugin_on_toggles.remove_object_number(current_toggle);
		plugin_show_toggles.remove_object_number(current_toggle);
	}
}

void TrackCanvas::refresh_plugintoggles()
{
	for(int i = 0; i < plugin_on_toggles.total; i++)
	{
		PluginOn *on = plugin_on_toggles.values[i];
		on->reposition_window(on->get_x(), on->get_y());
	}
	for(int i = 0; i < plugin_show_toggles.total; i++)
	{
		PluginShow *show = plugin_show_toggles.values[i];
		show->reposition_window(show->get_x(), show->get_y());
	}
}

void TrackCanvas::draw_drag_handle()
{
	if(mwindow->session->current_operation == DRAG_EDITHANDLE2 ||
		mwindow->session->current_operation == DRAG_PLUGINHANDLE2)
	{
		int pixel1 = round((mwindow->session->drag_position -
			mwindow->edl->local_session->view_start_pts) /
			mwindow->edl->local_session->zoom_time);
		set_color(GREEN);
		set_inverse();
		draw_line(pixel1, 0, pixel1, get_h());
		set_opaque();
	}
}


void TrackCanvas::draw_transitions()
{
	int x, y, w, h;

	if(!mwindow->edl->session->show_assets) return;

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
				int strip_w, strip_x, strip_y;
				edit_dimensions(track, edit->project_pts, edit->end_pts(),
					x, y, w, h);
				strip_x = x ;
				strip_y = y;
				if(mwindow->edl->session->show_titles)
					strip_y += mwindow->theme->get_image("title_bg_data")->get_h();

				get_transition_coords(x, y, w, h);
				strip_w = Units::round(edit->transition->length() /
					mwindow->edl->local_session->zoom_time);

				if(MWindowGUI::visible(x, x + w, 0, get_w()) &&
					MWindowGUI::visible(y, y + h, 0, get_h()))
				{
					PluginServer *server = mwindow->scan_plugindb(edit->transition->title,
						track->data_type);
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
				if(MWindowGUI::visible(strip_x, strip_x + strip_w, 0, get_w()) &&
					MWindowGUI::visible(strip_y, strip_y + h, 0, get_h()))
				{
					int x = strip_x, w = strip_w, left_margin = 5;
					if(x < 0)
					{
						w -= -x;
						x = 0;
					}
					if(w + x > get_w()) w -= (w + x) - get_w();

					draw_3segmenth(
						x, 
						strip_y, 
						w, 
						strip_x,
						strip_w,
						mwindow->theme->get_image("plugin_bg_data"),
						0);

				}
			}
		}
	}
}

void TrackCanvas::draw_loop_points()
{
	if(mwindow->edl->local_session->loop_playback)
	{
		int x = round((mwindow->edl->local_session->loop_start -
			mwindow->edl->local_session->view_start_pts) /
			mwindow->edl->local_session->zoom_time);

		if(MWindowGUI::visible(x, x + 1, 0, get_w()))
		{
			set_color(GREEN);
			draw_line(x, 0, x, get_h());
		}

		x = Units::round((mwindow->edl->local_session->loop_end -
			mwindow->edl->local_session->view_start_pts) /
			mwindow->edl->local_session->zoom_time);

		if(MWindowGUI::visible(x, x + 1, 0, get_w()))
		{
			set_color(GREEN);
			draw_line(x, 0, x, get_h());
		}
	}
}

void TrackCanvas::draw_brender_start()
{
	if(mwindow->preferences->use_brender)
	{
		int x = round((mwindow->edl->session->brender_start -
			mwindow->edl->local_session->view_start_pts) /
			mwindow->edl->local_session->zoom_time);

		if(MWindowGUI::visible(x, x + 1, 0, get_w()))
		{
			set_color(RED);
			draw_line(x, 0, x, get_h());
		}
	}
}

static int auto_colors[] = 
{
	BLUE,
	RED,
	GREEN,
	BLUE,
	RED,
	GREEN,
	BLUE,
	WHITE,
	0,
	0,
	0,
	0
};

// The operations which correspond to each automation type
static int auto_operations[] = 
{
	DRAG_MUTE,
	DRAG_CAMERA_X,
	DRAG_CAMERA_Y,
	DRAG_CAMERA_Z,
	DRAG_PROJECTOR_X,
	DRAG_PROJECTOR_Y,
	DRAG_PROJECTOR_Z,
	DRAG_FADE,
	DRAG_PAN,
	DRAG_MODE,
	DRAG_MASK,
	DRAG_NUDGE
};

// The buttonpress operations, so nothing changes unless the mouse moves
// a certain amount.  This allows the keyframe to be used to position the
// insertion point without moving itself.
static int pre_auto_operations[] =
{
	DRAG_MUTE,
	DRAG_CAMERA_X,
	DRAG_CAMERA_Y,
	DRAG_CAMERA_Z,
	DRAG_PROJECTOR_X,
	DRAG_PROJECTOR_Y,
	DRAG_PROJECTOR_Z,
	DRAG_FADE,
	DRAG_PAN_PRE,
	DRAG_MODE_PRE,
	DRAG_MASK_PRE,
	DRAG_NUDGE
};


int TrackCanvas::do_keyframes(int cursor_x, 
	int cursor_y, 
	int draw, 
	int buttonpress, 
	int &new_cursor,
	int &update_cursor,
	int &rerender)
{
// Note: button 3 (right mouse button) is not eaten to allow
// track context menu to appear
	int current_tool = 0;
	int result = 0;
	EDLSession *session = mwindow->edl->session;

	BC_Pixmap *auto_pixmaps[] = 
	{
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		pankeyframe_pixmap,
		modekeyframe_pixmap,
		maskkeyframe_pixmap,
		0,
	};

	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		Auto *auto_keyframe;
		Automation *automation = track->automation;

// Handle float autos
		for(int i = 0; i < AUTOMATION_TOTAL && !result; i++)
		{
// Event not trapped and automation visible
			Autos *autos = automation->autos[i];
			if(!result && session->auto_conf->autos[i] && autos)
			{
				switch(i)
				{
				case AUTOMATION_MODE:
				case AUTOMATION_PAN:
				case AUTOMATION_MASK:
					result = do_autos(track, 
						autos,
						cursor_x, 
						cursor_y, 
						draw, 
						buttonpress,
						auto_pixmaps[i],
						auto_keyframe);
					break;

				default:
					switch(autos->get_type())
					{
					case AUTOMATION_TYPE_FLOAT:
						{
							result = do_float_autos(track, 
								autos,
								cursor_x,
								cursor_y,
								draw,
								buttonpress,
								auto_colors[i],
								auto_keyframe);
						}
						break;

					case AUTOMATION_TYPE_INT:
						result = do_toggle_autos(track, 
							autos,
							cursor_x,
							cursor_y,
							draw, 
							buttonpress,
							auto_colors[i],
							auto_keyframe);
						break;
					}
					break;
				}

				if(result)
				{
					if(mwindow->session->current_operation == auto_operations[i])
						rerender = 1;
					if(buttonpress)
					{
						if (buttonpress != 3)
						{
							if(i == AUTOMATION_FADE) 
								synchronize_autos(0, 
									track, 
									(FloatAuto*)mwindow->session->drag_auto, 
									1);
							mwindow->session->current_operation = pre_auto_operations[i];
							rerender = 1;
						}
						else
						{
							gui->keyframe_menu->update(automation, autos, auto_keyframe);
							gui->keyframe_menu->activate_menu();
							rerender = 1; // the position changes
						}
					}
				}
			}
		}


		if(!result && 
			session->auto_conf->plugins &&
			mwindow->edl->session->show_assets)
		{
			Plugin *plugin;
			KeyFrame *keyframe;
			result = do_plugin_autos(track,
				cursor_x, 
				cursor_y, 
				draw, 
				buttonpress,
				plugin,
				keyframe);
			if(result && mwindow->session->current_operation == DRAG_PLUGINKEY)
			{
				rerender = 1;
			}
			if(result && (buttonpress == 1))
			{
				mwindow->session->current_operation = DRAG_PLUGINKEY_PRE;
				rerender = 1;
			} else
			if (result && (buttonpress == 3))
			{
				gui->keyframe_menu->update(plugin, keyframe);
				gui->keyframe_menu->activate_menu();
				rerender = 1; // the position changes
			}
		}
	}

// Final pass to trap event
	for(int i = 0; i < AUTOMATION_TOTAL; i++)
	{
		if(mwindow->session->current_operation == pre_auto_operations[i] ||
			mwindow->session->current_operation == auto_operations[i])
			result = 1;
	}

	if(mwindow->session->current_operation == DRAG_PLUGINKEY ||
		mwindow->session->current_operation == DRAG_PLUGINKEY_PRE)
	{
		result = 1;
	}

	update_cursor = 1;
	if(result)
	{
		new_cursor = UPRIGHT_ARROW_CURSOR;
	}

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

// Center
	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	CLAMP(y1, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(y2, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);

	if(y2 - 1 > y1)
	{
		set_color(BLACK);
		draw_box(x1 + 1, y1 + 1, x2 - x1, y2 - y1);
		set_color(color);
		draw_box(x1, y1, x2 - x1, y2 - y1);
	}

// In handle
	in_x1 = in_x - HANDLE_W / 2;
	in_x2 = in_x + HANDLE_W / 2;
	in_y1 = center_pixel + in_y - HANDLE_W / 2;
	in_y2 = center_pixel + in_y + HANDLE_W / 2;

	CLAMP(in_y1, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(in_y2, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(in_y, -zoom_track / 2, zoom_track / 2);

	if(in_y2 > in_y1)
	{
		set_color(BLACK);
		draw_line(x + 1, center_pixel + y + 1, in_x + 1, center_pixel + in_y + 1);
		draw_box(in_x1 + 1, in_y1 + 1, in_x2 - in_x1, in_y2 - in_y1);
		set_color(color);
		draw_line(x, center_pixel + y, in_x, center_pixel + in_y);
		draw_box(in_x1, in_y1, in_x2 - in_x1, in_y2 - in_y1);
	}


// Out handle
	out_x1 = out_x - HANDLE_W / 2;
	out_x2 = out_x + HANDLE_W / 2;
	out_y1 = center_pixel + out_y - HANDLE_W / 2;
	out_y2 = center_pixel + out_y + HANDLE_W / 2;

	CLAMP(out_y1, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(out_y2, center_pixel + -zoom_track / 2, center_pixel + zoom_track / 2);
	CLAMP(out_y, -zoom_track / 2, zoom_track / 2);

	if(out_y2 > out_y1)
	{
		set_color(BLACK);
		draw_line(x + 1, center_pixel + y + 1, out_x + 1, center_pixel + out_y + 1);
		draw_box(out_x1 + 1, out_y1 + 1, out_x2 - out_x1, out_y2 - out_y1);
		set_color(color);
		draw_line(x, center_pixel + y, out_x, center_pixel + out_y);
		draw_box(out_x1, out_y1, out_x2 - out_x1, out_y2 - out_y1);
	}
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
	int result = 0;

	x1 = x - HANDLE_W / 2;
	x2 = x + HANDLE_W / 2;
	y1 = center_pixel + y - HANDLE_W / 2;
	y2 = center_pixel + y + HANDLE_W / 2;

	if(y1 < center_pixel + -zoom_track / 2) y1 = center_pixel + -zoom_track / 2;
	if(y2 > center_pixel + zoom_track / 2) y2 = center_pixel + zoom_track / 2;

	if(cursor_x >= x1 && cursor_x < x2 && cursor_y >= y1 && cursor_y < y2)
	{
		if(buttonpress && buttonpress != 3)
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = current->value_to_percentage();
			mwindow->session->drag_start_postime = current->pos_time;
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

// Test value
	if(!ctrl_down() &&
		cursor_x >= x1 && 
		cursor_x < x2 && 
		cursor_y >= y1 && 
		cursor_y < y2)
	{
		if(buttonpress && (buttonpress != 3))
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = current->value_to_percentage();
			mwindow->session->drag_start_postime = current->pos_time;
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
		current->pos_time > 0)
	{
		if(buttonpress && (buttonpress != 3))
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = 
				current->invalue_to_percentage();
			mwindow->session->drag_start_postime = 
				((FloatAuto*)current)->control_in_pts;
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
		if(buttonpress && (buttonpress != 3))
		{
			mwindow->session->drag_auto = current;
			mwindow->session->drag_start_percentage = 
				current->outvalue_to_percentage();
			mwindow->session->drag_start_postime = 
				((FloatAuto*)current)->control_out_pts;
			mwindow->session->drag_origin_x = cursor_x;
			mwindow->session->drag_origin_y = cursor_y;
			mwindow->session->drag_handle = 2;
		}
		result = 1;
	}
	return result;
}

void TrackCanvas::draw_floatline(int center_pixel, 
	FloatAuto *previous,
	FloatAuto *next,
	FloatAutos *autos,
	double view_start,
	double xzoom,
	double yscale,
	int x1,
	int y1,
	int x2,
	int y2,
	int color)
{
// Solve bezier equation for either every pixel or a certain large number of
// points.
	int autogrouptype = autos->autogrouptype;

// Not using slope intercept
	x1 = MAX(0, x1);

	int prev_y;
// Call by reference fails for some reason here
	FloatAuto *previous1 = previous, *next1 = next;
	double automation_min = mwindow->edl->local_session->automation_mins[autogrouptype];
	double automation_max = mwindow->edl->local_session->automation_maxs[autogrouptype];
	double automation_range = automation_max - automation_min;

	for(int x = x1; x < x2; x++)
	{
		ptstime timpos = view_start + (x / xzoom);
		float value = autos->get_value(timpos, previous1, next1);
		AUTOMATIONCLAMPS(value, autogrouptype);

		int y = center_pixel + 
			(int)(((value - automation_min) / automation_range - 0.5) * -yscale);

		if(x > x1 && 
			y >= center_pixel - yscale / 2 && 
			y < center_pixel + yscale / 2 - 1)
		{
			set_color(BLACK);
			draw_line(x - 1, prev_y + 1, x, y + 1);
			set_color(color);
			draw_line(x - 1, prev_y, x, y);
		}
		prev_y = y;
	}
}

void TrackCanvas::synchronize_autos(float change, 
	Track *skip, 
	FloatAuto *fauto, 
	int fill_gangs)
{
// fill mwindow->session->drag_auto_gang
	if (fill_gangs == 1 && skip->gang)
	{
		for(Track *current = mwindow->edl->tracks->first;
			current;
			current = NEXT)
		{
			if(current->data_type == skip->data_type &&
				current->gang && 
				current->record && 
				current != skip)
			{
				FloatAutos *fade_autos = (FloatAutos*)current->automation->autos[AUTOMATION_FADE];
				ptstime position = fauto->pos_time;
				FloatAuto *previous = 0, *next = 0;

				float init_value = fade_autos->get_value(fauto->pos_time, previous, next);
				FloatAuto *keyframe;
				keyframe = (FloatAuto*)fade_autos->get_auto_at_position(position);

				if (!keyframe)
				{
// create keyframe at exactly this point in time
					keyframe = (FloatAuto*)fade_autos->insert_auto(fauto->pos_time);
					keyframe->value = init_value;
				} 
				else
				{ 
// keyframe exists, just change it
					keyframe->value += change;
				}

				keyframe->pos_time = fauto->pos_time;
				keyframe->control_out_pts = fauto->control_out_pts;
				keyframe->control_in_pts = fauto->control_in_pts;
				keyframe->control_out_value = fauto->control_out_value;
				keyframe->control_in_value = fauto->control_in_value;

				mwindow->session->drag_auto_gang->append((Auto *)keyframe);
			}
		}
	} else 
// move the gangs
	if (fill_gangs == 0)
	{
// Move the gang!
		for (int i = 0; i < mwindow->session->drag_auto_gang->total; i++)
		{
			FloatAuto *keyframe = (FloatAuto *)mwindow->session->drag_auto_gang->values[i];

			keyframe->value += change;
			keyframe->pos_time = fauto->pos_time;
			CLAMP(keyframe->value,
				mwindow->edl->local_session->automation_mins[keyframe->autos->autogrouptype],
				mwindow->edl->local_session->automation_maxs[keyframe->autos->autogrouptype]);
			keyframe->control_out_pts = fauto->control_out_pts;
			keyframe->control_in_pts = fauto->control_in_pts;
			keyframe->control_out_value = fauto->control_out_value;
			keyframe->control_in_value = fauto->control_in_value;
		}
	}
	else
// remove the gangs
	if (fill_gangs == -1)
	{
		for (int i = 0; i < mwindow->session->drag_auto_gang->total; i++)
		{
			FloatAuto *keyframe = (FloatAuto *)mwindow->session->drag_auto_gang->values[i];
			keyframe->autos->remove_nonsequential(
					keyframe);
		} 
		mwindow->session->drag_auto_gang->remove_all();
	}
}


int TrackCanvas::test_floatline(int center_pixel, 
	FloatAutos *autos,
	double view_start,
	double xzoom,
	double yscale,
	int x1,
	int x2,
	int cursor_x,
	int cursor_y,
	int buttonpress)
{
	int result = 0;
	int autogrouptype = autos->autogrouptype;

	double automation_min = mwindow->edl->local_session->automation_mins[autogrouptype];
	double automation_max = mwindow->edl->local_session->automation_maxs[autogrouptype];
	double automation_range = automation_max - automation_min;
	ptstime timepos = view_start + (cursor_x / xzoom);
// Call by reference fails for some reason here
	FloatAuto *previous = 0, *next = 0;
	float value = autos->get_value(timepos, previous, next);
	AUTOMATIONCLAMPS(value,autogrouptype);
	int y = center_pixel + 
		(int)(((value - automation_min) / automation_range - 0.5) * -yscale);

	if(cursor_x >= x1 && 
		cursor_x < x2 &&
		cursor_y >= y - HANDLE_W / 2 && 
		cursor_y < y + HANDLE_W / 2 &&
		!ctrl_down())
	{
		result = 1;
		if(buttonpress)
		{
			Auto *current;
			current = mwindow->session->drag_auto = autos->insert_auto(timepos);
			((FloatAuto*)current)->value = value;
			mwindow->session->drag_start_percentage = current->value_to_percentage();
			mwindow->session->drag_start_postime = current->pos_time;
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
				Auto *current;
				ptstime position = cursor_x * mwindow->edl->local_session->zoom_time +
						mwindow->edl->local_session->view_start_pts;
				int new_value = (int)((IntAutos*)autos)->get_automation_constant(position, position);

				current = mwindow->session->drag_auto = autos->insert_auto(position);
				((IntAuto*)current)->value = new_value;
				mwindow->session->drag_start_percentage = current->value_to_percentage();
				mwindow->session->drag_start_postime = current->pos_time;
				mwindow->session->drag_origin_x = cursor_x;
				mwindow->session->drag_origin_y = cursor_y;
			}
		}
	};
	return result;
}

void TrackCanvas::calculate_viewport(Track *track, 
	ptstime &view_start,   // Seconds
	ptstime &view_end,     // Seconds
	double &yscale,
	double &xzoom,
	int &center_pixel)
{
	view_start = mwindow->edl->local_session->view_start_pts;
	view_end = mwindow->edl->local_session->view_start_pts + 
		get_w() * mwindow->edl->local_session->zoom_time;
	yscale = mwindow->edl->local_session->zoom_track;
	xzoom = get_w() / (view_end - view_start);
	center_pixel = (int)(track->y_pixel + yscale / 2) + 
		(mwindow->edl->session->show_titles ? 
			mwindow->theme->get_image("title_bg_data")->get_h() : 
			0);
}

float TrackCanvas::percentage_to_value(float percentage, 
	int is_toggle,
	Auto *reference,
	int autogrouptype)
{
	float result;
	if(is_toggle)
	{
		if(percentage > 0.5) 
			result = 1;
		else
			result = 0;
	}
	else
	{
		float automation_min = mwindow->edl->local_session->automation_mins[autogrouptype];
		float automation_max = mwindow->edl->local_session->automation_maxs[autogrouptype];
		float automation_range = automation_max - automation_min;

		result = percentage * automation_range + automation_min;
		if(reference)
		{
			FloatAuto *ptr = (FloatAuto*)reference;
			result -= ptr->value;
		}
	}
	return result;
}


void TrackCanvas::calculate_auto_position(double *x, 
	double *y,
	double *in_x,
	double *in_y,
	double *out_x,
	double *out_y,
	Auto *current,
	ptstime start,
	double zoom,
	double yscale,
	int autogrouptype)
{
	double automation_min = mwindow->edl->local_session->automation_mins[autogrouptype];
	double automation_max = mwindow->edl->local_session->automation_maxs[autogrouptype];
	double automation_range = automation_max - automation_min;
	FloatAuto *ptr = (FloatAuto*)current;
	*x = (ptr->pos_time - start) * zoom;
	*y = ((ptr->value - automation_min) /
		automation_range - 0.5) * 
		-yscale;
	if(in_x)
	{
		*in_x = (ptr->pos_time + ptr->control_in_pts - start) *
			zoom;
	}
	if(in_y)
	{
		*in_y = (((ptr->value + ptr->control_in_value) -
			automation_min) /
			automation_range - 0.5) *
			-yscale;
	}
	if(out_x)
	{
		*out_x = (double)(ptr->pos_time + 
			ptr->control_out_pts - 
			start) *
			zoom;
	}
	if(out_y)
	{
		*out_y = (((ptr->value + ptr->control_out_value) -
			automation_min) /
			automation_range - 0.5) *
			-yscale;
	}
}


int TrackCanvas::do_float_autos(Track *track, 
		Autos *autos, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		int color,
		Auto* &auto_instance)
{
	int result = 0;

	ptstime view_start;
	ptstime view_end;
	double yscale;
	int center_pixel;
	double xzoom;
	double ax, ay, ax2, ay2;
	double in_x2, in_y2, out_x2, out_y2;
	int draw_auto;
	double slope;
	int skip = 0;
	Auto *current = 0;
	Auto *previous = 0;
	int empty = autos->first == autos->last;
	auto_instance = 0;

	calculate_viewport(track, 
		view_start,
		view_end,
		yscale,
		xzoom,
		center_pixel);

// Get first auto before start
	for(current = autos->last; 
		current && current->pos_time >= view_start;
		current = PREVIOUS);

	if(current)
	{
		calculate_auto_position(&ax, 
			&ay,
			0,
			0,
			0,
			0,
			current,
			view_start,
			xzoom,
			yscale,
			autos->autogrouptype);
		current = NEXT;
	}
	else
	{
		current = autos->first; 
		if(current)
		{
			calculate_auto_position(&ax, 
				&ay,
				0,
				0,
				0,
				0,
				current,
				view_start,
				xzoom,
				yscale,
				autos->autogrouptype);
			ax = 0;
		}
		else
		{
			ax = 0;
			ay = 0;
		}
	}

	do
	{
		skip = 0;
		draw_auto = !empty;

		if(current)
		{
			calculate_auto_position(&ax2, 
				&ay2,
				&in_x2,
				&in_y2,
				&out_x2,
				&out_y2,
				current,
				view_start,
				xzoom,
				yscale,
				autos->autogrouptype);
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
				if (result)
					auto_instance = current;
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

// Draw joining line
		if(!draw)
		{
			if(!result)
			{
				if(track->record && buttonpress != 3)
				{
					result = test_floatline(center_pixel, 
						(FloatAutos*)autos,
						view_start,
						xzoom,
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
				view_start,
				xzoom,
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
		current->pos_time <= view_end && 
		!result);

	if(ax < get_w() && !result)
	{
		ax2 = get_w();
		ay2 = ay;
		if(!draw)
		{
			if(track->record && buttonpress != 3)
			{
				result = test_floatline(center_pixel, 
					(FloatAutos*)autos,
					view_start,
					xzoom,
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
				view_start,
				xzoom,
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
		int color,
		Auto * &auto_instance)
{
	int result = 0;
	ptstime view_start;
	ptstime view_end;
	double yscale;
	int center_pixel;
	double xzoom;
	double ax, ay, ax2, ay2;
	int empty = autos->first == autos->last;

	auto_instance = 0;

	calculate_viewport(track, 
		view_start,
		view_end,
		yscale,
		xzoom,
		center_pixel);

	double high = -yscale * 0.8 / 2;
	double low = yscale * 0.8 / 2;

// Get first auto before start
	Auto *current;
	for(current = autos->last; current && 
		current->pos_time >= view_start; 
		current = PREVIOUS);

	if(current)
	{
		ax = 0;
		ay = ((IntAuto*)current)->value > 0 ? high : low;
		current = NEXT;
	}
	else
	{
		current = autos->first;
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
			ax2 = (double)(current->pos_time - view_start) * xzoom;
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
					if (result)
						auto_instance = current;
				}
			}
			else
			if(!empty)
				draw_auto(current,
					(int)ax2,
					(int)ay2,
					(int)center_pixel,
					(int)yscale,
					color);
			current = NEXT;
		}

		if(!draw)
		{
			if(!result)
			{
				if(track->record && buttonpress != 3)
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
	}while(current && current->pos_time <= view_end && !result);

	if(ax < get_w() && !result)
	{
		ax2 = get_w();
		ay2 = ay;
		if(!draw)
		{
			if(track->record && buttonpress != 3)
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
		BC_Pixmap *pixmap,
		Auto * &auto_instance)
{
	int result = 0;

	ptstime view_start;
	double unit_start;
	ptstime view_end;
	double unit_end;
	double yscale;
	int center_pixel;
	double xzoom;
	Auto *current;

	if(autos->first == autos->last)
		return 0;

	calculate_viewport(track, 
		view_start,
		view_end,
		yscale,
		xzoom,
		center_pixel);

	auto_instance = 0;

	for(current = autos->first; current && !result; current = NEXT)
	{
		if(current->pos_time >= view_start && current->pos_time < view_end)
		{
			int x, y;
			x = (int)((current->pos_time - view_start) *
				xzoom - (pixmap->get_w() / 2 + 0.5));
			y = center_pixel - pixmap->get_h() / 2;
			if(!draw)
			{
				if(cursor_x >= x && cursor_y >= y &&
					cursor_x < x + pixmap->get_w() &&
					cursor_y < y + pixmap->get_h())
				{
					result = 1;
					auto_instance = current;

					if(buttonpress && (buttonpress != 3))
					{
						mwindow->session->drag_auto = current;
						mwindow->session->drag_start_postime = current->pos_time;
						mwindow->session->drag_origin_x = cursor_x;
						mwindow->session->drag_origin_y = cursor_y;

						double position = current->pos_time;
						double center = (mwindow->edl->local_session->get_selectionstart(1) +
							mwindow->edl->local_session->get_selectionend(1)) / 
							2;

						if(!shift_down())
						{
							mwindow->edl->local_session->set_selectionstart(position);
							mwindow->edl->local_session->set_selectionend(position);
						}
						else
						if(position < center)
						{
							mwindow->edl->local_session->set_selectionstart(position);
						}
						else
							mwindow->edl->local_session->set_selectionend(position);
					}
				}
			}
			else
			{
				draw_pixmap(pixmap, x, y);
			}
		}
	}
	return result;
}

// so this means it is always >0 when keyframe is found 
int TrackCanvas::do_plugin_autos(Track *track, 
		int cursor_x, 
		int cursor_y, 
		int draw, 
		int buttonpress,
		Plugin* &keyframe_plugin,
		KeyFrame* &keyframe_instance)
{
	int result = 0;

	double view_start;
	double view_end;
	double yscale;
	int center_pixel;
	double xzoom;

	if(!track->expand_view) return 0;

	calculate_viewport(track, 
		view_start,
		view_end,
		yscale,
		xzoom,
		center_pixel);

	for(int i = 0; i < track->plugin_set.total && !result; i++)
	{
		PluginSet *plugin_set = track->plugin_set.values[i];
		int center_pixel = (int)(track->y_pixel + 
			mwindow->edl->local_session->zoom_track +
			(i + 0.5) * mwindow->theme->get_image("plugin_bg_data")->get_h() + 
			(mwindow->edl->session->show_titles ? mwindow->theme->get_image("title_bg_data")->get_h() : 0));

		for(Plugin *plugin = (Plugin*)plugin_set->first; 
			plugin && !result; 
			plugin = (Plugin*)plugin->next)
		{
			for(KeyFrame *keyframe = (KeyFrame*)plugin->keyframes->first; 
				keyframe && !result; 
				keyframe = (KeyFrame*)keyframe->next)
			{
				if(keyframe->pos_time >= view_start && keyframe->pos_time < view_end)
				{
					int x = (int)((keyframe->pos_time - view_start) * xzoom);
					int y = center_pixel - keyframe_pixmap->get_h() / 2;

					if(!draw)
					{
						if(cursor_x >= x && cursor_y >= y &&
							cursor_x < x + keyframe_pixmap->get_w() &&
							cursor_y < y + keyframe_pixmap->get_h())
						{
							result = 1;
							keyframe_plugin = plugin;
							keyframe_instance = keyframe;

							if(buttonpress)
							{
								mwindow->session->drag_auto = keyframe;
								mwindow->session->drag_start_postime = keyframe->pos_time;
								mwindow->session->drag_origin_x = cursor_x;
								mwindow->session->drag_origin_y = cursor_y;

								double position = keyframe->pos_time;
								double center = (mwindow->edl->local_session->get_selectionstart(1) +
									mwindow->edl->local_session->get_selectionend(1)) / 
									2;

								if(!shift_down())
								{
									mwindow->edl->local_session->set_selectionstart(position);
									mwindow->edl->local_session->set_selectionend(position);
								}
								else
								if(position < center)
								{
									mwindow->edl->local_session->set_selectionstart(position);
								}
								else
									mwindow->edl->local_session->set_selectionend(position);
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
	return result;
}

void TrackCanvas::draw_overlays()
{
	int new_cursor, update_cursor, rerender;

// Move background pixmap to foreground pixmap
	draw_pixmap(background_pixmap, 
		0, 
		0,
		get_w(),
		get_h(),
		0,
		0);

// Transitions
	if(mwindow->edl->session->auto_conf->transitions) draw_transitions();

// Plugins
	draw_plugins();

// Loop points
	draw_loop_points();
	draw_brender_start();

// Highlighted areas
	draw_highlighting();

// Automation
	do_keyframes(0, 
		0, 
		1, 
		0, 
		new_cursor, 
		update_cursor,
		rerender);

// Selection cursor
	if(gui->cursor) gui->cursor->restore(1);

// Handle dragging
	draw_drag_handle();

}

void TrackCanvas::activate()
{
	if(!active)
	{
		get_top_level()->deactivate();
		active = 1;
		set_active_subwindow(this);
		gui->cursor->activate();
	}
}

void TrackCanvas::deactivate()
{
	if(active)
	{
		active = 0;
		gui->cursor->deactivate();
	}
}


void TrackCanvas::update_drag_handle()
{
	ptstime new_position;

	new_position = get_cursor_x() * mwindow->edl->local_session->zoom_time +
		+ mwindow->edl->local_session->view_start_pts;
	new_position = 
		mwindow->edl->align_to_frame(new_position, 0);

	if(new_position != mwindow->session->drag_position)
	{
		mwindow->session->drag_position = new_position;
		gui->mainclock->update(new_position);
	}
}

#define UPDATE_DRAG_HEAD(do_clamp) \
	int result = 0; \
	int x = cursor_x - mwindow->session->drag_origin_x; \
	int y = cursor_y - mwindow->session->drag_origin_y; \
 \
	if(!current->autos->track->record) return 0; \
	ptstime view_start; \
	ptstime view_end; \
	double yscale; \
	int center_pixel; \
	double xzoom; \
 \
	calculate_viewport(current->autos->track,  \
		view_start, \
		view_end, \
		yscale, \
		xzoom, \
		center_pixel); \
 \
	float percentage = (float)(mwindow->session->drag_origin_y - cursor_y) / \
		yscale +  \
		mwindow->session->drag_start_percentage; \
	if(do_clamp) CLAMP(percentage, 0, 1); \
 \
	ptstime postime = ((cursor_x - mwindow->session->drag_origin_x) / xzoom + \
		mwindow->session->drag_start_postime); \
 \
	if((do_clamp) && postime < 0) postime = 0.0;









int TrackCanvas::update_drag_floatauto(int cursor_x, int cursor_y)
{
	FloatAuto *current = (FloatAuto*)mwindow->session->drag_auto;

	UPDATE_DRAG_HEAD(mwindow->session->drag_handle == 0);

	float value;
	float old_value;

	switch(mwindow->session->drag_handle)
	{
// Center
	case 0:
// Snap to nearby values
		old_value = current->value;
		if(shift_down())
		{
			double value1;
			double distance1;
			double value2;
			double distance2;

			if(current->previous)
			{
				int autogrouptype = current->previous->autos->autogrouptype;
				value = percentage_to_value(percentage, 0, 0, autogrouptype);
				value1 = ((FloatAuto*)current->previous)->value;
				distance1 = fabs(value - value1);
				current->value = value1;
			}

			if(current->next)
			{
				int autogrouptype = current->next->autos->autogrouptype;
				value = percentage_to_value(percentage, 0, 0, autogrouptype);
				value2 = ((FloatAuto*)current->next)->value;
				distance2 = fabs(value - value2);
				if(!current->previous || distance2 < distance1)
				{
					current->value = value2;
				}
			}

			if(!current->previous && !current->next)
			{
				current->value = ((FloatAutos*)current->autos)->default_value;
			}
			value = current->value;
		}
		else
		{
			int autogrouptype = current->autos->autogrouptype;
			value = percentage_to_value(percentage, 0, 0, autogrouptype);
		}

		if(current == current->autos->first)
			postime = current->pos_time;
		if(postime > current->autos->track->get_length())
			postime = current->autos->track->get_length();

		if(!EQUIV(value, old_value) || !PTSEQU(postime, current->pos_time))
		{
			result = 1;
			float change = value - old_value;
			current->value = value;
			current->pos_time = postime;
			synchronize_autos(change, current->autos->track, current, 0);

			char string[BCTEXTLEN];
			Units::totext(string, 
				current->pos_time,
				mwindow->edl->session->time_format,
				mwindow->edl->session->sample_rate,
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			gui->show_message("%s, %.2f", string, current->value);
		}
		break;

// In control
	case 1:
		{
			int autogrouptype = current->autos->autogrouptype;
			value = percentage_to_value(percentage, 0, current, autogrouptype);
			postime = MIN(0, postime);

			if(value != current->control_in_value || 
				!PTSEQU(postime, current->control_in_pts))
			{
				result = 1;
				current->control_in_value = value;
				current->control_in_pts = postime;
				synchronize_autos(0, current->autos->track, current, 0);

				char string[BCTEXTLEN];
				Units::totext(string, 
					current->control_in_pts,
					mwindow->edl->session->time_format,
					mwindow->edl->session->sample_rate,
					mwindow->edl->session->frame_rate,
					mwindow->edl->session->frames_per_foot);
				gui->show_message("%s, %.2f", string, current->control_in_value);
			}
		}
		break;

// Out control
	case 2:
		{
			int autogrouptype = current->autos->autogrouptype;
			value = percentage_to_value(percentage, 0, current, autogrouptype);
			postime = MAX(0, postime);
			if(value != current->control_out_value || 
				!PTSEQU(postime, current->control_out_pts))
			{
				result = 1;
				current->control_out_value = value;
				current->control_out_pts = postime;
				synchronize_autos(0, current->autos->track, current, 0);

				char string[BCTEXTLEN];
				Units::totext(string,
					((FloatAuto*)current)->control_out_pts,
					mwindow->edl->session->time_format,
					mwindow->edl->session->sample_rate,
					mwindow->edl->session->frame_rate,
					mwindow->edl->session->frames_per_foot);
				gui->show_message("%s, %.2f", string, 
					((FloatAuto*)current)->control_out_value);
			}
		}
		break;
	}

	return result;
}

int TrackCanvas::update_drag_toggleauto(int cursor_x, int cursor_y)
{
	IntAuto *current = (IntAuto*)mwindow->session->drag_auto;

	UPDATE_DRAG_HEAD(1);
	int value = (int)percentage_to_value(percentage, 1, 0, AUTOGROUPTYPE_INT255);

	if(current == current->autos->first)
		postime = current->pos_time;
	if(postime > current->autos->track->get_length())
		postime = current->autos->track->get_length();

	if(value != current->value || postime != current->pos_time)
	{
		result = 1;
		current->value = value;
		current->pos_time = postime;

		char string[BCTEXTLEN];
		Units::totext(string,
			current->pos_time,
			mwindow->edl->session->time_format,
			mwindow->edl->session->sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
		gui->show_message("%s, %d", string, current->value);
	}

	return result;
}

int TrackCanvas::update_drag_pluginauto(int cursor_x, int cursor_y)
{
	KeyFrame *current = (KeyFrame*)mwindow->session->drag_auto;

	UPDATE_DRAG_HEAD(1)
	if(!PTSEQU(postime, current->pos_time))
	{
		Track *track = current->autos->track;
		PluginSet *pluginset;
		Plugin *plugin;
// figure out the correct pluginset & correct plugin 
		int found = 0;
		for(int i = 0; i < track->plugin_set.total; i++)
		{
			pluginset = track->plugin_set.values[i];
			for(plugin = (Plugin *)pluginset->first; plugin; plugin = (Plugin *)plugin->next)
			{
				KeyFrames *keyframes = plugin->keyframes;
				for(KeyFrame *currentkeyframe = (KeyFrame *)keyframes->first; currentkeyframe; currentkeyframe = (KeyFrame *) currentkeyframe->next)
				{
					if (currentkeyframe == current) 
					{
						found = 1;
						break;
					}
 
				}
				if (found) 
					break;
			}
			if (found) 
				break;
		}

		mwindow->session->plugin_highlighted = plugin;
		mwindow->session->track_highlighted = track;
		result = 1;
		current->pos_time = postime;

		char string[BCTEXTLEN];
		Units::totext(string, 
			current->pos_time,
			mwindow->edl->session->time_format,
			mwindow->edl->session->sample_rate,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
		gui->show_message(string);

		ptstime position_f = current->pos_time;
		double center_f = (mwindow->edl->local_session->get_selectionstart(1) +
			mwindow->edl->local_session->get_selectionend(1)) / 
			2;
		if(!shift_down())
		{
			mwindow->edl->local_session->set_selectionstart(position_f);
			mwindow->edl->local_session->set_selectionend(position_f);
		}
		else
		if(position_f < center_f)
		{
			mwindow->edl->local_session->set_selectionstart(position_f);
		}
		else
			mwindow->edl->local_session->set_selectionend(position_f);
	}
	return result;
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
	result = 0;

// Default cursor
	switch(mwindow->edl->session->editing_mode)
	{
	case EDITING_ARROW:
		new_cursor = ARROW_CURSOR;
		break;
	case EDITING_IBEAM:
		new_cursor = IBEAM_CURSOR;
		break;
	}

	switch(mwindow->session->current_operation)
	{
	case DRAG_EDITHANDLE1:
// Outside threshold.  Upgrade status
		if(abs(get_cursor_x() - mwindow->session->drag_origin_x) > HANDLE_W)
		{
			mwindow->session->current_operation = DRAG_EDITHANDLE2;
			update_overlay = 1;
		}
		break;

	case DRAG_EDITHANDLE2:
		update_drag_handle();
		update_overlay = 1;
		break;

	case DRAG_PLUGINHANDLE1:
		if(abs(get_cursor_x() - mwindow->session->drag_origin_x) > HANDLE_W)
		{
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
	case DRAG_CZOOM:
	case DRAG_PZOOM:
	case DRAG_CAMERA_X:
	case DRAG_CAMERA_Y:
	case DRAG_CAMERA_Z:
	case DRAG_PROJECTOR_X:
	case DRAG_PROJECTOR_Y:
	case DRAG_PROJECTOR_Z:
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
	case DRAG_MASK_PRE:
	case DRAG_MODE_PRE:
	case DRAG_PLUGINKEY_PRE:
		if(abs(get_cursor_x() - mwindow->session->drag_origin_x) > HANDLE_W)
		{
			mwindow->session->current_operation++;
			update_overlay = 1;
		}
		break;

	case DRAG_PAN:
	case DRAG_MASK:
	case DRAG_MODE:
		rerender = update_overlay = 
			update_drag_pluginauto(get_cursor_x(), get_cursor_y());
		break;

	case DRAG_PLUGINKEY:
		rerender = update_overlay = 
			update_drag_pluginauto(get_cursor_x(), get_cursor_y());
		break;

	case SELECT_REGION:
	{
		cursor_x = get_cursor_x();
		cursor_y = get_cursor_y();
		position = cursor_x * mwindow->edl->local_session->zoom_time +
			mwindow->edl->local_session->view_start_pts;

		position = mwindow->edl->align_to_frame(position, 0);
		position = MAX(position, 0);

		if(position < selection_midpoint1)
		{
			mwindow->edl->local_session->set_selectionend(selection_midpoint1);
			mwindow->edl->local_session->set_selectionstart(position);
// Que the CWindow
			mwindow->cwindow->update(1, 0, 0, 0, 1);
// Update the faders
			mwindow->update_plugin_guis();
			gui->patchbay->update();
		}
		else
		{
			mwindow->edl->local_session->set_selectionstart(selection_midpoint1);
			mwindow->edl->local_session->set_selectionend(position);
// Don't que the CWindow
		}

		gui->cursor->hide(0);
		gui->cursor->draw(1);
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
			position = cursor_x * mwindow->edl->local_session->zoom_time +
				mwindow->edl->local_session->view_start_pts;
			position = mwindow->edl->align_to_frame(position, 0);
			update_clock = 1;

// Update cursor
			if(do_transitions(get_cursor_x(), 
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
			if(do_edit_handles(get_cursor_x(), 
				get_cursor_y(), 
				0, 
				rerender,
				update_overlay,
				new_cursor,
				update_cursor))
			{
				break;
			}
			else
// Plugin boundaries
			if(do_plugin_handles(get_cursor_x(), 
				get_cursor_y(), 
				0, 
				rerender,
				update_overlay,
				new_cursor,
				update_cursor))
			{
				break;
			}
			else
			if(do_edits(get_cursor_x(), 
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

	if(update_cursor && new_cursor != get_cursor())
	{
		set_cursor(new_cursor);
	}

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

	return result;
}

void TrackCanvas::start_dragscroll()
{
	if(!drag_scroll)
	{
		drag_scroll = 1;
		set_repeat(BC_WindowBase::get_resources()->scroll_repeat);
	}
}

void TrackCanvas::stop_dragscroll()
{
	if(drag_scroll)
	{
		drag_scroll = 0;
		unset_repeat(BC_WindowBase::get_resources()->scroll_repeat);
	}
}

void TrackCanvas::repeat_event(int duration)
{
	if(!drag_scroll) return;
	if(duration != BC_WindowBase::get_resources()->scroll_repeat) return;

	int sample_movement = 0;
	int track_movement = 0;
	int x_distance = 0;
	int y_distance = 0;
	double position = 0;

	switch(mwindow->session->current_operation)
	{
	case SELECT_REGION:
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
		break;
	}


	if(sample_movement)
	{
		position = (double)(get_cursor_x() + x_distance) *
			mwindow->edl->local_session->zoom_time +
			mwindow->edl->local_session->view_start_pts; 
		position = mwindow->edl->align_to_frame(position, 0);
		position = MAX(position, 0);

		switch(mwindow->session->current_operation)
		{
		case SELECT_REGION:
			if(position < selection_midpoint1)
			{
				mwindow->edl->local_session->set_selectionend(selection_midpoint1);
				mwindow->edl->local_session->set_selectionstart(position);
// Que the CWindow
				mwindow->cwindow->update(1, 0, 0);
// Update the faders
				mwindow->update_plugin_guis();
				gui->patchbay->update();
			}
			else
			{
				mwindow->edl->local_session->set_selectionstart(selection_midpoint1);
				mwindow->edl->local_session->set_selectionend(position);
// Don't que the CWindow
			}
			break;
		}

		mwindow->samplemovement(mwindow->edl->local_session->view_start_pts + 
			x_distance * mwindow->edl->local_session->zoom_time);
	}

	if(track_movement)
	{
		mwindow->trackmovement(mwindow->edl->local_session->track_start +
			y_distance);
	}
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
// delete the drag_auto_gang first and remove out of order keys
		synchronize_autos(0, 0, 0, -1); 
	case DRAG_CZOOM:
	case DRAG_PZOOM:
	case DRAG_PLAY:
	case DRAG_MUTE:
	case DRAG_MASK:
	case DRAG_MODE:
	case DRAG_PAN:
	case DRAG_CAMERA_X:
	case DRAG_CAMERA_Y:
	case DRAG_CAMERA_Z:
	case DRAG_PROJECTOR_X:
	case DRAG_PROJECTOR_Y:
	case DRAG_PROJECTOR_Z:
	case DRAG_PLUGINKEY:
		mwindow->session->current_operation = NO_OPERATION;
		mwindow->session->drag_handle = 0;
// Remove any out-of-order keyframe
		if(mwindow->session->drag_auto)
		{
			mwindow->session->drag_auto->autos->remove_nonsequential(
				mwindow->session->drag_auto);
			update_overlay = 1;
		}
		mwindow->undo->update_undo(_("keyframe"), LOAD_AUTOMATION);
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
			if(mwindow->session->current_operation == SELECT_REGION)
			{
				mwindow->undo->update_undo(_("select"), LOAD_SESSION, 0, 0);
			}

			mwindow->session->current_operation = NO_OPERATION;
			drag_scroll = 0;
		}
		break;
	}
	if (result) 
		cursor_motion_event();
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

int TrackCanvas::do_edit_handles(int cursor_x, 
	int cursor_y, 
	int button_press, 
	int &rerender,
	int &update_overlay,
	int &new_cursor,
	int &update_cursor)
{
	Edit *edit_result = 0;
	int handle_result = 0;
	int result = 0;

	if(!mwindow->edl->session->show_assets) return 0;

	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit && !result;
			edit = edit->next)
		{
			int edit_x, edit_y, edit_w, edit_h;
			edit_dimensions(track, edit->project_pts, edit->end_pts(),
				edit_x, edit_y, edit_w, edit_h);

			if(cursor_x >= edit_x && cursor_x <= edit_x + edit_w &&
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
		ptstime position;
		if(handle_result == 0)
		{
			position = edit_result->project_pts;
			new_cursor = LEFT_CURSOR;
		}
		else
		if(handle_result == 1)
		{
			position = edit_result->end_pts();
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
			rerender = start_selection(position);
			update_overlay = 1;
		}
	}

	return result;
}

int TrackCanvas::do_plugin_handles(int cursor_x, 
	int cursor_y, 
	int button_press,
	int &rerender,
	int &update_overlay,
	int &new_cursor,
	int &update_cursor)
{
	Plugin *plugin_result = 0;
	int handle_result = 0;
	int result = 0;

	if(!mwindow->edl->session->show_assets) return 0;

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
				int plugin_x, plugin_y, plugin_w, plugin_h;
				plugin_dimensions(plugin, plugin_x, plugin_y, plugin_w, plugin_h);

				if(cursor_x >= plugin_x && cursor_x <= plugin_x + plugin_w &&
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

			if(result && shift_down())
				mwindow->session->trim_edits = plugin_set;
		}
	}

	update_cursor = 1;
	if(result)
	{
		ptstime position;
		if(handle_result == 0)
		{
			position = plugin_result->project_pts;
			new_cursor = LEFT_CURSOR;
		}
		else
		if(handle_result == 1)
		{
			position = plugin_result->end_pts();
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

			rerender = start_selection(position);
			update_overlay = 1;
		}
	}
	
	return result;
}


int TrackCanvas::do_tracks(int cursor_x, 
		int cursor_y,
		int button_press)
{
	int result = 0;


	if(!mwindow->edl->session->show_assets) return 0;


	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		int track_x, track_y, track_w, track_h;
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

int TrackCanvas::do_edits(int cursor_x, 
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

	if(!mwindow->edl->session->show_assets) return 0;

	for(Track *track = mwindow->edl->tracks->first;
		track && !result;
		track = track->next)
	{
		for(Edit *edit = track->edits->first;
			edit != track->edits->last && !result;
			edit = edit->next)
		{
			int edit_x, edit_y, edit_w, edit_h;
			edit_dimensions(track, edit->project_pts, edit->end_pts(), 
				edit_x, edit_y, edit_w, edit_h);

// Cursor inside a track
// Cursor inside an edit
			if(cursor_x >= edit_x && cursor_x < edit_x + edit_w &&
				cursor_y >= edit_y && cursor_y < edit_y + edit_h)
			{
// Select duration of edit
				if(button_press)
				{
					if(get_double_click() && !drag_start)
					{
						mwindow->edl->local_session->set_selectionstart(edit->project_pts);
						mwindow->edl->local_session->set_selectionend(edit->end_pts());
						if(mwindow->edl->session->cursor_on_frames) 
						{
							mwindow->edl->local_session->set_selectionstart(
								mwindow->edl->align_to_frame(mwindow->edl->local_session->get_selectionstart(1), 0));
							mwindow->edl->local_session->set_selectionend(
								mwindow->edl->align_to_frame(mwindow->edl->local_session->get_selectionend(1), 1));
						}
						redraw = 1;
						rerender = 1;
						result = 1;
					}
				}
				else
				if(drag_start && track->record)
				{
					if(mwindow->edl->session->editing_mode == EDITING_ARROW)
					{
// Need to create drag window
						mwindow->session->current_operation = DRAG_EDIT;
						mwindow->session->drag_edit = edit;
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
								edit->project_pts,
								edit->track);
						}
						mwindow->session->drag_origin_x = cursor_x;
						mwindow->session->drag_origin_y = cursor_y;
// Where the drag started, so we know relative position inside the edit later
						mwindow->session->drag_position = cursor_x *
							mwindow->edl->local_session->zoom_time +
							mwindow->edl->local_session->view_start_pts;

						drag_popup = new BC_DragWindow(gui, 
							mwindow->theme->get_image("clip_icon"), 
							get_abs_cursor_x(0) - mwindow->theme->get_image("clip_icon")->get_w() / 2,
							get_abs_cursor_y(0) - mwindow->theme->get_image("clip_icon")->get_h() / 2);

						result = 1;
					}
				}
			}
		}
	}
	return result;
}

int TrackCanvas::do_plugins(int cursor_x, 
	int cursor_y, 
	int drag_start,
	int button_press,
	int &redraw,
	int &rerender)
{
	Plugin *plugin = 0;
	int result = 0;
	int done = 0;
	int x, y, w, h;
	Track *track = 0;


	if(!mwindow->edl->session->show_assets) return 0;


	for(track = mwindow->edl->tracks->first;
		track && !done;
		track = track->next)
	{
		if(!track->expand_view) continue;


		for(int i = 0; i < track->plugin_set.total && !done; i++)
		{
			// first check if plugins are visible at all
			if (!track->expand_view)
				continue;
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

	if(plugin)
	{
// Start plugin popup
		if(button_press)
		{
			if(get_buttonpress() == 3)
			{
				gui->plugin_menu->update(plugin);
				gui->plugin_menu->activate_menu();
				result = 1;
			} 
			else
// Select range of plugin on doubleclick over plugin
			if (get_double_click() && !drag_start)
			{
				mwindow->edl->local_session->set_selectionstart(plugin->project_pts);
				mwindow->edl->local_session->set_selectionend(plugin->end_pts());
				if(mwindow->edl->session->cursor_on_frames) 
				{
					mwindow->edl->local_session->set_selectionstart(
						mwindow->edl->align_to_frame(mwindow->edl->local_session->get_selectionstart(1), 0));
					mwindow->edl->local_session->set_selectionend(
						mwindow->edl->align_to_frame(mwindow->edl->local_session->get_selectionend(1), 1));
				}
				rerender = 1;
				redraw = 1;
				result = 1;
			}
		}
		else
// Move plugin
		if(drag_start && plugin->track->record)
		{
			if(mwindow->edl->session->editing_mode == EDITING_ARROW)
			{
				if(plugin->track->data_type == TRACK_AUDIO)
					mwindow->session->current_operation = DRAG_AEFFECT_COPY;
				else
				if(plugin->track->data_type == TRACK_VIDEO)
					mwindow->session->current_operation = DRAG_VEFFECT_COPY;

				mwindow->session->drag_plugin = plugin;


// Create picon
				switch(plugin->plugin_type)
				{
				case PLUGIN_STANDALONE:
				{
					PluginServer *server = mwindow->scan_plugindb(
						plugin->title,
						plugin->track->data_type);
					if (server) 
					{
						VFrame *frame = server->picon;

						drag_popup = new BC_DragWindow(gui, 
							frame, 
							get_abs_cursor_x(0) - frame->get_w() / 2,
							get_abs_cursor_y(0) - frame->get_h() / 2);
					}
					break;
				}

				case PLUGIN_SHAREDPLUGIN:
				case PLUGIN_SHAREDMODULE:
					drag_popup = new BC_DragWindow(gui, 
						mwindow->theme->get_image("clip_icon"), 
						get_abs_cursor_x(0) - mwindow->theme->get_image("clip_icon")->get_w() / 2,
						get_abs_cursor_y(0) - mwindow->theme->get_image("clip_icon")->get_h() / 2);
					break;
				}


				result = 1;
			}
		}
	}

	return result;
}

int TrackCanvas::do_transitions(int cursor_x, 
	int cursor_y, 
	int button_press,
	int &new_cursor,
	int &update_cursor)
{
	Transition *transition = 0;
	int result = 0;
	int x, y, w, h;

	if(!mwindow->edl->session->show_assets ||
		!mwindow->edl->session->auto_conf->transitions) return 0;


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
				edit_dimensions(track, edit->project_pts, edit->end_pts(), 
					x, y, w, h);
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

	cursor_x = get_cursor_x();
	cursor_y = get_cursor_y();
	mwindow->session->trim_edits = 0;

	if(is_event_win() && cursor_inside())
	{
		ptstime position = (double)cursor_x *
			mwindow->edl->local_session->zoom_time +
			mwindow->edl->local_session->view_start_pts;
		if(!active)
		{
			activate();
		}

		if(get_buttonpress() == 1)
		{
			gui->mbuttons->transport->handle_transport(STOP, 1, 0);
		}

		int update_overlay = 0, update_cursor = 0, rerender = 0;

		if(get_buttonpress() == 4)
		{
			if(shift_down())
				mwindow->expand_sample();
			else if(ctrl_down())
				mwindow->move_left(get_w()/ 10);
			else
				mwindow->move_up(get_h() / 10);
			result = 1;
		}
		else
		if(get_buttonpress() == 5)
		{
			if(shift_down())
				mwindow->zoom_in_sample();
			else if(ctrl_down())
				mwindow->move_right(get_w() / 10);
			else
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
				do_transitions(cursor_x, 
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
				get_buttonpress(), 
				new_cursor, 
				update_cursor,
				rerender))
			{
				break;
			}
			else
// Test edit boundaries
			if(do_edit_handles(cursor_x, 
				cursor_y, 
				1, 
				rerender,
				update_overlay,
				new_cursor, 
				update_cursor))
			{
				break;
			}
			else
// Test plugin boundaries
			if(do_plugin_handles(cursor_x, 
				cursor_y, 
				1, 
				rerender,
				update_overlay,
				new_cursor, 
				update_cursor))
			{
				break;
			}
			else
			if(do_edits(cursor_x, cursor_y, 1, 0, update_cursor, rerender, new_cursor, update_cursor))
			{
				break;
			}
			else
			if(do_plugins(cursor_x, cursor_y, 0, 1, update_cursor, rerender))
			{
				break;
			}
			else
			if(do_tracks(cursor_x, cursor_y, 1))
			{
				break;
			}
			break;
		}

// Test handles only and select a region
		case EDITING_IBEAM:
		{
			if(mwindow->edl->session->auto_conf->transitions && 
				do_transitions(cursor_x, 
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
				get_buttonpress(), 
				new_cursor, 
				update_cursor,
				rerender))
			{
				update_overlay = 1;
				break;
			}
			else
// Test edit boundaries
			if(do_edit_handles(cursor_x, 
				cursor_y, 
				1, 
				rerender,
				update_overlay,
				new_cursor, 
				update_cursor))
			{
				break;
			}
			else
// Test plugin boundaries
			if(do_plugin_handles(cursor_x, 
				cursor_y, 
				1, 
				rerender,
				update_overlay,
				new_cursor, 
				update_cursor))
			{
				break;
			}
			else
			if(do_edits(cursor_x, 
				cursor_y, 
				1, 
				0, 
				update_cursor, 
				rerender, 
				new_cursor, 
				update_cursor))
			{
				break;
			}
			else
			if(do_plugins(cursor_x, 
				cursor_y, 
				0, 
				1, 
				update_cursor, 
				rerender))
			{
				break;
			}
			else
			if(do_tracks(cursor_x, cursor_y, 1))
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

SET_TRACE
		if(update_overlay)
		{
			draw_overlays();
			flash();
		}

SET_TRACE
		if(update_cursor)
		{
			gui->timebar->update_highlights();
			gui->cursor->hide();
			gui->cursor->show();
			gui->zoombar->update();
			flash();
			result = 1;
		}

SET_TRACE


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
		double midpoint = (mwindow->edl->local_session->get_selectionstart(1) + 
			mwindow->edl->local_session->get_selectionend(1)) / 2;

		if(position < midpoint)
		{
			mwindow->edl->local_session->set_selectionstart(position);
			selection_midpoint1 = mwindow->edl->local_session->get_selectionend(1);
// Que the CWindow
			rerender = 1;
		}
		else
		{
			mwindow->edl->local_session->set_selectionend(position);
			selection_midpoint1 = mwindow->edl->local_session->get_selectionstart(1);
// Don't que the CWindow for the end
		}
	}
	else
// Start a new selection
	{
		mwindow->edl->local_session->set_selectionstart(position);
		mwindow->edl->local_session->set_selectionend(position);
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


ptstime TrackCanvas::time_visible(void)
{
	return (ptstime)get_w() * 
		mwindow->edl->local_session->zoom_time;
}
