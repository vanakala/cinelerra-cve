
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

#include "bcbar.h"
#include "bclistboxitem.h"
#include "bcresources.h"
#include "bctitle.h"
#include "bcsignals.h"
#include "clip.h"
#include "formattools.h"
#include "language.h"
#include "mwindow.h"
#include "performanceprefs.h"
#include "preferences.h"
#include <string.h>
#include "theme.h"


PerformancePrefs::PerformancePrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
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
	int maxw, curw, y1, ybx[2];

	node_list = 0;
	generate_node_list();

	xmargin1 = x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y;

	ybx[0] = y;
	win = add_subwindow(new BC_Title(x, y + 5, _("Cache size (MB):"), MEDIUMFONT, resources->text_default));
	maxw = win->get_w();
	ybx[1] = y += 30;
	win = add_subwindow(new BC_Title(x, y + 5, _("Seconds to preroll renders:")));
	if((curw = win->get_w()) > maxw)
		maxw = curw;
	maxw += x + 5;

// Cache size
	cache_size = new CICacheSize(maxw,
		ybx[0],
		pwindow, 
		this);

// Seconds to preroll renders
	PrefsRenderPreroll *preroll = new PrefsRenderPreroll(pwindow, 
		this, 
		maxw,
		ybx[1]);
	y += 30;
	add_subwindow(new PrefsForceUniprocessor(pwindow, x, y));

	y += 35;

