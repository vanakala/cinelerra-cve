// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "bcmenuitem.h"
#include "clip.h"
#include "language.h"
#include "motion.h"
#include "motionwindow.h"

PLUGIN_THREAD_METHODS

MotionWindow::MotionWindow(MotionMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	610, 
	650)
{
	int x1 = 10; 
	int x2 = 310;
	BC_Title *title;

	x = 10, y = 10;

	add_subwindow(global = new MotionGlobal(plugin,
		this,
		x1,
		y));

	add_subwindow(rotate = new MotionRotate(plugin,
		this,
		x2,
		y));
	y += 50;

	add_subwindow(title = new BC_Title(x1, 
		y, 
		_("Translation search radius:\n(W/H Percent of image)")));
	add_subwindow(global_range_w = new GlobalRange(plugin, 
		x1 + title->get_w() + 10, 
		y,
		&plugin->config.global_range_w));
	add_subwindow(global_range_h = new GlobalRange(plugin, 
		x1 + title->get_w() + 10 + global_range_w->get_w(), 
		y,
		&plugin->config.global_range_h));

	add_subwindow(title = new BC_Title(x2, 
		y, 
		_("Rotation search radius:\n(Degrees)")));
	add_subwindow(rotation_range = new RotationRange(plugin, 
		x2 + title->get_w() + 10, 
		y));

	y += 50;
	add_subwindow(title = new BC_Title(x1, 
		y, 
		_("Translation block size:\n(W/H Percent of image)")));
	add_subwindow(global_block_w = new BlockSize(plugin, 
		x1 + title->get_w() + 10, 
		y,
		&plugin->config.global_block_w));
	add_subwindow(global_block_h = new BlockSize(plugin, 
		x1 + title->get_w() + 10 + global_block_w->get_w(), 
		y,
		&plugin->config.global_block_h));

	add_subwindow(title = new BC_Title(x2, 
		y, 
		_("Rotation block size:\n(W/H Percent of image)")));
	add_subwindow(rotation_block_w = new BlockSize(plugin, 
		x2 + title->get_w() + 10, 
		y,
		&plugin->config.rotation_block_w));
	add_subwindow(rotation_block_h = new BlockSize(plugin, 
		x2 + title->get_w() + 10 + rotation_block_w->get_w(), 
		y,
		&plugin->config.rotation_block_h));

	y += 50;
	add_subwindow(title = new BC_Title(x1, y, _("Translation search steps:")));
	add_subwindow(global_search_positions = new GlobalSearchPositions(plugin, 
		x1 + title->get_w() + 10, 
		y, 
		80));

	add_subwindow(title = new BC_Title(x2, y, _("Rotation search steps:")));
	add_subwindow(rotation_search_positions = new RotationSearchPositions(plugin, 
		x2 + title->get_w() + 10, 
		y, 
		80));

	y += 50;
	add_subwindow(title = new BC_Title(x, y, _("Translation direction:")));
	add_subwindow(mode3 = new Mode3(plugin, 
		this, 
		x + title->get_w() + 10, 
		y));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Block X:")));
	add_subwindow(block_x = new MotionBlockX(plugin, 
		this, 
		x + title->get_w() + 10, 
		y));
	add_subwindow(block_x_text = new MotionBlockXText(plugin, 
		this, 
		x + title->get_w() + 10 + block_x->get_w() + 10, 
		y + 10));

	int xpmd;
	xpmd = x + title->get_w() + 10 + block_x->get_w() + 10 +
		block_x_text->get_w() + 30;
	add_subwindow(title = new BC_Title(xpmd, y + 10, _("Stabilize X gain:")));
	add_subwindow(stab_x_gain = new Motion_FloatTextBox(plugin,
		xpmd + title->get_w() + 10, y + 10, &plugin->config.stab_gain_x));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Block Y:")));
	add_subwindow(block_y = new MotionBlockY(plugin, 
		this, 
		x + title->get_w() + 10, 
		y));
	add_subwindow(block_y_text = new MotionBlockYText(plugin, 
		this, 
		x + title->get_w() + 10 + block_y->get_w() + 10, 
		y + 10));

	xpmd = x + title->get_w() + 10 + block_y->get_w() + 10 +
		block_y_text->get_w() + 30;
	add_subwindow(title = new BC_Title(xpmd, y + 10, _("Stabilize Y gain:")));
	add_subwindow(stab_y_gain = new Motion_FloatTextBox(plugin,
		xpmd + title->get_w() + 10, y + 10, &plugin->config.stab_gain_y));

	y += 50;
	add_subwindow(title = new BC_Title(x, y + 10, _("Maximum absolute offset:")));
	add_subwindow(magnitude = new MotionMagnitude(plugin, 
		x + title->get_w() + 10, 
		y));

	y += 40;
	add_subwindow(title = new BC_Title(x, y + 10, _("Settling speed:")));
	add_subwindow(return_speed = new MotionReturnSpeed(plugin,
		x + title->get_w() + 10, 
		y));

	y += 40;
	add_subwindow(vectors = new MotionDrawVectors(plugin,
		this,
		x,
		y));

	y += 40;
	add_subwindow(track_single = new TrackSingleFrame(plugin, 
		this,
		x, 
		y));
	add_subwindow(title = new BC_Title(x + track_single->get_w() + 20, 
		y, 
		_("Abs position (s):")));
	add_subwindow(track_frame_number = new TrackFrameNumber(plugin, 
		this,
		x + track_single->get_w() + title->get_w() + 20, 
		y));
	add_subwindow(addtrackedframeoffset = new AddTrackedFrameOffset(plugin,
		this,
		x + track_single->get_w() + title->get_w() + 10,
		y + track_frame_number->get_h() + 10));

	y += 20;
	add_subwindow(track_previous = new TrackPreviousFrame(plugin, 
		this,
		x, 
		y));

	y += 20;
	add_subwindow(previous_same = new PreviousFrameSameBlock(plugin, 
		this,
		x, 
		y));

	y += 40;

	add_subwindow(title = new BC_Title(x, y, _("Action:")));
	add_subwindow(mode1 = new Mode1(plugin, 
		this,
		x + title->get_w() + 10, 
		y));
	y += 30;

	add_subwindow(title = new BC_Title(x, y, _("Calculation:")));
	add_subwindow(mode2 = new Mode2(plugin, 
		this, 
		x + title->get_w() + 10, 
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void MotionWindow::update()
{
	char string[BCTEXTLEN];

	sprintf(string, "%d", plugin->config.global_positions);
	global_search_positions->set_text(string);

	sprintf(string, "%d", plugin->config.rotate_positions);
	rotation_search_positions->set_text(string);

	global_block_w->update(plugin->config.global_block_w);
	global_block_h->update(plugin->config.global_block_h);
	rotation_block_w->update(plugin->config.rotation_block_w);
	rotation_block_h->update(plugin->config.rotation_block_h);
	block_x->update(plugin->config.block_x);
	block_y->update(plugin->config.block_y);
	block_x_text->update(plugin->config.block_x);
	block_y_text->update(plugin->config.block_y);
	stab_x_gain->update(plugin->config.stab_gain_x);
	stab_y_gain->update(plugin->config.stab_gain_y);
	magnitude->update(plugin->config.magnitude);
	return_speed->update(plugin->config.return_speed);
	track_single->update(plugin->config.mode3 == MotionConfig::TRACK_SINGLE);
	track_frame_number->update(plugin->config.track_pts);
	track_previous->update(plugin->config.mode3 == MotionConfig::TRACK_PREVIOUS);
	previous_same->update(plugin->config.mode3 == MotionConfig::PREVIOUS_SAME_BLOCK);
	if(plugin->config.mode3 != MotionConfig::TRACK_SINGLE)
		track_frame_number->disable();
	else
		track_frame_number->enable();

	mode1->set_text(Mode1::to_text(plugin->config.mode1));
	mode2->set_text(Mode2::to_text(plugin->config.mode2));
	mode3->set_text(Mode3::to_text(plugin->config.horizontal_only, 
		plugin->config.vertical_only));
	update_mode();
}

void MotionWindow::update_mode()
{
	global_range_w->update(plugin->config.global_range_w,
		MIN_RADIUS,
		MAX_RADIUS);
	global_range_h->update(plugin->config.global_range_h,
		MIN_RADIUS,
		MAX_RADIUS);
	rotation_range->update(plugin->config.rotation_range,
		MIN_ROTATION,
		MAX_ROTATION);
	vectors->update(plugin->config.draw_vectors);
	global->update(plugin->config.global);
	rotate->update(plugin->config.rotate);
	addtrackedframeoffset->update(plugin->config.addtrackedframeoffset);
}


GlobalRange::GlobalRange(MotionMain *plugin, 
	int x, 
	int y,
	int *value)
 : BC_IPot(x, y,
	*value,
	MIN_RADIUS,
	MAX_RADIUS)
{
	this->plugin = plugin;
	this->value = value;
}

int GlobalRange::handle_event()
{
	*value = get_value();
	plugin->send_configure_change();
	return 1;
}


RotationRange::RotationRange(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, y,
	plugin->config.rotation_range,
	MIN_ROTATION,
	MAX_ROTATION)
{
	this->plugin = plugin;
}

int RotationRange::handle_event()
{
	plugin->config.rotation_range = get_value();
	plugin->send_configure_change();
	return 1;
}


BlockSize::BlockSize(MotionMain *plugin, 
	int x, 
	int y,
	int *value)
 : BC_IPot(x, y, *value,
	MIN_BLOCK,
	MAX_BLOCK)
{
	this->plugin = plugin;
	this->value = value;
}

int BlockSize::handle_event()
{
	*value = get_value();
	plugin->send_configure_change();
	return 1;
}


GlobalSearchPositions::GlobalSearchPositions(MotionMain *plugin, 
	int x, 
	int y,
	int w)
 : BC_PopupMenu(x,
	y,
	w,
	"64",
	POPUPMENU_USE_COORDS)
{
	this->plugin = plugin;
	add_item(new BC_MenuItem("64"));
	add_item(new BC_MenuItem("128"));
	add_item(new BC_MenuItem("256"));
	add_item(new BC_MenuItem("512"));
	add_item(new BC_MenuItem("1024"));
	add_item(new BC_MenuItem("2048"));
	add_item(new BC_MenuItem("4096"));
	add_item(new BC_MenuItem("8192"));
	add_item(new BC_MenuItem("16384"));
	add_item(new BC_MenuItem("32768"));
	add_item(new BC_MenuItem("65536"));
	add_item(new BC_MenuItem("131072"));
}

int GlobalSearchPositions::handle_event()
{
	plugin->config.global_positions = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}


RotationSearchPositions::RotationSearchPositions(MotionMain *plugin, 
	int x, 
	int y,
	int w)
 : BC_PopupMenu(x,
	y,
	w,
	"4",
	POPUPMENU_USE_COORDS)
{
	this->plugin = plugin;
	add_item(new BC_MenuItem("4"));
	add_item(new BC_MenuItem("8"));
	add_item(new BC_MenuItem("16"));
	add_item(new BC_MenuItem("32"));
}

int RotationSearchPositions::handle_event()
{
	plugin->config.rotate_positions = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}


MotionMagnitude::MotionMagnitude(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, y, plugin->config.magnitude, 0, 100)
{
	this->plugin = plugin;
}

int MotionMagnitude::handle_event()
{
	plugin->config.magnitude = get_value();
	plugin->send_configure_change();
	return 1;
}


MotionReturnSpeed::MotionReturnSpeed(MotionMain *plugin, 
	int x, 
	int y)
 : BC_IPot(x, y, plugin->config.return_speed, 0, 100)
{
	this->plugin = plugin;
}

int MotionReturnSpeed::handle_event()
{
	plugin->config.return_speed = get_value();
	plugin->send_configure_change();
	return 1;
}


AddTrackedFrameOffset::AddTrackedFrameOffset(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x, 
	y, 
	plugin->config.addtrackedframeoffset,
	_("Add (loaded) offset from tracked frame"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int AddTrackedFrameOffset::handle_event()
{
	plugin->config.addtrackedframeoffset = get_value();
	plugin->send_configure_change();
	return 1;
}


MotionGlobal::MotionGlobal(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x, 
	y, 
	plugin->config.global,
	_("Track translation"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int MotionGlobal::handle_event()
{
	plugin->config.global = get_value();
	plugin->send_configure_change();
	return 1;
}


MotionRotate::MotionRotate(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x, 
	y, 
	plugin->config.rotate,
	_("Track rotation"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int MotionRotate::handle_event()
{
	plugin->config.rotate = get_value();
	plugin->send_configure_change();
	return 1;
}


MotionBlockX::MotionBlockX(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_FPot(x,
	y,
	plugin->config.block_x,
	0.0,
	100.0)
{
	this->plugin = plugin;
	this->gui = gui;
}

int MotionBlockX::handle_event()
{
	plugin->config.block_x = get_value();
	gui->block_x_text->update(plugin->config.block_x);
	plugin->send_configure_change();
	return 1;
}

MotionBlockY::MotionBlockY(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_FPot(x, y,
	plugin->config.block_y,
	0.0,
	100.0)
{
	this->plugin = plugin;
	this->gui = gui;
}

int MotionBlockY::handle_event()
{
	plugin->config.block_y = get_value();
	gui->block_y_text->update(plugin->config.block_y);
	plugin->send_configure_change();
	return 1;
}

MotionBlockXText::MotionBlockXText(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_TextBox(x,
	y,
	75,
	1,
	plugin->config.block_x)
{
	this->plugin = plugin;
	this->gui = gui;
	set_precision(4);
}

int MotionBlockXText::handle_event()
{
	plugin->config.block_x = atof(get_text());
	gui->block_x->update(plugin->config.block_x);
	plugin->send_configure_change();
	return 1;
}


MotionBlockYText::MotionBlockYText(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_TextBox(x,
	y,
	75,
	1,
	plugin->config.block_y)
{
	this->plugin = plugin;
	this->gui = gui;
	set_precision(4);
}

int MotionBlockYText::handle_event()
{
	plugin->config.block_y = atof(get_text());
	gui->block_y->update(plugin->config.block_y);
	plugin->send_configure_change();
	return 1;
}


MotionDrawVectors::MotionDrawVectors(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_CheckBox(x,
	y,
	plugin->config.draw_vectors,
	_("Draw vectors"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int MotionDrawVectors::handle_event()
{
	plugin->config.draw_vectors = get_value();
	plugin->send_configure_change();
	return 1;
}


TrackSingleFrame::TrackSingleFrame(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.mode3 == MotionConfig::TRACK_SINGLE, 
	_("Track single frame"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int TrackSingleFrame::handle_event()
{
	plugin->config.mode3 = MotionConfig::TRACK_SINGLE;
	gui->track_previous->update(0);
	gui->previous_same->update(0);
	gui->track_frame_number->enable();
	plugin->send_configure_change();
	return 1;
}


Motion_FloatTextBox::Motion_FloatTextBox(MotionMain *plugin,
	int x, int y, double *property)
 : BC_TextBox(x, y, 100, 1, *property)
{
	this->plugin = plugin;
	this->property = property;
}

int Motion_FloatTextBox::handle_event()
{
	*property = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


TrackFrameNumber::TrackFrameNumber(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_TextBox(x, y, 100, 1, plugin->config.track_pts)
{
	this->plugin = plugin;
	this->gui = gui;
	if(plugin->config.mode3 != MotionConfig::TRACK_SINGLE) disable();
}

int TrackFrameNumber::handle_event()
{
	plugin->config.track_pts = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


TrackPreviousFrame::TrackPreviousFrame(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.mode3 == MotionConfig::TRACK_PREVIOUS, 
	_("Track previous frame"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int TrackPreviousFrame::handle_event()
{
	plugin->config.mode3 = MotionConfig::TRACK_PREVIOUS;
	gui->track_single->update(0);
	gui->previous_same->update(0);
	gui->track_frame_number->disable();
	plugin->send_configure_change();
	return 1;
}


PreviousFrameSameBlock::PreviousFrameSameBlock(MotionMain *plugin, 
	MotionWindow *gui,
	int x, 
	int y)
 : BC_Radial(x, 
	y,
	plugin->config.mode3 == MotionConfig::PREVIOUS_SAME_BLOCK, 
	_("Previous frame same block"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int PreviousFrameSameBlock::handle_event()
{
	plugin->config.mode3 = MotionConfig::PREVIOUS_SAME_BLOCK;
	gui->track_single->update(0);
	gui->track_previous->update(0);
	gui->track_frame_number->disable();
	plugin->send_configure_change();
	return 1;
}

Mode1::Mode1(MotionMain *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x, 
	y, 
	calculate_w(gui),
	to_text(plugin->config.mode1))
{
	this->plugin = plugin;
	this->gui = gui;
	add_item(new BC_MenuItem(to_text(MotionConfig::TRACK)));
	add_item(new BC_MenuItem(to_text(MotionConfig::TRACK_PIXEL)));
	add_item(new BC_MenuItem(to_text(MotionConfig::STABILIZE)));
	add_item(new BC_MenuItem(to_text(MotionConfig::STABILIZE_PIXEL)));
	add_item(new BC_MenuItem(to_text(MotionConfig::NOTHING)));
}

int Mode1::handle_event()
{
	plugin->config.mode1 = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

int Mode1::from_text(const char *text)
{
	if(!strcmp(text, _("Track Subpixel"))) return MotionConfig::TRACK;
	if(!strcmp(text, _("Track Pixel"))) return MotionConfig::TRACK_PIXEL;
	if(!strcmp(text, _("Stabilize Subpixel"))) return MotionConfig::STABILIZE;
	if(!strcmp(text, _("Stabilize Pixel"))) return MotionConfig::STABILIZE_PIXEL;
	if(!strcmp(text, _("Do Nothing"))) return MotionConfig::NOTHING;
	return MotionConfig::NOTHING;
}

const char* Mode1::to_text(int mode)
{
	switch(mode)
	{
	case MotionConfig::TRACK:
		return _("Track Subpixel");
		break;
	case MotionConfig::TRACK_PIXEL:
		return _("Track Pixel");
		break;
	case MotionConfig::STABILIZE:
		return _("Stabilize Subpixel");
		break;
	case MotionConfig::STABILIZE_PIXEL:
		return _("Stabilize Pixel");
		break;
	case MotionConfig::NOTHING:
		return _("Do Nothing");
		break;
	}
	return _("Unknown");
}

int Mode1::calculate_w(MotionWindow *gui)
{
	int result = 0;

	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionConfig::TRACK)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionConfig::TRACK_PIXEL)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionConfig::STABILIZE)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionConfig::STABILIZE_PIXEL)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionConfig::NOTHING)));
	return result + 50;
}


Mode2::Mode2(MotionMain *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x, 
	y,
	calculate_w(gui),
	to_text(plugin->config.mode2))
{
	this->plugin = plugin;
	this->gui = gui;
	add_item(new BC_MenuItem(to_text(MotionConfig::NO_CALCULATE)));
	add_item(new BC_MenuItem(to_text(MotionConfig::RECALCULATE)));
	add_item(new BC_MenuItem(to_text(MotionConfig::SAVE)));
	add_item(new BC_MenuItem(to_text(MotionConfig::LOAD)));
}

int Mode2::handle_event()
{
	plugin->config.mode2 = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

int Mode2::from_text(const char *text)
{
	if(!strcmp(text, _("Don't Calculate"))) return MotionConfig::NO_CALCULATE;
	if(!strcmp(text, _("Recalculate"))) return MotionConfig::RECALCULATE;
	if(!strcmp(text, _("Save coords to /tmp"))) return MotionConfig::SAVE;
	if(!strcmp(text, _("Load coords from /tmp"))) return MotionConfig::LOAD;
	return MotionConfig::NO_CALCULATE;
}

const char* Mode2::to_text(int mode)
{
	switch(mode)
	{
	case MotionConfig::NO_CALCULATE:
		return _("Don't Calculate");

	case MotionConfig::RECALCULATE:
		return _("Recalculate");

	case MotionConfig::SAVE:
		return _("Save coords to /tmp");

	case MotionConfig::LOAD:
		return _("Load coords from /tmp");
	}
	return _("Unknown");
}

int Mode2::calculate_w(MotionWindow *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionConfig::NO_CALCULATE)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionConfig::RECALCULATE)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionConfig::SAVE)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(MotionConfig::LOAD)));
	return result + 50;
}


Mode3::Mode3(MotionMain *plugin, MotionWindow *gui, int x, int y)
 : BC_PopupMenu(x, 
	y,
	calculate_w(gui),
	to_text(plugin->config.horizontal_only, plugin->config.vertical_only))
{
	this->plugin = plugin;
	this->gui = gui;
	add_item(new BC_MenuItem(to_text(1, 0)));
	add_item(new BC_MenuItem(to_text(0, 1)));
	add_item(new BC_MenuItem(to_text(0, 0)));
}

int Mode3::handle_event()
{
	from_text(&plugin->config.horizontal_only, &plugin->config.vertical_only, get_text());
	plugin->send_configure_change();
	return 1;
}

void Mode3::from_text(int *horizontal_only, int *vertical_only, const char *text)
{
	*horizontal_only = 0;
	*vertical_only = 0;
	if(!strcmp(text, to_text(1, 0))) *horizontal_only = 1;
	if(!strcmp(text, to_text(0, 1))) *vertical_only = 1;
}

const char* Mode3::to_text(int horizontal_only, int vertical_only)
{
	if(horizontal_only) return _("Horizontal only");
	if(vertical_only) return _("Vertical only");
	return _("Both");
}

int Mode3::calculate_w(MotionWindow *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(1, 0)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(0, 1)));
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(0, 0)));
	return result + 50;
}
