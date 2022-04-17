// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbitmap.h"
#include "bchash.h"
#include "bctoggle.h"
#include "clip.h"
#include "colorspaces.h"
#include "filexml.h"
#include "language.h"
#include "loadbalance.h"
#include "fonts.h"
#include "videoscope.h"
#include "vframe.h"
#include "videoscope.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

const double FLOAT_MIN = -0.1;
const double FLOAT_MAX =  1.1;

// Waveform and Vectorscope layout
const char LABEL_WIDTH_SAMPLE[] = "100 ";
const int H_SPACE =  5;
const int V_INSET = 10;

// Widget layout
const char WIDGET_HSPACE_SAMPLE[] = "    ";
const int WIDGET_VSPACE = 3;

VideoScopeConfig::VideoScopeConfig()
{
	show_srgb_limits    = 0;
	show_ycbcr_limits    = 0;
	draw_lines_inverse = 0;
}

VideoScopeWaveform::VideoScopeWaveform(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
}

VideoScopeVectorscope::VideoScopeVectorscope(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
}

VideoScopeWindow::VideoScopeWindow(VideoScopeEffect *plugin, 
	int x, 
	int y)
 : PluginWindow(plugin,
	x,
	y, 
	640,
	260)
{
	int w = 640;
	int h = 260;

	waveform_bitmap = 0;
	vector_bitmap = 0;

// Widgets
	const int widget_hspace = get_text_width(MEDIUMFONT, (char *) WIDGET_HSPACE_SAMPLE);
	const int widget_height = get_widget_area_height();
	x = widget_hspace;
	y = h - widget_height + WIDGET_VSPACE;
	set_color(get_resources()->get_bg_color());
	draw_box(0, h - widget_height, w, widget_height);
	add_subwindow(show_srgb_limits = new VideoScopeShowSRGBLimits(plugin, x, y));
	x += show_srgb_limits->get_w() + widget_hspace;
	add_subwindow(show_ycbcr_limits = new VideoScopeShowYCbCrLimits(plugin, x, y));
	x += show_ycbcr_limits->get_w() + widget_hspace;
	add_subwindow(draw_lines_inverse = new VideoScopeDrawLinesInverse(plugin, x, y));

	calculate_sizes(w, h - widget_height - WIDGET_VSPACE);

	add_subwindow(waveform = new VideoScopeWaveform(plugin, 
		wave_x, 
		wave_y, 
		wave_w, 
		wave_h));
	add_subwindow(vectorscope = new VideoScopeVectorscope(plugin, 
		vector_x, 
		vector_y, 
		vector_w, 
		vector_h));
	allocate_bitmaps();

	waveform->calculate_graduations();
	vectorscope->calculate_graduations();
	waveform->draw_graduations();
	vectorscope->draw_graduations();
	draw_labels();
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

VideoScopeWindow::~VideoScopeWindow()
{
	if(waveform_bitmap) delete waveform_bitmap;
	if(vector_bitmap) delete vector_bitmap;
}

VideoScopeGraduation::VideoScopeGraduation()
{
	memset(label, 0, sizeof(label));
}

void VideoScopeWindow::calculate_sizes(int w, int h)
{
	const int w_midpoint = w / 2;
	const int label_width = get_label_width();

// Waveform is a rectangle in left half of window.
// Labels are outside the Waveform widget.
	wave_x = label_width + H_SPACE;
	wave_y = V_INSET;
	wave_w = w_midpoint - H_SPACE - wave_x;
	wave_h = h - V_INSET - wave_y;

// Vectorscope is square and centered in right half of window
// Labels are outside the Vectorscope widget.
	const int vec_max_width  = w_midpoint - H_SPACE - label_width;
	const int vec_max_height = h - 2 * V_INSET;
	const int square = MIN(vec_max_width, vec_max_height);
	vector_x = w_midpoint + label_width + (w_midpoint - square- H_SPACE - label_width) / 2;
	vector_y = (h - square) / 2;
	vector_w = square;
	vector_h = square;
}

int VideoScopeWindow::get_label_width()
{
	return get_text_width(SMALLFONT, (char *) LABEL_WIDTH_SAMPLE);
}

int VideoScopeWindow::get_widget_area_height()
{
// Don't know how to get the widget height before it's drawn, so
// instead use twice the font height as the height for where the
// widgets are drawn.
	return 2 * get_text_height(MEDIUMFONT);
}

void VideoScopeWaveform::redraw()
{
	clear_box(0, 0, get_w(), get_h());
	draw_graduations();
	flash();
}

void VideoScopeWindow::allocate_bitmaps()
{
	if(waveform_bitmap) delete waveform_bitmap;
	if(vector_bitmap) delete vector_bitmap;

	waveform_bitmap = new_bitmap(wave_w, wave_h);
	vector_bitmap = new_bitmap(vector_w, vector_h);
}

// Convert polar to cartesian for vectorscope.
// Hue is angle (degrees), Saturation is distance from center [0, 1].
// Value (intensity) is not plotted.
static void polar_to_cartesian(int h, double s, double radius,
	int &x, int &y)
{
	double adjacent = cos(h / 360.0 * 2 * M_PI);
	double opposite = sin(h / 360.0 * 2 * M_PI);
	double r = (s - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN) * radius;
	x = round(radius + adjacent * r);
	y = round(radius - opposite * r);
}

// Calculate graduations based on current window size.
void VideoScopeWaveform::calculate_graduations()
{
	int height = get_h();

	for(int i = 0; i <= WAVEFORM_DIVISIONS; i++)
	{
		int y = height * i / WAVEFORM_DIVISIONS;
		char string[BCTEXTLEN];
		sprintf(string, "%d", 
			(int)round((FLOAT_MAX - 
			i * (FLOAT_MAX - FLOAT_MIN) / WAVEFORM_DIVISIONS) * 100));
		grads[i].set(string, CLAMP(y, 0, height - 1));
	}

	// Special limits.
	limit_ycbcr_white = round(height * (FLOAT_MAX - 235.0/255.0) /
		(FLOAT_MAX - FLOAT_MIN));
	limit_ycbcr_black = round(height * (FLOAT_MAX -  16.0/255.0) /
		(FLOAT_MAX - FLOAT_MIN));
}

// Calculate graduations based on current window size.
void VideoScopeVectorscope::calculate_graduations()
{
	// graduations/labels
	const int radius = get_h() / 2;  // vector_w == vector_h

	for(int i = 1, g = 0; i <= VECTORSCOPE_DIVISIONS - 1; i += 2)
	{
		int y = radius - radius * i / VECTORSCOPE_DIVISIONS;
		char string[BCTEXTLEN];
		sprintf(string, "%d", 
			(int)round((FLOAT_MIN + 
				(FLOAT_MAX - FLOAT_MIN) / VECTORSCOPE_DIVISIONS * i) * 100));
		grads[g++].set(string, y);
	}

	// color axes
	color_axis_font = radius > 200 ? MEDIUMFONT : SMALLFONT;
	const int ascent_half = get_text_ascent(color_axis_font) / 2;

	for (int i = 0;  i < Vectorscope_HSV_axes_count; ++i)
	{
		const double hue = Vectorscope_HSV_axes[i].hue;
		polar_to_cartesian(hue, 0, radius, axes[i].x1, axes[i].y1);
		polar_to_cartesian(hue, 1, radius, axes[i].x2, axes[i].y2);

		// Draw color axis label halfway between outer circle and
		// edge of bitmap. Color axis labels are within the
		// rectangular vectorscope region, and are redrawn with each
		// frame.

		polar_to_cartesian(hue, 1 + (FLOAT_MAX - 1) / 2, radius, axes[i].text_x, axes[i].text_y);
		axes[i].text_x -= get_text_width(color_axis_font, const_cast<char *>(Vectorscope_HSV_axes[i].label)) / 2;
		axes[i].text_y += ascent_half;
	}
}

void VideoScopeWindow::draw_labels()
{
	set_color(LTGREY);
	set_font(SMALLFONT);
	const int sm_text_ascent_half = get_text_ascent(SMALLFONT) / 2;
	const int label_width_half = get_label_width() / 2;

// Waveform labels
	if(waveform)
	{
		const int text_x  = wave_x - label_width_half;

		for (int i = 0; i < VideoScopeWaveform::NUM_GRADS; ++i)
			draw_center_text(text_x,
				waveform->grads[i].y + wave_y + sm_text_ascent_half,
				 waveform->grads[i].label);
	}

// Vectorscope labels
	if(vectorscope)
	{
		const int text_x = vector_x - label_width_half;

		for (int i = 0; i < VideoScopeVectorscope::NUM_GRADS; ++i)
			draw_center_text(text_x,
				vectorscope->grads[i].y + vector_y + sm_text_ascent_half,
				vectorscope->grads[i].label);
	}

#ifdef DEBUG_PLACEMENT
	set_color(GREEN);
	draw_rectangle(wave_x - 2 * label_width_half, wave_y - sm_text_ascent_half,
		2 * label_width_half, wave_h + 2 * sm_text_ascent_half);
	draw_rectangle(vector_x - 2 * label_width_half, vector_y,
		2 * label_width_half, vector_h);
	set_color(BLUE);
	waveform->draw_rectangle(0, 0, wave_w, wave_h);
	set_color(RED);
	vectorscope->draw_rectangle(0, 0, vector_w, vector_h);
#endif // DEBUG_PLACEMENT

	set_font(MEDIUMFONT);
	waveform->flash();
	vectorscope->flash();
	flush();
}

// Draws horizontal lines in waveform bitmap.
void VideoScopeWaveform::draw_graduations()
{
	VideoScopeConfig &config = plugin->config;

	if(config.draw_lines_inverse)
		set_inverse();

	const int w = get_w();
	const int h = get_h();

	for(int i = 0; i < NUM_GRADS; ++i)
	{
		// 1 & 11 correspond to 100% & 0%.
		set_color((config.show_srgb_limits && (i == 1 || i == 11))
			? WHITE : MDGREY);
		int y = grads[i].y;
		draw_line(0, y, w, y);
	}
	if(config.show_ycbcr_limits)
	{
		set_color(WHITE);
		draw_line(0, limit_ycbcr_white, w, limit_ycbcr_white);
		draw_line(0, limit_ycbcr_black, w, limit_ycbcr_black);
	}
	if(config.draw_lines_inverse)
		set_opaque();
}

void VideoScopeVectorscope::draw_graduations()
{
	const int diameter = get_h();  // diameter; vector_w == vector_h

	set_color(MDGREY);

	for(int i = 0; i < NUM_GRADS; ++i)
	{
		int top_left     = grads[i].y;
		int width_height = diameter - 2 * top_left;
		draw_circle(top_left, top_left, width_height, width_height);
	}

	// RGB+CYM axes and labels.
	set_font(color_axis_font);
	for(int i = 0;  i < Vectorscope_HSV_axes_count; ++i)
	{
		set_color(MDGREY);
		draw_line(axes[i].x1, axes[i].y1,
			axes[i].x2, axes[i].y2);

		set_color(Vectorscope_HSV_axes[i].color);
		draw_text(axes[i].text_x, axes[i].text_y, const_cast<char *>(Vectorscope_HSV_axes[i].label));
	}
}

void VideoScopeGraduation::set(const char * label, int y)
{
	strcpy(this->label, label);
	this->y    = y;
}


VideoScopeShowSRGBLimits::VideoScopeShowSRGBLimits(VideoScopeEffect *plugin,
		int x,
		int y)
 : BC_CheckBox(x, y, plugin->config.show_srgb_limits, _("sRGB-range"))
{
	this->plugin = plugin;
	set_tooltip(_("Indicate sRGB-range [0-255]"));
}

int VideoScopeShowSRGBLimits::handle_event()
{
	plugin->config.show_srgb_limits = get_value();
	plugin->send_configure_change();
	return 1;
}

VideoScopeShowYCbCrLimits::VideoScopeShowYCbCrLimits(VideoScopeEffect *plugin,
		int x,
		int y)
 : BC_CheckBox(x, y, plugin->config.show_ycbcr_limits, _("Y'CbCr"))
{
	this->plugin = plugin;
	set_tooltip(_("Indicate  Y'-range [16-235]"));
}

int VideoScopeShowYCbCrLimits::handle_event()
{
	plugin->config.show_ycbcr_limits = get_value();
	plugin->send_configure_change();
	return 1;
}

VideoScopeDrawLinesInverse::VideoScopeDrawLinesInverse(VideoScopeEffect *plugin,
		int x,
		int y)
 : BC_CheckBox(x, y, plugin->config.draw_lines_inverse, _("Inverse"))
{
	this->plugin = plugin;
	set_tooltip(_("Draw graduation lines so points underneath are visible"));
}

int VideoScopeDrawLinesInverse::handle_event()
{
	plugin->config.draw_lines_inverse = get_value();
	plugin->send_configure_change();
	return 1;
}

PLUGIN_THREAD_METHODS

REGISTER_PLUGIN


VideoScopeEffect::VideoScopeEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

VideoScopeEffect::~VideoScopeEffect()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void VideoScopeEffect::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
	}
}

