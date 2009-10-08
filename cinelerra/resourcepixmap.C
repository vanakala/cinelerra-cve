
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

#include "aedit.h"
#include "asset.h"
#include "asset.inc"
#include "cache.h"
#include "clip.h"
#include "colormodels.h"
#include "datatype.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "framecache.h"
#include "indexfile.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "resourcethread.h"
#include "resourcepixmap.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "vedit.h"
#include "vframe.h"
#include "wavecache.h"


ResourcePixmap::ResourcePixmap(MWindow *mwindow, 
	TrackCanvas *canvas, 
	Edit *edit, 
	int w, 
	int h)
 : BC_Pixmap(canvas, w, h)
{
	reset();

	this->mwindow = mwindow;
	this->canvas = canvas;
	startsource = edit->startsource;
	data_type = edit->track->data_type;
	source_framerate = edit->asset->frame_rate;
	project_framerate = edit->edl->session->frame_rate;
	source_samplerate = edit->asset->sample_rate;
	project_samplerate = edit->edl->session->sample_rate;
	edit_id = edit->id;
}

ResourcePixmap::~ResourcePixmap()
{
}


void ResourcePixmap::reset()
{
	edit_x = 0;
	pixmap_x = 0;
	pixmap_w = 0;
	pixmap_h = 0;
	zoom_sample = 0;
	zoom_track = 0;
	zoom_y = 0;
	visible = 1;
}
	
void ResourcePixmap::resize(int w, int h)
{
	int new_w = (w > get_w()) ? w : get_w();
	int new_h = (h > get_h()) ? h : get_h();

	BC_Pixmap::resize(new_w, new_h);
}


