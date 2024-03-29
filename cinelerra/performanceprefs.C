// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbar.h"
#include "bclistboxitem.h"
#include "bcresources.h"
#include "bctitle.h"
#include "bcsignals.h"
#include "clip.h"
#include "cache.inc"
#include "formattools.h"
#include "guielements.h"
#include "language.h"
#include "performanceprefs.h"
#include "preferences.h"
#include <string.h>
#include "theme.h"


PerformancePrefs::PerformancePrefs(PreferencesWindow *pwindow)
 : PreferencesDialog(pwindow)
{
	hot_node = -1;
}

PerformancePrefs::~PerformancePrefs()
{
	delete brender_tools;
	nodes[0].remove_all_objects();
	nodes[1].remove_all_objects();
	nodes[2].remove_all_objects();
	nodes[3].remove_all_objects();
}

void PerformancePrefs::show()
{
	int x, y;
	int xmargin1;
	int xmargin2 = 170;
	int xmargin3 = 250;
	int xmargin4 = 380;
	char string[BCTEXTLEN];
	BC_Resources *resources = BC_WindowBase::get_resources();
	BC_WindowBase *win;
	int maxw, curw, y1, ybx[3];

	node_list = 0;
	generate_node_list();

	xmargin1 = x = theme_global->preferencesoptions_x;
	y = theme_global->preferencesoptions_y;

	ybx[0] = y;
	win = add_subwindow(new BC_Title(x, y + 5, _("Cache size (MB):"), MEDIUMFONT, resources->text_default));
	maxw = win->get_w();
	ybx[1] = y += 30;
	win = add_subwindow(new BC_Title(x, y + 5, _("Seconds to preroll renders:")));
	if((curw = win->get_w()) > maxw)
		maxw = curw;
	ybx[2] = y += 30;
	win = add_subwindow(new BC_Title(x, y + 5, _("Maximum number of threads:")));
	if((curw = win->get_w()) > maxw)
		maxw = curw;
	maxw += x + 5;

// Cache size
	new ValueTumbleTextBox(maxw, ybx[0], this,
		&pwindow->thread->preferences->cache_size,
		MIN_CACHE_SIZE, MAX_CACHE_SIZE);
// Seconds to preroll renders
	new DblValueTumbleTextBox(maxw, ybx[1], this,
		&pwindow->thread->preferences->render_preroll, 0.0, 10.0, 0.1,
		GUIELEM_VAL_W, 2);
// Number of threads
	new ValueTumbleTextBox(maxw, ybx[2], this,
		&pwindow->thread->preferences->max_threads, 0, 32);
	y += 35;

// Background rendering
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 5;

	add_subwindow(new BC_Title(x, y, _("Background Rendering (Video only)"), LARGEFONT, resources->text_default));
	y1 = y += 30;
	win = add_subwindow(new CheckBox(x, y, _("Use background rendering"),
		&pwindow->thread->preferences->use_brender));
	y += win->get_h() + 10;
	win = add_subwindow(new BC_Title(x, y, _("Frames per background rendering job:")));

	y += win->get_h() + 5;
	ValueTumbleTextBox *ttb = new ValueTumbleTextBox(x + xmargin3, y, this,
		&pwindow->thread->preferences->brender_fragment, 1, 1024);
	y += ttb->get_h() + 5;
	win = add_subwindow(new BC_Title(x, y, _("Frames to preroll background:")));
	y += win->get_h() + 5;

	ttb = new ValueTumbleTextBox(x + xmargin3, y + 5, this,
		&pwindow->thread->preferences->brender_preroll, 0, 100);
	y += ttb->get_h() + 20;

	x += xmargin4;
	add_subwindow(new BC_Title(x, y1, _("Output for background rendering:")));
	y1 += 20;
	brender_tools = new FormatTools(this,
		pwindow->thread->brender_asset,
		x,
		y1,
		SUPPORTS_VIDEO,     // Include tools for video
		0,
		SUPPORTS_VIDEO,
		0,  // If nonzero, prompt for insertion strategy
		1); // Supply file formats for background rendering
	x = xmargin1;

// Renderfarm
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 5;

	add_subwindow(new BC_Title(x, y, _("Render Farm"), LARGEFONT, resources->text_default));
	y += 25;

	add_subwindow(new CheckBox(x, y, _("Use render farm"),
		&pwindow->thread->preferences->use_renderfarm));
	add_subwindow(new BC_Title(x + xmargin4, y, _("Nodes:")));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Hostname:")));
	add_subwindow(new BC_Title(x + xmargin3, y, _("Port:")));
	add_subwindow(node_list = new PrefsRenderFarmNodes(pwindow, 
		this, 
		x + xmargin4, 
		y - 5));

	add_subwindow(win = new BC_Title(x + xmargin4, y + node_list->get_h(),
		_("Master node framerate:")));
	add_subwindow(master_rate = new BC_Title(x + xmargin4 + win->get_w() + 6,
		y + node_list->get_h()));
	master_rate->update(pwindow->thread->preferences->local_rate);

	y += 25;
	add_subwindow(edit_node = new PrefsRenderFarmEditNode(pwindow, 
		this, 
		x, 
		y));

	edit_port = new ValueTumbleTextBox(x + xmargin3, y, this,
		&pwindow->thread->preferences->renderfarm_port, 1025, 65535);

	y += 30;

	add_subwindow(new PrefsRenderFarmReplaceNode(pwindow, 
		this, 
		x, 
		y));
	add_subwindow(new PrefsRenderFarmNewNode(pwindow, 
		this, 
		x + xmargin2, 
		y));
	y += 30;
	add_subwindow(new PrefsRenderFarmDelNode(pwindow, 
		this, 
		x + xmargin2, 
		y));
	add_subwindow(new PrefsRenderFarmSortNodes(pwindow, 
		this, 
		x, 
		y));
	y += 30;
	add_subwindow(new PrefsRenderFarmReset(pwindow, 
		this, 
		x, 
		y));
	y += 35;
	add_subwindow(new BC_Title(x, 
		y, 
		_("Total jobs to create:")));
	add_subwindow(new BC_Title(x, 
		y + 30, 
		_("(overridden if new file at each label is checked)")));
	new ValueTumbleTextBox(x + xmargin3, y, this,
		&pwindow->thread->preferences->renderfarm_job_count,
		1, 64);
}