PLUGIN_CLASS_METHODS

int VideoScopeEffect::load_configuration()
{
	KeyFrame *keyframe = get_first_keyframe();

	if(!keyframe)
		return 0;

	read_data(keyframe);
	return 1;
}

void VideoScopeEffect::load_defaults()
{
	defaults = load_defaults_file("videoscope.rc");

	config.show_srgb_limits = defaults->get("SHOW_709_LIMITS", config.show_srgb_limits);
	config.show_srgb_limits = defaults->get("SHOW_SRGB_LIMITS", config.show_srgb_limits);
	config.show_ycbcr_limits = defaults->get("SHOW_601_LIMITS", config.show_ycbcr_limits);
	config.show_ycbcr_limits = defaults->get("SHOW_YCBCR_LIMITS", config.show_ycbcr_limits);
	config.draw_lines_inverse = defaults->get("DRAW_LINES_INVERSE", config.draw_lines_inverse);
}

void VideoScopeEffect::save_defaults()
{
	defaults->update("SHOW_SRGB_LIMITS", config.show_srgb_limits);
	defaults->update("SHOW_YCBCR_LIMITS",    config.show_ycbcr_limits);
	defaults->update("DRAW_LINES_INVERSE", config.draw_lines_inverse);
	defaults->save();
}

