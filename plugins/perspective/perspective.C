#include "cursors.h"
#include "perspective.h"






REGISTER_PLUGIN(PerspectiveMain)



PerspectiveConfig::PerspectiveConfig()
{
	x1 = 0;
	y1 = 0;
	x2 = 100;
	y2 = 0;
	x3 = 100;
	y3 = 100;
	x4 = 0;
	y4 = 100;
	mode = PERSPECTIVE;
	window_w = 400;
	window_h = 450;
	current_point = 0;
	forward = 1;
}

int PerspectiveConfig::equivalent(PerspectiveConfig &that)
{
	return 
		EQUIV(x1, that.x1) &&
		EQUIV(y1, that.y1) &&
		EQUIV(x2, that.x2) &&
		EQUIV(y2, that.y2) &&
		EQUIV(x3, that.x3) &&
		EQUIV(y3, that.y3) &&
		EQUIV(x4, that.x4) &&
		EQUIV(y4, that.y4) &&
		mode == that.mode &&
		forward == that.forward;
}

void PerspectiveConfig::copy_from(PerspectiveConfig &that)
{
	x1 = that.x1;
	y1 = that.y1;
	x2 = that.x2;
	y2 = that.y2;
	x3 = that.x3;
	y3 = that.y3;
	x4 = that.x4;
	y4 = that.y4;
	mode = that.mode;
	window_w = that.window_w;
	window_h = that.window_h;
	current_point = that.current_point;
	forward = that.forward;
}

void PerspectiveConfig::interpolate(PerspectiveConfig &prev, 
	PerspectiveConfig &next, 
	long prev_frame, 
	long next_frame, 
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->x1 = prev.x1 * prev_scale + next.x1 * next_scale;
	this->y1 = prev.y1 * prev_scale + next.y1 * next_scale;
	this->x2 = prev.x2 * prev_scale + next.x2 * next_scale;
	this->y2 = prev.y2 * prev_scale + next.y2 * next_scale;
	this->x3 = prev.x3 * prev_scale + next.x3 * next_scale;
	this->y3 = prev.y3 * prev_scale + next.y3 * next_scale;
	this->x4 = prev.x4 * prev_scale + next.x4 * next_scale;
	this->y4 = prev.y4 * prev_scale + next.y4 * next_scale;
	mode = prev.mode;
	forward = prev.forward;
}









PLUGIN_THREAD_OBJECT(PerspectiveMain, PerspectiveThread, PerspectiveWindow)



PerspectiveWindow::PerspectiveWindow(PerspectiveMain *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x,
	y,
	plugin->config.window_w, 
	plugin->config.window_h, 
	plugin->config.window_w,
	plugin->config.window_h,
	0, 
	1)
{
//printf("PerspectiveWindow::PerspectiveWindow 1 %d %d\n", plugin->config.window_w, plugin->config.window_h);
	this->plugin = plugin; 
}

PerspectiveWindow::~PerspectiveWindow()
{
}

int PerspectiveWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(canvas = new PerspectiveCanvas(plugin, 
		x, 
		y, 
		get_w() - 20, 
		get_h() - 140));
	canvas->set_cursor(CROSS_CURSOR);
	y += canvas->get_h() + 10;
	add_subwindow(new BC_Title(x, y, "Current X:"));
	x += 80;
	this->x = new PerspectiveCoord(this, 
		plugin, 
		x, 
		y, 
		plugin->get_current_x(),
		1);
	this->x->create_objects();
	x += 140;
	add_subwindow(new BC_Title(x, y, "Y:"));
	x += 20;
	this->y = new PerspectiveCoord(this, 
		plugin, 
		x, 
		y, 
		plugin->get_current_y(),
		0);
	this->y->create_objects();
	y += 30;
	x = 10;
	add_subwindow(new PerspectiveReset(plugin, x, y));
	x += 100;
	add_subwindow(mode_perspective = new PerspectiveMode(plugin, 
		x, 
		y, 
		PerspectiveConfig::PERSPECTIVE,
		"Perspective"));
	x += 120;
	add_subwindow(mode_sheer = new PerspectiveMode(plugin, 
		x, 
		y, 
		PerspectiveConfig::SHEER,
		"Sheer"));
	x = 110;
	y += 30;
	add_subwindow(mode_stretch = new PerspectiveMode(plugin, 
		x, 
		y, 
		PerspectiveConfig::STRETCH,
		"Stretch"));
	update_canvas();
	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, "Perspective direction:"));
	x += 170;
	add_subwindow(forward = new PerspectiveDirection(plugin, 
		x, 
		y, 
		1,
		"Forward"));
	x += 100;
	add_subwindow(reverse = new PerspectiveDirection(plugin, 
		x, 
		y, 
		0,
		"Reverse"));

	show_window();
	flush();
	return 0;
}

int PerspectiveWindow::close_event()
{
// Set result to 1 to indicate a plugin side close
	set_done(1);
	return 1;
}

int PerspectiveWindow::resize_event(int w, int h)
{
	return 1;
}

void PerspectiveWindow::update_canvas()
{
	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	int x1, y1, x2, y2, x3, y3, x4, y4;
	calculate_canvas_coords(x1, y1, x2, y2, x3, y3, x4, y4);

// printf("PerspectiveWindow::update_canvas %d,%d %d,%d %d,%d %d,%d\n",
// x1,
// y1,
// x2,
// y2,
// x3,
// y3,
// x4,
// y4);
	canvas->set_color(BLACK);

#define DIVISIONS 10
	for(int i = 0; i <= DIVISIONS; i++)
	{
// latitude
		canvas->draw_line(
			x1 + (x4 - x1) * i / DIVISIONS,
			y1 + (y4 - y1) * i / DIVISIONS,
			x2 + (x3 - x2) * i / DIVISIONS,
			y2 + (y3 - y2) * i / DIVISIONS);
// longitude
		canvas->draw_line(
			x1 + (x2 - x1) * i / DIVISIONS,
			y1 + (y2 - y1) * i / DIVISIONS,
			x4 + (x3 - x4) * i / DIVISIONS,
			y4 + (y3 - y4) * i / DIVISIONS);
	}

// Corners
#define RADIUS 5
	if(plugin->config.current_point == 0)
		canvas->draw_disc(x1 - RADIUS, y1 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x1 - RADIUS, y1 - RADIUS, RADIUS * 2, RADIUS * 2);

	if(plugin->config.current_point == 1)
		canvas->draw_disc(x2 - RADIUS, y2 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x2 - RADIUS, y2 - RADIUS, RADIUS * 2, RADIUS * 2);

	if(plugin->config.current_point == 2)
		canvas->draw_disc(x3 - RADIUS, y3 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x3 - RADIUS, y3 - RADIUS, RADIUS * 2, RADIUS * 2);

	if(plugin->config.current_point == 3)
		canvas->draw_disc(x4 - RADIUS, y4 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x4 - RADIUS, y4 - RADIUS, RADIUS * 2, RADIUS * 2);

	canvas->flash();
	canvas->flush();
}

