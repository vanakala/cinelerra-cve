// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "asset.h"
#include "bcsignals.h"
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
#include "mainerror.h"
#include "resourcethread.h"
#include "resourcepixmap.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "vframe.h"
#include "wavecache.h"


ResourcePixmap::ResourcePixmap(ResourceThread *resource_thread,
	TrackCanvas *canvas, Edit *edit, int w, int h)
 : BC_Pixmap(canvas, w, h)
{
	edit_x = 0;
	pixmap_x = 0;
	pixmap_w = 0;
	pixmap_h = 0;
	zoom_track = 0;
	zoom_y = 0;
	visible = 1;
	zoom_time = 0;
	aframe = 0;
	this->resource_thread = resource_thread;
	this->canvas = canvas;
	source_pts = edit->get_source_pts();
	data_type = edit->track->data_type;
	edit_id = 0;
	last_picon_size = -1;
	num_picons = 0;
}

ResourcePixmap::~ResourcePixmap()
{
	delete aframe;
}

void ResourcePixmap::resize(int w, int h)
{
	int new_w = (w > get_w()) ? w : get_w();
	int new_h = (h > get_h()) ? h : get_h();

	BC_Pixmap::resize(new_w, new_h);
}

void ResourcePixmap::draw_data(Edit *edit,
	int edit_x, int edit_w,
	int pixmap_x, int pixmap_w, int pixmap_h,
	int mode)
{
// Get new areas to fill in relative to pixmap
// Area to redraw relative to pixmap
	int refresh_x = 0;
	int refresh_w = 0;
	int y = 0;
	Track *track = edit->edits->track;

	if(edlsession->show_titles)
		y += theme_global->get_image("title_bg_data")->get_h();

// If index can't be drawn, don't do anything.
	if(mode & WUPD_INDEXES)
	{
		int need_redraw = 0;

		if(int index_zoom = edit->asset->indexfiles[edit->stream].zoom)
		{
			if(data_type == TRACK_AUDIO)
			{
				double asset_over_session =
					(double)edit->asset->sample_rate /
					edlsession->sample_rate;
				if(index_zoom > edlsession->sample_rate *
						master_edl->local_session->zoom_time *
						asset_over_session)
					return;
			}
		}
	}

// Redraw everything
	if(!PTSEQU(edit->get_source_pts(), this->source_pts) ||
		!PTSEQU(master_edl->local_session->zoom_time, zoom_time) ||
		master_edl->local_session->zoom_track != zoom_track ||
		this->pixmap_h != pixmap_h ||
		(data_type == TRACK_AUDIO &&
			master_edl->local_session->zoom_y != zoom_y) ||
		(mode & (WUPD_CANVREDRAW | WUPD_INDEXES)))
	{
// Shouldn't draw at all if zoomed in below index zoom.
		refresh_x = 0;
		refresh_w = pixmap_w;
	}
	else
	{
// Start translated right
		if(pixmap_w == this->pixmap_w &&
			edit_x < this->edit_x && edit_w != pixmap_w)
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
				copy_area(refresh_w, y, refresh_x,
					master_edl->local_session->zoom_track,
					0, y);
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
				copy_area(0, y, this->pixmap_w - refresh_w,
					master_edl->local_session->zoom_track,
					refresh_w, y);
			}
		}
		else
// Start translated right and pixmap came off of right side
		if(pixmap_w < this->pixmap_w && edit_x < this->edit_x && 
			this->edit_x + edit_w > this->pixmap_x + this->pixmap_w)
		{
			refresh_w = (this->edit_x + edit_w) -
				(this->pixmap_x + this->pixmap_w);
			refresh_x = pixmap_w - refresh_w;

			if(refresh_w >= pixmap_w)
			{
				refresh_x = 0;
				refresh_w = pixmap_w;
			}
			else
			{
				copy_area(this->edit_x - edit_x, y,
					pixmap_w - refresh_w, 
					master_edl->local_session->zoom_track,
					0, y);
			}
		}
		else