void VideoScopeEffect::save_data(KeyFrame *keyframe)
{
	FileXML file;

	file.tag.set_title("VIDEOSCOPE");
	file.tag.set_property("SHOW_SRGB_LIMITS", config.show_srgb_limits);
	file.tag.set_property("SHOW_YCBCR_LIMITS", config.show_ycbcr_limits);
	file.tag.set_property("DRAW_LINES_INVERSE", config.draw_lines_inverse);
	file.append_tag();
	file.tag.set_title("/VIDEOSCOPE");
	file.append_tag();
	keyframe->set_data(file.string);
}

void VideoScopeEffect::read_data(KeyFrame *keyframe)
{
	FileXML file;

	file.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!file.read_tag())
	{
		config.show_srgb_limits = file.tag.get_property("SHOW_709_LIMITS", config.show_srgb_limits);
		config.show_srgb_limits = file.tag.get_property("SHOW_SRGB_LIMITS", config.show_srgb_limits);
		config.show_ycbcr_limits = file.tag.get_property("SHOW_601_LIMITS", config.show_ycbcr_limits);
		config.show_ycbcr_limits = file.tag.get_property("SHOW_YCBCR_LIMITS", config.show_ycbcr_limits);
		config.draw_lines_inverse = file.tag.get_property("DRAW_LINES_INVERSE", config.draw_lines_inverse);
	}
}