void PerspectiveWindow::update_mode()
{
	mode_perspective->update(plugin->config.mode == PerspectiveConfig::PERSPECTIVE);
	mode_sheer->update(plugin->config.mode == PerspectiveConfig::SHEER);
	mode_stretch->update(plugin->config.mode == PerspectiveConfig::STRETCH);
	forward->update(plugin->config.forward);
	reverse->update(!plugin->config.forward);
}

void PerspectiveWindow::update_coord()
{
	x->update(plugin->get_current_x());
	y->update(plugin->get_current_y());
}

void PerspectiveWindow::calculate_canvas_coords(int &x1, 
	int &y1, 
	int &x2, 
	int &y2, 
	int &x3, 
	int &y3, 
	int &x4, 
	int &y4)
{
	int w = canvas->get_w() - 1;
	int h = canvas->get_h() - 1;
	if(plugin->config.mode == PerspectiveConfig::PERSPECTIVE ||
		plugin->config.mode == PerspectiveConfig::STRETCH)
	{
		x1 = (int)(plugin->config.x1 * w / 100);
		y1 = (int)(plugin->config.y1 * h / 100);
		x2 = (int)(plugin->config.x2 * w / 100);
		y2 = (int)(plugin->config.y2 * h / 100);
		x3 = (int)(plugin->config.x3 * w / 100);
		y3 = (int)(plugin->config.y3 * h / 100);
		x4 = (int)(plugin->config.x4 * w / 100);
		y4 = (int)(plugin->config.y4 * h / 100);
	}
	else
	{
		x1 = (int)(plugin->config.x1 * w) / 100;
		y1 = 0;
		x2 = x1 + w;
		y2 = 0;
		x4 = (int)(plugin->config.x4 * w) / 100;
		y4 = h;
		x3 = x4 + w;
		y3 = h;
	}
}




PerspectiveCanvas::PerspectiveCanvas(PerspectiveMain *plugin, 
	int x, 
	int y, 
	int w,
	int h)
 : BC_SubWindow(x, y, w, h, 0xffffff)
{
	this->plugin = plugin;
	state = PerspectiveCanvas::NONE;
}



#define DISTANCE(x1, y1, x2, y2) \
(sqrt(((x2) - (x1)) * ((x2) - (x1)) + ((y2) - (y1)) * ((y2) - (y1))))

int PerspectiveCanvas::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
// Set current point
		int x1, y1, x2, y2, x3, y3, x4, y4;
		int cursor_x = get_cursor_x();
		int cursor_y = get_cursor_y();
		plugin->thread->window->calculate_canvas_coords(x1, y1, x2, y2, x3, y3, x4, y4);

		float distance1 = DISTANCE(cursor_x, cursor_y, x1, y1);
		float distance2 = DISTANCE(cursor_x, cursor_y, x2, y2);
		float distance3 = DISTANCE(cursor_x, cursor_y, x3, y3);
		float distance4 = DISTANCE(cursor_x, cursor_y, x4, y4);
// printf("PerspectiveCanvas::button_press_event %f %d %d %d %d\n", 
// distance3,
// cursor_x,
// cursor_y,
// x3,
// y3);
		float min = distance1;
		plugin->config.current_point = 0;
		if(distance2 < min)
		{
			min = distance2;
			plugin->config.current_point = 1;
		}
		if(distance3 < min)
		{
			min = distance3;
			plugin->config.current_point = 2;
		}
		if(distance4 < min)
		{
			min = distance4;
			plugin->config.current_point = 3;
		}

		if(plugin->config.mode == PerspectiveConfig::SHEER)
		{
			if(plugin->config.current_point == 1)
				plugin->config.current_point = 0;
			else
			if(plugin->config.current_point == 2)
				plugin->config.current_point = 3;
		}
		start_cursor_x = cursor_x;
		start_cursor_y = cursor_y;

		if(alt_down() || shift_down())
		{
			if(alt_down())
				state = PerspectiveCanvas::DRAG_FULL;
			else
				state = PerspectiveCanvas::ZOOM;

// Get starting positions
			start_x1 = plugin->config.x1;
			start_y1 = plugin->config.y1;
			start_x2 = plugin->config.x2;
			start_y2 = plugin->config.y2;
			start_x3 = plugin->config.x3;
			start_y3 = plugin->config.y3;
			start_x4 = plugin->config.x4;
			start_y4 = plugin->config.y4;
		}
		else
		{
			state = PerspectiveCanvas::DRAG;

// Get starting positions
			start_x1 = plugin->get_current_x();
			start_y1 = plugin->get_current_y();
		}
		plugin->thread->window->update_coord();
		plugin->thread->window->update_canvas();
		return 1;
	}

	return 0;
}

int PerspectiveCanvas::button_release_event()
{
	if(state != PerspectiveCanvas::NONE)
	{
		state = PerspectiveCanvas::NONE;
		return 1;
	}
	return 0;
}