void PerformancePrefs::generate_node_list()
{
	int selected_row = node_list ? node_list->get_selection_number(0, 0) : -1;

	nodes[0].remove_all_objects();
	nodes[1].remove_all_objects();
	nodes[2].remove_all_objects();
	nodes[3].remove_all_objects();

	for(int i = 0; 
		i < pwindow->thread->preferences->renderfarm_nodes.total; 
		i++)
	{
		BC_ListBoxItem *item;
		nodes[0].append(item = new BC_ListBoxItem(
			(char*)(pwindow->thread->preferences->renderfarm_enabled.values[i] ? "X" : " ")));
		if(i == selected_row) item->set_selected(1);

		nodes[1].append(item = new BC_ListBoxItem(
			pwindow->thread->preferences->renderfarm_nodes.values[i]));
		if(i == selected_row) item->set_selected(1);

		char string[BCTEXTLEN];
		sprintf(string, "%d", pwindow->thread->preferences->renderfarm_ports.values[i]);
		nodes[2].append(item = new BC_ListBoxItem(string));
		if(i == selected_row) item->set_selected(1);

		sprintf(string, "%0.3f", pwindow->thread->preferences->renderfarm_rate.values[i]);
		nodes[3].append(item = new BC_ListBoxItem(string));
		if(i == selected_row) item->set_selected(1);
	}
}

static const char *titles[] = 
{
	N_("On"),
	N_("Hostname"),
	N_("Port"),
	N_("Framerate")
};

static int widths[] = 
{
	30,
	150,
	50,
	50
};

void PerformancePrefs::update_node_list()
{
	node_list->update(nodes,
			titles,
			widths,
			4,
			node_list->get_xposition(),
			node_list->get_yposition(),
			node_list->get_selection_number(0, 0));
}


