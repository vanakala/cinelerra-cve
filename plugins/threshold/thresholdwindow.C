#include "bcdisplayinfo.h"
#include "histogramengine.h"
#include "language.h"
#include "threshold.h"
#include "thresholdwindow.h"





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













ThresholdWindow::ThresholdWindow(ThresholdMain *plugin, int x, int y)
: BC_Window(plugin->gui_string, x, y, 440, 350, 440, 350, 0, 1)
{
	this->plugin = plugin;
}

ThresholdWindow::~ThresholdWindow()
{
}

int ThresholdWindow::create_objects()
{
	int x1 = 10, x = 10;
	int y = 10;
	add_subwindow(canvas = new ThresholdCanvas(plugin,
		this,
		x,
		y,
		get_w() - x - 10,
		get_h() - 100));
	canvas->draw();
	y += canvas->get_h() + 10;

	add_subwindow(new BC_Title(x, y, _("Min:")));
	x += 50;
	min = new ThresholdMin(plugin,
		this,
		x,
		y,
		100);
	min->create_objects();
	min->set_increment(0.1);

	x += 200;
	add_subwindow(new BC_Title(x, y, _("Max:")));
	x += 50;
	max = new ThresholdMax(plugin,
		this,
		x,
		y,
		100);
	max->create_objects();
	max->set_increment(0.1);

	y += max->get_h();
	x = x1;

	add_subwindow(plot = new ThresholdPlot(plugin, x, y));

	show_window(1);
}

WINDOW_CLOSE_EVENT(ThresholdWindow)





PLUGIN_THREAD_OBJECT(ThresholdMain, ThresholdThread, ThresholdWindow)