int PerspectiveCanvas::cursor_motion_event()
{
	if(state != PerspectiveCanvas::NONE)
	{
		int w = get_w() - 1;
		int h = get_h() - 1;
		if(state == PerspectiveCanvas::DRAG)
		{
			plugin->set_current_x((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x1);
			plugin->set_current_y((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y1);
		}
		else
		if(state == PerspectiveCanvas::DRAG_FULL)
		{
			plugin->config.x1 = ((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x1);
			plugin->config.y1 = ((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y1);
			plugin->config.x2 = ((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x2);
			plugin->config.y2 = ((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y2);
			plugin->config.x3 = ((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x3);
			plugin->config.y3 = ((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y3);
			plugin->config.x4 = ((float)(get_cursor_x() - start_cursor_x) / w * 100 + start_x4);
			plugin->config.y4 = ((float)(get_cursor_y() - start_cursor_y) / h * 100 + start_y4);
		}
		else
		if(state == PerspectiveCanvas::ZOOM)
		{
			float center_x = (start_x1 +
				start_x2 +
				start_x3 +
				start_x4) / 4;
			float center_y = (start_y1 +
				start_y2 +
				start_y3 +
				start_y4) / 4;
			float zoom = (float)(get_cursor_y() - start_cursor_y + 640) / 640;
			plugin->config.x1 = center_x + (start_x1 - center_x) * zoom;
			plugin->config.y1 = center_y + (start_y1 - center_y) * zoom;
			plugin->config.x2 = center_x + (start_x2 - center_x) * zoom;
			plugin->config.y2 = center_y + (start_y2 - center_y) * zoom;
			plugin->config.x3 = center_x + (start_x3 - center_x) * zoom;
			plugin->config.y3 = center_y + (start_y3 - center_y) * zoom;
			plugin->config.x4 = center_x + (start_x4 - center_x) * zoom;
			plugin->config.y4 = center_y + (start_y4 - center_y) * zoom;
		}
		plugin->thread->window->update_canvas();
		plugin->thread->window->update_coord();
		plugin->send_configure_change();
		return 1;
	}

	return 0;
}






PerspectiveCoord::PerspectiveCoord(PerspectiveWindow *gui,
	PerspectiveMain *plugin, 
	int x, 
	int y,
	float value,
	int is_x)
 : BC_TumbleTextBox(gui, value, (float)0, (float)100, x, y, 100)
{
	this->plugin = plugin;
	this->is_x = is_x;
}

int PerspectiveCoord::handle_event()
{
	if(is_x)
		plugin->set_current_x(atof(get_text()));
	else
		plugin->set_current_y(atof(get_text()));
	plugin->thread->window->update_canvas();
	plugin->send_configure_change();
	return 1;
}








PerspectiveReset::PerspectiveReset(PerspectiveMain *plugin, 
	int x, 
	int y)
 : BC_GenericButton(x, y, "Reset")
{
	this->plugin = plugin;
}
int PerspectiveReset::handle_event()
{
	plugin->config.x1 = 0;
	plugin->config.y1 = 0;
	plugin->config.x2 = 100;
	plugin->config.y2 = 0;
	plugin->config.x3 = 100;
	plugin->config.y3 = 100;
	plugin->config.x4 = 0;
	plugin->config.y4 = 100;
	plugin->thread->window->update_canvas();
	plugin->thread->window->update_coord();
	plugin->send_configure_change();
	return 1;
}











PerspectiveMode::PerspectiveMode(PerspectiveMain *plugin, 
	int x, 
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->config.mode == value, text)
{
	this->plugin = plugin;
	this->value = value;
}
int PerspectiveMode::handle_event()
{
	plugin->config.mode = value;
	plugin->thread->window->update_mode();
	plugin->thread->window->update_canvas();
	plugin->send_configure_change();
	return 1;
}




PerspectiveDirection::PerspectiveDirection(PerspectiveMain *plugin, 
	int x, 
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->config.forward == value, text)
{
	this->plugin = plugin;
	this->value = value;
}
int PerspectiveDirection::handle_event()
{
	plugin->config.forward = value;
	plugin->thread->window->update_mode();
	plugin->send_configure_change();
	return 1;
}












PerspectiveMain::PerspectiveMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	temp = 0;
}

PerspectiveMain::~PerspectiveMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	if(engine) delete engine;
	if(temp) delete temp;
}

char* PerspectiveMain::plugin_title() { return "Perspective"; }
int PerspectiveMain::is_realtime() { return 1; }


NEW_PICON_MACRO(PerspectiveMain)

SHOW_GUI_MACRO(PerspectiveMain, PerspectiveThread)

SET_STRING_MACRO(PerspectiveMain)

RAISE_WINDOW_MACRO(PerspectiveMain)

LOAD_CONFIGURATION_MACRO(PerspectiveMain, PerspectiveConfig)



void PerspectiveMain::update_gui()
{
	if(thread)
	{
//printf("PerspectiveMain::update_gui 1\n");
		thread->window->lock_window();
//printf("PerspectiveMain::update_gui 2\n");
		load_configuration();
		thread->window->update_coord();
		thread->window->update_mode();
		thread->window->update_canvas();
		thread->window->unlock_window();
//printf("PerspectiveMain::update_gui 3\n");
	}
}


int PerspectiveMain::load_defaults()
{
	char directory[1024], string[1024];
// set the default directory
	sprintf(directory, "%sperspective.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	config.x1 = defaults->get("X1", config.x1);
	config.x2 = defaults->get("X2", config.x2);
	config.x3 = defaults->get("X3", config.x3);
	config.x4 = defaults->get("X4", config.x4);
	config.y1 = defaults->get("Y1", config.y1);
	config.y2 = defaults->get("Y2", config.y2);
	config.y3 = defaults->get("Y3", config.y3);
	config.y4 = defaults->get("Y4", config.y4);

	config.mode = defaults->get("MODE", config.mode);
	config.forward = defaults->get("FORWARD", config.forward);
	config.window_w = defaults->get("WINDOW_W", config.window_w);
	config.window_h = defaults->get("WINDOW_H", config.window_h);
	return 0;
}


int PerspectiveMain::save_defaults()
{
	defaults->update("X1", config.x1);
	defaults->update("X2", config.x2);
	defaults->update("X3", config.x3);
	defaults->update("X4", config.x4);
	defaults->update("Y1", config.y1);
	defaults->update("Y2", config.y2);
	defaults->update("Y3", config.y3);
	defaults->update("Y4", config.y4);

	defaults->update("MODE", config.mode);
	defaults->update("FORWARD", config.forward);
	defaults->update("WINDOW_W", config.window_w);
	defaults->update("WINDOW_H", config.window_h);
	defaults->save();
	return 0;
}



void PerspectiveMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("PERSPECTIVE");

	output.tag.set_property("X1", config.x1);
	output.tag.set_property("X2", config.x2);
	output.tag.set_property("X3", config.x3);
	output.tag.set_property("X4", config.x4);
	output.tag.set_property("Y1", config.y1);
	output.tag.set_property("Y2", config.y2);
	output.tag.set_property("Y3", config.y3);
	output.tag.set_property("Y4", config.y4);

	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("FORWARD", config.forward);
	output.tag.set_property("WINDOW_W", config.window_w);
	output.tag.set_property("WINDOW_H", config.window_h);
	output.append_tag();
	output.terminate_string();
}

void PerspectiveMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("PERSPECTIVE"))
			{
				config.x1 = input.tag.get_property("X1", config.x1);
				config.x2 = input.tag.get_property("X2", config.x2);
				config.x3 = input.tag.get_property("X3", config.x3);
				config.x4 = input.tag.get_property("X4", config.x4);
				config.y1 = input.tag.get_property("Y1", config.y1);
				config.y2 = input.tag.get_property("Y2", config.y2);
				config.y3 = input.tag.get_property("Y3", config.y3);
				config.y4 = input.tag.get_property("Y4", config.y4);

				config.mode = input.tag.get_property("MODE", config.mode);
				config.forward = input.tag.get_property("FORWARD", config.forward);
				config.window_w = input.tag.get_property("WINDOW_W", config.window_w);
				config.window_h = input.tag.get_property("WINDOW_H", config.window_h);
			}
		}
	}
}

float PerspectiveMain::get_current_x()
{
	switch(config.current_point)
	{
		case 0:
			return config.x1;
			break;
		case 1:
			return config.x2;
			break;
		case 2:
			return config.x3;
			break;
		case 3:
			return config.x4;
			break;
	}
}

float PerspectiveMain::get_current_y()
{
	switch(config.current_point)
	{
		case 0:
			return config.y1;
			break;
		case 1:
			return config.y2;
			break;
		case 2:
			return config.y3;
			break;
		case 3:
			return config.y4;
			break;
	}
}

void PerspectiveMain::set_current_x(float value)
{
	switch(config.current_point)
	{
		case 0:
			config.x1 = value;
			break;
		case 1:
			config.x2 = value;
			break;
		case 2:
			config.x3 = value;
			break;
		case 3:
			config.x4 = value;
			break;
	}
}

void PerspectiveMain::set_current_y(float value)
{
	switch(config.current_point)
	{
		case 0:
			config.y1 = value;
			break;
		case 1:
			config.y2 = value;
			break;
		case 2:
			config.y3 = value;
			break;
		case 3:
			config.y4 = value;
			break;
	}
}



int PerspectiveMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	int need_reconfigure = load_configuration();


	if(!engine) engine = new PerspectiveEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);

	this->input = input_ptr;
	this->output = output_ptr;

	if( EQUIV(config.x1, 0)   && EQUIV(config.y1, 0) &&
		EQUIV(config.x2, 100) && EQUIV(config.y2, 0) &&
		EQUIV(config.x3, 100) && EQUIV(config.y3, 100) &&
		EQUIV(config.x4, 0)   && EQUIV(config.y4, 100))
	{
		if(input_ptr->get_rows()[0] != output_ptr->get_rows()[0])
			output_ptr->copy_from(input_ptr);
		return 1;
	}

	int w = input_ptr->get_w();
	int h = input_ptr->get_h();
	int color_model = input_ptr->get_color_model();

	if(temp && 
		config.mode == PerspectiveConfig::STRETCH &&
		(temp->get_w() != w * OVERSAMPLE ||
			temp->get_h() != h * OVERSAMPLE))
	{
		delete temp;
		temp = 0;
	}
	else
	if(temp &&
		(config.mode == PerspectiveConfig::PERSPECTIVE ||
		config.mode == PerspectiveConfig::SHEER) &&
		(temp->get_w() != w ||
			temp->get_h() != h))
	{
		delete temp;
		temp = 0;
	}

	if(config.mode == PerspectiveConfig::STRETCH)
	{
		if(!temp)
		{
			temp = new VFrame(0,
					w * OVERSAMPLE,
					h * OVERSAMPLE,
					color_model);
		}
		temp->clear_frame();
	}

	if(config.mode == PerspectiveConfig::PERSPECTIVE ||
		config.mode == PerspectiveConfig::SHEER)
	{
		if(input_ptr->get_rows()[0] == output_ptr->get_rows()[0])
		{
			if(!temp) 
			{
				temp = new VFrame(0,
					w,
					h,
					color_model);
			}
			temp->copy_from(input);
			input = temp;
		}
		output->clear_frame();
	}



	engine->process_packages();




// Resample

	if(config.mode == PerspectiveConfig::STRETCH)
	{
#define RESAMPLE(type, components, chroma_offset) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		type *out_row = (type*)output->get_rows()[i]; \
		type *in_row1 = (type*)temp->get_rows()[i * OVERSAMPLE]; \
		type *in_row2 = (type*)temp->get_rows()[i * OVERSAMPLE + 1]; \
		for(int j = 0; j < w; j++) \
		{ \
			out_row[0] = (in_row1[0] +  \
					in_row1[components] +  \
					in_row2[0] +  \
					in_row2[components]) /  \
				OVERSAMPLE /  \
				OVERSAMPLE; \
			out_row[1] = ((in_row1[1] +  \
						in_row1[components + 1] +  \
						in_row2[1] +  \
						in_row2[components + 1]) -  \
					chroma_offset *  \
					OVERSAMPLE *  \
					OVERSAMPLE) /  \
				OVERSAMPLE /  \
				OVERSAMPLE + \
				chroma_offset; \
			out_row[2] = ((in_row1[2] +  \
						in_row1[components + 2] +  \
						in_row2[2] +  \
						in_row2[components + 2]) -  \
					chroma_offset *  \
					OVERSAMPLE *  \
					OVERSAMPLE) /  \
				OVERSAMPLE /  \
				OVERSAMPLE + \
				chroma_offset; \
			if(components == 4) \
			{ \
				out_row[2] = (in_row1[3] +  \
						in_row1[components + 3] +  \
						in_row2[3] +  \
						in_row2[components + 3]) /  \
					OVERSAMPLE /  \
					OVERSAMPLE; \
			} \
			out_row += components; \
			in_row1 += components * OVERSAMPLE; \
			in_row2 += components * OVERSAMPLE; \
		} \
	} \
}

		switch(input_ptr->get_color_model())
		{
			case BC_RGB888:
				RESAMPLE(unsigned char, 3, 0)
				break;
			case BC_RGBA8888:
				RESAMPLE(unsigned char, 4, 0)
				break;
			case BC_YUV888:
				RESAMPLE(unsigned char, 3, 0x80)
				break;
			case BC_YUVA8888:
				RESAMPLE(unsigned char, 4, 0x80)
				break;
			case BC_RGB161616:
				RESAMPLE(uint16_t, 3, 0)
				break;
			case BC_RGBA16161616:
				RESAMPLE(uint16_t, 4, 0)
				break;
			case BC_YUV161616:
				RESAMPLE(uint16_t, 3, 0x8000)
				break;
			case BC_YUVA16161616:
				RESAMPLE(uint16_t, 4, 0x8000)
				break;
		}
	}

	return 1;
}











