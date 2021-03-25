// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bclistboxitem.h"
#include "bctitle.h"
#include "cinelerra.h"
#include "titlewindow.h"
#include "bcfontentry.h"
#include "clip.h"

#include <string.h>
#include <libintl.h>
#include <wchar.h>

PLUGIN_THREAD_METHODS

TitleWindow::TitleWindow(TitleMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	770,
	360)
{
	int w1, y1;

	x = y = 10;

	timecodeformats.append(new BC_ListBoxItem(TIME_SECONDS__STR));
	timecodeformats.append(new BC_ListBoxItem(TIME_HMS__STR));
	timecodeformats.append(new BC_ListBoxItem(TIME_HMS2__STR));
	timecodeformats.append(new BC_ListBoxItem(TIME_HMS3__STR));
	timecodeformats.append(new BC_ListBoxItem(TIME_HMSF__STR));
	timecodeformats.append(new BC_ListBoxItem(TIME_FRAMES__STR));

	sizes.append(new BC_ListBoxItem("8"));
	sizes.append(new BC_ListBoxItem("9"));
	sizes.append(new BC_ListBoxItem("10"));
	sizes.append(new BC_ListBoxItem("11"));
	sizes.append(new BC_ListBoxItem("12"));
	sizes.append(new BC_ListBoxItem("13"));
	sizes.append(new BC_ListBoxItem("14"));
	sizes.append(new BC_ListBoxItem("16"));
	sizes.append(new BC_ListBoxItem("18"));
	sizes.append(new BC_ListBoxItem("20"));
	sizes.append(new BC_ListBoxItem("22"));
	sizes.append(new BC_ListBoxItem("24"));
	sizes.append(new BC_ListBoxItem("26"));
	sizes.append(new BC_ListBoxItem("28"));
	sizes.append(new BC_ListBoxItem("32"));
	sizes.append(new BC_ListBoxItem("36"));
	sizes.append(new BC_ListBoxItem("40"));
	sizes.append(new BC_ListBoxItem("48"));
	sizes.append(new BC_ListBoxItem("56"));
	sizes.append(new BC_ListBoxItem("64"));
	sizes.append(new BC_ListBoxItem("72"));
	sizes.append(new BC_ListBoxItem("100"));
	sizes.append(new BC_ListBoxItem("128"));

	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(NO_MOTION)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(BOTTOM_TO_TOP)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(TOP_TO_BOTTOM)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(RIGHT_TO_LEFT)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(LEFT_TO_RIGHT)));

// Construct font list
	ArrayList<BC_FontEntry*> *fontlist = plugin->get_fontlist();

	for(int i = 0; i < fontlist->total; i++)
	{
		int exists = 0;
		for(int j = 0; j < fonts.total; j++)
		{
			if(!strcasecmp(fonts.values[j]->get_text(), 
				fontlist->values[i]->displayname))
			{
				exists = 1;
				break;
			}
		}

		if(!exists)
			fonts.append(new BC_ListBoxItem(fontlist->values[i]->displayname));
	}

