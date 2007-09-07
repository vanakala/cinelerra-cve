#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "plugincolors.h"
#include "pluginvclient.h"
#include "fonts.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>




const float FLOAT_MIN = -0.1;
const float FLOAT_MAX =  1.1;
const int   WAVEFORM_DIVISIONS    = 12;
const int   VECTORSCOPE_DIVISIONS = 12;
const int   RGB_MIN = 48;


// Vectorscope HSV axes and labels
const struct Vectorscope_HSV_axes
{
	float  hue;   // angle, degrees
	char   label[4];
	int    color; // label color
}
Vectorscope_HSV_axes[] =
	{
		{   0, "R",  RED      },
		{  60, "Yl", YELLOW   },
		{ 120, "G",  GREEN    },
		{ 180, "Cy", LTCYAN   },
		{ 240, "B",  BLUE     },
		{ 300, "Mg", MDPURPLE },
	};
const int Vectorscope_HSV_axes_count = sizeof(Vectorscope_HSV_axes) / sizeof(struct Vectorscope_HSV_axes);


// Define to display outlines around waveform and vectorscope.
// Useful for debugging window resize.
//#define DEBUG_PLACEMENT



class VideoScopeEffect;
class VideoScopeEngine;


class VideoScopeConfig
{
public:
	VideoScopeConfig();
};

class VideoScopeGraduation
{
// One VideoScopeGraduation represents one line (or circle) and associated
// label. We use arrays of VideoScopeGraduations.
public:
	typedef enum { LEFT, RIGHT } side_t;
	VideoScopeGraduation();
	void set(const char * label, int y, side_t side = LEFT);

	char    label[4];   // Maximum label size is 3 characters
	int     y;
	side_t  side;
};

class VideoScopeWaveform : public BC_SubWindow
{
public:
	VideoScopeWaveform(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h);
	VideoScopeEffect *plugin;

	void calculate_graduations();
	void draw_graduations();

	// All standard divisions + the one at the end + 7.5% line
	static const int NUM_GRADS = WAVEFORM_DIVISIONS + 2;
	VideoScopeGraduation  grads[NUM_GRADS];
};


class VideoScopeVectorscope : public BC_SubWindow
{
public:
	VideoScopeVectorscope(VideoScopeEffect *plugin, 
		int x, 
		int y,
		int w,
		int h);
	VideoScopeEffect *plugin;

	void calculate_graduations();
	void draw_graduations();

	// Draw only every other division.
	static const int NUM_GRADS = VECTORSCOPE_DIVISIONS / 2;
	VideoScopeGraduation  grads[NUM_GRADS];

private:
	int color_axis_font;
	struct {
		int x1, y1, x2, y2;
		int text_x, text_y;
	} axes[Vectorscope_HSV_axes_count];
};

class VideoScopeWindow : public BC_Window
{
public:
	VideoScopeWindow(VideoScopeEffect *plugin, int x, int y);
	~VideoScopeWindow();

	void calculate_sizes(int w, int h);
	void create_objects();
	int close_event();
	int resize_event(int w, int h);
	void allocate_bitmaps();
	void draw_labels();

	VideoScopeEffect *plugin;
	VideoScopeWaveform *waveform;
	VideoScopeVectorscope *vectorscope;
	BC_Bitmap *waveform_bitmap;
	BC_Bitmap *vector_bitmap;

	int vector_x, vector_y, vector_w, vector_h;
	int wave_x, wave_y, wave_w, wave_h;
};

PLUGIN_THREAD_HEADER(VideoScopeEffect, VideoScopeThread, VideoScopeWindow)




class VideoScopePackage : public LoadPackage
{
public:
	VideoScopePackage();
	int row1, row2;
};


class VideoScopeUnit : public LoadClient
{
public:
	VideoScopeUnit(VideoScopeEffect *plugin, VideoScopeEngine *server);
	void process_package(LoadPackage *package);
	VideoScopeEffect *plugin;
	YUV yuv;
private:
	template<typename TYPE, typename TEMP_TYPE,
		 int MAX, int COMPONENTS, bool USE_YUV>
	void render_data(LoadPackage *package);
};