PerspectiveMatrix::PerspectiveMatrix()
{
	bzero(values, sizeof(values));
}

void PerspectiveMatrix::identity()
{
	bzero(values, sizeof(values));
	values[0][0] = 1;
	values[1][1] = 1;
	values[2][2] = 1;
}

void PerspectiveMatrix::translate(double x, double y)
{
	double g = values[2][0];
	double h = values[2][1];
	double i = values[2][2];
	values[0][0] += x * g;
	values[0][1] += x * h;
	values[0][2] += x * i;
	values[1][0] += y * g;
	values[1][1] += y * h;
	values[1][2] += y * i;
}

void PerspectiveMatrix::scale(double x, double y)
{
	values[0][0] *= x;
	values[0][1] *= x;
	values[0][2] *= x;

	values[1][0] *= y;
	values[1][1] *= y;
	values[1][2] *= y;
}

void PerspectiveMatrix::multiply(PerspectiveMatrix *dst)
{
	int i, j;
	PerspectiveMatrix tmp;
	double t1, t2, t3;

  	for (i = 0; i < 3; i++)
    {
    	t1 = values[i][0];
    	t2 = values[i][1];
    	t3 = values[i][2];
    	for (j = 0; j < 3; j++)
		{
			tmp.values[i][j]  = t1 * dst->values[0][j];
			tmp.values[i][j] += t2 * dst->values[1][j];
			tmp.values[i][j] += t3 * dst->values[2][j];
		}
    }
	dst->copy_from(&tmp);
}

