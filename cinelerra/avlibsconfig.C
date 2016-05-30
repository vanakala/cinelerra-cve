
/*
 * CINELERRA
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>
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
#include "avlibsconfig.h"
#include "bcwindow.h"
#include "bcsignals.h"
#include "fileavlibs.h"
#include "formattools.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"
#include "mwindow.h"
#include "paramlist.h"
#include "subselection.h"
#include "theme.h"

#include <unistd.h>

extern Theme *theme_global;

#define PARAM_WIN_MARGIN 20
#define PARAM_WIN_MAXW 1024
#define PARAM_WIN_MAXH 600

AVlibsConfig::AVlibsConfig(Asset *asset, int options)
 : BC_Window(MWindow::create_title(N_("Compression")),
	100,
	100,
	200,
	100)
{
	BC_WindowBase *win;
	AVOutputFormat *oformat;
	const char *name;
	int x1, base_w;
	int codec_w = 0;

	left = 10;
	top = 10;
	globopts = 0;
	codecs = 0;
	fmtopts = 0;
	codecpopup = 0;
	codecopts = 0;
	this->options = options;

	if(!(name = FileAVlibs::encoder_formatname(asset->format)))
	{
		errormsg("AVLibs does not support '%s'",
			ContainerSelection::container_to_text(asset->format));
		return;
	}

	globopts = FileAVlibs::scan_global_options(options);
	fmtopts = FileAVlibs::scan_format_options(asset->format, options, &oformat);
	codecs = FileAVlibs::scan_codecs(oformat, options);

	for(Param *p = codecs->first; p; p = p->next)
	{
		if((x1 = get_text_width(MEDIUMFONT, p->name)) > codec_w)
			codec_w = x1;
	}

	globthread = new AVlibsParamThread(globopts, "AVlib options");
	sprintf(string, "'%s' options",
		ContainerSelection::container_to_text(asset->format));
	fmtthread = new AVlibsParamThread(fmtopts, string);
	codecthread = new AVlibsParamThread(&codecopts, "Codec options");
	win = add_subwindow(new BC_Title(left, top, string));
	top += win->get_h() + 10;
	win = add_subwindow(new AVlibsConfigButton(left, top, globopts, this));
	x1 = win->get_w() + 20;
	add_subwindow(new BC_Title(x1, top, "Library options"));
	top += win->get_h();
	win = add_subwindow(new AVlibsConfigButton(left, top, fmtopts, this));
	add_subwindow(new BC_Title(x1, top, "Format options"));
	top += win->get_h();
	win = add_subwindow(new AVlibsCodecConfigButton(left, top, &codecopts, this));
	base_w = win->get_w();
	if(options & SUPPORTS_AUDIO)
	{
		win = add_subwindow(new BC_Title(x1, top, "Audio codec:"));
	}
	else if(options & SUPPORTS_VIDEO)
	{
		win = add_subwindow(new BC_Title(x1, top, "Video codec:"));
	}
	base_w += win->get_w() + 10;
	codecpopup = new AVlibsCodecConfigPopup(x1 + base_w,
		top, codec_w + 10, this, codecs);
	top += win->get_h();
	base_w += codecpopup->get_w() + x1;
	int h = top + BC_WindowBase::get_resources()->ok_images[0]->get_h() + 30;
	reposition_window((get_root_w(1) - base_w) / 2, (get_root_h(1) - h) / 2,
		base_w, h);
	add_subwindow(new BC_OKButton(this));
}


AVlibsConfig::~AVlibsConfig()
{
	if(globthread->running())
		globthread->window->set_done(1);
	delete globthread;

	if(fmtthread->running())
		globthread->window->set_done(1);
	delete fmtthread;

	if(codecthread->running())
		codecthread->window->set_done(1);
	delete codecthread;

	delete globopts;
	delete codecs;
	delete fmtopts;
	delete codecopts;
	delete codecpopup;
}

void AVlibsConfig::open_paramwin(Paramlist *list)
{
	AVlibsParamThread *thread;

	if(list == globopts)
		thread = globthread;
	if(list == fmtopts)
		thread = fmtthread;
	if(list == codecopts)
		thread = codecthread;

	if(!thread->running())
		thread->start();
	else
		thread->window->raise_window();
}

int AVlibsConfig::handle_event()
{
	if(codecopts)
	{
		if(codecs->selectedint == current_codec)
			return 0;
		if(codecthread->running())
			codecthread->window->set_done(1);
		codecthread->wait_window();
		delete codecopts;
	}
	current_codec = codecs->selectedint;
	codecopts = FileAVlibs::scan_encoder_opts((AVCodecID)current_codec, options);
	sprintf(string, "%s options", codecopts->name);
	codecthread->set_window_title(string);
	return 1;
}


AVlibsConfigButton::AVlibsConfigButton(int x, int y, Paramlist *list, AVlibsConfig *cfg)
 : BC_Button(x, y, theme_global->get_image_set("wrench"))
{
	this->list = list;
	config = cfg;
}

int AVlibsConfigButton::handle_event()
{
	config->open_paramwin(list);
	return 1;
}

AVlibsCodecConfigButton::AVlibsCodecConfigButton(int x, int y,
	Paramlist **list, AVlibsConfig *cfg)
 : BC_Button(x, y, theme_global->get_image_set("wrench"))
{
	this->listp = list;
	config = cfg;
}

int AVlibsCodecConfigButton::handle_event()
{
	config->handle_event();
	config->open_paramwin(*listp);
	return 1;
}

AVlibsCodecConfigPopup::AVlibsCodecConfigPopup(int x, int y, int w, 
	AVlibsConfig *cfg, Paramlist *paramlist)
 : SubSelectionPopup(x, y, w, cfg, paramlist)
{
	config = cfg;
}

int AVlibsCodecConfigPopup::handle_event()
{
	int rv = SubSelectionPopup::handle_event();
	if(rv)
		config->handle_event();
	return rv;
}

AVlibsParamThread::AVlibsParamThread(Paramlist *params, const char *name)
 : Thread()
{
	strcpy(window_title, name);
	this->params = params;
	paramp = &this->params;
	window_lock = new Mutex("AVlibsParamThread::window_lock");
}

AVlibsParamThread::AVlibsParamThread(Paramlist **paramp, const char *name)
 : Thread()
{
	strcpy(window_title, name);
	this->paramp = paramp;
	window_lock = new Mutex("AVlibsParamThread::window_lock");
}

void AVlibsParamThread::set_window_title(const char *name)
{
	strcpy(window_title, name);
}

void AVlibsParamThread::run()
{
	window_lock->lock("AVlibsParamThread::run");
	window = new AVlibsParamWindow(*paramp, window_title);
	window->run_window();
	delete window;
	window_lock->unlock();
}

void AVlibsParamThread::wait_window()
{
	window_lock->lock("AVlibsParamThread::wait_window");
	window_lock->unlock();
}

AVlibsParamThread::~AVlibsParamThread()
{
	window_lock->lock("AVlibsParamThread::~AVlibsParamThread");
	delete window_lock;
}

AVlibsParamWindow::AVlibsParamWindow(Paramlist *params, const char *winname)
 : BC_Window(winname,
	200,
	100,
	200,
	200)
{
	Param *current, *subparam;
	BC_WindowBase *win = 0;
	int w1 = 0, h1 = 0, tw;
	int name_width;

	top = left = 10;
	base_w = 0;
	base_y = top;
	bot_max = 0;
	new_column = 0;
	name_width = 0;

	for(current = params->first; current; current = current->next)
	{
		if((tw = get_text_width(MEDIUMFONT, current->name)) > name_width)
			name_width = tw;
	}
	name_width += 5;
	bottom_margin = PARAM_WIN_MAXH -
		BC_WindowBase::get_resources()->ok_images[0]->get_h() - 40;

	for(current = params->first; current; current = current->next)
	{
		calc_pos(h1, w1);
		h1 = w1 = 0;
		switch(current->type & PARAMTYPE_MASK)
		{
		case PARAMTYPE_LNG:
			if(current->subparams)
			{
				add_subwindow(win = new SubSelection(left,
					top, name_width - 5, this, current));
				w1 = win->get_w();
			}
			else
			{
				add_subwindow(new BC_Title(left, top, current->name));
				add_subwindow(win = new Parami64Txtbx(left + name_width,
					top, current, &current->longvalue));
				w1 = name_width + win->get_w();
			}
			h1 = win->get_h();
			break;
		case PARAMTYPE_STR:
			add_subwindow(new BC_Title(left, top, current->name));
			add_subwindow(win = new ParamStrTxtbx(left + name_width,
				top, current, current->stringvalue));
			w1 = name_width + win->get_w();
			h1 = win->get_h();
			break;
		case PARAMTYPE_DBL:
			add_subwindow(new BC_Title(left, top, current->name));
			add_subwindow(win = new ParamDblTxtbx(left + name_width,
				top, current, &current->floatvalue));
			w1 = name_width + win->get_w();
			h1 = win->get_h();
			break;
		default:
			break;
		}
	}
	calc_pos(h1, w1);

	int w = left + base_w + PARAM_WIN_MARGIN;
	if(new_column && left > base_w)
		w -= base_w;
	int h = bot_max + BC_WindowBase::get_resources()->ok_images[0]->get_h() + 40;
	reposition_window((get_root_w(1) - w) / 2, (get_root_h(1) - h) / 2,
		w, h);
	add_subwindow(new BC_OKButton(this));
}

void AVlibsParamWindow::calc_pos(int h, int w)
{
	top += h + 5;

	if(w > base_w)
		base_w = w;

	if(bot_max < top)
		bot_max = top;

	if(top > bottom_margin)
	{
		top = base_y;
		left += base_w + 20;
		new_column = 1;
	}
	else
		new_column = 0;
}


Parami64Txtbx::Parami64Txtbx(int x, int y, Param *param, int64_t *val)
 : BC_TextBox(x, y, 100, 1, *val)
{
	this->param = param;
	if(param->helptext)
		set_tooltip(param->helptext);
	valptr = val;
}

int Parami64Txtbx::handle_event()
{
	*valptr = atol(get_text());
	return 1;
}


ParamStrTxtbx::ParamStrTxtbx(int x, int y, Param *param, const char *str)
 : BC_TextBox(x, y, 100, 1, str)
{
	this->param = param;
	if(param->helptext)
		set_tooltip(param->helptext);
}

ParamStrTxtbx::~ParamStrTxtbx()
{
	param->set_string(get_text());
}

ParamDblTxtbx::ParamDblTxtbx(int x, int y, Param *param, double *val)
 : BC_TextBox(x, y, 100, 1, (float)*val)
{
	this->param = param;
	if(param->helptext)
		set_tooltip(param->helptext);
	valptr = val;
}

int ParamDblTxtbx::handle_event()
{
	*valptr = atof(get_text());
	return 1;
}