// Start translated right and reduced in size on the right.
		if(pixmap_w < this->pixmap_w && edit_x < this->edit_x)
		{
			refresh_x = 0;
			refresh_w = 0;
			copy_area(this->pixmap_w - pixmap_w, y,
				pixmap_w,
				master_edl->local_session->zoom_track,
				0, y);
		}
		else
// Start translated left and pixmap came off left side
		if(edit_x >= 0 && this->edit_x < 0)
		{
			refresh_x = 0;
			refresh_w = -this->edit_x;

			if(refresh_w > pixmap_w)
				refresh_w = pixmap_w;
			else
				copy_area(0, y,
					this->pixmap_w,
					master_edl->local_session->zoom_track,
					refresh_w, y);
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
				copy_area(-edit_x, y,
					refresh_x,
					master_edl->local_session->zoom_track,
					0, y);
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
				copy_area(0, y,
					this->pixmap_w,
					master_edl->local_session->zoom_track,
					refresh_w, y);
		}
	}

// Update pixmap settings
	this->edit_id = edit->id;
	this->source_pts = edit->get_source_pts();
	this->edit_x = edit_x;
	this->pixmap_x = pixmap_x;
	this->pixmap_w = pixmap_w;
	this->pixmap_h = pixmap_h;
	this->zoom_time = master_edl->local_session->zoom_time;
	this->zoom_track = master_edl->local_session->zoom_track;
	this->zoom_y = master_edl->local_session->zoom_y;

// Draw in new background
	if(refresh_w > 0)
	{
		theme_global->draw_resource_bg(canvas, this,
			edit_x, edit_w,
			pixmap_x,
			refresh_x, y,
			refresh_x + refresh_w,
			master_edl->local_session->zoom_track + y);
	}

// Draw media
	if(track->draw && edlsession->show_assets)
	{
		switch(data_type)
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
				refresh_w);
			break;
		}
	}

// Draw title
	if(edlsession->show_titles)
		draw_title(edit, edit_x, edit_w, pixmap_x, pixmap_w);
}