class VideoScopeEngine : public LoadServer
{
public:
	VideoScopeEngine(VideoScopeEffect *plugin, int cpus);
	~VideoScopeEngine();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	VideoScopeEffect *plugin;
};

class VideoScopeEffect : public PluginVClient
{
public:
	VideoScopeEffect(PluginServer *server);
	~VideoScopeEffect();

	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	char* plugin_title();
	VFrame* new_picon();
	int load_defaults();
	int save_defaults();
	int show_gui();
	int set_string();
	void raise_window();
	void render_gui(void *input);
	int load_configuration();

	int w, h;
	VFrame *input;
	VideoScopeConfig config;
	VideoScopeEngine *engine;
	BC_Hash *defaults;
	VideoScopeThread *thread;
};













VideoScopeConfig::VideoScopeConfig()
{
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
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	plugin->w, 
	plugin->h, 
	50, 
	50, 
	1, 
	0,
	1,
	BLACK)
{
	this->plugin = plugin;
	waveform_bitmap = 0;
	vector_bitmap = 0;
}

VideoScopeWindow::~VideoScopeWindow()
{
	if(waveform_bitmap) delete waveform_bitmap;
	if(vector_bitmap) delete vector_bitmap;
}

VideoScopeGraduation::VideoScopeGraduation()
{
	bzero(label, sizeof(label));
}

void VideoScopeWindow::calculate_sizes(int w, int h)
{
	// Waveform is a rectangle in left half of window
	wave_x = 30;
	wave_y = 10;
	wave_w = w / 2 - 5 - wave_x;
	wave_h = h - 20 - wave_y;

	// Vectorscope is square and centered in right half of window
	int max_width  = w / 2 - 40;
	int max_height = h - 20;
	int square = MIN(max_width, max_height);
	vector_x = w / 2 + 10 + (w / 2 - 10 - square) / 2;
	vector_y = (h - square) / 2;
	vector_w = square;
	vector_h = square;
}

void VideoScopeWindow::create_objects()
{
	calculate_sizes(get_w(), get_h());

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

	show_window();
	flush();
	
}

WINDOW_CLOSE_EVENT(VideoScopeWindow)

int VideoScopeWindow::resize_event(int w, int h)
{

	clear_box(0, 0, w, h);
	plugin->w = w;
	plugin->h = h;
	calculate_sizes(w, h);
	waveform->reposition_window(wave_x, wave_y, wave_w, wave_h);
	vectorscope->reposition_window(vector_x, vector_y, vector_w, vector_h);
	waveform->clear_box(0, 0, wave_w, wave_h);
	vectorscope->clear_box(0, 0, wave_w, wave_h);
	allocate_bitmaps();

	waveform->calculate_graduations();
	vectorscope->calculate_graduations();
	waveform->draw_graduations();
	vectorscope->draw_graduations();
	draw_labels();

	flash();

	return 1;
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
static void polar_to_cartesian(float h,
			       float s,
			       float radius,
			       int & x,
			       int & y)
{
	float adjacent = cos(h / 360 * 2 * M_PI);
	float opposite = sin(h / 360 * 2 * M_PI);
	float r = (s - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN) * radius;
	x = (int)roundf(radius + adjacent * r);
	y = (int)roundf(radius - opposite * r);
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
	// 7.5% SMPTE minimum black. Draw text on right side (no room on left).
	grads[WAVEFORM_DIVISIONS + 1].set("7.5", (int)round(height * (FLOAT_MAX - 0.075) / (FLOAT_MAX - FLOAT_MIN)), VideoScopeGraduation::RIGHT);
}

// Calculate graduations based on current window size.
void VideoScopeVectorscope::calculate_graduations()
{
	// graduations/labels
	const int radius = get_h() / 2;	// vector_w == vector_h
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
		const float hue = Vectorscope_HSV_axes[i].hue;
		polar_to_cartesian(hue, 0, radius, axes[i].x1, axes[i].y1);
		polar_to_cartesian(hue, 1, radius, axes[i].x2, axes[i].y2);

		// Draw color axis label halfway between outer circle and
		// edge of bitmap. Color axis labels are within the
		// rectangular vectorscope region that is redrawn with each
		// frame.

		polar_to_cartesian(hue, 1 + (FLOAT_MAX - 1) / 2, radius, axes[i].text_x, axes[i].text_y);
		axes[i].text_y += ascent_half;
	}
}