// Sort font list
	int done = 0;
	while(!done)
	{
		done = 1;
		for(int i = 0; i < fonts.total - 1; i++)
		{
			if(strcmp(fonts.values[i]->get_text(), fonts.values[i + 1]->get_text()) > 0)
			{
				BC_ListBoxItem *temp = fonts.values[i + 1];
				fonts.values[i + 1] = fonts.values[i];
				fonts.values[i] = temp;
				done = 0;
			}
		}
	}

	add_tool(font_title = new BC_Title(x, y, _("Font:")));
	font = new TitleFont(plugin, this, x, y + 20);
	x += 230;
	add_subwindow(font_tumbler = new TitleFontTumble(plugin, this, x, y + 20));
	x += 30;
	char string[BCTEXTLEN];
	add_tool(size_title = new BC_Title(x, y, _("Size:")));
	sprintf(string, "%d", plugin->config.size);
	size = new TitleSize(plugin, this, x, y + 20, string);
	x += 180;

	add_tool(style_title = new BC_Title(x, y, _("Style:")));
	add_tool(italic = new TitleItalic(plugin, this, x, y + 20));
	w1 = italic->get_w();
	add_tool(bold = new TitleBold(plugin, this, x, y + 50));
	w1 = MAX(bold->get_w(), w1);
	add_tool(stroke = new TitleStroke(plugin, this, x, y + 80));
	w1 = MAX(stroke->get_w(), w1);
	x += w1 + 10;
	add_tool(justify_title = new BC_Title(x, y, _("Justify:")));
	add_tool(left = new TitleLeft(plugin, this, x, y + 20));
	w1 = left->get_w();
	add_tool(center = new TitleCenter(plugin, this, x, y + 50));
	w1 = MAX(center->get_w(), w1);
	add_tool(right = new TitleRight(plugin, this, x, y + 80));
	w1 = MAX(right->get_w(), w1);

	x += w1 + 10;
	add_tool(top = new TitleTop(plugin, this, x, y + 20));
	add_tool(mid = new TitleMid(plugin, this, x, y + 50));
	add_tool(bottom= new TitleBottom(plugin, this, x, y + 80));

	y += 50;
	x = 10;

	add_tool(x_title = new BC_Title(x, y, _("X:")));
	title_x = new TitleX(plugin, this, x, y + 20);
	x += 90;

	add_tool(y_title = new BC_Title(x, y, _("Y:")));
	title_y = new TitleY(plugin, this, x, y + 20);
	x += 90;

	add_tool(motion_title = new BC_Title(x, y, _("Motion type:")));

	motion = new TitleMotion(plugin, this, x, y + 20);
	x += 150;

	add_tool(loop = new TitleLoop(plugin, x, y + 20));
	x += 100;

	x = 10;
	y += 50;

	add_tool(dropshadow_title = new BC_Title(x, y, _("Drop shadow:")));
	dropshadow = new TitleDropShadow(plugin, this, x, y + 20);
	x += MAX(dropshadow_title->get_w(), dropshadow->get_w()) + 10;

	add_tool(fadein_title = new BC_Title(x, y, _("Fade in (sec):")));
	add_tool(fade_in = new TitleFade(plugin, this, &plugin->config.fade_in, x, y + 20));
	x += MAX(fadein_title->get_w(), fade_in->get_w()) + 10;

	add_tool(fadeout_title = new BC_Title(x, y, _("Fade out (sec):")));
	add_tool(fade_out = new TitleFade(plugin, this, &plugin->config.fade_out, x, y + 20));
	x += MAX(fadeout_title->get_w(), fade_out->get_w()) + 10;

	add_tool(speed_title = new BC_Title(x, y, _("Speed:")));
	speed = new TitleSpeed(plugin, this, x, y + 20);
	x += MAX(speed_title->get_w(), speed->get_w()) + 10;

	add_tool(color_button = new TitleColorButton(plugin, this, x, y + 20));
	x += color_button->get_w() + 5;
	color_x = x;
	color_y = y + 16;
	color_thread = new TitleColorThread(plugin, this);

	x = 10;
	y += 50;
	y1 = y + 40;
	add_tool(text_title = new BC_Title(x, y1, _("Text:")));

	x += MAX(100, text_title->get_w()) + 10;
	add_tool(timecode = new TitleTimecode(plugin, x, y));

	BC_SubWindow *thisw;
	y1 = y + 30;
	add_tool(thisw = new BC_Title(x, y1, _("Format:")));
	w1 = thisw->get_w() + 5;
	timecodeformat = new TitleTimecodeFormat(plugin, this, x + w1, y1);

	x += w1 + timecodeformat->get_w() + 10;
	add_tool(strokewidth_title = new BC_Title(x, y, _("Outline width:")));
	stroke_width = new TitleStrokeW(plugin,
		this, 
		x + strokewidth_title->get_w() + 10,
		y);

	add_tool(color_stroke_button = new TitleColorStrokeButton(plugin,
		this, 
		x, 
		y1));
	color_stroke_x = x + color_stroke_button->get_w() + 10;
	color_stroke_y = y1;
	color_stroke_thread = new TitleColorStrokeThread(plugin, this);

	x = 10;
	y = y1 + 35;
	text = new TitleText(plugin,
		this, 
		x, 
		y, 
		get_w() - x - 10, 
		get_h() - y - 10);
	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_color();
}

TitleWindow::~TitleWindow()
{
	sizes.remove_all_objects();
	timecodeformats.remove_all_objects();
	delete timecodeformat;
	delete color_thread;
	delete color_stroke_thread;
	delete title_x;
	delete title_y;
}

