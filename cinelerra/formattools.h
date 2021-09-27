// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FORMATTOOLS_H
#define FORMATTOOLS_H

#include "asset.inc"
#include "bcrecentlist.h"
#include "browsebutton.h"
#include "file.inc"
#include "mwindow.inc"
#include "selection.h"
#include "paramlistwindow.inc"
#include "pluginserver.inc"

class FormatAParams;
class FormatVParams;
class FormatFParams;
class FormatAThread;
class FormatVThread;
class FormatPathButton;
class FormatPathText;
class FormatAudio;
class FormatVideo;
class FormatMultiple;
class FormatPopup;
class ContainerSelection;

struct container_type
{
	const char *text;
	int value;
	const char *prefix;
	const char *extension;
};


class FormatTools
{
public:
	FormatTools(MWindow *mwindow,
		BC_WindowBase *window,
		Asset *asset,
		int &init_x,
		int &init_y,
		int support,
		int checkbox,
		int details,
		int *strategy,
		int brender = 0,
		int horizontal_layout = 0);
	virtual ~FormatTools();

// enable/disable supported streams
	void enable_supported();

	void reposition_window(int &init_x, int &init_y);
// Put new asset's parameters in and change asset.
	void update(Asset *asset, int *strategy);
// Update filename extension when format is changed.
	void update_extension();
	void close_format_windows();
	Asset* get_asset();

// Handle change in path text.  Used in BatchRender.
	virtual int handle_event() { return 0; };

	void set_audio_options();
	void set_video_options();
	void set_format_options();
	int get_w();
	void format_changed();

	BC_WindowBase *window;
	Asset *asset;

	FormatAParams *aparams_button;
	FormatVParams *vparams_button;
	FormatFParams *fparams_button;

	FormatAThread *aparams_thread;
	FormatVThread *vparams_thread;
	ParamlistThread *fparams_thread;

	BrowseButton *path_button;
	FormatPathText *path_textbox;
	BC_RecentList *path_recent;
	BC_Title *format_title;
	FormatPopup *format_popup;

	BC_Title *audio_title;
	FormatAudio *audio_switch;

	BC_Title *video_title;
	FormatVideo *video_switch;

	FormatMultiple *multiple_files;
	MWindow *mwindow;
	int use_brender;
	int do_audio;
	int do_video;
	int support;
	int checkbox;
	int details;
	int *strategy;
	int w;
};


class FormatPathText : public BC_TextBox
{
public:
	FormatPathText(int x, int y, FormatTools *format);

	int handle_event();

	FormatTools *format;
};


class FormatAParams : public BC_Button
{
public:
	FormatAParams(MWindow *mwindow, FormatTools *format, int x, int y);

	int handle_event();

	FormatTools *format;
};

class FormatVParams : public BC_Button
{
public:
	FormatVParams(MWindow *mwindow, FormatTools *format, int x, int y);

	int handle_event();

	FormatTools *format;
};

class FormatFParams : public BC_Button
{
public:
	FormatFParams(MWindow *mwindow, FormatTools *format, int x, int y);

	int handle_event();
private:
	FormatTools *format;
};

class FormatAThread : public Thread
{
public:
	FormatAThread(FormatTools *format);
	~FormatAThread();

	void run();

	FormatTools *format;
	File *file;
};

class FormatVThread : public Thread
{
public:
	FormatVThread(FormatTools *format);
	~FormatVThread();

	void run();

	FormatTools *format;
	File *file;
};

class FormatAudio : public BC_CheckBox
{
public:
	FormatAudio(int x, int y, FormatTools *format, int default_);

	int handle_event();

	FormatTools *format;
};

class FormatVideo : public BC_CheckBox
{
public:
	FormatVideo(int x, int y, FormatTools *format, int default_);

	int handle_event();

	FormatTools *format;
};

class FormatToTracks : public BC_CheckBox
{
public:
	FormatToTracks(int x, int y, int *output);

	int handle_event();

	int *output;
};

class FormatMultiple : public BC_CheckBox
{
public:
	FormatMultiple(MWindow *mwindow, int x, int y, int *output);

	int handle_event();
	void update(int *output);

	int *output;
	MWindow *mwindow;
};


class FormatPopup
{
public:
	FormatPopup(BC_WindowBase *parent, int x, int y,
		int *output, FormatTools *tools, int use_brender);
	~FormatPopup();

	int get_h();
	void update(int value);
	void reposition_window(int x, int y);

private:
	ContainerSelection *selection;
	static int brender_menu[];
	static int frender_menu1[];
	static int frender_menu2[];
	struct selection_int *current_menu;
};


class ContainerSelection : public Selection
{
public:
	ContainerSelection(int x, int y, BC_WindowBase *base,
		selection_int *menu, int *value, FormatTools *tools);

	void update(int value);
	const char *format_to_text(int format);
	int handle_event();
	static const struct container_type *get_item(int value);
	static const char *container_to_text(int format);
	static int text_to_container(const char *string);
	static const char *container_prefix(int format);
	static const char *container_extension(int format);
	static int prefix_to_container(const char *string);

private:
	FormatTools *tools;
	static const struct container_type media_containers[];
};

#endif
