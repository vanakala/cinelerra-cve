
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
#include "assetedit.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcbar.h"
#include "bcprogressbox.h"
#include "bctitle.h"
#include "cache.h"
#include "cinelerra.h"
#include "clip.h"
#include "cplayback.h"
#include "cwindow.h"
#include "file.h"
#include "filesystem.h"
#include "formatpresets.h"
#include "indexfile.h"
#include "language.h"
#include "mainindexes.h"
#include "mainerror.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "selection.h"
#include "theme.h"
#include "new.h"
#include "preferences.h"
#include "edl.h"
#include "edlsession.h"
#include "units.h"
#include "vwindow.h"

#include <string.h>


AssetEdit::AssetEdit(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	asset = 0;
	window = 0;
}

void AssetEdit::edit_asset(Asset *asset)
{
	if(asset)
	{
// Allow more than one window
		this->asset = asset;
		Thread::start();
	}
}

void AssetEdit::set_asset(Asset *asset)
{
	this->asset = asset;
}

void AssetEdit::run()
{
	if(asset)
	{
		new_asset = new Asset(asset->path);
		*new_asset = *asset;
		window = new AssetEditWindow(mwindow, this);
		window->raise_window();

		if(!window->run_window())
		{
			if(!asset->equivalent(*new_asset, 1, 1))
			{
				if(new_asset->audio_data && SampleRateSelection::limits(&new_asset->sample_rate) < 0)
					errorbox(_("Sample rate is out of limits (%d..%d).\nCorrection applied."),
						MIN_SAMPLE_RATE, MAX_SAMPLE_RATE);
				if(new_asset->video_data && FrameRateSelection::limits(&new_asset->frame_rate) < 0)
					errorbox(_("Frame rate is out of limits (%d..%d).\nCorrection applied."),
						MIN_FRAME_RATE, MAX_FRAME_RATE);
				int newidx = asset->audio_data
					&& !asset->equivalent(*new_asset, 1, 0);
				mwindow->remove_asset_from_caches(asset);
// Omit index status from copy since an index rebuild may have been
// happening when new_asset was created but not be happening anymore.
				asset->copy_from(new_asset, 0);

				mwindow->gui->update(WUPD_CANVREDRAW);

// Start index rebuilding
				if(newidx)
				{
					char source_filename[BCTEXTLEN];
					char index_filename[BCTEXTLEN];

					IndexFile::get_index_filename(source_filename, 
						mwindow->preferences->index_directory,
						index_filename, 
						asset->path, asset->audio_streamno - 1);
					remove(index_filename);
					asset->index_status = INDEX_NOTTESTED;
					mwindow->mainindexes->add_next_asset(0, asset);
					mwindow->mainindexes->start_build();
				}

				mwindow->awindow->gui->async_update_assets();
				mwindow->vwindow->change_source();

				mwindow->restart_brender();
				mwindow->sync_parameters(CHANGE_ALL);
			}
		}

		Garbage::delete_object(new_asset);
		delete window;
		window = 0;
	}
}


