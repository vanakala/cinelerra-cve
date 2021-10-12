// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef AVLIBSCONFIG_H
#define AVLIBSCONFIG_H

#include "asset.inc"

#include "avlibsconfig.inc"
#include "bcbutton.h"
#include "bcwindow.h"
#include "bctextbox.h"
#include "bctoggle.h"
#include "paramlist.h"
#include "paramlistwindow.inc"
#include "pipeconfig.inc"
#include "subselection.h"
#include "thread.h"

class AVlibsCodecConfigButton;
class Streamopts;

class AVlibsConfig : public BC_Window
{
public:
	AVlibsConfig(Asset *asset, Paramlist *codecs, int options);
	~AVlibsConfig();

	void draw_paramlist(Paramlist *params);
	void open_paramwin(Paramlist *list);
	int handle_event();
	void merge_saved_options(Paramlist *optlist, const char *config_name,
		const char *suffix);
	static void save_encoder_options(Asset *asset, int config_ix,
		const char *config_name, const char *suffix);

	static Paramlist *load_options(Asset *asset, const char *config_name,
		const char *suffix = 0);
	static char *config_path(Asset *asset, const char *config_name,
		const char *suffix = 0);

	Asset *asset;

	Paramlist *codecs;
	Paramlist *codecopts;
	Paramlist *codec_private;

	ParamlistThread *codecthread;
	ParamlistThread *privthread;
	PipeConfigWindow *pipeconfig;

	int current_codec;

private:
	void draw_bottomhalf(Param *codec, Param *defs);
	int left;
	int top;
	int options;
	int base_w;
	int base_left;
	int tophalf_base;
	SubSelectionPopup *codecpopup;
	AVlibsCodecConfigButton *privbutton;
	BC_Title *privtitle;
	BC_OKButton *okbutton;
	BC_CancelButton *cancelbutton;
	Streamopts *streamopts;
	char string[BCTEXTLEN];
};


class AVlibsCodecConfigButton : public BC_Button
{
public:
	AVlibsCodecConfigButton(int x, int y, Paramlist **listp, AVlibsConfig *cfg);

	int handle_event();

private:
	Paramlist **listp;
	AVlibsConfig *config;
};


class AVlibsCodecConfigPopup : public SubSelectionPopup
{
public:
	AVlibsCodecConfigPopup(int x, int y, int w,
		AVlibsConfig *cfg, Paramlist *paramlist);

	int handle_event();
private:
	AVlibsConfig *config;
};

class Streamopts : public BC_SubWindow
{
public:
	Streamopts(int x, int y);

	void show(Param *encoder);
};

#endif