void ResourcePixmap::draw_title(Edit *edit,
	int edit_x,
	int edit_w,
	int pixmap_x,
	int pixmap_w)
{
// coords relative to pixmap
	int total_x = edit_x - pixmap_x, total_w = edit_w;
	int x = total_x, w = total_w;
	int left_margin = 10;
	int stream_type = edit->asset->stream_type(data_type);

	if(x < 0) 
	{
		w -= -x;
		x = 0;
	}
	if(w > pixmap_w)
		w -= w - pixmap_w;

	canvas->draw_3segmenth(x, 
		0, 
		w, 
		total_x,
		total_w,
		theme_global->get_image("title_bg_data"),
		this);

	if(total_x > -BC_INFINITY)
	{
		char title[BCTEXTLEN];
		FileSystem fs;

		fs.extract_name(title, edit->asset->path);
		char *s = &title[strlen(title)];

		if(edit->asset->stream_count(stream_type) > 1)
			sprintf(s, " %d:%d", edit->stream + 1, edit->channel + 1);
		else
			sprintf(s, " #%d", edit->channel + 1);

		canvas->set_color(theme_global->title_color);
		canvas->set_font(theme_global->title_font);

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
	Asset *asset = edit->asset;
	int stream = edit->stream;

	if(w <= 0)
		return;

	double asset_over_session = (double)asset->streams[stream].sample_rate /
		edlsession->sample_rate;

// Develop strategy for drawing
	switch(asset->indexfiles[stream].status)
	{
	case INDEX_NOTTESTED:
		return;

	case INDEX_BUILDING:
	case INDEX_READY:
		asset->indexfiles[stream].open_index(asset, stream);

		if(asset->indexfiles[stream].zoom >
				master_edl->local_session->zoom_time *
				edlsession->sample_rate * asset_over_session)
			draw_audio_source(edit, x, w);
		else
			asset->indexfiles[stream].draw_index(this, edit, x, w);
		break;
	}
}

void ResourcePixmap::draw_audio_source(Edit *edit, int x, int w)
{
	File *source = resource_thread->audio_cache->check_out(edit->asset, edit->stream);

	if(!source)
	{
		errorbox(_("Failed to check out %s for drawing."), edit->asset->path);
		return;
	}

	w++;
	double asset_over_session = (double)edit->asset->sample_rate / 
		edlsession->sample_rate;
	int source_len = round(w * master_edl->local_session->zoom_time *
		edlsession->sample_rate);
	int center_pixel = master_edl->local_session->zoom_track / 2;
	if(edlsession->show_titles)
		center_pixel += theme_global->get_image("title_bg_data")->get_h();

// Single sample zoom
	if(round(master_edl->local_session->zoom_time *
		edlsession->sample_rate) == 1)
	{
		ptstime src_pts = (pixmap_x - edit_x + x)
				* master_edl->local_session->zoom_time
				+ edit->get_source_pts();
		// 1.6 should be enough to compensate rounding above
		ptstime len_pts = w * master_edl->local_session->zoom_time * 1.6;
		int total_source_samples = round(len_pts * edit->asset->sample_rate);

		samplenum source_start = (((pixmap_x - edit_x + x) *
			round(master_edl->local_session->zoom_time *
				edlsession->sample_rate) +
			edit->track->to_units(edit->get_source_pts())) *
			asset_over_session);

		if(!aframe)
			aframe = new AFrame(total_source_samples);
		else
			aframe->check_buffer(total_source_samples);

		aframe->set_samplerate(edit->asset->sample_rate);
		aframe->channel = edit->channel;
		aframe->set_fill_request(source_start, total_source_samples);

		canvas->set_color(theme_global->audio_color);

		if(!source->get_samples(aframe))
		{
			double oldsample, newsample;

			oldsample = newsample = *aframe->buffer;
			for(int x1 = x, x2 = x + w, i = 0; x1 < x2; x1++, i++)
			{
				oldsample = newsample;
				newsample = aframe->buffer[(int)round(i *
						asset_over_session)];
				canvas->draw_line(x1 - 1,
					round(center_pixel - oldsample *
						master_edl->local_session->zoom_y / 2),
					x1,
					round(center_pixel - newsample *
						master_edl->local_session->zoom_y / 2),
					this);
			}
		}

		canvas->check_hourglass();
	}
	else
// Multiple sample zoom
	{
		int first_pixel = 1;
		int prev_y1 = -1;
		int prev_y2 = -1;
		int y1;
		int y2;

		canvas->set_color(theme_global->audio_color);
// Draw each pixel from the cache
		while(x < w)
		{
// Starting sample of pixel relative to asset rate.
			samplenum source_start = (((pixmap_x - edit_x + x) * 
				round(master_edl->local_session->zoom_time *
					edlsession->sample_rate) +
				edit->track->to_units(edit->get_source_pts())) *
				asset_over_session);
			samplenum source_end = (((pixmap_x - edit_x + x + 1) * 
				round(master_edl->local_session->zoom_time *
					edlsession->sample_rate) +
				edit->track->to_units(edit->get_source_pts())) *
				asset_over_session);
			WaveCacheItem *item = resource_thread->wave_cache->get_wave(
					edit->asset, edit->channel,
					source_start, source_end);
			if(item)
			{
				y1 = round(center_pixel -
					item->low * master_edl->local_session->zoom_y / 2);
				y2 = round(center_pixel -
					item->high * master_edl->local_session->zoom_y / 2);
				if(first_pixel)
				{
					canvas->draw_line(x, y1, x, y2, this);
					first_pixel = 0;
				}
				else
					canvas->draw_line(x, MIN(y1, prev_y2),
						x, MAX(y2, prev_y1), this);

				prev_y1 = y1;
				prev_y2 = y2;
				first_pixel = 0;
				resource_thread->wave_cache->unlock();
			}
			else
			{
				first_pixel = 1;
				resource_thread->add_wave(this,
					edit->asset,
					edit->stream,
					x,
					edit->channel,
					source_start,
					source_end);
			}

			x++;
		}
	}
	resource_thread->audio_cache->check_in(edit->asset, edit->stream);
}

void ResourcePixmap::draw_wave(int x, double high, double low)
{
	int top_pixel = 0;

	if(edlsession->show_titles)
		top_pixel = theme_global->get_image("title_bg_data")->get_h();
	int center_pixel = master_edl->local_session->zoom_track / 2 + top_pixel;
	int bottom_pixel = top_pixel + master_edl->local_session->zoom_track;
	int y1 = round(center_pixel -
		low * master_edl->local_session->zoom_y / 2);
	int y2 = round(center_pixel -
		high * master_edl->local_session->zoom_y / 2);

	CLAMP(y1, top_pixel, bottom_pixel);
	CLAMP(y2, top_pixel, bottom_pixel);
	canvas->set_color(theme_global->audio_color);
	canvas->draw_line(x, y1, x, y2, this);
}

void ResourcePixmap::draw_video_resource(Edit *edit, 
	int edit_x,
	int edit_w,
	int pixmap_x,
	int pixmap_w,
	int refresh_x,
	int refresh_w)
{
// pixels spanned by a picon
	int picon_w = round(edit->picon_w());
	int picon_h = edit->picon_h();
// Current pixel relative to pixmap
	int x;
	int y = 0;
	int refresh_end;
// Number of picons to redraw
	double picons, picount;
// Start and duration of a picon
	ptstime picon_src, picon_len;

// Don't draw video if edit is tiny
	if(edit_w < 2)
		return;

	if(edlsession->show_titles)
		y += theme_global->get_image("title_bg_data")->get_h();

	picons = (pixmap_x + refresh_x - edit_x) / edit->picon_w();
	picount = floor(picons);
	x = -round((picons - picount) * edit->picon_w()) + refresh_x;
	picon_len = edit->picon_w() * zoom_time;
	picon_src = picount * picon_len;
// Draw only cached frames
	refresh_end = refresh_x + refresh_w;

	if(!PTSEQU(last_picon_size, picon_len))
	{
		resource_thread->frame_cache->change_duration(picon_len,
			edit->channel, BC_RGB888, picon_w, picon_h,
			edit->asset, edit->stream);
		last_picon_size = picon_len;
	}
	num_picons = pixmap_w / picon_w + 1;

	while(x < refresh_end)
	{
		ptstime source_pts = edit->get_source_pts() + picon_src;
		VFrame *picon_frame;

		if(picon_frame = resource_thread->frame_cache->get_frame_ptr(source_pts,
			edit->channel, BC_RGB888, picon_w, picon_h,
			edit->asset, edit->stream))
		{
			draw_vframe(picon_frame, x, y, picon_w, picon_h,
				0, 0);
			resource_thread->frame_cache->unlock();
		}
		else
		{
// Set picon thread to display from file
			canvas->resource_thread->add_picon(this,
				x, y, picon_w, picon_h,
				source_pts, picon_len,
				edit->channel,
				edit->asset, edit->stream);
		}
		picon_src += picon_len;
		x += picon_w;
		canvas->check_hourglass();
	}
}

void ResourcePixmap::dump(int indent)
{
	const char *type;

	switch(data_type)
	{
	case TRACK_AUDIO:
		type = "Audio";
		break;

	case TRACK_VIDEO:
		type = "Video";
		break;

	default:
		type = "Unknown";
		break;
	}

	printf("%*sResourcePixmap %p (%s) dump:\n", indent, "", this, type);
	indent += 2;
	printf("%*sedit_id %d edit_x %d pixmap_x %d [%d,%d] visible %d\n",
		indent, "", edit_id, edit_x, pixmap_x, pixmap_w, pixmap_h, visible);
	if(data_type == TRACK_VIDEO)
		printf("%*spicon_size %.2f num_picons %d\n", indent, "",
			last_picon_size, num_picons);
}
