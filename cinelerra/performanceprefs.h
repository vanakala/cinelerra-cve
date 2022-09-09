// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PERFORMANCEPREFS_H
#define PERFORMANCEPREFS_H

#include "formattools.inc"
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

	CICacheSize *cache_size;

	ArrayList<BC_ListBoxItem*> nodes[4];
	PrefsRenderFarmEditNode *edit_node;
	PrefsRenderFarmPort *edit_port;
	PrefsRenderFarmNodes *node_list;
	FormatTools *brender_tools;
	BC_Title *master_rate;
};


class PrefsBRenderFragment : public BC_TumbleTextBox
{
public:
	PrefsBRenderFragment(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class PrefsRenderPreroll : public BC_TumbleTextBox
{
public:
	PrefsRenderPreroll(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class PrefsBRenderPreroll : public BC_TumbleTextBox
{
public:
	PrefsBRenderPreroll(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class PrefsNumberOfThreads : public BC_TumbleTextBox
{
public:
	PrefsNumberOfThreads(PreferencesWindow *pwindow,
		PerformancePrefs *subwindow,
		int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class PrefsRenderFarm : public BC_CheckBox
{
public:
	PrefsRenderFarm(PreferencesWindow *pwindow, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class PrefsRenderFarmConsolidate : public BC_CheckBox
{
public:
	PrefsRenderFarmConsolidate(PreferencesWindow *pwindow, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class PrefsRenderFarmPort : public BC_TumbleTextBox
{
public:
	PrefsRenderFarmPort(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class PrefsRenderFarmJobs : public BC_TumbleTextBox
{
public:
	PrefsRenderFarmJobs(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class PrefsRenderFarmMountpoint : public BC_TextBox
{
public:
	PrefsRenderFarmMountpoint(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y);

	int handle_event();

	PreferencesWindow *pwindow;
	PerformancePrefs *subwindow;
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


class CICacheSize : public BC_TumbleTextBox
{
public:
	CICacheSize(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow);
	int handle_event();

	PreferencesWindow *pwindow;
};

#endif
