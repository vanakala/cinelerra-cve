#include "bcdisplayinfo.h"
#include "histogramengine.h"
#include "language.h"
#include "threshold.h"
#include "thresholdwindow.h"

#define COLOR_W 100
#define COLOR_H 30




ThresholdMin::ThresholdMin(ThresholdMain *plugin,
	ThresholdWindow *gui,
	int x,
	int y,
	int w)
 : BC_TumbleTextBox(gui, 
	plugin->config.min,
	HISTOGRAM_MIN,
	HISTOGRAM_MAX,
	x, 
	y, 
	w)
{
	this->plugin = plugin;
	this->gui = gui;
}


int ThresholdMin::handle_event()
{
	plugin->config.min = atof(get_text());
	gui->canvas->draw();
	plugin->send_configure_change();
	return 1;
}









ThresholdMax::ThresholdMax(ThresholdMain *plugin,
	ThresholdWindow *gui,
	int x,
	int y,
	int w)
 : BC_TumbleTextBox(gui, 
	plugin->config.max,
	HISTOGRAM_MIN,
	HISTOGRAM_MAX,
	x, 
	y, 
	w)
{
	this->plugin = plugin;
	this->gui = gui;
}

int ThresholdMax::handle_event()
{
	plugin->config.max = atof(get_text());
	gui->canvas->draw();
	plugin->send_configure_change();
	return 1;
}







ThresholdPlot::ThresholdPlot(ThresholdMain *plugin,
	int x,
	int y)
 : BC_CheckBox(x, y, plugin->config.plot, _("Plot histogram"))
{
	this->plugin = plugin;
}

int ThresholdPlot::handle_event()
{
	plugin->config.plot = get_value();
	
	plugin->send_configure_change();
	return 1;
}






ThresholdCanvas::ThresholdCanvas(ThresholdMain *plugin,
	ThresholdWindow *gui,
	int x,
	int y,
	int w,
	int h)
 : BC_SubWindow(x, y, w, h)
{
	this->plugin = plugin;
	this->gui = gui;
	state = NO_OPERATION;
}

int ThresholdCanvas::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		activate();
		state = DRAG_SELECTION;
		if(shift_down())
		{
			x1 = (int)((plugin->config.min - HISTOGRAM_MIN) / 
				(HISTOGRAM_MAX - HISTOGRAM_MIN) * 
				get_w());
			x2 = (int)((plugin->config.max - HISTOGRAM_MIN) /
				(HISTOGRAM_MAX - HISTOGRAM_MIN) *
				get_w());
			center_x = (x2 + x1) / 2;
			if(abs(get_cursor_x() - x1) < abs(get_cursor_x() - x2))
			{
				x1 = get_cursor_x();
				center_x = x2;
			}
			else
			{
				x2 = get_cursor_x();
				center_x = x1;
			}
		}
		else
		{
			x1 = x2 = center_x = get_cursor_x();
		}

		plugin->config.min = x1 * 
			(HISTOGRAM_MAX - HISTOGRAM_MIN) / 
			get_w() + 
			HISTOGRAM_MIN;
		plugin->config.max = x2 * 
			(HISTOGRAM_MAX - HISTOGRAM_MIN) / 
			get_w() + 
			HISTOGRAM_MIN;

		draw();
		return 1;
	}
	return 0;
}

int ThresholdCanvas::button_release_event()
{
	if(state == DRAG_SELECTION)
	{
		state = NO_OPERATION;
		return 1;
	}
	return 0;
}