AssetEditWindow::AssetEditWindow(MWindow *mwindow, AssetEdit *asset_edit)
 : BC_Window(MWindow::create_title(N_("Asset Info")),
	mwindow->gui->get_abs_cursor_x(1) - 400 / 2, 
	mwindow->gui->get_abs_cursor_y(1) - 550 / 2, 
	400, 
	720,
	400,
	560,
	0,
	0,
	1)
{
	int y;
	int x0 = 10, x1 = 20, x2 = 170;
	char string[BCTEXTLEN];
	Interlaceautofix *ilacefixoption_chkboxw;
	AssetEditPathText *path_text;
	BC_WindowBase *win;
	int numaudio, numvideo;
	int strnum;
	int have_bar = 0;

	this->mwindow = mwindow;
	this->asset_edit = asset_edit;
	this->asset = asset_edit->new_asset;

	set_icon(mwindow->theme->get_image("awindow_icon"));
	add_subwindow(path_text = new AssetEditPathText(this, x0, y, 300));
	win = add_subwindow(new AssetEditPath(mwindow,
		this, 
		path_text,
		x0 + path_text->get_w() + 5, y,
		asset->path, 
		MWindow::create_title(N_("Asset path")), _("Select a file for this asset:")));
	y += win->get_h() + 5;

	add_subwindow(new BC_Title(x1, y, _("File format:")));
	win = add_subwindow(new BC_Title(x2, y, _(ContainerSelection::container_to_text(asset->format)),
		MEDIUMFONT, mwindow->theme->edit_font_color));
	y += win->get_h() + 5;

	add_subwindow(new BC_Title(x1, y, _("Bytes:")));
	sprintf(string, "%" PRId64, asset->file_length);
	Units::punctuate(string);

	win = add_subwindow(new BC_Title(x2, y, string, MEDIUMFONT, mwindow->theme->edit_font_color));
	y += win->get_h() + 5;

	numaudio = numvideo = 0;
	if(!asset->single_image)
	{
		ptstime l, length = 0;
		uintmax_t bitrate;

		for(int i = 0; i < asset->nb_streams; i++)
		{
			if(asset->streams[i].options & STRDSC_AUDIO)
				numaudio++;
			if(asset->streams[i].options & STRDSC_VIDEO)
				numvideo++;
			l = asset->streams[i].end - asset->streams[i].start;
			if(length < l)
				length = l;
		}
		if(length > 0)
		{
			bitrate = asset->file_length / length * 8;
			add_subwindow(new BC_Title(x1, y, _("Bitrate (bits/sec):")));
			sprintf(string, "%jd", bitrate);
			Units::punctuate(string);
			win = add_subwindow(new BC_Title(x2, y, string,
				MEDIUMFONT, mwindow->theme->edit_font_color));
			y += win->get_h() + 5;
		}
	}
	y += 2;

	have_bar = 0;
	strnum = 0;
	for(int i = 0; i < asset->nb_streams; i++)
	{
		ptstime l;

		if(asset->streams[i].options & STRDSC_AUDIO)
		{
			if(!have_bar)
			{
				add_subwindow(new BC_Bar(x0, y, get_w() - x0 * 2));
				y += 7;
				win = add_subwindow(new BC_Title(x0, y, _("Audio:"),
					LARGEFONT, RED));
				y += win->get_h() + 5;
				have_bar = 1;
			}

			if(numaudio > 1)
			{
				sprintf(string, _("%d. stream:"), ++strnum);
				win = add_subwindow(new BC_Title(x0, y, string));
				y += win->get_h() + 5;
			}

			if(asset->streams[i].codec[0])
			{
				add_subwindow(new BC_Title(x1, y, _("Codec:")));
				win = add_subwindow(new BC_Title(x2, y,
					asset->streams[i].codec,
					MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}

			if(asset->streams[i].channels)
			{
				add_subwindow(new BC_Title(x1, y, _("Channels:")));
				sprintf(string, "%d", asset->channels);
				win = add_subwindow(new BC_Title(x2, y, string, MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}

			if(asset->streams[i].layout[0])
			{
				add_subwindow(new BC_Title(x1, y, _("Layout:")));
				win = add_subwindow(new BC_Title(x2, y,
					asset->streams[i].layout,
					MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}

			if(asset->streams[i].sample_rate)
			{
				add_subwindow(new BC_Title(x1, y, _("Sample rate:")));
				sprintf(string, "%d", asset->streams[i].sample_rate);
				win = add_subwindow(new BC_Title(x2, y, string, MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}

			if(asset->streams[i].samplefmt[0])
			{
				add_subwindow(new BC_Title(x1, y, _("Sample format:")));
				win = add_subwindow(new BC_Title(x2, y,
					asset->streams[i].samplefmt,
					MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}

			if(l = asset->streams[i].end - asset->streams[i].start)
			{
				add_subwindow(new BC_Title(x1, y, _("Length (sec):")));
				sprintf(string, "%.2f", l);
				win = add_subwindow(new BC_Title(x2, y, string, MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}
		}
	}
	y += 2;

	strnum = 0;
	have_bar = 0;
	for(int i = 0; i < asset->nb_streams; i++)
	{
		ptstime l;

		if(asset->streams[i].options & STRDSC_VIDEO)
		{
			if(!have_bar)
			{
				add_subwindow(new BC_Bar(x0, y, get_w() - x0 * 2));
				y += 7;
				win = add_subwindow(new BC_Title(x0, y, _("Video:"),
					LARGEFONT, RED));
				y += win->get_h() + 5;
				have_bar = 1;
			}

			if(numvideo > 1)
			{
				sprintf(string, _("%d. stream:"), ++strnum);
				win = add_subwindow(new BC_Title(x0, y, string));
				y += win->get_h() + 5;
			}

			if(asset->streams[i].codec[0])
			{
				add_subwindow(new BC_Title(x1, y, _("Codec:")));
				win = add_subwindow(new BC_Title(x2, y,
					asset->streams[i].codec,
					MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}
			if(asset->streams[i].frame_rate)
			{
				add_subwindow(new BC_Title(x1, y, _("Frame rate:")));
				sprintf(string, "%.2f", asset->streams[i].frame_rate);
				win = add_subwindow(new BC_Title(x2, y, string, MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}

			if(asset->streams[i].width && asset->streams[i].height)
			{
				add_subwindow(new BC_Title(x1, y, _("Size:")));
				sprintf(string, "%d x %d", asset->width, asset->height);
				add_subwindow(new BC_Title(x2, y, string, MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}

			if(asset->streams[i].samplefmt[0])
			{
				add_subwindow(new BC_Title(x1, y, _("Sample format:")));
				win = add_subwindow(new BC_Title(x2, y,
					asset->streams[i].samplefmt,
					MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}

			if(asset->streams[i].sample_aspect_ratio > 0)
			{
				add_subwindow(new BC_Title(x1, y, _("Aspect ratio:")));
				sprintf(string, "%2.3f", asset->streams[i].sample_aspect_ratio *
					asset->streams[i].width / asset->streams[i].height);
				add_subwindow(new BC_Title(x2, y, string, MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}

			if(l = asset->streams[i].end - asset->streams[i].start)
			{
				add_subwindow(new BC_Title(x1, y, _("Length (sec):")));
				sprintf(string, "%.2f", l);
				win = add_subwindow(new BC_Title(x2, y, string, MEDIUMFONT,
					mwindow->theme->edit_font_color));
				y += win->get_h() + 5;
			}
		}
	}

// --------------------
	if(numvideo)
	{
		add_subwindow(new BC_Title(x1, y, _("Fix interlacing:")));
		add_subwindow(ilacefixoption_chkboxw = new Interlaceautofix(mwindow, this, x2, y));
		y += ilacefixoption_chkboxw->get_h() + 5;

// --------------------
		add_subwindow(new BC_Title(x1, y, _("Asset's interlacing:")));
		add_subwindow(ilacemode_selection = new AssetInterlaceMode(x2, y,
			this, &asset->interlace_mode));
		ilacemode_selection->update(asset->interlace_mode);
		ilacemode_selection->autofix = ilacefixoption_chkboxw;
		y += ilacemode_selection->get_h() + 5;

// --------------------
		add_subwindow(new BC_Title(x1, y, _("Interlace correction:")));
		add_subwindow(ilacefix_selection = new InterlaceFixSelection(x2, y,
			this, &asset->interlace_fixmethod));
		ilacefixoption_chkboxw->showhideotherwidgets();
		y += ilacefix_selection->get_h() + 5;
	}

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	flush();
}


Interlaceautofix::Interlaceautofix(MWindow *mwindow,AssetEditWindow *fwindow, int x, int y)
 : BC_CheckBox(x, y, fwindow->asset->interlace_autofixoption, _("Automatically"))
{
	this->fwindow = fwindow;
	this->mwindow = mwindow;
}

int Interlaceautofix::handle_event()
{
	int value = get_value();

	fwindow->asset->interlace_autofixoption = value;
	showhideotherwidgets();
	return 1;
}

void Interlaceautofix::showhideotherwidgets()
{
	if(get_value())
	{
		int method = InterlaceFixSelection::automode(mwindow->edl->session->interlace_mode,
			fwindow->asset->interlace_mode);
		fwindow->ilacefix_selection->disable(0);
		fwindow->ilacefix_selection->update(method);
	}
	else
	{
		fwindow->ilacefix_selection->enable(1);
		fwindow->ilacefix_selection->update(fwindow->asset->interlace_fixmethod);
	}
}


AssetInterlaceMode::AssetInterlaceMode(int x, int y, BC_WindowBase *base_gui, int *value)
 : AInterlaceModeSelection(x, y, base_gui, value)
{
	autofix = 0;
}

int AssetInterlaceMode::handle_event()
{
	int result = Selection::handle_event();

	if(autofix)
		autofix->showhideotherwidgets();
	return result;
}

AssetEditPathText::AssetEditPathText(AssetEditWindow *fwindow, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, fwindow->asset->path)
{
	this->fwindow = fwindow; 
}

int AssetEditPathText::handle_event() 
{
	strcpy(fwindow->asset->path, get_text());
	return 1;
}

AssetEditPath::AssetEditPath(MWindow *mwindow, AssetEditWindow *fwindow,
	BC_TextBox *textbox, int x, int y, const char *text,
	const char *window_title, const char *window_caption)
 : BrowseButton(mwindow, fwindow, textbox, x, y, text, window_title, window_caption, 0)
{ 
	this->fwindow = fwindow; 
}