void ResourcePixmap::draw_data(Edit *edit,
	int64_t edit_x,
	int64_t edit_w, 
	int64_t pixmap_x, 
	int64_t pixmap_w,
	int64_t pixmap_h,
	int mode,
	int indexes_only)
{
// Get new areas to fill in relative to pixmap
// Area to redraw relative to pixmap
	int refresh_x = 0;
	int refresh_w = 0;

// Ignore if called by resourcethread.
	if(mode == 3) return;

	int y = 0;
	if(mwindow->edl->session->show_titles) y += mwindow->theme->get_image("title_bg_data")->get_h();
	Track *track = edit->edits->track;


// If index can't be drawn, don't do anything.
	int need_redraw = 0;
	int64_t index_zoom = 0;
	if(indexes_only)
	{
		IndexFile indexfile(mwindow);
		if(!indexfile.open_index(edit->asset))
		{
			index_zoom = edit->asset->index_zoom;
			indexfile.close_index();
		}

		if(index_zoom)
		{
			if(data_type == TRACK_AUDIO)
			{
				double asset_over_session = (double)edit->asset->sample_rate / 
					mwindow->edl->session->sample_rate;
					asset_over_session;
				if(index_zoom <= mwindow->edl->local_session->zoom_sample *
					asset_over_session)
					need_redraw = 1;
			}
		}

		if(!need_redraw)
			return;
	}


// Redraw everything
	if(edit->startsource != this->startsource ||
/* Incremental drawing is not possible with resource thread */
		(data_type == TRACK_AUDIO /* && 
			edit->asset->sample_rate != source_samplerate*/ ) ||
		(data_type == TRACK_VIDEO /* && 
			!EQUIV(edit->asset->frame_rate, source_framerate) */ ) ||
		mwindow->edl->session->sample_rate != project_samplerate ||
		mwindow->edl->session->frame_rate != project_framerate ||
		mwindow->edl->local_session->zoom_sample != zoom_sample || 
		mwindow->edl->local_session->zoom_track != zoom_track ||
		this->pixmap_h != pixmap_h ||
		(data_type == TRACK_AUDIO && 
			mwindow->edl->local_session->zoom_y != zoom_y) ||
		(mode == 2) ||
		need_redraw)
	{
// Shouldn't draw at all if zoomed in below index zoom.
		refresh_x = 0;
		refresh_w = pixmap_w;
	}
	else
	{
// Start translated right
		if(pixmap_w == this->pixmap_w && edit_x < this->edit_x && edit_w != pixmap_w)
		{
			refresh_w = this->edit_x - edit_x;
			refresh_x = this->pixmap_w - refresh_w;

// Moved completely off the pixmap
			if(refresh_w > this->pixmap_w)
			{
				refresh_w = this->pixmap_w;
				refresh_x = 0;
			}
			else
			{
				copy_area(refresh_w, 
					y, 
					refresh_x, 
					mwindow->edl->local_session->zoom_track, 
					0, 
					y);
			}
		}
 		else
// Start translated left
 		if(pixmap_w == this->pixmap_w && edit_x > this->edit_x && edit_w != pixmap_w)
 		{
			refresh_x = 0;
			refresh_w = edit_x - this->edit_x;

// Moved completely off the pixmap
			if(refresh_w > this->pixmap_w)
			{
				refresh_w = this->pixmap_w;
			}
			else
			{
				copy_area(0, 
					y, 
					this->pixmap_w - refresh_w, 
					mwindow->edl->local_session->zoom_track, 
					refresh_w, 
					y);
			}
 		}
		else
// Start translated right and pixmap came off of right side
		if(pixmap_w < this->pixmap_w && edit_x < this->edit_x && 
			this->edit_x + edit_w > this->pixmap_x + this->pixmap_w)
		{
			refresh_w = (this->edit_x + edit_w) - (this->pixmap_x + this->pixmap_w);
			refresh_x = pixmap_w - refresh_w;
			
			if(refresh_w >= pixmap_w)
			{
				refresh_x = 0;
				refresh_w = pixmap_w;
			}
			else
			{
				copy_area(this->edit_x - edit_x, 
					y, 
					pixmap_w - refresh_w, 
					mwindow->edl->local_session->zoom_track, 
					0, 
					y);
			}
		}
		else
// Start translated right and reduced in size on the right.
		if(pixmap_w < this->pixmap_w && edit_x < this->edit_x)
		{
			refresh_x = 0;
			refresh_w = 0;

			copy_area(this->pixmap_w - pixmap_w, 
				y, 
				pixmap_w, 
				mwindow->edl->local_session->zoom_track, 
				0, 
				y);
		}
		else
// Start translated left and pixmap came off left side
		if(edit_x >= 0 && this->edit_x < 0)
		{
			refresh_x = 0;
			refresh_w = -this->edit_x;

			if(refresh_w > pixmap_w)
			{
				refresh_w = pixmap_w;
			}
			else
			{
				copy_area(0, 
						y, 
						this->pixmap_w, 
						mwindow->edl->local_session->zoom_track, 
						refresh_w, 
						y);
			}
		}
		else
// Start translated left and reduced in size on the right
		if(pixmap_w < this->pixmap_w && edit_x > this->edit_x)
		{
			refresh_x = 0;
			refresh_w = 0;
		}
		else
// Start translated right and left went into left side.
		if(pixmap_w > this->pixmap_w && edit_x < 0 && this->edit_x > 0)
		{
			refresh_w = pixmap_w - (edit_x + this->pixmap_w);
			refresh_x = pixmap_w - refresh_w;

// Moved completely off new pixmap
			if(refresh_w > pixmap_w)
			{
				refresh_w = pixmap_w;
				refresh_x = 0;
			}
			else
			{
				copy_area(-edit_x, 
					y,
					refresh_x,
					mwindow->edl->local_session->zoom_track,
					0,
					y);
			}
		}
		else
// Start translated right and increased in size on the right
		if(pixmap_w > this->pixmap_w && edit_x <= this->edit_x)
		{
			refresh_w = pixmap_w - this->pixmap_w;
			refresh_x = pixmap_w - refresh_w;
		}
		else
// Start translated left and increased in size on the right
		if(pixmap_w > this->pixmap_w && edit_x > this->edit_x)
		{
			refresh_x = 0;
			refresh_w = edit_x - this->edit_x;

// Moved completely off new pixmap
			if(refresh_w > this->pixmap_w)
			{
				refresh_w = pixmap_w;
				refresh_x = 0;
			}
// Shift and insert
			else
			{
				copy_area(0, 
					y,
					this->pixmap_w,
					mwindow->edl->local_session->zoom_track,
					refresh_w,
					y);
			}
		}
	}

// Update pixmap settings
	this->edit_id = edit->id;
	this->startsource = edit->startsource;
	this->source_framerate = edit->asset->frame_rate;
	this->source_samplerate = edit->asset->sample_rate;
	this->project_framerate = edit->edl->session->frame_rate;
	this->project_samplerate = edit->edl->session->sample_rate;
	this->edit_x = edit_x;
	this->pixmap_x = pixmap_x;
	this->pixmap_w = pixmap_w;
	this->pixmap_h = pixmap_h;
	this->zoom_sample = mwindow->edl->local_session->zoom_sample;
	this->zoom_track = mwindow->edl->local_session->zoom_track;
	this->zoom_y = mwindow->edl->local_session->zoom_y;



// Draw in new background
	if(refresh_w > 0)
		mwindow->theme->draw_resource_bg(canvas,
			this, 
			edit_x,
			edit_w,
			pixmap_x,
			refresh_x, 
			y,
			refresh_x + refresh_w,
			mwindow->edl->local_session->zoom_track + y);
//printf("ResourcePixmap::draw_data 70\n");


// Draw media
	if(track->draw)
	{
		switch(track->data_type)
		{
			case TRACK_AUDIO:
				draw_audio_resource(edit, refresh_x, refresh_w);
				break;

			case TRACK_VIDEO:
				draw_video_resource(edit, 
					edit_x, 
					edit_w, 
					pixmap_x,
					pixmap_w,
					refresh_x, 
					refresh_w,
					mode);
				break;
		}
	}

// Draw title
	if(mwindow->edl->session->show_titles)
		draw_title(edit, edit_x, edit_w, pixmap_x, pixmap_w);
}