VFrame *VideoScopeEffect::process_tmpframe(VFrame *input)
{
	int color_model = input->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return input;
	}

	render_gui(input);
	return input;
}

void VideoScopeEffect::render_gui(void *input)
{
	if(thread)
	{
		VideoScopeWindow *window = thread->window;

		this->input = (VFrame*)input;

		if(!engine)
		{
			engine = new VideoScopeEngine(this,
				get_project_smp());
		}

// Clear bitmaps
		memset(window->waveform_bitmap->get_data(), 0,
			window->waveform_bitmap->get_h() * 
			window->waveform_bitmap->get_bytes_per_line());
		memset(window->vector_bitmap->get_data(), 0,
			window->vector_bitmap->get_h() * 
			window->vector_bitmap->get_bytes_per_line());

		engine->process_packages();

		window->waveform->draw_bitmap(window->waveform_bitmap, 
			1,
			0,
			0);

		window->vectorscope->draw_bitmap(window->vector_bitmap, 
			1,
			0,
			0);

		window->waveform->draw_graduations();
		window->vectorscope->draw_graduations();
		window->waveform->flash();
		window->vectorscope->flash();
	}
}


VideoScopePackage::VideoScopePackage()
 : LoadPackage()
{
}


VideoScopeUnit::VideoScopeUnit(VideoScopeEffect *plugin, 
	VideoScopeEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}