double PerspectiveMatrix::determinant()
{
	double determinant;

	determinant  = 
        values[0][0] * (values[1][1] * values[2][2] - values[1][2] * values[2][1]);
	determinant -= 
        values[1][0] * (values[0][1] * values[2][2] - values[0][2] * values[2][1]);
	determinant += 
        values[2][0] * (values[0][1] * values[1][2] - values[0][2] * values[1][1]);

	return determinant;
}

void PerspectiveMatrix::invert(PerspectiveMatrix *dst)
{
	double det_1;

	det_1 = determinant();

	if(det_1 == 0.0)
      	return;

	det_1 = 1.0 / det_1;

	dst->values[0][0] =   
      (values[1][1] * values[2][2] - values[1][2] * values[2][1]) * det_1;

	dst->values[1][0] = 
      - (values[1][0] * values[2][2] - values[1][2] * values[2][0]) * det_1;

	dst->values[2][0] =   
      (values[1][0] * values[2][1] - values[1][1] * values[2][0]) * det_1;

	dst->values[0][1] = 
      - (values[0][1] * values[2][2] - values[0][2] * values[2][1] ) * det_1;

	dst->values[1][1] = 
      (values[0][0] * values[2][2] - values[0][2] * values[2][0]) * det_1;

	dst->values[2][1] = 
      - (values[0][0] * values[2][1] - values[0][1] * values[2][0]) * det_1;

	dst->values[0][2] =
      (values[0][1] * values[1][2] - values[0][2] * values[1][1]) * det_1;

	dst->values[1][2] = 
      - (values[0][0] * values[1][2] - values[0][2] * values[1][0]) * det_1;

	dst->values[2][2] = 
      (values[0][0] * values[1][1] - values[0][1] * values[1][0]) * det_1;
}

void PerspectiveMatrix::copy_from(PerspectiveMatrix *src)
{
	memcpy(&values[0][0], &src->values[0][0], sizeof(values));
}

void PerspectiveMatrix::transform_point(float x, 
	float y, 
	float *newx, 
	float *newy)
{
	double w;

	w = values[2][0] * x + values[2][1] * y + values[2][2];

	if (w == 0.0)
    	w = 1.0;
	else
    	w = 1.0 / w;

	*newx = (values[0][0] * x + values[0][1] * y + values[0][2]) * w;
	*newy = (values[1][0] * x + values[1][1] * y + values[1][2]) * w;
}

void PerspectiveMatrix::dump()
{
	printf("PerspectiveMatrix::dump\n");
	printf("%f %f %f\n", values[0][0], values[0][1], values[0][2]);
	printf("%f %f %f\n", values[1][0], values[1][1], values[1][2]);
	printf("%f %f %f\n", values[2][0], values[2][1], values[2][2]);
}





PerspectivePackage::PerspectivePackage()
 : LoadPackage()
{
}




PerspectiveUnit::PerspectiveUnit(PerspectiveEngine *server, 
	PerspectiveMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}