int ThresholdCanvas::cursor_motion_event()
{
	if(state == DRAG_SELECTION)
	{
		if(get_cursor_x() > center_x)
		{
			x1 = center_x;
			x2 = get_cursor_x();
		}
		else
		{
			x1 = get_cursor_x();
			x2 = center_x;
		}

		plugin->config.min = x1 * 
			(HISTOGRAM_MAX - HISTOGRAM_MIN) / 
			get_w() + 
			HISTOGRAM_MIN;

		plugin->config.max = x2 * 
			(HISTOGRAM_MAX - HISTOGRAM_MIN) / 
			get_w() + 
			HISTOGRAM_MIN;

		gui->min->update(plugin->config.min);
		gui->max->update(plugin->config.max);
		draw();
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}

void ThresholdCanvas::draw()
{
	int max = 0;
	set_color(WHITE);
	draw_box(0, 0, get_w(), get_h());
	int border_x1 = (int)((0 - HISTOGRAM_MIN) / 
		(HISTOGRAM_MAX - HISTOGRAM_MIN) *
		get_w());
	int border_x2 = (int)((1.0 - HISTOGRAM_MIN) / 
		(HISTOGRAM_MAX - HISTOGRAM_MIN) *
		get_w());

	int x1 = (int)((plugin->config.min - HISTOGRAM_MIN) / 
		(HISTOGRAM_MAX - HISTOGRAM_MIN) * 
		get_w());
	int x2 = (int)((plugin->config.max - HISTOGRAM_MIN) /
		(HISTOGRAM_MAX - HISTOGRAM_MIN) *
		get_w());

	if(plugin->engine)
	{
		int64_t *array = plugin->engine->accum[HISTOGRAM_VALUE];


// Get normalizing factor
		for(int i = 0; i < get_w(); i++)
		{
			int division1 = i * HISTOGRAM_RANGE / get_w();
			int division2 = (i + 1) * HISTOGRAM_RANGE / get_w();
			int total = 0;
			for(int j = division1; j < division2; j++)
			{
				total += array[j];
			}
			if(total > max) max = total;
		}

		for(int i = 0; i < get_w(); i++)
		{
			int division1 = i * HISTOGRAM_RANGE / get_w();
			int division2 = (i + 1) * HISTOGRAM_RANGE / get_w();
			int total = 0;
			for(int j = division1; j < division2; j++)
			{
				total += array[j];
			}

			int pixels;
			if(max)
				pixels = total * get_h() / max;
			else
				pixels = 0;
			if(i >= x1 && i < x2)
			{
				set_color(BLUE);
				draw_line(i, 0, i, get_h() - pixels);
				set_color(WHITE);
				draw_line(i, get_h(), i, get_h() - pixels);
			}
			else
			{
				set_color(BLACK);
				draw_line(i, get_h(), i, get_h() - pixels);
			}
		}
	}
	else
	{


		set_color(BLUE);
		draw_box(x1, 0, x2 - x1, get_h());
	}

	set_color(RED);
	draw_line(border_x1, 0, border_x1, get_h());
	draw_line(border_x2, 0, border_x2, get_h());

	flash(1);
}






ThresholdLowColorButton::ThresholdLowColorButton(ThresholdMain *plugin, ThresholdWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Low Color"))
{
	this->plugin = plugin;
	this->window = window;
}

int ThresholdLowColorButton::handle_event()
{
	RGBA & color = plugin->config.low_color;
	window->low_color_thread->start_window(
		color.getRGB(),
		color.a);
	return 1;
}





ThresholdMidColorButton::ThresholdMidColorButton(ThresholdMain *plugin, ThresholdWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Mid Color"))
{
	this->plugin = plugin;
	this->window = window;
}

int ThresholdMidColorButton::handle_event()
{
	RGBA & color = plugin->config.mid_color;
	window->mid_color_thread->start_window(
		color.getRGB(),
		color.a);
	return 1;
}





ThresholdHighColorButton::ThresholdHighColorButton(ThresholdMain *plugin, ThresholdWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("High Color"))
{
	this->plugin = plugin;
	this->window = window;
}

int ThresholdHighColorButton::handle_event()
{
	RGBA & color = plugin->config.high_color;
	window->high_color_thread->start_window(
		color.getRGB(),
		color.a);
	return 1;
}





ThresholdLowColorThread::ThresholdLowColorThread(ThresholdMain *plugin, ThresholdWindow *window)
 : ColorThread(1, _("Low color"))
{
	this->plugin = plugin;
	this->window = window;
}

int ThresholdLowColorThread::handle_new_color(int output, int alpha)
{
	plugin->config.low_color.set(output, alpha);
	window->update_low_color();
	window->flush();
	plugin->send_configure_change();
}





ThresholdMidColorThread::ThresholdMidColorThread(ThresholdMain *plugin, ThresholdWindow *window)
 : ColorThread(1, _("Mid color"))
{
	this->plugin = plugin;
	this->window = window;
}

int ThresholdMidColorThread::handle_new_color(int output, int alpha)
{
	plugin->config.mid_color.set(output, alpha);
	window->update_mid_color();
	window->flush();
	plugin->send_configure_change();
}





ThresholdHighColorThread::ThresholdHighColorThread(ThresholdMain *plugin, ThresholdWindow *window)
 : ColorThread(1, _("High color"))
{
	this->plugin = plugin;
	this->window = window;
}

int ThresholdHighColorThread::handle_new_color(int output, int alpha)
{
	plugin->config.high_color.set(output, alpha);
	window->update_high_color();
	window->flush();
	plugin->send_configure_change();
}






ThresholdWindow::ThresholdWindow(ThresholdMain *plugin, int x, int y)
: BC_Window(plugin->gui_string, x, y, 450, 450, 450, 450, 0, 1)
{
	this->plugin = plugin;
	this->min = 0;
	this->max = 0;
	this->canvas = 0;
	this->plot = 0;
	this->low_color = 0;
	this->mid_color = 0;
	this->high_color = 0;
	this->low_color_thread = 0;
	this->mid_color_thread = 0;
	this->high_color_thread = 0;
}

ThresholdWindow::~ThresholdWindow()
{
}

int ThresholdWindow::create_objects()
{
	int x = 10;
	int y = 10;
	add_subwindow(canvas = new ThresholdCanvas(plugin,
		this,
		x,
		y,
		get_w() - x - 10,
		get_h() - 160));
	canvas->draw();
	y += canvas->get_h() + 10;

	add_subwindow(plot = new ThresholdPlot(plugin, x, y));
	y += plot->get_h() + 10;


	add_subwindow(low_color = new ThresholdLowColorButton(plugin, this, x, y));
	low_color_x = x + 10;
	low_color_y = y + low_color->get_h() + 10;
	x += low_color->get_w() + 10;

	add_subwindow(mid_color = new ThresholdMidColorButton(plugin, this, x, y));
	mid_color_x = x + 10;
	mid_color_y = y + mid_color->get_h() + 10;
	x += mid_color->get_w() + 10;

	add_subwindow(high_color = new ThresholdHighColorButton(plugin, this, x, y));
	high_color_x = x + 10;
	high_color_y = y + high_color->get_h() + 10;

	y += low_color->get_h() + COLOR_H + 10 + 10;

	x = 30;
	BC_Title * min_title;
	add_subwindow(min_title = new BC_Title(x, y, _("Min:")));
	x += min_title->get_w() + 10;
	min = new ThresholdMin(plugin,
		this,
		x,
		y,
		100);
	min->create_objects();
	min->set_increment(0.1);


	x = mid_color->get_x() + mid_color->get_w() / 2;
	BC_Title * max_title;
	add_subwindow(max_title = new BC_Title(x, y, _("Max:")));
	x += max_title->get_w() + 10;
	max = new ThresholdMax(plugin,
		this,
		x,
		y,
		100);
	max->create_objects();
	max->set_increment(0.1);


	low_color_thread  = new ThresholdLowColorThread(plugin, this);
	mid_color_thread  = new ThresholdMidColorThread(plugin, this);
	high_color_thread = new ThresholdHighColorThread(plugin, this);
	update_low_color();
	update_mid_color();
	update_high_color();

	show_window(1);
}

void ThresholdWindow::update_low_color()
{
	set_color(plugin->config.low_color.getRGB());
	draw_box(low_color_x, low_color_y, COLOR_W, COLOR_H);
	flash(low_color_x, low_color_y, COLOR_W, COLOR_H);
}

void ThresholdWindow::update_mid_color()
{
	set_color(plugin->config.mid_color.getRGB());
	draw_box(mid_color_x, mid_color_y, COLOR_W, COLOR_H);
	flash(mid_color_x, mid_color_y, COLOR_W, COLOR_H);
}

void ThresholdWindow::update_high_color()
{
	set_color(plugin->config.high_color.getRGB());
	draw_box(high_color_x, high_color_y, COLOR_W, COLOR_H);
	flash(high_color_x, high_color_y, COLOR_W, COLOR_H);
}



WINDOW_CLOSE_EVENT(ThresholdWindow)




PLUGIN_THREAD_OBJECT(ThresholdMain, ThresholdThread, ThresholdWindow)