void ResourcePixmap::draw_title(Edit *edit,
	int64_t edit_x, 
	int64_t edit_w, 
	int64_t pixmap_x, 
	int64_t pixmap_w)
{
// coords relative to pixmap
	int64_t total_x = edit_x - pixmap_x, total_w = edit_w;
	int64_t x = total_x, w = total_w;
	int left_margin = 10;

	if(x < 0) 
	{
		w -= -x;
		x = 0;
	}
	if(w > pixmap_w) w -= w - pixmap_w;

	canvas->draw_3segmenth(x, 
		0, 
		w, 
		total_x,
		total_w,
		mwindow->theme->get_image("title_bg_data"),
		this);

	if(total_x > -BC_INFINITY)
	{
		char title[BCTEXTLEN], channel[BCTEXTLEN];
		FileSystem fs;

		if(edit->user_title[0])
			strcpy(title, edit->user_title);
		else
		{
			fs.extract_name(title, edit->asset->path);

			sprintf(channel, " #%d", edit->channel + 1);
			strcat(title, channel);
		}

		canvas->set_color(mwindow->theme->title_color);
		canvas->set_font(mwindow->theme->title_font);
//printf("ResourcePixmap::draw_title 1 %d\n", total_x + 10);
		
// Justify the text on the left boundary of the edit if it is visible.
// Otherwise justify it on the left side of the screen.
		int text_x = total_x + left_margin;
		text_x = MAX(left_margin, text_x);
		canvas->draw_text(text_x, 
			canvas->get_text_ascent(MEDIUMFONT_3D) + 2, 
			title,
			strlen(title),
			this);
	}
}


// Need to draw one more x
void ResourcePixmap::draw_audio_resource(Edit *edit, int x, int w)
{
	if(w <= 0) return;
	double asset_over_session = (double)edit->asset->sample_rate / 
		mwindow->edl->session->sample_rate;

// Develop strategy for drawing
	switch(edit->asset->index_status)
	{
		case INDEX_NOTTESTED:
			return;
			break;
// Disabled.  All files have an index.
//		case INDEX_TOOSMALL:
//			draw_audio_source(edit, x, w);
//			break;
		case INDEX_BUILDING:
		case INDEX_READY:
		{
			IndexFile indexfile(mwindow);
			if(!indexfile.open_index(edit->asset))
			{
				if(edit->asset->index_zoom > 
						mwindow->edl->local_session->zoom_sample * 
						asset_over_session)
				{
					draw_audio_source(edit, x, w);
				}
				else
					indexfile.draw_index(this, edit, x, w);
				indexfile.close_index();
			}
			break;
		}
	}
}


















