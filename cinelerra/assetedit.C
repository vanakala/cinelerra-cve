// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "assetedit.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcbar.h"
#include "bcprogressbox.h"
#include "bcresources.h"
#include "bcsignals.h"
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
#include "selection.h"
#include "theme.h"
#include "new.h"
#include "paramlistwindow.h"
#include "preferences.h"
#include "edl.h"
#include "edlsession.h"
#include "units.h"
#include "vwindow.h"

#include <string.h>
#include <inttypes.h>

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
	int absx, absy;

	if(asset)
	{
		new_asset = new Asset(asset->path);
		*new_asset = *asset;
		new_asset->update_decoder_format_defaults();

		mwindow->get_abs_cursor_pos(&absx, &absy);
		window = new AssetEditWindow(mwindow, this, absx, absy);
		window->raise_window();

		if(!window->run_window())
		{
			if(!asset->equivalent(*new_asset, STRDSC_ALLTYP))
			{
				int newidx = 0;

				newidx = asset->stream_count(STRDSC_AUDIO)
					&& !asset->equivalent(*new_asset, STRDSC_AUDIO);

				mwindow->remove_asset_from_caches(asset);
// Omit index status from copy since an index rebuild may have been
// happening when new_asset was created but not be happening anymore.
				asset->copy_from(new_asset, 0);
				asset->set_decoder_parameters();
				mwindow->update_gui(WUPD_CANVREDRAW);

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
					mwindow->mainindexes->add_next_asset(asset);
					mwindow->mainindexes->start_build();
				}

				mwindow->awindow->gui->async_update_assets();
				mwindow->vwindow->change_source();

				mwindow->sync_parameters();
			}
		}

		delete new_asset;
		delete window;
		window = 0;
	}
}