static void draw_point(BC_Bitmap *bitmap,
	int x, 
	int y, 
	int r, 
	int g, 
	int b)
{
	int color_model = bitmap->get_color_model();
	unsigned char *pixel = bitmap->get_data() +
		y * bitmap->get_bytes_per_line() +
		ColorModels::calculate_pixelsize(color_model) * x;

	switch(color_model)
	{
	case BC_BGR8888:
		pixel[0] = b;
		pixel[1] = g;
		pixel[2] = r;
		break;
	case BC_RGB565:
		pixel[0] = (r & 0xf8) | (g >> 5);
		pixel[1] = ((g & 0xfc) << 5) | (b >> 3);
		break;
	}
}


// Brighten value and decrease contrast so low levels are visible against black.
// Value v is either R, G, or B.
static int brighten(int v)
{
	return (((256 - RGB_MIN) * v) + (256 * RGB_MIN))/256;
}


void VideoScopeUnit::process_package(LoadPackage *package)
{
	VideoScopeWindow *window = plugin->thread->window;
	VideoScopePackage *pkg = (VideoScopePackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	int waveform_h = window->wave_h;
	int waveform_w = window->wave_w;
	int vector_h = window->vector_bitmap->get_h();
	int vector_w = window->vector_bitmap->get_w();
	float radius = vector_h / 2.0;

	switch(plugin->input->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = pkg->row1; i < pkg->row2; i++)
		{
			uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				uint16_t *in_pixel = in_row + j * 4;
				double intensity;
				int h;
				double s, v;
				int r, g, b;

				r = in_pixel[0];
				g = in_pixel[1];
				b = in_pixel[2];

				ColorSpaces::rgb_to_hsv(r, g, b, &h, &s, &v);
				// Calculate point's RGB,
				// used in both waveform and vectorscope.
				int ri, gi, bi;

				// Brighten & decrease contrast
				// so low levels are visible against black.
				ri = brighten(r >> 8);
				gi = brighten(g >> 8);
				bi = brighten(b >> 8);
				// Calculate waveform
				intensity = v;
				int y = waveform_h -
					(int)round((intensity - FLOAT_MIN) /
						(FLOAT_MAX - FLOAT_MIN) *
						waveform_h);
				int x = j * waveform_w / w;

				if(x >= 0 && x < waveform_w && y >= 0 && y < waveform_h)
					draw_point(window->waveform_bitmap,
						x, y, ri, gi, bi);
				// Calculate vectorscope
				polar_to_cartesian(h, s, radius, x, y);
				CLAMP(x, 0, vector_w - 1);
				CLAMP(y, 0, vector_h - 1);
				draw_point(window->vector_bitmap,
					x, y, ri, gi, bi);
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = pkg->row1; i < pkg->row2; i++)
		{
			uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				uint16_t *in_pixel = in_row + j * 4;
				double intensity;

				// Analyze pixel
				intensity = (double)in_pixel[1] / 0xffff;

				int h;
				double s, v;
				int r, g, b;

				ColorSpaces::yuv_to_rgb_16(r, g, b,
					in_pixel[1],
					in_pixel[2],
					in_pixel[3]);

				ColorSpaces::rgb_to_hsv(r, g, b,
					&h, &s, &v);

				// Calculate point's RGB, used in both
				//  waveform and vectorscope.
				int ri, gi, bi;

				// Brighten & decrease contrast so low
				// levels are visible against black.
				ri = brighten(r >> 8);
				gi = brighten(g >> 8);
				bi = brighten(b >> 8);

				// Calculate waveform
				int y = waveform_h -
					(int)round((intensity - FLOAT_MIN) /
						(FLOAT_MAX - FLOAT_MIN) *
						waveform_h);

				int x = j * waveform_w / w;
				if(x >= 0 && x < waveform_w && y >= 0 && y < waveform_h)
					draw_point(window->waveform_bitmap,
						x, y, ri, gi, bi);

				// Calculate vectorscope
				polar_to_cartesian(h, s, radius, x, y);
				CLAMP(x, 0, vector_w - 1);
				CLAMP(y, 0, vector_h - 1);
				draw_point(window->vector_bitmap,
					x, y, ri, gi, bi);
			}
		}
		break;
	}
}


VideoScopeEngine::VideoScopeEngine(VideoScopeEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}


void VideoScopeEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		VideoScopePackage *pkg = (VideoScopePackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}


LoadClient* VideoScopeEngine::new_client()
{
	return new VideoScopeUnit(plugin, this);
}

LoadPackage* VideoScopeEngine::new_package()
{
	return new VideoScopePackage;
}
