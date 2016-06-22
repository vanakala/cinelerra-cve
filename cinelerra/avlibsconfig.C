
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
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "fileavlibs.h"
#include "formattools.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"
#include "mwindow.h"
#include "paramlist.h"
#include "paramlistwindow.h"
#include "subselection.h"
#include "theme.h"

#include <unistd.h>

extern MWindow *mwindow;
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
	Param *param;

	left = 10;
	top = 10;
	globopts = 0;
	codecs = 0;
	fmtopts = 0;
	codecpopup = 0;
	codecopts = 0;
	this->options = options;
	this->asset = asset;

	if(!(name = FileAVlibs::encoder_formatname(asset->format)))
	{
		errormsg("AVLibs does not support '%s'",
			ContainerSelection::container_to_text(asset->format));
		return;
	}

	globopts = FileAVlibs::scan_global_options(options);
	merge_saved_options(globopts, FILEAVLIBS_GLOBAL_CONFIG, 0);
	if(asset->library_parameters)
		globopts->copy_values(asset->library_parameters);
	fmtopts = FileAVlibs::scan_format_options(asset->format, options, &oformat);
	merge_saved_options(fmtopts, FILEAVLIBS_FORMAT_CONFIG, name);
	if(asset->format_parameters)
		fmtopts->copy_values(asset->format_parameters);
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
		if(param = codecs->find(asset->acodec))
			codecs->selectedint = param->intvalue;
	}
	else if(options & SUPPORTS_VIDEO)
	{
		win = add_subwindow(new BC_Title(x1, top, "Video codec:"));
		if(param = codecs->find(asset->vcodec))
			codecs->selectedint = param->intvalue;
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
	Paramlist *aparm;

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
	merge_saved_options(codecopts, options & SUPPORTS_VIDEO ?
		FILEAVLIBS_VCODEC_CONFIG : FILEAVLIBS_ACODEC_CONFIG, codecopts->name);
	aparm = options & SUPPORTS_VIDEO ? asset->vcodec_parameters :
		asset->vcodec_parameters;
	if(aparm)
		codecopts->copy_values(aparm);
	sprintf(string, "%s options", codecopts->name);
	codecthread->set_window_title(string);
	return 1;
}

void AVlibsConfig::merge_saved_options(Paramlist *optlist, const char *config_name,
	const char *suffix)
{
	FileXML file;
	Paramlist *savedopts;

	if(!file.read_from_file(config_path(config_name, suffix), 1) && !file.read_tag())
	{
		savedopts = new Paramlist("");
		savedopts->load_list(&file);

		optlist->copy_values(savedopts);

		delete savedopts;
	}
}

void AVlibsConfig::save_options(Paramlist *optlist, const char *config_name,
	const char *suffix, Paramlist *defaults)
{
	FileXML file;

	optlist->remove_equiv(defaults);

	if(optlist->total() > 0)
	{
		optlist->save_list(&file);
		file.write_to_file(config_path(config_name, suffix));
	}
	else
		unlink(config_path(config_name, suffix));
}

Paramlist *AVlibsConfig::load_options(const char *config_name, const char *suffix)
{
	FileXML file;
	Paramlist *opts = 0;

	if(!file.read_from_file(config_path(config_name, suffix), 1) && !file.read_tag())
	{
		opts = new Paramlist("");
		opts->load_list(&file);
	}
	return opts;
}

char *AVlibsConfig::config_path(const char *config_name, const char *suffix)
{
	static char pathbuf[BCTEXTLEN];

	mwindow->edl->session->configuration_path(config_name, pathbuf);

	if(suffix)
		strcat(pathbuf, suffix);

	strcat(pathbuf, FILEAVLIBS_CONFIG_EXT);
	return pathbuf;
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
	add_subwindow(listwin = new ParamlistWindow(0, 10, PARAMLIST_WIN_MAXH, params));
	listwin->draw_list();

	int w = listwin->get_w() + 40;
	int h = listwin->get_h() + BC_WindowBase::get_resources()->ok_images[0]->get_h() + 40;
	reposition_window((get_root_w(1) - w) / 2, (get_root_h(1) - h) / 2,
		w, h);
	add_subwindow(new BC_OKButton(this));
}