void PerspectiveUnit::calculate_matrix(
	double in_x1,
	double in_y1,
	double in_x2,
	double in_y2,
	double out_x1,
	double out_y1,
	double out_x2,
	double out_y2,
	double out_x3,
	double out_y3,
	double out_x4,
	double out_y4,
	PerspectiveMatrix *result)
{
	PerspectiveMatrix matrix;
	double scalex;
	double scaley;

	scalex = scaley = 1.0;

	if((in_x2 - in_x1) > 0)
      	scalex = 1.0 / (double)(in_x2 - in_x1);

	if((in_y2 - in_y1) > 0)
      	scaley = 1.0 / (double)(in_y2 - in_y1);

/* Determine the perspective transform that maps from
 * the unit cube to the transformed coordinates
 */
    double dx1, dx2, dx3, dy1, dy2, dy3;
    double det1, det2;

    dx1 = out_x2 - out_x4;
    dx2 = out_x3 - out_x4;
    dx3 = out_x1 - out_x2 + out_x4 - out_x3;

    dy1 = out_y2 - out_y4;
    dy2 = out_y3 - out_y4;
    dy3 = out_y1 - out_y2 + out_y4 - out_y3;
// printf("PerspectiveUnit::calculate_matrix %f %f %f %f %f %f\n",
// dx1,
// dx2,
// dx3,
// dy1,
// dy2,
// dy3
// );

/*  Is the mapping affine?  */
    if((dx3 == 0.0) && (dy3 == 0.0))
    {
        matrix.values[0][0] = out_x2 - out_x1;
        matrix.values[0][1] = out_x4 - out_x2;
        matrix.values[0][2] = out_x1;
        matrix.values[1][0] = out_y2 - out_y1;
        matrix.values[1][1] = out_y4 - out_y2;
        matrix.values[1][2] = out_y1;
        matrix.values[2][0] = 0.0;
        matrix.values[2][1] = 0.0;
    }
    else
    {
        det1 = dx3 * dy2 - dy3 * dx2;
        det2 = dx1 * dy2 - dy1 * dx2;
        matrix.values[2][0] = det1 / det2;
        det1 = dx1 * dy3 - dy1 * dx3;
        det2 = dx1 * dy2 - dy1 * dx2;
        matrix.values[2][1] = det1 / det2;

        matrix.values[0][0] = out_x2 - out_x1 + matrix.values[2][0] * out_x2;
        matrix.values[0][1] = out_x3 - out_x1 + matrix.values[2][1] * out_x3;
        matrix.values[0][2] = out_x1;

        matrix.values[1][0] = out_y2 - out_y1 + matrix.values[2][0] * out_y2;
        matrix.values[1][1] = out_y3 - out_y1 + matrix.values[2][1] * out_y3;
        matrix.values[1][2] = out_y1;
    }

    matrix.values[2][2] = 1.0;

// printf("PerspectiveUnit::calculate_matrix 1 %f %f\n", dx3, dy3);
// matrix.dump();

	result->identity();
	result->translate(-in_x1, -in_y1);
	result->scale(scalex, scaley);
	matrix.multiply(result);
// double test[3][3] = { { 0.0896, 0.0, 0.0 },
// 				  { 0.0, 0.0896, 0.0 },
// 				  { -0.00126, 0.0, 1.0 } };
// memcpy(&result->values[0][0], test, sizeof(test));
// printf("PerspectiveUnit::calculate_matrix 4 %p\n", result);
// result->dump();


}

float PerspectiveUnit::transform_cubic(float dx,
                               float jm1,
                               float j,
                               float jp1,
                               float jp2)
{
/* Catmull-Rom - not bad */
  	float result = ((( ( - jm1 + 3 * j - 3 * jp1 + jp2 ) * dx +
            	       ( 2 * jm1 - 5 * j + 4 * jp1 - jp2 ) ) * dx +
            	       ( - jm1 + jp1 ) ) * dx + (j + j) ) / 2.0;

  	return result;
}