void VideoScopeWindow::draw_labels()
{
	set_color(LTGREY);
	set_font(SMALLFONT);
	const int sm_text_ascent_half = get_text_ascent(SMALLFONT) / 2;

// Waveform labels
	if (waveform) {
		const int text_x_left  = wave_x - 20;
		const int text_x_right = wave_x + wave_w + 15;
		for (int i = 0; i < VideoScopeWaveform::NUM_GRADS; ++i)
			draw_center_text((waveform->grads[i].side == VideoScopeGraduation::LEFT) ? text_x_left : text_x_right,
					 waveform->grads[i].y + wave_y + sm_text_ascent_half,
					 waveform->grads[i].label);
			
	}

// Vectorscope labels
	if (vectorscope) {
		const int text_x = vector_x - 10;
		for (int i = 0; i < VideoScopeVectorscope::NUM_GRADS; ++i)
			// Always on left (ignore side)
			draw_center_text(text_x,
					 vectorscope->grads[i].y + vector_y + sm_text_ascent_half,
					 vectorscope->grads[i].label);
	}

#ifdef DEBUG_PLACEMENT
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
	set_color(MDGREY);
	const int w = get_w();
	const int h = get_h();
	for (int i = 0; i < NUM_GRADS; ++i)
	{
		int y = grads[i].y;
		draw_line(0, y, w, y);
	}
}

void VideoScopeVectorscope::draw_graduations()
{
	set_color(MDGREY);
	const int diameter = get_w();  // diameter; vector_w == vector_h
	for (int i = 0; i < NUM_GRADS; ++i)
	{
		int top_left     = grads[i].y;
		int width_height = diameter - 2 * top_left;
		draw_circle(top_left, top_left, width_height, width_height);
	}

	// RGB+CYM axes and labels.
	set_font(color_axis_font);
	for (int i = 0;  i < Vectorscope_HSV_axes_count; ++i)
	{
		set_color(MDGREY);
		draw_line(axes[i].x1, axes[i].y1,
			  axes[i].x2, axes[i].y2);

		set_color(Vectorscope_HSV_axes[i].color);
		draw_center_text(axes[i].text_x, axes[i].text_y, const_cast<char *>(Vectorscope_HSV_axes[i].label));
	}
}

void VideoScopeGraduation::set(const char * label, int y, side_t side)
{
	assert(strlen(label) <= 3);
	strcpy(this->label, label);
	this->y    = y;
	this->side = side;
}








PLUGIN_THREAD_OBJECT(VideoScopeEffect, VideoScopeThread, VideoScopeWindow)





REGISTER_PLUGIN(VideoScopeEffect)






VideoScopeEffect::VideoScopeEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	w = 640;
	h = 260;
	PLUGIN_CONSTRUCTOR_MACRO
}

VideoScopeEffect::~VideoScopeEffect()
{
	PLUGIN_DESTRUCTOR_MACRO

	if(engine) delete engine;
}



char* VideoScopeEffect::plugin_title() { return N_("VideoScope"); }
int VideoScopeEffect::is_realtime() { return 1; }

int VideoScopeEffect::load_configuration()
{
	return 0;
}

NEW_PICON_MACRO(VideoScopeEffect)

SHOW_GUI_MACRO(VideoScopeEffect, VideoScopeThread)

RAISE_WINDOW_MACRO(VideoScopeEffect)

