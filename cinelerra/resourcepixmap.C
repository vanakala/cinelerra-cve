#include "aedit.h"
#include "assets.h"
#include "assets.inc"
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
#include "indexfile.h"
#include "localsession.h"
#include "mwindow.h"
#include "resourcepixmap.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "vedit.h"
#include "vframe.h"


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
	long edit_x,
	long edit_w, 
	long pixmap_x, 
	long pixmap_w,
	long pixmap_h,
	int force,
	int indexes_only)
{
// Get new areas to fill in relative to pixmap
// Area to redraw relative to pixmap
	int refresh_x = 0, refresh_w = 0;
	long index_zoom = 0;

//printf("ResourcePixmap::draw_data 1\n");
	if(indexes_only)
	{
		IndexFile indexfile(mwindow);
		if(!indexfile.open_index(edit->asset))
		{
			index_zoom = edit->asset->index_zoom;
			indexfile.close_index();
		}
	}

//printf("ResourcePixmap::draw_data 1\n");
// printf("ResourcePixmap::draw_data 1 edit_x=%d edit_w=%d pixmap_x=%d pixmap_w=%d	pixmap_h=%d\n",
// edit_x, edit_w, pixmap_x, pixmap_w, pixmap_h);
	int y = 0;
	if(mwindow->edl->session->show_titles) y += mwindow->theme->title_bg_data->get_h();
	Track *track = edit->edits->track;

//printf("ResourcePixmap::draw_data 1\n");


// Redraw everything
	if(edit->startsource != this->startsource ||
		(data_type == TRACK_AUDIO && 
			edit->asset->sample_rate != source_samplerate) ||
		(data_type == TRACK_VIDEO && 
			!EQUIV(edit->asset->frame_rate, source_framerate)) ||
		mwindow->edl->session->sample_rate != project_samplerate ||
		mwindow->edl->session->frame_rate != project_framerate ||
		mwindow->edl->local_session->zoom_sample != zoom_sample || 
		mwindow->edl->local_session->zoom_track != zoom_track ||
		this->pixmap_h != pixmap_h ||
		(data_type == TRACK_AUDIO && 
			mwindow->edl->local_session->zoom_y != zoom_y) ||
		force ||
		(indexes_only && mwindow->edl->local_session->zoom_sample >= index_zoom))
	{
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

//printf("ResourcePixmap::draw_data 1\n");
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
//printf("ResourcePixmap::draw_data 2 refresh_x1=%d refresh_w1=%d refresh_x2=%d refresh_w2=%d\n", 
// 	refresh_x1, refresh_w1, refresh_x2, refresh_w2);


//printf("ResourcePixmap::draw_data 2\n");

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


//printf("ResourcePixmap::draw_data 3\n");
// Draw media
	if(track->draw)
	{
		switch(track->data_type)
		{
			case TRACK_AUDIO:
//printf("ResourcePixmap::draw_data 4\n");
				draw_audio_resource(edit, refresh_x, refresh_w);
//printf("ResourcePixmap::draw_data 5\n");
				break;

			case TRACK_VIDEO:
//printf("ResourcePixmap::draw_data 6\n");
				draw_video_resource(edit, 
					edit_x, 
					edit_w, 
					pixmap_x,
					pixmap_w,
					refresh_x, 
					refresh_w);
//printf("ResourcePixmap::draw_data 7\n");
				break;
		}
	}

// Draw title
	if(mwindow->edl->session->show_titles)
		draw_title(edit, edit_x, edit_w, pixmap_x, pixmap_w);

//printf("ResourcePixmap::draw_data 3 refresh_x=%d refresh_w=%d\n", 
//refresh_x, 
//refresh_w);
//printf("ResourcePixmap::draw_data 10\n");
}

void ResourcePixmap::draw_title(Edit *edit,
	long edit_x, 
	long edit_w, 
	long pixmap_x, 
	long pixmap_w)
{
// coords relative to pixmap
	long total_x = edit_x - pixmap_x, total_w = edit_w;
	long x = total_x, w = total_w;

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
		mwindow->theme->title_bg_data,
		this);

	if(total_x > -BC_INFINITY)
	{
		char title[BCTEXTLEN], channel[BCTEXTLEN];
		FileSystem fs;
		fs.extract_name(title, edit->asset->path);

		sprintf(channel, " #%d", edit->channel + 1);
		strcat(title, channel);

		canvas->set_color(mwindow->theme->title_color);
		canvas->set_font(mwindow->theme->title_font);
		canvas->draw_text(total_x + 10, 
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
//printf("ResourcePixmap::draw_audio_resource 1 %s %d\n", edit->asset->path, edit->asset->index_status);
	switch(edit->asset->index_status)
	{
		case INDEX_NOTTESTED:
			return;
			break;
// Disabled.  Always draw from index.
		case INDEX_TOOSMALL:
			draw_audio_source(edit, x, w);
			break;
		case INDEX_BUILDING:
		case INDEX_READY:
		{
//printf("ResourcePixmap::draw_audio_resource 1 status=%d index_zoom=%d zoom_sample=%d\n", 
//edit->asset->index_status,
//edit->asset->index_zoom,
//mwindow->edl->local_session->zoom_sample);
			IndexFile indexfile(mwindow);
			if(!indexfile.open_index(edit->asset))
			{
//printf("ResourcePixmap::draw_audio_resource 2\n");
				if(edit->asset->index_zoom > 
					mwindow->edl->local_session->zoom_sample * 
					asset_over_session)
					draw_audio_source(edit, x, w);
				else
					indexfile.draw_index(this, edit, x, w);
				indexfile.close_index();
			}
//printf("ResourcePixmap::draw_audio_resource 3 %d\n", edit->asset->index_status);
			break;
		}
	}
//printf("ResourcePixmap::draw_audio_resource 3\n");
}


















void ResourcePixmap::draw_audio_source(Edit *edit, int x, int w)
{
	File *source = mwindow->audio_cache->check_out(edit->asset);

	if(!source)
	{
		printf("ResourcePixmap::draw_audio_source: failed to check out %s for drawing.\n", edit->asset->path);
		return;
	}

	w++;
	long source_start = (pixmap_x - edit_x + x) * mwindow->edl->local_session->zoom_sample + edit->startsource;
	long source_len = w * mwindow->edl->local_session->zoom_sample;
	int center_pixel = mwindow->edl->local_session->zoom_track / 2;
	if(mwindow->edl->session->show_titles) center_pixel += mwindow->theme->title_bg_data->get_h();
	double asset_over_session = (double)edit->asset->sample_rate / 
		mwindow->edl->session->sample_rate;

// Single sample zoom
	if(mwindow->edl->local_session->zoom_sample == 1)
	{

		double oldsample, newsample;
		long total_source_samples = (long)((double)(source_len + 1) * 
			asset_over_session);
		double *buffer = new double[total_source_samples];

		source->set_audio_position((int)((double)source_start *
				asset_over_session), 
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
				newsample = buffer[(long)(i * asset_over_session)];
				canvas->draw_line(x1 - 1, 
					(int)(center_pixel - oldsample * mwindow->edl->local_session->zoom_y / 2),
					x1,
					(int)(center_pixel - newsample * mwindow->edl->local_session->zoom_y / 2),
					this);
			}
		}

		delete [] buffer;
	}
	else
// Multiple sample zoom
	{
		long fragmentsize;
		long buffersize = fragmentsize = 65536;
		double *buffer = new double[buffersize + 1];
		double highsample, lowsample;
		float sample_of_pixel = 0;
		long total_source_samples = (long)(source_len * asset_over_session);
		double asset_samples_per_pixel = 
			mwindow->edl->local_session->zoom_sample *
			asset_over_session;
		int first_pixel = 1;
		int prev_y1 = -1;
		int prev_y2 = -1;
		int y1;
		int y2;

		source->set_audio_position((long)(source_start * asset_over_session), 
			edit->asset->sample_rate);
		source->set_channel(edit->channel);
		canvas->set_color(mwindow->theme->audio_color);

		for(long source_sample = 0; 
			source_sample < total_source_samples; 
			source_sample += buffersize)
		{
			fragmentsize = buffersize;
			if(total_source_samples - source_sample < buffersize)
				fragmentsize = total_source_samples - source_sample;

			if(source_sample == 0)
			{
				highsample = buffer[0];
				lowsample = buffer[0];
			}

			if(!source->read_samples(buffer, 
				fragmentsize, 
				edit->asset->sample_rate))
			{


// Draw samples for this buffer
				for(long bufferposition = 0; 
					bufferposition < fragmentsize; 
					bufferposition++)
				{
// Replicate
					if(asset_samples_per_pixel < 1)
					{
						int x1 = x;
						int x2 = (int)(x + 1 / asset_samples_per_pixel);

						highsample = lowsample = buffer[bufferposition];
						y1 = (int)(center_pixel - 
								highsample * 
								mwindow->edl->local_session->zoom_y / 
								2);
						y2 = (int)(center_pixel - 
								lowsample * 
								mwindow->edl->local_session->zoom_y / 
								2);
						canvas->draw_line(
							x1, 
							y1,
							x2,
							y2,
							this);
					}
					else
					if(asset_samples_per_pixel >= 1 &&
						sample_of_pixel >= asset_samples_per_pixel)
					{
// Draw column and reset
						y1 = (int)(center_pixel - 
							highsample * 
							mwindow->edl->local_session->zoom_y / 
							2);
						y2 = (int)(center_pixel - 
							lowsample * 
							mwindow->edl->local_session->zoom_y / 
							2);

						int current_y1;
						int current_y2;
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
						sample_of_pixel -= asset_samples_per_pixel;
						x++;
						lowsample = highsample = buffer[bufferposition];
						prev_y1 = y1;
						prev_y2 = y2;
					}

					if(sample_of_pixel >= 1)
					{
// update lowsample and highsample
						if(buffer[bufferposition] < lowsample) 
							lowsample = buffer[bufferposition];
						else 
						if(buffer[bufferposition] > highsample) 
							highsample = buffer[bufferposition];
					}


					sample_of_pixel++;
				}





			}
		}
		delete [] buffer;
	}

	mwindow->audio_cache->check_in(edit->asset);
}




















void ResourcePixmap::draw_video_resource(Edit *edit, 
	long edit_x, 
	long edit_w, 
	long pixmap_x,
	long pixmap_w,
	int refresh_x, 
	int refresh_w)
{
//printf("ResourcePixmap::draw_video_resource 1\n");
// pixels spanned by a picon
	long picon_w = Units::round(edit->picon_w());
	long picon_h = edit->picon_h();

//printf("ResourcePixmap::draw_video_resource 1\n");
// Don't draw video if picon is bigger than edit
	if(picon_w > edit_w) return;

//printf("ResourcePixmap::draw_video_resource 1\n");
// pixels spanned by a frame
	double frame_w = edit->frame_w();

//printf("ResourcePixmap::draw_video_resource 1\n");
// Frames spanned by a picon
	double frames_per_picon = edit->frames_per_picon();

//printf("ResourcePixmap::draw_video_resource 1\n");
// Current pixel relative to pixmap
	int x = 0;
	int y = 0;
	if(mwindow->edl->session->show_titles) 
		y += mwindow->theme->title_bg_data->get_h();
// Frame in project touched by current pixel
	long project_frame;

//printf("ResourcePixmap::draw_video_resource 1\n");
// Get first frame touched by x and fix x to start of frame
	if(frames_per_picon > 1)
	{
		int picon = Units::to_long((double)(refresh_x + pixmap_x - edit_x) / picon_w);
		x = picon_w * picon + edit_x - pixmap_x;
		project_frame = Units::to_long((double)picon * frames_per_picon);
	}
	else
	{
		project_frame = Units::to_long((double)(refresh_x + pixmap_x - edit_x) / 
			frame_w);
		x = Units::round((double)project_frame * frame_w + edit_x - pixmap_x);
	}

//printf("ResourcePixmap::draw_video_resource 1 %s\n", edit->asset->path);
	File *source = mwindow->video_cache->check_out(edit->asset);
//printf("ResourcePixmap::draw_video_resource 2\n");
	if(!source) return;


//printf("ResourcePixmap::draw_video_resource 2 project_frame=%d frame_w=%f refresh_x=%d refresh_w=%d x=%d\n",
//project_frame, frame_w, refresh_x, refresh_w, x);
	while(x < refresh_x + refresh_w)
	{
		long source_frame = project_frame + edit->startsource;
		source->set_layer(edit->channel);
		source->set_video_position(source_frame, 
			mwindow->edl->session->frame_rate);
//printf("ResourcePixmap::draw_video_resource 3 %p\n", source);
		VFrame *src = source->read_frame(BC_RGB888);
//printf("ResourcePixmap::draw_video_resource 4 %p\n", src);
		if(src)
			draw_vframe(src, 
				x, 
				y, 
				picon_w, 
				picon_h, 
				0, 
				0);

		
		
		
		if(frames_per_picon > 1)
		{
			x += Units::round(picon_w);
			project_frame = Units::to_long(frames_per_picon * (long)((double)(x + pixmap_x - edit_x) / picon_w));
		}
		else
		{
			x += Units::round(frame_w);
			project_frame = (long)((double)(x + pixmap_x - edit_x) / frame_w);
		}
	}

	if(source)
	{
		mwindow->video_cache->check_in(edit->asset);
	}
//printf("ResourcePixmap::draw_video_resource 5\n");
}


void ResourcePixmap::dump()
{
	printf("ResourcePixmap %p\n", this);
	printf(" edit %x edit_x %d pixmap_x %d pixmap_w %d visible %d\n", edit_id, edit_x, pixmap_x, pixmap_w, visible);
}