PrefsRenderFarmNodes::PrefsRenderFarmNodes(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, int x, int y)
 : BC_ListBox(x, 
		y, 
		340, 
		230,
		subwindow->nodes,
		0,
		titles,
		widths,
		4)
{
	this->subwindow = subwindow;
	this->pwindow = pwindow;
}

int PrefsRenderFarmNodes::column_resize_event()
{
	for(int i = 0; i < 3; i++)
		widths[i] = get_column_width(i);
	return 1;
}

int PrefsRenderFarmNodes::handle_event()
{
	if(get_selection_number(0, 0) >= 0)
	{
		subwindow->hot_node = get_selection_number(1, 0);
		subwindow->edit_node->update(get_selection(1, 0)->get_text());
		subwindow->edit_port->update(get_selection(2, 0)->get_text());
		if(get_cursor_x() < widths[0])
		{
			pwindow->thread->preferences->renderfarm_enabled.values[subwindow->hot_node] = 
				!pwindow->thread->preferences->renderfarm_enabled.values[subwindow->hot_node];
			subwindow->generate_node_list();
			subwindow->update_node_list();
		}
	}
	else
	{
		subwindow->hot_node = -1;
		subwindow->edit_node->update("");
	}
	return 1;
}

void PrefsRenderFarmNodes::selection_changed()
{
	handle_event();
}


PrefsRenderFarmEditNode::PrefsRenderFarmEditNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y)
 : BC_TextBox(x, y, 240, 1, "")
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsRenderFarmEditNode::handle_event()
{
	return 1;
}


PrefsRenderFarmNewNode::PrefsRenderFarmNewNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y)
 : BC_GenericButton(x, y, _("Add Node"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsRenderFarmNewNode::handle_event()
{
	pwindow->thread->preferences->add_node(subwindow->edit_node->get_text(),
		pwindow->thread->preferences->renderfarm_port,
		1,
		0.0);
	pwindow->thread->preferences->reset_rates();
	subwindow->generate_node_list();
	subwindow->update_node_list();
	subwindow->hot_node = -1;
	return 1;
}


PrefsRenderFarmReplaceNode::PrefsRenderFarmReplaceNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y)
 : BC_GenericButton(x, y, _("Apply Changes"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsRenderFarmReplaceNode::handle_event()
{
	if(subwindow->hot_node >= 0)
	{
		pwindow->thread->preferences->edit_node(subwindow->hot_node, 
			subwindow->edit_node->get_text(),
			pwindow->thread->preferences->renderfarm_port,
			pwindow->thread->preferences->renderfarm_enabled.values[subwindow->hot_node]);
		subwindow->generate_node_list();
		subwindow->update_node_list();
	}
	return 1;
}


PrefsRenderFarmDelNode::PrefsRenderFarmDelNode(PreferencesWindow *pwindow, PerformancePrefs *subwindow, int x, int y)
 : BC_GenericButton(x, y, _("Delete Node"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsRenderFarmDelNode::handle_event()
{
	if(subwindow->hot_node >= 0)
	{
		pwindow->thread->preferences->delete_node(subwindow->hot_node);
		subwindow->generate_node_list();
		subwindow->update_node_list();
		subwindow->hot_node = -1;
	}
	return 1;
}


PrefsRenderFarmSortNodes::PrefsRenderFarmSortNodes(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Sort nodes"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsRenderFarmSortNodes::handle_event()
{
	pwindow->thread->preferences->sort_nodes();
	subwindow->generate_node_list();
	subwindow->update_node_list();
	subwindow->hot_node = -1;
	return 1;
}


PrefsRenderFarmReset::PrefsRenderFarmReset(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Reset rates"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsRenderFarmReset::handle_event()
{
	pwindow->thread->preferences->reset_rates();
	subwindow->generate_node_list();
	subwindow->update_node_list();
	subwindow->master_rate->update(pwindow->thread->preferences->local_rate);
	subwindow->hot_node = -1;
	return 1;
}
