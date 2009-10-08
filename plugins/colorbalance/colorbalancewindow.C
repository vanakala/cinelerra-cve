
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

#include "bcdisplayinfo.h"
#include "colorbalancewindow.h"
#include "language.h"





PLUGIN_THREAD_OBJECT(ColorBalanceMain, ColorBalanceThread, ColorBalanceWindow)






ColorBalanceWindow::ColorBalanceWindow(ColorBalanceMain *client, int x, int y)
 : BC_Window(client->gui_string, x,
 	y,
	330, 
	250, 
	330, 
	250, 
	0, 
	0)
{ 
	this->client = client; 
}

ColorBalanceWindow::~ColorBalanceWindow()
{
}

int ColorBalanceWindow::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Color Balance")));
	y += 25;
	add_tool(new BC_Title(x, y, _("Cyan")));
	add_tool(cyan = new ColorBalanceSlider(client, &(client->config.cyan), x + 70, y));
	add_tool(new BC_Title(x + 270, y, _("Red")));
	y += 25;
	add_tool(new BC_Title(x, y, _("Magenta")));
	add_tool(magenta = new ColorBalanceSlider(client, &(client->config.magenta), x + 70, y));
	add_tool(new BC_Title(x + 270, y, _("Green")));
	y += 25;
	add_tool(new BC_Title(x, y, _("Yellow")));
	add_tool(yellow = new ColorBalanceSlider(client, &(client->config.yellow), x + 70, y));
	add_tool(new BC_Title(x + 270, y, _("Blue")));
	y += 25;
	add_tool(preserve = new ColorBalancePreserve(client, x + 70, y));
	y += preserve->get_h() + 10;
	add_tool(lock_params = new ColorBalanceLock(client, x + 70, y));
	y += lock_params->get_h() + 10;
	add_tool(new ColorBalanceWhite(client, this, x, y));
	y += lock_params->get_h() + 10;
	add_tool(new ColorBalanceReset(client, this, x, y));

	show_window();
	flush();
	return 0;
}

void ColorBalanceWindow::update()
{
	cyan->update((int64_t)client->config.cyan);
	magenta->update((int64_t)client->config.magenta);
	yellow->update((int64_t)client->config.yellow);
}

WINDOW_CLOSE_EVENT(ColorBalanceWindow)

ColorBalanceSlider::ColorBalanceSlider(ColorBalanceMain *client, 
	float *output, int x, int y)
 : BC_ISlider(x, 
 	y, 
	0, 
	200, 
	200,
	-1000, 
	1000, 
	(int)*output)
{
	this->client = client;
	this->output = output;
    old_value = *output;
}

ColorBalanceSlider::~ColorBalanceSlider()
{
}

int ColorBalanceSlider::handle_event()
{
	float difference = get_value() - *output;
	*output = get_value();
    client->synchronize_params(this, difference);
	client->send_configure_change();
	return 1;
}

char* ColorBalanceSlider::get_caption()
{
	float fraction = client->calculate_transfer(*output);
	sprintf(string, "%0.4f", fraction);
	return string;
}




ColorBalancePreserve::ColorBalancePreserve(ColorBalanceMain *client, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	client->config.preserve, 
	_("Preserve luminosity"))
{
	this->client = client;
}
ColorBalancePreserve::~ColorBalancePreserve()
{
}

int ColorBalancePreserve::handle_event()
{
	client->config.preserve = get_value();
	client->send_configure_change();
	return 1;
}

ColorBalanceLock::ColorBalanceLock(ColorBalanceMain *client, int x, int y)
 : BC_CheckBox(x, 
 	y, 
	client->config.lock_params, 
	_("Lock parameters"))
{
	this->client = client;
}
ColorBalanceLock::~ColorBalanceLock()
{
}

int ColorBalanceLock::handle_event()
{
	client->config.lock_params = get_value();
	client->send_configure_change();
	return 1;
}


ColorBalanceWhite::ColorBalanceWhite(ColorBalanceMain *plugin, 
	ColorBalanceWindow *gui,
	int x, 
	int y)
 : BC_GenericButton(x, y, _("White balance"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ColorBalanceWhite::handle_event()
{
// Get colorpicker value
	float red = plugin->get_red();
	float green = plugin->get_green();
	float blue = plugin->get_blue();
// // Get maximum value
// 	float max = MAX(red, green);
// 	max  = MAX(max, blue);
// // Get factors required to normalize other values to maximum
// 	float r_factor = max / red;
// 	float g_factor = max / green;
// 	float b_factor = max / blue;
// Get minimum value.  Can't use maximum because the sliders run out of room.
	float min = MIN(red, green);
	min = MIN(min, blue);
// Get factors required to normalize other values to minimum
	float r_factor = min / red;
	float g_factor = min / green;
	float b_factor = min / blue;

// Try normalizing to green like others do it
	r_factor = green / red;
	g_factor = 1.0;
	b_factor = green / blue;
// Convert factors to slider values
	plugin->config.cyan = plugin->calculate_slider(r_factor);
	plugin->config.magenta = plugin->calculate_slider(g_factor);
	plugin->config.yellow = plugin->calculate_slider(b_factor);
	gui->update();

	plugin->send_configure_change();
	return 1;
}


ColorBalanceReset::ColorBalanceReset(ColorBalanceMain *plugin, 
	ColorBalanceWindow *gui, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ColorBalanceReset::handle_event()
{
	plugin->config.cyan = 0;
	plugin->config.magenta = 0;
	plugin->config.yellow = 0;
	gui->update();
	plugin->send_configure_change();
	return 1;
}