// Background rendering
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += 5;

	add_subwindow(new BC_Title(x, y, _("Background Rendering (Video only)"), LARGEFONT, resources->text_default));
	y1 = y += 30;

	win = add_subwindow(new PrefsUseBRender(pwindow, 
		x,
		y));
	y += win->get_h() + 10;
	win = add_subwindow(new BC_Title(x, y, _("Frames per background rendering job:")));

	y += win->get_h() + 5;
	PrefsBRenderFragment *brender_fragment = new PrefsBRenderFragment(pwindow, 
		this, 
		x + xmargin3,
		y);
	y += brender_fragment->get_h() + 5;
	win = add_subwindow(new BC_Title(x, y, _("Frames to preroll background:")));
	y += win->get_h() + 5;
	PrefsBRenderPreroll *bpreroll = new PrefsBRenderPreroll(pwindow, 
		this, 
		x + xmargin3, 
		y + 5);
	y += bpreroll->get_h() + 20;

	x += xmargin4;
	add_subwindow(new BC_Title(x, y1, _("Output for background rendering:")));
	y1 += 20;
	brender_tools = new FormatTools(this,
		pwindow->thread->preferences->brender_asset,
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

	add_subwindow(new PrefsRenderFarm(pwindow, x, y));
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
	edit_port = new PrefsRenderFarmPort(pwindow, 
		this, 
		x + xmargin3, 
		y);

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
	PrefsRenderFarmJobs *jobs = new PrefsRenderFarmJobs(pwindow, 
		this, 
		x + xmargin3, 
		y);
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


PrefsUseBRender::PrefsUseBRender(PreferencesWindow *pwindow, 
	int x,
	int y)
 : BC_CheckBox(x, 
	y,
	pwindow->thread->preferences->use_brender, 
	_("Use background rendering"))
{
	this->pwindow = pwindow;
}

int PrefsUseBRender::handle_event()
{
	pwindow->thread->redraw_overlays = 1;
	pwindow->thread->redraw_times = 1;
	pwindow->thread->preferences->use_brender = get_value();
	return 1;
}


PrefsBRenderFragment::PrefsBRenderFragment(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, 
	int x, 
	int y)
 : BC_TumbleTextBox(subwindow, 
	pwindow->thread->preferences->brender_fragment,
	1,
	65535,
	x,
	y,
	100)
{
	this->pwindow = pwindow;
}

int PrefsBRenderFragment::handle_event()
{
	pwindow->thread->preferences->brender_fragment = atol(get_text());
	return 1;
}


CICacheSize::CICacheSize(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow)
 : BC_TumbleTextBox(subwindow,
	(int)(pwindow->thread->preferences->cache_size / 0x100000),
	(int)(MIN_CACHE_SIZE / 0x100000),
	(int)(MAX_CACHE_SIZE / 0x100000),
	x, 
	y, 
	100)
{ 
	this->pwindow = pwindow;
	set_increment(1);
}

int CICacheSize::handle_event()
{
	size_t result;

	result = atol(get_text()) * 0x100000;
	CLAMP(result, MIN_CACHE_SIZE, MAX_CACHE_SIZE);
	pwindow->thread->preferences->cache_size = result;
	return 1;
}


PrefsRenderPreroll::PrefsRenderPreroll(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y)
 : BC_TumbleTextBox(subwindow, 
	(float)pwindow->thread->preferences->render_preroll,
	(float)0, 
	(float)100,
	x,
	y,
	100)
{
	this->pwindow = pwindow;
	set_increment(0.1);
}

int PrefsRenderPreroll::handle_event()
{
	pwindow->thread->preferences->render_preroll = atof(get_text());
	return 1;
}


PrefsBRenderPreroll::PrefsBRenderPreroll(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y)
 : BC_TumbleTextBox(subwindow, 
	pwindow->thread->preferences->brender_preroll,
	0,
	100,
	x,
	y,
	100)
{
	this->pwindow = pwindow;
}

int PrefsBRenderPreroll::handle_event()
{
	pwindow->thread->preferences->brender_preroll = atol(get_text());
	return 1;
}


PrefsRenderFarm::PrefsRenderFarm(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, 
	y,
	pwindow->thread->preferences->use_renderfarm,
	_("Use render farm"))
{
	this->pwindow = pwindow;
}

int PrefsRenderFarm::handle_event()
{
	pwindow->thread->preferences->use_renderfarm = get_value();
	return 1;
}


PrefsForceUniprocessor::PrefsForceUniprocessor(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, 
	y, 
	pwindow->thread->preferences->force_uniprocessor,
	_("Force single processor use"))
{
	this->pwindow = pwindow;
}

int PrefsForceUniprocessor::handle_event()
{
	pwindow->thread->preferences->force_uniprocessor = get_value();
	return 1;
}


PrefsRenderFarmConsolidate::PrefsRenderFarmConsolidate(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, 
	y, 
	pwindow->thread->preferences->renderfarm_consolidate,
	_("Consolidate output files on completion"))
{
	this->pwindow = pwindow;
}

int PrefsRenderFarmConsolidate::handle_event()
{
	pwindow->thread->preferences->renderfarm_consolidate = get_value();
	return 1;
}


PrefsRenderFarmPort::PrefsRenderFarmPort(PreferencesWindow *pwindow, 
	PerformancePrefs *subwindow, 
	int x, 
	int y)
 : BC_TumbleTextBox(subwindow, 
	pwindow->thread->preferences->renderfarm_port,
	1,
	65535,
	x,
	y,
	100)
{
	this->pwindow = pwindow;
}

int PrefsRenderFarmPort::handle_event()
{
	pwindow->thread->preferences->renderfarm_port = atol(get_text());
	return 1;
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


PrefsRenderFarmJobs::PrefsRenderFarmJobs(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y)
 : BC_TumbleTextBox(subwindow, 
	pwindow->thread->preferences->renderfarm_job_count,
	1,
	100,
	x,
	y,
	100)
{
	this->pwindow = pwindow;
}

int PrefsRenderFarmJobs::handle_event()
{
	pwindow->thread->preferences->renderfarm_job_count = atol(get_text());
	return 1;
}


PrefsRenderFarmMountpoint::PrefsRenderFarmMountpoint(PreferencesWindow *pwindow, 
		PerformancePrefs *subwindow, 
		int x, 
		int y)
 : BC_TextBox(x, 
	y,
	100,
	1,
	pwindow->thread->preferences->renderfarm_mountpoint)
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsRenderFarmMountpoint::handle_event()
{
	strcpy(pwindow->thread->preferences->renderfarm_mountpoint, get_text());
	return 1;
}
