// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PERFORMANCEPREFS_H
#define PERFORMANCEPREFS_H

#include "formattools.inc"
#include "guielements.inc"
#include "preferencesthread.h"

class CICacheSize;
class PrefsRenderFarmEditNode;
class PrefsRenderFarmNodes;
class PrefsRenderFarmPort;

class PerformancePrefs : public PreferencesDialog
{
public:
	PerformancePrefs(PreferencesWindow *pwindow);
	~PerformancePrefs();

	void show();

	void generate_node_list();
	void update_node_list();

	int hot_node;

	ArrayList<BC_ListBoxItem*> nodes[4];
	PrefsRenderFarmEditNode *edit_node;
	ValueTumbleTextBox *edit_port;
	PrefsRenderFarmNodes *node_list;
	FormatTools *brender_tools;
	BC_Title *master_rate;
};


class PrefsRenderFarmNodes : public BC_ListBox
{
public:
	PrefsRenderFarmNodes(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);

	int handle_event();
	void selection_changed();
	int column_resize_event();

	PreferencesWindow *pwindow;
	PerformancePrefs *subwindow;
};


class PrefsRenderFarmEditNode : public BC_TextBox
{
public:
	PrefsRenderFarmEditNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};


class PrefsRenderFarmNewNode : public BC_GenericButton
{
public:
	PrefsRenderFarmNewNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmReplaceNode : public BC_GenericButton
{
public:
	PrefsRenderFarmReplaceNode(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y);

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmDelNode : public BC_GenericButton
{
public:
	PrefsRenderFarmDelNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

class PrefsRenderFarmSortNodes : public BC_GenericButton
{
public:
	PrefsRenderFarmSortNodes(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y);

	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};


class PrefsRenderFarmReset : public BC_GenericButton
{
public:
	PrefsRenderFarmReset(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y);
	int handle_event();

	PerformancePrefs *subwindow;
	PreferencesWindow *pwindow;
};

#endif