void PerspectiveUnit::process_package(LoadPackage *package)
{
	PerspectivePackage *pkg = (PerspectivePackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	int maxw = w - 1;
	int maxh = h - 1;

// Calculate real coords
	float out_x1, out_y1, out_x2, out_y2, out_x3, out_y3, out_x4, out_y4;
	if(plugin->config.mode == PerspectiveConfig::STRETCH ||
		plugin->config.mode == PerspectiveConfig::PERSPECTIVE)
	{
		out_x1 = (float)plugin->config.x1 * w / 100;
		out_y1 = (float)plugin->config.y1 * h / 100;
		out_x2 = (float)plugin->config.x2 * w / 100;
		out_y2 = (float)plugin->config.y2 * h / 100;
		out_x3 = (float)plugin->config.x3 * w / 100;
		out_y3 = (float)plugin->config.y3 * h / 100;
		out_x4 = (float)plugin->config.x4 * w / 100;
		out_y4 = (float)plugin->config.y4 * h / 100;
	}
	else
	{
		out_x1 = (float)plugin->config.x1 * w / 100;
		out_y1 = 0;
		out_x2 = out_x1 + w;
		out_y2 = 0;
		out_x4 = (float)plugin->config.x4 * w / 100;
		out_y4 = h;
		out_x3 = out_x4 + w;
		out_y3 = h;
	}



	if(plugin->config.mode == PerspectiveConfig::PERSPECTIVE ||
		plugin->config.mode == PerspectiveConfig::SHEER)
	{
		PerspectiveMatrix matrix;
		float temp;
		temp = out_x4;
		out_x4 = out_x3;
		out_x3 = temp;
		temp = out_y4;
		out_y4 = out_y3;
		out_y3 = temp;



// printf("PerspectiveUnit::process_package 10 %f %f %f %f %f %f %f %f\n", 
// out_x1,
// out_y1,
// out_x2,
// out_y2,
// out_x3,
// out_y3,
// out_x4,
// out_y4);



		calculate_matrix(
			0,
			0,
			w,
			h,
			out_x1,
			out_y1,
			out_x2,
			out_y2,
			out_x3,
			out_y3,
			out_x4,
			out_y4,
			&matrix);

		int interpolate = 1;
		int reverse = !plugin->config.forward;
		float tx, ty, tw;
		float xinc, yinc, winc;
		PerspectiveMatrix m, im;
		float ttx, tty;
		int itx, ity;
		int tx1, ty1, tx2, ty2;

		if(reverse)
		{
			m.copy_from(&matrix);
			m.invert(&im);
			matrix.copy_from(&im);
		}
		else
		{
			matrix.invert(&m);
		}

		float dx1, dy1;
		float dx2, dy2;
		float dx3, dy3;
		float dx4, dy4;
		matrix.transform_point(0, 0, &dx1, &dy1);
		matrix.transform_point(w, 0, &dx2, &dy2);
		matrix.transform_point(0, h, &dx3, &dy3);
		matrix.transform_point(w, h, &dx4, &dy4);

#define ROUND(x) ((int)((x > 0) ? (x) + 0.5 : (x) - 0.5))
#define MIN4(a,b,c,d) MIN(MIN(MIN(a,b),c),d)
#define MAX4(a,b,c,d) MAX(MAX(MAX(a,b),c),d)
#define CUBIC_ROW(in_row, chroma_offset) \
	transform_cubic(dx, \
		in_row[col1_offset] - chroma_offset, \
		in_row[col2_offset] - chroma_offset, \
		in_row[col3_offset] - chroma_offset, \
		in_row[col4_offset] - chroma_offset)

    	tx1 = ROUND(MIN4(dx1, dx2, dx3, dx4));
    	ty1 = ROUND(MIN4(dy1, dy2, dy3, dy4));

    	tx2 = ROUND(MAX4(dx1, dx2, dx3, dx4));
    	ty2 = ROUND(MAX4(dy1, dy2, dy3, dy4));

		CLAMP(ty1, pkg->y1, pkg->y2);
		CLAMP(ty2, pkg->y1, pkg->y2);

		xinc = m.values[0][0];
		yinc = m.values[1][0];
		winc = m.values[2][0];
//printf("PerspectiveUnit::process_package 1 %d %d %d %d %f %f\n", tx1, ty1, tx2, ty2, out_x4, out_y4);


#define TRANSFORM(components, type, chroma_offset, max) \
{ \
	type **in_rows = (type**)plugin->input->get_rows(); \
	for(int y = ty1; y < ty2; y++) \
	{ \
		type *out_row = (type*)plugin->output->get_rows()[y]; \
 \
		if(!interpolate) \
		{ \
        	tx = xinc * (tx1 + 0.5) + m.values[0][1] * (y + 0.5) + m.values[0][2]; \
        	ty = yinc * (tx1 + 0.5) + m.values[1][1] * (y + 0.5) + m.values[1][2]; \
        	tw = winc * (tx1 + 0.5) + m.values[2][1] * (y + 0.5) + m.values[2][2]; \
		} \
      	else \
        { \
        	tx = xinc * tx1 + m.values[0][1] * y + m.values[0][2]; \
        	ty = yinc * tx1 + m.values[1][1] * y + m.values[1][2]; \
        	tw = winc * tx1 + m.values[2][1] * y + m.values[2][2]; \
        } \
 \
 \
		out_row += tx1 * components; \
		for(int x = tx1; x < tx2; x++) \
		{ \
/* Normalize homogeneous coords */ \
			if(tw == 0.0) \
			{ \
				ttx = 0.0; \
				tty = 0.0; \
			} \
			else \
			if(tw != 1.0) \
			{ \
				ttx = tx / tw; \
				tty = ty / tw; \
			} \
			else \
			{ \
				ttx = tx; \
				tty = ty; \
			} \
			itx = (int)ttx; \
			ity = (int)tty; \
 \
/* Set destination pixels */ \
			if(!interpolate && x >= 0 && x < w) \
			{ \
				if(itx >= 0 && itx < w && \
					ity >= 0 && ity < h && \
					x >= 0 && x < w) \
				{ \
					type *src = in_rows[ity] + itx * components; \
					*out_row++ = *src++; \
					*out_row++ = *src++; \
					*out_row++ = *src++; \
					if(components == 4) *out_row++ = *src; \
				} \
				else \
/* Fill with chroma */ \
				{ \
					*out_row++ = 0; \
					*out_row++ = chroma_offset; \
					*out_row++ = chroma_offset; \
					if(components == 4) *out_row++ = 0; \
				} \
			} \
			else \
/* Bicubic algorithm */ \
			if(interpolate && x >= 0 && x < w) \
			{ \
				if ((itx + 2) >= 0 && (itx - 1) < w && \
                  	(ity + 2) >= 0 && (ity - 1) < h) \
                { \
                	float dx, dy; \
 \
/* the fractional error */ \
                	dx = ttx - itx; \
                	dy = tty - ity; \
 \
/* Row and column offsets in cubic block */ \
					int col1 = itx - 1; \
					int col2 = itx; \
					int col3 = itx + 1; \
					int col4 = itx + 2; \
					int row1 = ity - 1; \
					int row2 = ity; \
					int row3 = ity + 1; \
					int row4 = ity + 2; \
					CLAMP(col1, 0, maxw); \
					CLAMP(col2, 0, maxw); \
					CLAMP(col3, 0, maxw); \
					CLAMP(col4, 0, maxw); \
					CLAMP(row1, 0, maxh); \
					CLAMP(row2, 0, maxh); \
					CLAMP(row3, 0, maxh); \
					CLAMP(row4, 0, maxh); \
					int col1_offset = col1 * components; \
					int col2_offset = col2 * components; \
					int col3_offset = col3 * components; \
					int col4_offset = col4 * components; \
					type *row1_ptr = in_rows[row1]; \
					type *row2_ptr = in_rows[row2]; \
					type *row3_ptr = in_rows[row3]; \
					type *row4_ptr = in_rows[row4]; \
					int r, g, b, a; \
 \
					r = (int)(transform_cubic(dy, \
                    		 CUBIC_ROW(row1_ptr, 0x0), \
                    		 CUBIC_ROW(row2_ptr, 0x0), \
                    		 CUBIC_ROW(row3_ptr, 0x0), \
                    		 CUBIC_ROW(row4_ptr, 0x0)) +  \
							 0.5); \
 \
					row1_ptr++; \
					row2_ptr++; \
					row3_ptr++; \
					row4_ptr++; \
					g = ROUND(transform_cubic(dy, \
                    		 CUBIC_ROW(row1_ptr, chroma_offset), \
                    		 CUBIC_ROW(row2_ptr, chroma_offset), \
                    		 CUBIC_ROW(row3_ptr, chroma_offset), \
                    		 CUBIC_ROW(row4_ptr, chroma_offset))); \
					g += chroma_offset; \
 \
					row1_ptr++; \
					row2_ptr++; \
					row3_ptr++; \
					row4_ptr++; \
					b = ROUND(transform_cubic(dy, \
                    		 CUBIC_ROW(row1_ptr, chroma_offset), \
                    		 CUBIC_ROW(row2_ptr, chroma_offset), \
                    		 CUBIC_ROW(row3_ptr, chroma_offset), \
                    		 CUBIC_ROW(row4_ptr, chroma_offset))); \
					b += chroma_offset; \
 \
					if(components == 4) \
					{ \
						row1_ptr++; \
						row2_ptr++; \
						row3_ptr++; \
						row4_ptr++; \
						a = (int)(transform_cubic(dy, \
                    			 CUBIC_ROW(row1_ptr, 0x0), \
                    			 CUBIC_ROW(row2_ptr, 0x0), \
                    			 CUBIC_ROW(row3_ptr, 0x0), \
                    			 CUBIC_ROW(row4_ptr, 0x0)) +  \
								 0.5); \
					} \
 \
					*out_row++ = CLIP(r, 0, max); \
					*out_row++ = CLIP(g, 0, max); \
					*out_row++ = CLIP(b, 0, max); \
					if(components == 4) *out_row++ = CLIP(a, 0, max); \
                } \
				else \
/* Fill with chroma */ \
				{ \
					*out_row++ = 0; \
					*out_row++ = chroma_offset; \
					*out_row++ = chroma_offset; \
					if(components == 4) *out_row++ = 0; \
				} \
			} \
			else \
			{ \
				out_row += components; \
			} \
 \
/*  increment the transformed coordinates  */ \
			tx += xinc; \
			ty += yinc; \
			tw += winc; \
		} \
	} \
}




		switch(plugin->input->get_color_model())
		{
			case BC_RGB888:
				TRANSFORM(3, unsigned char, 0x0, 0xff)
				break;
			case BC_RGBA8888:
				TRANSFORM(4, unsigned char, 0x0, 0xff)
				break;
			case BC_YUV888:
				TRANSFORM(3, unsigned char, 0x80, 0xff)
				break;
			case BC_YUVA8888:
				TRANSFORM(4, unsigned char, 0x80, 0xff)
				break;
			case BC_RGB161616:
				TRANSFORM(3, uint16_t, 0x0, 0xffff)
				break;
			case BC_RGBA16161616:
				TRANSFORM(4, uint16_t, 0x0, 0xffff)
				break;
			case BC_YUV161616:
				TRANSFORM(3, uint16_t, 0x8000, 0xffff)
				break;
			case BC_YUVA16161616:
				TRANSFORM(4, uint16_t, 0x8000, 0xffff)
				break;
		}

//printf("PerspectiveUnit::process_package 50\n");
	}
	else
	{
		int max_x = w * OVERSAMPLE - 1;
		int max_y = h * OVERSAMPLE - 1;
//printf("PerspectiveUnit::process_package 50 %d %d\n", max_x, max_y);
		float top_w = out_x2 - out_x1;
		float bottom_w = out_x3 - out_x4;
		float left_h = out_y4 - out_y1;
		float right_h = out_y3 - out_y2;
		float out_w_diff = bottom_w - top_w;
		float out_left_diff = out_x4 - out_x1;
		float out_h_diff = right_h - left_h;
		float out_top_diff = out_y2 - out_y1;
		float distance1 = DISTANCE(out_x1, out_y1, out_x2, out_y2);
		float distance2 = DISTANCE(out_x2, out_y2, out_x3, out_y3);
		float distance3 = DISTANCE(out_x3, out_y3, out_x4, out_y4);
		float distance4 = DISTANCE(out_x4, out_y4, out_x1, out_y1);
		float max_v = MAX(distance1, distance3);
		float max_h = MAX(distance2, distance4);
		float max_dimension = MAX(max_v, max_h);
		float min_dimension = MIN(h, w);
		float step = min_dimension / max_dimension / OVERSAMPLE;
		float h_f = h;
		float w_f = w;

// Projection
#define PERSPECTIVE(type, components) \
{ \
	type **in_rows = (type**)plugin->input->get_rows(); \
	type **out_rows = (type**)plugin->temp->get_rows(); \
 \
	for(float in_y = pkg->y1; in_y < pkg->y2; in_y += step) \
	{ \
		int i = (int)in_y; \
		type *in_row = in_rows[i]; \
		for(float in_x = 0; in_x < w; in_x += step) \
		{ \
			int j = (int)in_x; \
			float in_x_fraction = in_x / w_f; \
			float in_y_fraction = in_y / h_f; \
			int out_x = (int)((out_x1 + \
				out_left_diff * in_y_fraction + \
				(top_w + out_w_diff * in_y_fraction) * in_x_fraction) *  \
				OVERSAMPLE); \
			int out_y = (int)((out_y1 +  \
				out_top_diff * in_x_fraction + \
				(left_h + out_h_diff * in_x_fraction) * in_y_fraction) * \
				OVERSAMPLE); \
			CLAMP(out_x, 0, max_x); \
			CLAMP(out_y, 0, max_y); \
			type *dst = out_rows[out_y] + out_x * components; \
			type *src = in_row + j * components; \
			dst[0] = src[0]; \
			dst[1] = src[1]; \
			dst[2] = src[2]; \
			if(components == 4) dst[3] = src[3]; \
		} \
	} \
}

		switch(plugin->input->get_color_model())
		{
			case BC_RGB888:
				PERSPECTIVE(unsigned char, 3)
				break;
			case BC_RGBA8888:
				PERSPECTIVE(unsigned char, 4)
				break;
			case BC_YUV888:
				PERSPECTIVE(unsigned char, 3)
				break;
			case BC_YUVA8888:
				PERSPECTIVE(unsigned char, 4)
				break;
			case BC_RGB161616:
				PERSPECTIVE(uint16_t, 3)
				break;
			case BC_RGBA16161616:
				PERSPECTIVE(uint16_t, 4)
				break;
			case BC_YUV161616:
				PERSPECTIVE(uint16_t, 3)
				break;
			case BC_YUVA16161616:
				PERSPECTIVE(uint16_t, 4)
				break;
		}
	}




}






PerspectiveEngine::PerspectiveEngine(PerspectiveMain *plugin, 
	int total_clients,
	int total_packages)
 : LoadServer(
//1, 1 
total_clients, total_packages 
)
{
	this->plugin = plugin;
}

void PerspectiveEngine::init_packages()
{
	int package_h = (int)((float)plugin->output->get_h() / 
			total_packages + 1);
	int y1 = 0;
	for(int i = 0; i < total_packages; i++)
	{
		PerspectivePackage *package = (PerspectivePackage*)packages[i];
		package->y1 = y1;
		package->y2 = y1 + package_h;
		package->y1 = MIN(plugin->output->get_h(), package->y1);
		package->y2 = MIN(plugin->output->get_h(), package->y2);
		y1 = package->y2;
	}
}

LoadClient* PerspectiveEngine::new_client()
{
	return new PerspectiveUnit(this, plugin);
}

LoadPackage* PerspectiveEngine::new_package()
{
	return new PerspectivePackage;
}