SET_STRING_MACRO(VideoScopeEffect)

int VideoScopeEffect::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%svideoscope.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	w = defaults->get("W", w);
	h = defaults->get("H", h);
	return 0;
}

int VideoScopeEffect::save_defaults()
{
	defaults->update("W", w);
	defaults->update("H", h);
	defaults->save();
	return 0;
}

int VideoScopeEffect::process_realtime(VFrame *input, VFrame *output)
{

	send_render_gui(input);
//printf("VideoScopeEffect::process_realtime 1\n");
	if(input->get_rows()[0] != output->get_rows()[0])
		output->copy_from(input);
	return 1;
}

void VideoScopeEffect::render_gui(void *input)
{
	if(thread)
	{
		VideoScopeWindow *window = thread->window;
		window->lock_window();

//printf("VideoScopeEffect::process_realtime 1\n");
		this->input = (VFrame*)input;
//printf("VideoScopeEffect::process_realtime 1\n");


		if(!engine)
		{
			engine = new VideoScopeEngine(this, 
				(PluginClient::smp + 1));
		}

//printf("VideoScopeEffect::process_realtime 1 %d\n", PluginClient::smp);
// Clear bitmaps
		bzero(window->waveform_bitmap->get_data(), 
			window->waveform_bitmap->get_h() * 
			window->waveform_bitmap->get_bytes_per_line());
		bzero(window->vector_bitmap->get_data(), 
			window->vector_bitmap->get_h() * 
			window->vector_bitmap->get_bytes_per_line());

		engine->process_packages();
//printf("VideoScopeEffect::process_realtime 2\n");
//printf("VideoScopeEffect::process_realtime 1\n");

		window->waveform->draw_bitmap(window->waveform_bitmap, 
			1,
			0,
			0);

//printf("VideoScopeEffect::process_realtime 1\n");
		window->vectorscope->draw_bitmap(window->vector_bitmap, 
			1,
			0,
			0);


		window->waveform->draw_graduations();
		window->vectorscope->draw_graduations();
		window->waveform->flash();
		window->vectorscope->flash();


		window->unlock_window();
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


static void draw_point(unsigned char **rows, 
	int color_model, 
	int x, 
	int y, 
	int r, 
	int g, 
	int b)
{
	switch(color_model)
	{
		case BC_BGR8888:
		{
			unsigned char *pixel = rows[y] + x * 4;
			pixel[0] = b;
			pixel[1] = g;
			pixel[2] = r;
			break;
		}
		case BC_BGR888:
			break;
		case BC_RGB565:
		{
			unsigned char *pixel = rows[y] + x * 2;
			pixel[0] = (r & 0xf8) | (g >> 5);
			pixel[1] = ((g & 0xfc) << 5) | (b >> 3);
			break;
		}
		case BC_BGR565:
			break;
		case BC_RGB8:
			break;
	}
}



// Brighten value and decrease contrast so low levels are visible against black.
// Value v is either R, G, or B.
static int brighten(int v)
{
	return (((256 - RGB_MIN) * v) + (256 * RGB_MIN))/256;
}



template<typename TYPE, typename TEMP_TYPE, int MAX, int COMPONENTS, bool USE_YUV>
void VideoScopeUnit::render_data(LoadPackage *package)
{
	VideoScopeWindow *window = plugin->thread->window;
	VideoScopePackage *pkg = (VideoScopePackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	int waveform_h = window->wave_h;
	int waveform_w = window->wave_w;
	int waveform_cmodel = window->waveform_bitmap->get_color_model();
	unsigned char **waveform_rows = window->waveform_bitmap->get_row_pointers();
	int vector_h = window->vector_bitmap->get_h();
	int vector_w = window->vector_bitmap->get_w();
	int vector_cmodel = window->vector_bitmap->get_color_model();
	unsigned char **vector_rows = window->vector_bitmap->get_row_pointers();
	float radius = vector_w / 2.0;

	for(int i = pkg->row1; i < pkg->row2; i++)
	{
		TYPE *in_row = (TYPE*)plugin->input->get_rows()[i];
		for(int j = 0; j < w; j++)
		{
			TYPE *in_pixel = in_row + j * COMPONENTS;
			float intensity;

/* Analyze pixel */
			if(USE_YUV) intensity = (float)*in_pixel / MAX;

			float h, s, v;
			TEMP_TYPE r, g, b;
			if(USE_YUV)
			{
				if(sizeof(TYPE) == 2)
				{
					yuv.yuv_to_rgb_16(r,
						g,
						b,
						in_pixel[0],
						in_pixel[1],
						in_pixel[2]);
				}
				else
				{
					yuv.yuv_to_rgb_8(r,
						g,
						b,
						in_pixel[0],
						in_pixel[1],
						in_pixel[2]);
				}
			}
			else
			{
				r = in_pixel[0];
				g = in_pixel[1];
				b = in_pixel[2];
			}

			HSV::rgb_to_hsv((float)r / MAX,
					(float)g / MAX,
					(float)b / MAX,
					h,
					s,
					v);

/* Calculate point's RGB, used in both waveform and vectorscope. */
			int ri, gi, bi;
			if(sizeof(TYPE) == 2)
			{
				ri = (int)(r/256);
				gi = (int)(g/256);
				bi = (int)(b/256);
			}
			else
			if(sizeof(TYPE) == 4)
			{
				ri = (int)(CLIP(r, 0, 1) * 0xff);
				gi = (int)(CLIP(g, 0, 1) * 0xff);
				bi = (int)(CLIP(b, 0, 1) * 0xff);
			}
			else
			{
				ri = (int)r;
				gi = (int)g;
				bi = (int)b;
			}

/* Brighten & decrease contrast so low levels are visible against black. */
			ri = brighten(ri);
			gi = brighten(gi);
			bi = brighten(bi);

/* Calculate waveform */
			if(!USE_YUV) intensity = v;
			int y = waveform_h -
				(int)roundf((intensity - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN) *
					    waveform_h);
			int x = j * waveform_w / w;
			if(x >= 0 && x < waveform_w && y >= 0 && y < waveform_h)
				draw_point(waveform_rows,
					waveform_cmodel,
					x,
					y,
					ri,
					gi,
					bi);

/* Calculate vectorscope */
			polar_to_cartesian(h, s, radius, x, y);
			CLAMP(x, 0, vector_w - 1);
			CLAMP(y, 0, vector_h - 1);
			draw_point(vector_rows,
				vector_cmodel,
				x,
				y,
				ri,
				gi,
				bi);
		}
	}
}



void VideoScopeUnit::process_package(LoadPackage *package)
{
	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			render_data<unsigned char, int, 0xff, 3, 0>(package);
			break;

		case BC_RGB_FLOAT:
			render_data<float, float, 1, 3, 0>(package);
			break;

		case BC_YUV888:
			render_data<unsigned char, int, 0xff, 3, 1>(package);
			break;

		case BC_RGB161616:
			render_data<uint16_t, int, 0xffff, 3, 0>(package);
			break;

		case BC_YUV161616:
			render_data<uint16_t, int, 0xffff, 3, 1>(package);
			break;

		case BC_RGBA8888:
			render_data<unsigned char, int, 0xff, 4, 0>(package);
			break;

		case BC_RGBA_FLOAT:
			render_data<float, float, 1, 4, 0>(package);
			break;

		case BC_YUVA8888:
			render_data<unsigned char, int, 0xff, 4, 1>(package);
			break;

		case BC_RGBA16161616:
			render_data<uint16_t, int, 0xffff, 4, 0>(package);
			break;

		case BC_YUVA16161616:
			render_data<uint16_t, int, 0xffff, 4, 1>(package);
			break;
	}
}






VideoScopeEngine::VideoScopeEngine(VideoScopeEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

VideoScopeEngine::~VideoScopeEngine()
{
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

