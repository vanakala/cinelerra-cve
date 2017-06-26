
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
#include "bcresources.h"
#include "bcsignals.h"
#include "bctitle.h"
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
#include "vframe.h"

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
	BC_WindowBase *win, *twin;
	AVOutputFormat *oformat;
	const char *name;
	int x1;
	int codec_w = 0;
	Param *param, *param2;

	left = 10;
	top = 10;
	codecs = 0;
	codecpopup = 0;
	codecopts = 0;
	streamopts = 0;
	codec_private = 0;
	privbutton = 0;
	privtitle = 0;
	okbutton = 0;
	this->options = options;
	this->asset = asset;

	if(!(name = FileAVlibs::encoder_formatname(asset->format)))
	{
		errormsg("AVLibs does not support '%s'",
			ContainerSelection::container_to_text(asset->format));
		return;
	}

	oformat = FileAVlibs::output_format(asset->format);
	codecs = FileAVlibs::scan_codecs(oformat, asset, options);

	for(Param *p = codecs->first; p; p = p->next)
	{
		if((x1 = get_text_width(MEDIUMFONT, p->name)) > codec_w)
			codec_w = x1;
	}

	sprintf(string, "'%s' options",
		ContainerSelection::container_to_text(asset->format));
	codecthread = new ParamlistThread(&codecopts, "Codec options");
	privthread = new ParamlistThread(&codec_private, "Codec private options");
	win = add_subwindow(new BC_Title(left, top, string));
	top += win->get_h() + 10;

	win = add_subwindow(new AVlibsCodecConfigButton(left, top, &codecopts, this));
	base_left = win->get_w() + 20;
	base_w = win->get_w();

	if(options & SUPPORTS_AUDIO)
	{
		twin = add_subwindow(new BC_Title(base_left, top, "Audio codec:"));
		if(param = codecs->find(asset->acodec))
			codecs->set_selected(param->intvalue);
		param2 = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(AVL_PARAM_CODEC_AUDIO);
	}
	else if(options & SUPPORTS_VIDEO)
	{
		twin = add_subwindow(new BC_Title(base_left, top, "Video codec:"));
		if(param = codecs->find(asset->vcodec))
			codecs->set_selected(param->intvalue);
		param2 = asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(AVL_PARAM_CODEC_VIDEO);
	}
	base_w += twin->get_w() + 10;
	codecpopup = new AVlibsCodecConfigPopup(base_left + base_w,
		top, codec_w + 10, this, codecs);
	tophalf_base  = top + win->get_h() + 10;
	base_w += codecpopup->get_w() + x1;
	draw_bottomhalf(param, param2);
	handle_event();
}


AVlibsConfig::~AVlibsConfig()
{
	if(codecthread->running())
		codecthread->window->set_done(1);
	delete codecthread;

	if(privthread->running())
		privthread->window->set_done(1);
	delete privthread;
	delete codecs;
	delete codecopts;
	delete codecpopup;
	delete codec_private;
}

void AVlibsConfig::draw_bottomhalf(Param *codec, Param *defs)
{
	Param *p1, *p2;
	int haveopts;
	int subw = base_w;

	if(codec && codec->subparams && defs && defs->subparams)
	{
		if(defs->stringvalue && !strcmp(defs->stringvalue, codec->name))
		{
			for(p2 = defs->subparams->first; p2; p2 = p2->next)
			{
				if((p1 = codec->subparams->find(p2->name)) && p1->subparams)
				{
					if(p2->type & PARAMTYPE_INT)
						p1->subparams->set_selected(p2->intvalue);
					if(p2->type & PARAMTYPE_LNG)
						p1->subparams->set_selected(p2->longvalue);
					if(p2->type & PARAMTYPE_DBL)
						p1->subparams->set_selected(p2->floatvalue);
				}
			}
		}
	}
	top = tophalf_base;
	haveopts = 0;
	delete streamopts;
	streamopts = 0;
	if(codec && codec->subparams)
	{
		for(p1 = codec->subparams->first; p1; p1 = p1->next)
		{
			if(p1->subparams)
			{
				if(!(p1->subparams->type & PARAMTYPE_HIDN) &&
						p1->subparams->total() > 1)
					haveopts = 1;
				else
					p1->subparams->type |= PARAMTYPE_HIDN;
			}
		}
		if(haveopts)
		{
			add_subwindow(streamopts = new Streamopts(base_left, top));
			streamopts->show(codec);
			top += streamopts->get_h();
			if(streamopts->get_w() > subw)
				subw = streamopts->get_w();
		}
	}
	if(!privbutton)
	{
		add_subwindow(privbutton = new AVlibsCodecConfigButton(left, top, &codec_private, this));
		add_subwindow(privtitle = new BC_Title(base_left, top, "Codec private options"));
	}
	else
	{
		privbutton->reposition_window(left, top);
		privtitle->reposition_window(base_left, top);
	}
	top += privbutton->get_h();

	int h = top + BC_WindowBase::get_resources()->ok_images[0]->get_h() + 30;
	subw += base_left;
	reposition_window((get_root_w(1) - subw) / 2, (get_root_h(1) - h) / 2,
		subw, h);
	if(okbutton)
		okbutton->reposition_window(10, top + 30);
	else
		add_subwindow(okbutton = new BC_OKButton(this));
}