AssetEditWindow::AssetEditWindow(MWindow *mwindow, AssetEdit *asset_edit,
	int abs_x, int abs_y)
 : BC_Window(MWindow::create_title(N_("Asset Info")),
	abs_x - 400 / 2,
	abs_y - 400 / 2,
	400,
	400,
	400,
	400,
	1,
	0,
	1)
{
	int y = 10;
	int x0 = 10, x1 = 20, x2 = 170;
	char string[BCTEXTLEN];
	Interlaceautofix *ilacefixoption_chkboxw;
	AssetEditPathText *path_text;
	BC_WindowBase *win;
	BC_WindowBase *audiobar, *videobar;
	int winw = get_w();
	int win_x = get_x();
	int win_y = get_y();
	ptstime l, length = 0;
	uintmax_t bitrate;

	this->mwindow = mwindow;
	this->asset_edit = asset_edit;
	this->asset = asset_edit->new_asset;

	set_icon(mwindow->awindow->get_window_icon());
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

	if(asset->decoder_parameters[ASSET_DFORMAT_IX])
	{
		int hpos = x2 + win->get_w() + 10;
		add_subwindow(fmt_button = new AssetEditConfigButton(hpos, y - 5,
			asset->decoder_parameters[ASSET_DFORMAT_IX]));
	}
	else
		fmt_button = 0;
	y += win->get_h() + 5;

	add_subwindow(new BC_Title(x1, y, _("Bytes:")));
	sprintf(string, "%" PRId64, asset->file_length);
	Units::punctuate(string);

	win = add_subwindow(new BC_Title(x2, y, string, MEDIUMFONT, mwindow->theme->edit_font_color));
	y += win->get_h() + 5;

	numaudio = numvideo = 0;

	for(int i = 0; i < asset->last_active; i++)
	{
		if(asset->active_streams[i]->options & STRDSC_AUDIO)
		{
			add_subwindow(aiwin[numaudio] =
				new AudioInfoWindow(asset->active_streams[i],
				mwindow->theme->edit_font_color));
			numaudio++;
		}
		if(asset->active_streams[i]->options & STRDSC_VIDEO)
		{
			add_subwindow(viwin[numvideo] =
				new VideoInfoWindow(asset->active_streams[i],
				mwindow->theme->edit_font_color));
			numvideo++;
		}
		l = asset->active_streams[i]->end - asset->active_streams[i]->start;
		if(length < l)
			length = l;
	}
	if(!asset->single_image && length > 0)
	{
		bitrate = asset->file_length / length * 8;
		add_subwindow(new BC_Title(x1, y, _("Bitrate (bits/sec):")));
		sprintf(string, "%jd", bitrate);
		Units::punctuate(string);
		win = add_subwindow(new BC_Title(x2, y, string,
			MEDIUMFONT, mwindow->theme->edit_font_color));
		y += win->get_h() + 5;
	}
	y += 2;

	audiobar = videobar = 0;

	if(numaudio)
	{
		audiobar = add_subwindow(new BC_Bar(x0, y, get_w() - x0 * 2));
		y += 7;
		win = add_subwindow(new BC_Title(x0, y, _("Audio:"),
			LARGEFONT, RED));
			y += win->get_h() + 5;
		int swx = x1;
		int swh = 0;
		for(int i = 0; i < numaudio; i++)
		{
			aiwin[i]->draw_window();
			aiwin[i]->reposition_window(swx, y,
				aiwin[i]->width, aiwin[i]->height);
			if(aiwin[i]->height > swh)
				swh = aiwin[i]->height;
			swx += aiwin[i]->width + 10;
		}
		if(winw < swx)
			winw = swx;
		y += swh + 5;
	}

	if(numvideo)
	{
		videobar = add_subwindow(new BC_Bar(x0, y, get_w() - x0 * 2));
		y += 7;
		win = add_subwindow(new BC_Title(x0, y, _("Video:"),
			LARGEFONT, RED));
			y += win->get_h() + 5;
		int swx = x1;
		int swh = 0;
		for(int i = 0; i < numvideo; i++)
		{
			viwin[i]->draw_window();
			viwin[i]->reposition_window(swx, y,
				viwin[i]->width, viwin[i]->height);
			if(viwin[i]->height > swh)
				swh = viwin[i]->height;
			swx += viwin[i]->width;
		}
		if(winw < swx)
			winw = swx;
		y += swh + 5;
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
	winw += x0;

	if(audiobar)
		audiobar->reposition_window(x0, audiobar->get_y(),
			winw - x0 * 2, 0);
	if(videobar)
		videobar->reposition_window(x0, videobar->get_y(),
			winw - x0 * 2, 0);
	y += BC_WindowBase::get_resources()->ok_images[0]->get_h() + 30;
	reposition_window(win_x, win_y, winw, y);
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	flush();
}

AssetEditWindow::~AssetEditWindow()
{
// Close all button windows
	if(fmt_button)
		fmt_button->cancel_window();
	for(int i = 0; i < numaudio; i++)
		aiwin[i]->cancel_window();
	for(int i = 0; i < numvideo; i++)
		viwin[i]->cancel_window();
}

AssetInfoWindow::AssetInfoWindow(struct streamdesc *dsc,
	int edit_font_color)
 : BC_SubWindow(0, 0, 200, 100)
{
	stnum = 0;
	font_color = edit_font_color;
	desc = dsc;
	height = 5;
	width = 0;
	p_prompt = 10;
	p_value = 160;
	button = 0;
}

void AssetInfoWindow::cancel_window()
{
	if(button)
		button->cancel_window();
}

void AssetInfoWindow::show_line(const char *prompt, const char *value)
{
	BC_WindowBase *win;
	int w;

	add_subwindow(new BC_Title(p_prompt, height, prompt));
	win = add_subwindow(new BC_Title(p_value, height, value,
		MEDIUMFONT, font_color));
	last_line_pos = height;
	w = win->get_w() + p_value;
	if(width < w)
		width = w;
	height += win->get_h() + 5;
}

void AssetInfoWindow::show_line(const char *prompt, int value)
{
	char string[64];

	sprintf(string, "%d", value);
	show_line(prompt, string);
}

void AssetInfoWindow::show_line(const char *prompt, double value)
{
	char string[64];

	sprintf(string, "%.2f", value);
	show_line(prompt, string);
}

void AssetInfoWindow::show_line(const char *prompt, ptstime start, ptstime end)
{
	char string[64];
	ptstime l;

	if((l = end - start) > EPSILON)
	{
		sprintf(string, "%.2f", l);
		show_line(prompt, string);
	}
}

void AssetInfoWindow::show_streamnum()
{
	BC_WindowBase *win;
	char string[64];

	if(stnum > 0)
	{
		sprintf(string, _("%d. stream:"), stnum);
		win = add_subwindow(new BC_Title(0, height, string));
		height += win->get_h() + 5;
	}
}

void AssetInfoWindow::show_button(Paramlist *params)
{
	add_subwindow(button = new AssetEditConfigButton(width + 10, last_line_pos - 5, params));
	width = width + 10 + button->get_w();
}

AudioInfoWindow::AudioInfoWindow(struct streamdesc *desc,
	int edit_font_color)
 : AssetInfoWindow(desc, edit_font_color)
{
}

void AudioInfoWindow::draw_window()
{
	show_streamnum();

	if(desc->codec[0])
		show_line(_("Codec:"), desc->codec);

	if(desc->decoding_params[ASSET_DFORMAT_IX])
		show_button(desc->decoding_params[ASSET_DFORMAT_IX]);

	if(desc->channels)
		show_line(_("Channels:"), desc->channels);

	if(desc->layout[0])
		show_line(_("Layout:"), desc->layout);

	if(desc->sample_rate)
		show_line(_("Sample rate:"), desc->sample_rate);

	if(desc->samplefmt[0])
		show_line(_("Sample format:"), desc->samplefmt);

	show_line(_("Length (sec):"), desc->start, desc->end);
}

VideoInfoWindow::VideoInfoWindow(struct streamdesc *desc,
	int edit_font_color)
 : AssetInfoWindow(desc, edit_font_color)
{
}

void VideoInfoWindow::draw_window()
{
	char string[64];

	show_streamnum();

	if(desc->codec[0])
		show_line(_("Codec:"), desc->codec);

	if(desc->decoding_params[ASSET_DFORMAT_IX])
		show_button(desc->decoding_params[ASSET_DFORMAT_IX]);

	if(desc->frame_rate)
		show_line(_("Frame rate:"), desc->frame_rate);

	if(desc->width && desc->height)
	{
		sprintf(string, "%d x %d", desc->width, desc->height);
		show_line(_("Size:"), string);
	}

	if(desc->samplefmt[0])
		show_line(_("Sample format:"), desc->samplefmt);

	if(desc->sample_aspect_ratio > 0)
	{
		sprintf(string, "%2.3f", desc->sample_aspect_ratio *
			desc->width / desc->height);
		show_line(_("Aspect ratio:"), string);
	}

	show_line(_("Length (sec):"), desc->start, desc->end);
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
		int method = InterlaceFixSelection::automode(edlsession->interlace_mode,
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

AssetEditConfigButton::AssetEditConfigButton(int x, int y, Paramlist *params)
 : BC_Button(x, y, theme_global->get_image_set("wrench"))
{
	list = params;
	thread = new ParamlistThread(&list, _("Decoder options"),
		mwindow_global->awindow->get_window_icon());
}

AssetEditConfigButton::~AssetEditConfigButton()
{
	delete thread;
}

void AssetEditConfigButton::cancel_window()
{
	delete thread;
	thread = 0;
}

int AssetEditConfigButton::handle_event()
{
	thread->show_window();
	return 1;
}