void ResourcePixmap::draw_audio_source(Edit *edit, int x, int w)
{
	File *source = mwindow->audio_cache->check_out(edit->asset,
		mwindow->edl);

	if(!source)
	{
		printf(_("ResourcePixmap::draw_audio_source: failed to check out %s for drawing.\n"), edit->asset->path);
		return;
	}

	w++;
	double asset_over_session = (double)edit->asset->sample_rate / 
		mwindow->edl->session->sample_rate;
	int source_len = w * mwindow->edl->local_session->zoom_sample;
	int center_pixel = mwindow->edl->local_session->zoom_track / 2;
	if(mwindow->edl->session->show_titles) center_pixel += mwindow->theme->get_image("title_bg_data")->get_h();

// Single sample zoom
	if(mwindow->edl->local_session->zoom_sample == 1)
	{
		int64_t source_start = (int64_t)(((pixmap_x - edit_x + x) * 
			mwindow->edl->local_session->zoom_sample + edit->startsource) *
			asset_over_session);
		double oldsample, newsample;
		int total_source_samples = (int)((double)(source_len + 1) * 
			asset_over_session);
		double *buffer = new double[total_source_samples];

		source->set_audio_position(source_start, 
			edit->asset->sample_rate);
		source->set_channel(edit->channel);
		canvas->set_color(mwindow->theme->audio_color);

		if(!source->read_samples(buffer, 
			total_source_samples, 
			edit->asset->sample_rate))
		{
			oldsample = newsample = *buffer;
			for(int x1 = x, x2 = x + w, i = 0; 
				x1 < x2; 
				x1++, i++)
			{
				oldsample = newsample;
				newsample = buffer[(int)(i * asset_over_session)];
				canvas->draw_line(x1 - 1, 
					(int)(center_pixel - oldsample * mwindow->edl->local_session->zoom_y / 2),
					x1,
					(int)(center_pixel - newsample * mwindow->edl->local_session->zoom_y / 2),
					this);
			}
		}

		delete [] buffer;
		canvas->test_timer();
	}
	else
// Multiple sample zoom
	{
		int first_pixel = 1;
		int prev_y1 = -1;
		int prev_y2 = -1;
		int y1;
		int y2;

		canvas->set_color(mwindow->theme->audio_color);
// Draw each pixel from the cache
		while(x < w)
		{
// Starting sample of pixel relative to asset rate.
			int64_t source_start = (int64_t)(((pixmap_x - edit_x + x) * 
				mwindow->edl->local_session->zoom_sample + edit->startsource) *
				asset_over_session);
			int64_t source_end = (int64_t)(((pixmap_x - edit_x + x + 1) * 
				mwindow->edl->local_session->zoom_sample + edit->startsource) *
				asset_over_session);
			WaveCacheItem *item = mwindow->wave_cache->get_wave(edit->asset->id,
					edit->channel,
					source_start,
					source_end);
			if(item)
			{
				y1 = (int)(center_pixel - 
					item->low * mwindow->edl->local_session->zoom_y / 2);
				y2 = (int)(center_pixel - 
					item->high * mwindow->edl->local_session->zoom_y / 2);
				if(first_pixel)
				{
					canvas->draw_line(x, 
						y1,
						x,
						y2,
						this);
					first_pixel = 0;
				}
				else
					canvas->draw_line(x, 
						MIN(y1, prev_y2),
						x,
						MAX(y2, prev_y1),
						this);
				prev_y1 = y1;
				prev_y2 = y2;
				first_pixel = 0;
				mwindow->wave_cache->unlock();
			}
			else
			{
				first_pixel = 1;
				canvas->resource_thread->add_wave(this,
					edit->asset,
					x,
					edit->channel,
					source_start,
					source_end);
			}

			x++;
		}
	}

	mwindow->audio_cache->check_in(edit->asset);
}