void TitleWindow::previous_font()
{
	int current_font = font->get_number();
	current_font--;
	if(current_font < 0) current_font = fonts.total - 1;

	if(current_font < 0 || current_font >= fonts.total) return;

	for(int i = 0; i < fonts.total; i++)
	{
		fonts.values[i]->set_selected(i == current_font);
	}

	font->update(fonts.values[current_font]->get_text());
	strcpy(plugin->config.font, fonts.values[current_font]->get_text());
	plugin->send_configure_change();
}

void  TitleWindow::next_font()
{
	int current_font = font->get_number();
	current_font++;
	if(current_font >= fonts.total) current_font = 0;

	if(current_font < 0 || current_font >= fonts.total) return;

	for(int i = 0; i < fonts.total; i++)
	{
		fonts.values[i]->set_selected(i == current_font);
	}

	font->update(fonts.values[current_font]->get_text());
	strcpy(plugin->config.font, fonts.values[current_font]->get_text());
	plugin->send_configure_change();
}

void TitleWindow::update_color()
{
	set_color(plugin->config.color());
	draw_box(color_x, color_y, 100, 30);
	set_color(plugin->config.color_stroke());
	draw_box(color_stroke_x, color_stroke_y, 100, 30);
}

void TitleWindow::update_justification()
{
	left->update(plugin->config.hjustification == JUSTIFY_LEFT);
	center->update(plugin->config.hjustification == JUSTIFY_CENTER);
	right->update(plugin->config.hjustification == JUSTIFY_RIGHT);
	top->update(plugin->config.vjustification == JUSTIFY_TOP);
	mid->update(plugin->config.vjustification == JUSTIFY_MID);
	bottom->update(plugin->config.vjustification == JUSTIFY_BOTTOM);
}

void TitleWindow::update()
{
	title_x->update(plugin->config.x);
	title_y->update(plugin->config.y);
	italic->update(plugin->config.style & FONT_ITALIC);
	bold->update(plugin->config.style & FONT_BOLD);
	stroke->update(plugin->config.style & FONT_OUTLINE);
	size->update(plugin->config.size);
	timecode->update(plugin->config.timecode);
	timecodeformat->update(plugin->config.timecodeformat);
	motion->update(TitleMain::motion_to_text(plugin->config.motion_strategy));
	loop->update(plugin->config.loop);
	dropshadow->update(plugin->config.dropshadow);
	fade_in->update(plugin->config.fade_in);
	fade_out->update(plugin->config.fade_out);
	stroke_width->update(plugin->config.stroke_width);
	font->update(plugin->config.font);
	text->update(plugin->config.text);
	speed->update(plugin->config.pixels_per_second);
	update_justification();
	update_color();
	color_thread->update_gui(plugin->config.color_red,
		plugin->config.color_green, plugin->config.color_blue, 0);
	color_stroke_thread->update_gui(plugin->config.color_stroke_red,
		plugin->config.color_stroke_green, plugin->config.color_stroke_blue, 0);
}