void AVlibsConfig::open_paramwin(Paramlist *list)
{
	ParamlistThread *thread;

	if(list == codecopts)
		thread = codecthread;
	if(list == codec_private)
		thread = privthread;

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
		if(privthread->running())
			privthread->window->set_done(1);
		privthread->wait_window();
		delete codec_private;
	}
	current_codec = codecs->selectedint;
	codecopts = FileAVlibs::scan_encoder_opts((AVCodecID)current_codec, options);
	merge_saved_options(codecopts, options & SUPPORTS_VIDEO ?
		FILEAVLIBS_VCODEC_CONFIG : FILEAVLIBS_ACODEC_CONFIG, codecopts->name);
	aparm = options & SUPPORTS_VIDEO ? asset->encoder_parameters[FILEAVLIBS_VCODEC_IX] :
		asset->encoder_parameters[FILEAVLIBS_ACODEC_IX];
	if(aparm && !strcmp(aparm->name, codecopts->name))
		codecopts->copy_values(aparm);
	sprintf(string, "%s options", codecopts->name);
	codecthread->set_window_title(string);
	codec_private = FileAVlibs::scan_encoder_private_opts((AVCodecID)current_codec, options);
	if(codec_private)
	{
		merge_saved_options(codec_private, options & SUPPORTS_VIDEO ?
			FILEAVLIBS_VPRIVT_CONFIG : FILEAVLIBS_APRIVT_CONFIG, codec_private->name);
		aparm = options & SUPPORTS_VIDEO ? asset->encoder_parameters[FILEAVLIBS_VPRIVT_IX] :
			asset->encoder_parameters[FILEAVLIBS_APRIVT_IX];
		if(aparm && !strcmp(aparm->name, codec_private->name))
			codec_private->copy_values(aparm);
		sprintf(string, "%s private options", codec_private->name);
		privthread->set_window_title(string);
		privbutton->enable();
	} else
		privbutton->disable();

	draw_bottomhalf(codecs->find_value(current_codec),
		asset->encoder_parameters[FILEAVLIBS_CODECS_IX]->find(options & SUPPORTS_VIDEO ?
			AVL_PARAM_CODEC_VIDEO : AVL_PARAM_CODEC_AUDIO));
	return 1;
}

void AVlibsConfig::merge_saved_options(Paramlist *optlist, const char *config_name,
	const char *suffix)
{
	FileXML file;
	Paramlist *savedopts;

	if(!file.read_from_file(config_path(asset, config_name, suffix), 1) && !file.read_tag())
	{
		savedopts = new Paramlist();
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
		file.write_to_file(config_path(asset, config_name, suffix));
	}
	else
		unlink(config_path(asset, config_name, suffix));
}

Paramlist *AVlibsConfig::load_options(Asset *asset, const char *config_name,
	const char *suffix)
{
	FileXML file;
	Paramlist *opts = 0;

	if(!file.read_from_file(config_path(asset, config_name, suffix), 1) && !file.read_tag())
	{
		opts = new Paramlist();
		opts->load_list(&file);
	}
	return opts;
}

char *AVlibsConfig::config_path(Asset *asset, const char *config_name, const char *suffix)
{
	static char pathbuf[BCTEXTLEN];

	asset->profile_config_path(config_name, pathbuf);

	if(suffix)
		strcat(pathbuf, suffix);

	strcat(pathbuf, FILEAVLIBS_CONFIG_EXT);
	return pathbuf;
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
	if(*listp)
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


Streamopts::Streamopts(int x, int y)
 : BC_SubWindow(x, y, 1, 1)
{
}

void Streamopts::show(Param *encoder)
{
	SubSelectionPopup *win;
	Paramlist *pl;
	int tb_width, tl_width;
	int top, left, l;

	top = left = 0;
	tb_width = 0;

	if(encoder->subparams)
	{
		for(int i = 0; FileAVlibs::encoder_params[i].name; i++)
		{
			if((l = get_text_width(MEDIUMFONT, FileAVlibs::encoder_params[i].prompt)) > left)
				left = l;
		}
		left += 5;

		for(Param *p = encoder->subparams->first; p; p = p->next)
		{
			if(!(p->subparams->type & PARAMTYPE_HIDN))
				tb_width = ParamlistSubWindow::max_name_size(p->subparams, this, tb_width);
		}

		tb_width += 10;
		for(Param *p = encoder->subparams->first; p; p = p->next)
		{
			add_subwindow(new BC_Title(0, top + 4, FileAVlibs::enc_prompt(p->name)));
			if(pl = p->subparams)
			{
				if(pl->type & PARAMTYPE_HIDN)
					continue;
				win = new SubSelectionPopup(left, top,
					tb_width, this, pl);
				top += win->get_h() + 5;
			}
		}
	}
	reposition_window(get_x(), get_y(), left + win->get_w() + 10, top);
}