void ResourcePixmap::draw_wave(int x, double high, double low)
{
	int top_pixel = 0;
	if(mwindow->edl->session->show_titles) 
		top_pixel = mwindow->theme->get_image("title_bg_data")->get_h();
	int center_pixel = mwindow->edl->local_session->zoom_track / 2 + top_pixel;
	int bottom_pixel = top_pixel + mwindow->edl->local_session->zoom_track;
	int y1 = (int)(center_pixel - 
		low * mwindow->edl->local_session->zoom_y / 2);
	int y2 = (int)(center_pixel - 
		high * mwindow->edl->local_session->zoom_y / 2);
	CLAMP(y1, top_pixel, bottom_pixel);
	CLAMP(y2, top_pixel, bottom_pixel);
	canvas->set_color(mwindow->theme->audio_color);
	canvas->draw_line(x, 
		y1,
		x,
		y2,
		this);
}




















void ResourcePixmap::draw_video_resource(Edit *edit, 
	int64_t edit_x, 
	int64_t edit_w, 
	int64_t pixmap_x,
	int64_t pixmap_w,
	int refresh_x, 
	int refresh_w,
	int mode)
{
// pixels spanned by a picon
	int64_t picon_w = Units::round(edit->picon_w());
	int64_t picon_h = edit->picon_h();


// Don't draw video if picon is bigger than edit
	if(picon_w > edit_w) return;

// pixels spanned by a frame
	double frame_w = edit->frame_w();

// Frames spanned by a picon
	double frames_per_picon = edit->frames_per_picon();

// Current pixel relative to pixmap
	int x = 0;
	int y = 0;
	if(mwindow->edl->session->show_titles) 
		y += mwindow->theme->get_image("title_bg_data")->get_h();
// Frame in project touched by current pixel
	int64_t project_frame;

// Get first frame touched by x and fix x to start of frame
	if(frames_per_picon > 1)
	{
		int picon = Units::to_int64(
			(double)((int64_t)refresh_x + pixmap_x - edit_x) / 
			picon_w);
		x = picon_w * picon + edit_x - pixmap_x;
		project_frame = Units::to_int64((double)picon * frames_per_picon);
	}
	else
	{
		project_frame = Units::to_int64((double)((int64_t)refresh_x + pixmap_x - edit_x) / 
			frame_w);
		x = Units::round((double)project_frame * frame_w + edit_x - pixmap_x);
 	}


// Draw only cached frames
	while(x < refresh_x + refresh_w)
	{
		int64_t source_frame = project_frame + edit->startsource;
		VFrame *picon_frame = 0;
		int use_cache = 0;

		if((picon_frame = mwindow->frame_cache->get_frame_ptr(source_frame,
			edit->channel,
			mwindow->edl->session->frame_rate,
			BC_RGB888,
			picon_w,
			picon_h,
			edit->asset->id)) != 0)
		{
			use_cache = 1;
		}
		else
		{
// Set picon thread to display from file
			if(mode != 3)
			{

				canvas->resource_thread->add_picon(this, 
					x, 
					y, 
					picon_w,
					picon_h,
					mwindow->edl->session->frame_rate,
					source_frame,
					edit->channel,
					edit->asset);
			}
		}

		if(picon_frame)
			draw_vframe(picon_frame, 
				x, 
				y, 
				picon_w, 
				picon_h, 
				0, 
				0);


// Unlock the get_frame_ptr command
		if(use_cache)
			mwindow->frame_cache->unlock();
		
		if(frames_per_picon > 1)
		{
			x += Units::round(picon_w);
			project_frame = Units::to_int64(frames_per_picon * (int64_t)((double)(x + pixmap_x - edit_x) / picon_w));
		}
		else
		{
			x += Units::round(frame_w);
			project_frame = (int64_t)((double)(x + pixmap_x - edit_x) / frame_w);
		}


		canvas->test_timer();
	}
}


void ResourcePixmap::dump()
{
	printf("ResourcePixmap %p\n", this);
	printf(" edit %x edit_x %d pixmap_x %d pixmap_w %d visible %d\n", edit_id, edit_x, pixmap_x, pixmap_w, visible);
}