TitleFontTumble::TitleFontTumble(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Tumbler(x, y)
{
	this->client = client;
	this->window = window;
}
void TitleFontTumble::handle_up_event()
{
	window->previous_font();
}

void TitleFontTumble::handle_down_event()
{
	window->next_font();
}


TitleBold::TitleBold(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_CheckBox(x, y, client->config.style & FONT_BOLD, _("Bold"))
{
	this->client = client;
	this->window = window;
}

int TitleBold::handle_event()
{
	client->config.style = (client->config.style & ~FONT_BOLD) | (get_value() ? FONT_BOLD : 0);
	client->send_configure_change();
	return 1;
}


TitleItalic::TitleItalic(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_CheckBox(x, y, client->config.style & FONT_ITALIC, _("Italic"))
{
	this->client = client;
	this->window = window;
}

int TitleItalic::handle_event()
{
	client->config.style = (client->config.style & ~FONT_ITALIC) | (get_value() ? FONT_ITALIC : 0);
	client->send_configure_change();
	return 1;
}


TitleStroke::TitleStroke(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_CheckBox(x, y, client->config.style & FONT_OUTLINE, _("Outline"))
{
	this->client = client;
	this->window = window;
}

int TitleStroke::handle_event()
{
	client->config.style = 
		(client->config.style & ~FONT_OUTLINE) | 
		(get_value() ? FONT_OUTLINE : 0);
	client->send_configure_change();
	return 1;
}

TitleSize::TitleSize(TitleMain *client, TitleWindow *window, int x, int y, char *text)
 : BC_PopupTextBox(window, 
		&window->sizes,
		text,
		x, 
		y, 
		100,
		300)
{
	this->client = client;
	this->window = window;
}

int TitleSize::handle_event()
{
	client->config.size = atoi(get_text());
	client->send_configure_change();
	return 1;
}

void TitleSize::update(int size)
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", size);
	BC_PopupTextBox::update(string);
}


TitleColorButton::TitleColorButton(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Color..."))
{
	this->client = client;
	this->window = window;
}

int TitleColorButton::handle_event()
{
	window->color_thread->start_window(client->config.color_red,
		client->config.color_green, client->config.color_blue, 0);
	return 1;
}


TitleColorStrokeButton::TitleColorStrokeButton(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Outline color..."))
{
	this->client = client;
	this->window = window;
}

int TitleColorStrokeButton::handle_event()
{
	window->color_stroke_thread->start_window(client->config.color_stroke_red,
		client->config.color_stroke_green,
		client->config.color_stroke_blue, 0);
	return 1;
}

TitleMotion::TitleMotion(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_PopupTextBox(window, 
		&window->paths,
		client->motion_to_text(client->config.motion_strategy),
		x, 
		y, 
		120,
		100)
{
	this->client = client;
	this->window = window;
}

int TitleMotion::handle_event()
{
	client->config.motion_strategy = client->text_to_motion(get_text());
	client->send_configure_change();
	return 1;
}


TitleLoop::TitleLoop(TitleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.loop, _("Loop"))
{
	this->client = client;
}

int TitleLoop::handle_event()
{
	client->config.loop = get_value();
	client->send_configure_change();
	return 1;
}


TitleTimecode::TitleTimecode(TitleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.timecode, _("Stamp timecode"))
{
	this->client = client;
}

int TitleTimecode::handle_event()
{
	client->config.timecode = get_value();
	client->send_configure_change();
	return 1;
}


TitleTimecodeFormat::TitleTimecodeFormat(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_PopupTextBox(window, 
		&window->timecodeformats,
		client->config.timecodeformat,
		x, 
		y, 
		140,
		160)
{
	this->client = client;
	this->window = window;
}

int TitleTimecodeFormat::handle_event()
{
	strcpy(client->config.timecodeformat, get_text());
	client->send_configure_change();
	return 1;
}


TitleFade::TitleFade(TitleMain *client, 
	TitleWindow *window, 
	double *value, 
	int x, 
	int y)
 : BC_TextBox(x, y, 90, 1, *value)
{
	this->client = client;
	this->window = window;
	this->value = value;
}

int TitleFade::handle_event()
{
	*value = atof(get_text());
	client->send_configure_change();
	return 1;
}


TitleFont::TitleFont(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_PopupTextBox(window, 
		&window->fonts,
		client->config.font,
		x, 
		y, 
		200,
		500)
{
	this->client = client;
	this->window = window;
}

int TitleFont::handle_event()
{
	strcpy(client->config.font, get_text());
	client->send_configure_change();
	return 1;
}


TitleText::TitleText(TitleMain *client, 
	TitleWindow *window, 
	int x, 
	int y, 
	int w, 
	int h)
 : BC_ScrollTextBox(window, 
		x, 
		y, 
		w,
		BC_TextBox::pixels_to_rows(window, MEDIUMFONT, h),
		client->config.wtext)
{
	this->client = client;
	this->window = window;
}

int TitleText::handle_event()
{
	wcscpy(client->config.wtext, get_wtext(&client->config.wtext_length));
	client->send_configure_change();
	return 1;
}


TitleDropShadow::TitleDropShadow(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window,
	client->config.dropshadow,
	0,
	1000,
	x, 
	y, 
	70)
{
	this->client = client;
	this->window = window;
}

int TitleDropShadow::handle_event()
{
	client->config.dropshadow = atoi(get_text());
	client->send_configure_change();
	return 1;
}


TitleX::TitleX(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window,
	(int)client->config.x,
	-MAX_FRAME_WIDTH,
	MAX_FRAME_WIDTH,
	x, 
	y, 
	60)
{
	this->client = client;
	this->window = window;
	set_precision(0);
}

int TitleX::handle_event()
{
	client->config.x = atof(get_text());
	client->send_configure_change();
	return 1;
}


TitleY::TitleY(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window,
	(int)client->config.y,
	-MAX_FRAME_HEIGHT,
	MAX_FRAME_HEIGHT,
	x, 
	y, 
	60)
{
	this->client = client;
	this->window = window;
	set_precision(0);
}

int TitleY::handle_event()
{
	client->config.y = atof(get_text());
	client->send_configure_change();
	return 1;
}


TitleStrokeW::TitleStrokeW(TitleMain *client, 
	TitleWindow *window, 
	int x, 
	int y)
 : BC_TumbleTextBox(window,
	client->config.stroke_width,
	0.5,
	24.0,
	x, 
	y, 
	60)
{
	this->client = client;
	this->window = window;
	set_precision(2);
}

int TitleStrokeW::handle_event()
{
	client->config.stroke_width = atof(get_text());
	client->send_configure_change();
	return 1;
}


TitleSpeed::TitleSpeed(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window,
	(int)client->config.pixels_per_second,
	1,
	2 * MAX_FRAME_HEIGHT,
	x, 
	y, 
	70)
{
	this->client = client;
	set_precision(0);
}

int TitleSpeed::handle_event()
{
	client->config.pixels_per_second = atof(get_text());
	client->send_configure_change();
	return 1;
}


TitleLeft::TitleLeft(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.hjustification == JUSTIFY_LEFT, _("Left"))
{
	this->client = client;
	this->window = window;
}

int TitleLeft::handle_event()
{
	client->config.hjustification = JUSTIFY_LEFT;
	client->send_configure_change();
	return 1;
}


TitleCenter::TitleCenter(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.hjustification == JUSTIFY_CENTER, _("Center"))
{
	this->client = client;
	this->window = window;
}

int TitleCenter::handle_event()
{
	client->config.hjustification = JUSTIFY_CENTER;
	client->send_configure_change();
	return 1;
}


TitleRight::TitleRight(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.hjustification == JUSTIFY_RIGHT, _("Right"))
{
	this->client = client;
	this->window = window;
}

int TitleRight::handle_event()
{
	client->config.hjustification = JUSTIFY_RIGHT;
	client->send_configure_change();
	return 1;
}


TitleTop::TitleTop(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.vjustification == JUSTIFY_TOP, _("Top"))
{
	this->client = client;
	this->window = window;
}

int TitleTop::handle_event()
{
	client->config.vjustification = JUSTIFY_TOP;
	client->send_configure_change();
	return 1;
}


TitleMid::TitleMid(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.vjustification == JUSTIFY_MID, _("Mid"))
{
	this->client = client;
	this->window = window;
}

int TitleMid::handle_event()
{
	client->config.vjustification = JUSTIFY_MID;
	client->send_configure_change();
	return 1;
}


TitleBottom::TitleBottom(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.vjustification == JUSTIFY_BOTTOM, _("Bottom"))
{
	this->client = client;
	this->window = window;
}

int TitleBottom::handle_event()
{
	client->config.vjustification = JUSTIFY_BOTTOM;
	client->send_configure_change();
	return 1;
}


TitleColorThread::TitleColorThread(TitleMain *client, TitleWindow *window)
 : ColorThread(0, "Text", client->new_picon())
{
	this->client = client;
	this->window = window;
}

int TitleColorThread::handle_new_color(int red, int green, int blue, int alpha)
{
	client->config.color_red = red;
	client->config.color_green = green;
	client->config.color_blue = blue;
	client->send_configure_change();
	return 1;
}


TitleColorStrokeThread::TitleColorStrokeThread(TitleMain *client, TitleWindow *window)
 : ColorThread(0, "Stroke", client->new_picon())
{
	this->client = client;
	this->window = window;
}

int TitleColorStrokeThread::handle_new_color(int red, int green, int blue, int alpha)
{
	client->config.color_stroke_red = red;
	client->config.color_stroke_green = green;
	client->config.color_stroke_blue = blue;
	client->send_configure_change();
	return 1;
}
