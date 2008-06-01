#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "histogramengine.h"
#include "language.h"
#include "plugincolors.h"
#include "threshold.h"
#include "thresholdwindow.h"
#include "vframe.h"

#include <string.h>
#include <string>

using std::string;



ThresholdConfig::ThresholdConfig()
{
	reset();
}

int ThresholdConfig::equivalent(ThresholdConfig &that)
{
	return EQUIV(min, that.min) &&
		EQUIV(max, that.max) &&
		plot == that.plot &&
		low_color == that.low_color &&
		mid_color == that.mid_color &&
		high_color == that.high_color;
}

void ThresholdConfig::copy_from(ThresholdConfig &that)
{
	min = that.min;
	max = that.max;
	plot = that.plot;
	low_color = that.low_color;
	mid_color = that.mid_color;
	high_color = that.high_color;
}

// General purpose scale function.
template<typename T>
T interpolate(const T & prev, const double & prev_scale, const T & next, const double & next_scale)
{
	return static_cast<T>(prev * prev_scale + next * next_scale);
}

void ThresholdConfig::interpolate(ThresholdConfig &prev,
	ThresholdConfig &next,
	int64_t prev_frame, 
	int64_t next_frame, 
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / 
		(next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / 
		(next_frame - prev_frame);

	min = ::interpolate(prev.min, prev_scale, next.min, next_scale);
	max = ::interpolate(prev.max, prev_scale, next.max, next_scale);
	plot = prev.plot;

	low_color =  ::interpolate(prev.low_color,  prev_scale, next.low_color,  next_scale);
	mid_color =  ::interpolate(prev.mid_color,  prev_scale, next.mid_color,  next_scale);
	high_color = ::interpolate(prev.high_color, prev_scale, next.high_color, next_scale);
}

void ThresholdConfig::reset()
{
	min = 0.0;
	max = 1.0;
	plot = 1;
	low_color.set (0x0,  0x0,  0x0,  0xff);
	mid_color.set (0xff, 0xff, 0xff, 0xff);
	high_color.set(0x0,  0x0,  0x0,  0xff);
}

void ThresholdConfig::boundaries()
{
	CLAMP(min, HISTOGRAM_MIN, max);
	CLAMP(max, min, HISTOGRAM_MAX);
}








REGISTER_PLUGIN(ThresholdMain)

ThresholdMain::ThresholdMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	engine = 0;
	threshold_engine = 0;
}

ThresholdMain::~ThresholdMain()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete engine;
	delete threshold_engine;
}

int ThresholdMain::is_realtime()
{
	return 1;
}

char* ThresholdMain::plugin_title() 
{ 
	return N_("Threshold"); 
}


#include "picon_png.h"
NEW_PICON_MACRO(ThresholdMain)

SHOW_GUI_MACRO(ThresholdMain, ThresholdThread)

SET_STRING_MACRO(ThresholdMain)

RAISE_WINDOW_MACRO(ThresholdMain)

LOAD_CONFIGURATION_MACRO(ThresholdMain, ThresholdConfig)







int ThresholdMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	int use_opengl = get_use_opengl() &&
		(!config.plot || !gui_open());

	read_frame(frame,
		0,
		get_source_position(),
		get_framerate(),
		use_opengl);

	if(use_opengl) return run_opengl();

	send_render_gui(frame);

	if(!threshold_engine)
		threshold_engine = new ThresholdEngine(this);
	threshold_engine->process_packages(frame);
	
	return 0;
}

int ThresholdMain::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%sthreshold.rc", BCASTDIR);
	defaults = new BC_Hash(directory);
	defaults->load();
	config.min = defaults->get("MIN", config.min);
	config.max = defaults->get("MAX", config.max);
	config.plot = defaults->get("PLOT", config.plot);
	config.low_color.load_default(defaults,  "LOW_COLOR");
	config.mid_color.load_default(defaults,  "MID_COLOR");
	config.high_color.load_default(defaults, "HIGH_COLOR");
	config.boundaries();
	return 0;
}

int ThresholdMain::save_defaults()
{
	defaults->update("MIN", config.min);
	defaults->update("MAX", config.max);
	defaults->update("PLOT", config.plot);
	config.low_color.save_defaults(defaults,  "LOW_COLOR");
	config.mid_color.save_defaults(defaults,  "MID_COLOR");
	config.high_color.save_defaults(defaults, "HIGH_COLOR");
	defaults->save();
}

void ThresholdMain::save_data(KeyFrame *keyframe)
{
	FileXML file;
	file.set_shared_string(keyframe->data, MESSAGESIZE);
	file.tag.set_title("THRESHOLD");
	file.tag.set_property("MIN", config.min);
	file.tag.set_property("MAX", config.max);
	file.tag.set_property("PLOT", config.plot);
	config.low_color.set_property(file.tag,  "LOW_COLOR");
	config.mid_color.set_property(file.tag,  "MID_COLOR");
	config.high_color.set_property(file.tag, "HIGH_COLOR");
	file.append_tag();
	file.tag.set_title("/THRESHOLD");
	file.append_tag();
	file.terminate_string();
}

void ThresholdMain::read_data(KeyFrame *keyframe)
{
	FileXML file;
	file.set_shared_string(keyframe->data, strlen(keyframe->data));
	int result = 0;
	while(!result)
	{
		result = file.read_tag();
		if(!result)
		{
			config.min = file.tag.get_property("MIN", config.min);
			config.max = file.tag.get_property("MAX", config.max);
			config.plot = file.tag.get_property("PLOT", config.plot);
			config.low_color = config.low_color.get_property(file.tag, "LOW_COLOR");
			config.mid_color = config.mid_color.get_property(file.tag, "MID_COLOR");
			config.high_color = config.high_color.get_property(file.tag, "HIGH_COLOR");
		}
	}
	config.boundaries();
}

void ThresholdMain::update_gui()
{
	if(thread)
	{
		thread->window->lock_window("ThresholdMain::update_gui");
		if(load_configuration())
		{
			thread->window->min->update(config.min);
			thread->window->max->update(config.max);
			thread->window->plot->update(config.plot);
			thread->window->update_low_color();
			thread->window->update_mid_color();
			thread->window->update_high_color();
			thread->window->low_color_thread->update_gui(config.low_color.getRGB(), config.low_color.a);
			thread->window->mid_color_thread->update_gui(config.mid_color.getRGB(), config.mid_color.a);
			thread->window->high_color_thread->update_gui(config.high_color.getRGB(), config.high_color.a);
		}
		thread->window->unlock_window();
	}
}

void ThresholdMain::render_gui(void *data)
{
	if(thread)
	{
		calculate_histogram((VFrame*)data);
		thread->window->lock_window("ThresholdMain::render_gui");
		thread->window->canvas->draw();
		thread->window->unlock_window();
	}
}

void ThresholdMain::calculate_histogram(VFrame *frame)
{
	if(!engine) engine = new HistogramEngine(get_project_smp() + 1,
		get_project_smp() + 1);
	engine->process_packages(frame);
}

int ThresholdMain::handle_opengl()
{
#ifdef HAVE_GL
	static char *rgb_shader = 
		"uniform sampler2D tex;\n"
		"uniform float min;\n"
		"uniform float max;\n"
		"uniform vec4 low_color;\n"
		"uniform vec4 mid_color;\n"
		"uniform vec4 high_color;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
		"	float v = dot(pixel.rgb, vec3(0.299, 0.587, 0.114));\n"
		"	if(v < min)\n"
		"		pixel = low_color;\n"
		"	else if(v < max)\n"
		"		pixel = mid_color;\n"
		"	else\n"
		"		pixel = high_color;\n"
		"	gl_FragColor = pixel;\n"
		"}\n";

	static char *yuv_shader = 
		"uniform sampler2D tex;\n"
		"uniform float min;\n"
		"uniform float max;\n"
		"uniform vec4 low_color;\n"
		"uniform vec4 mid_color;\n"
		"uniform vec4 high_color;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
		"	if(pixel.r < min)\n"
		"		pixel = low_color;\n"
		"	else if(pixel.r < max)\n"
		"		pixel = mid_color;\n"
		"	else\n"
		"		pixel = high_color;\n"
		"	gl_FragColor = pixel;\n"
		"}\n";

	get_output()->to_texture();
	get_output()->enable_opengl();

	unsigned int shader = 0;
	int color_model = get_output()->get_color_model();
	bool is_yuv = cmodel_is_yuv(color_model);
	bool has_alpha = cmodel_has_alpha(color_model);
	if(is_yuv)
		shader = VFrame::make_shader(0, yuv_shader, 0);
	else
		shader = VFrame::make_shader(0, rgb_shader, 0);

	if(shader > 0)
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		glUniform1f(glGetUniformLocation(shader, "min"), config.min);
		glUniform1f(glGetUniformLocation(shader, "max"), config.max);

		if (is_yuv)
		{
			float y_low,  u_low,  v_low;
			float y_mid,  u_mid,  v_mid;
			float y_high, u_high, v_high;

			YUV::rgb_to_yuv_f((float)config.low_color.r / 0xff,
					  (float)config.low_color.g / 0xff,
					  (float)config.low_color.b / 0xff,
					  y_low,
					  u_low,
					  v_low);
			u_low += 0.5;
			v_low += 0.5;
			YUV::rgb_to_yuv_f((float)config.mid_color.r / 0xff,
					  (float)config.mid_color.g / 0xff,
					  (float)config.mid_color.b / 0xff,
					  y_mid,
					  u_mid,
					  v_mid);
			u_mid += 0.5;
			v_mid += 0.5;
			YUV::rgb_to_yuv_f((float)config.high_color.r / 0xff,
					  (float)config.high_color.g / 0xff,
					  (float)config.high_color.b / 0xff,
					  y_high,
					  u_high,
					  v_high);
			u_high += 0.5;
			v_high += 0.5;

			glUniform4f(glGetUniformLocation(shader, "low_color"),
				    y_low, u_low, v_low,
				    has_alpha ? (float)config.low_color.a / 0xff : 1.0);
			glUniform4f(glGetUniformLocation(shader, "mid_color"),
				    y_mid, u_mid, v_mid,
				    has_alpha ? (float)config.mid_color.a / 0xff : 1.0);
			glUniform4f(glGetUniformLocation(shader, "high_color"),
				    y_high, u_high, v_high,
				    has_alpha ? (float)config.high_color.a / 0xff : 1.0);
		} else {
			glUniform4f(glGetUniformLocation(shader, "low_color"),
				    (float)config.low_color.r / 0xff,
				    (float)config.low_color.g / 0xff,
				    (float)config.low_color.b / 0xff,
				    has_alpha ? (float)config.low_color.a / 0xff : 1.0);
			glUniform4f(glGetUniformLocation(shader, "mid_color"),
				    (float)config.mid_color.r / 0xff,
				    (float)config.mid_color.g / 0xff,
				    (float)config.mid_color.b / 0xff,
				    has_alpha ? (float)config.mid_color.a / 0xff : 1.0);
			glUniform4f(glGetUniformLocation(shader, "high_color"),
				    (float)config.high_color.r / 0xff,
				    (float)config.high_color.g / 0xff,
				    (float)config.high_color.b / 0xff,
				    has_alpha ? (float)config.high_color.a / 0xff : 1.0);
		}
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
}























ThresholdPackage::ThresholdPackage()
 : LoadPackage()
{
	start = end = 0;
}











ThresholdUnit::ThresholdUnit(ThresholdEngine *server)
 : LoadClient(server)
{
	this->server = server;
}

// Coerces pixel component to int.
static inline int get_component(unsigned char v)
{
	return (v << 8) | v;
}

static inline int get_component(float v)
{
	return (int)(v * 0xffff);
}

static inline int get_component(uint16_t v)
{
	return v;
}

// Rescales value in range [0, 255] to range appropriate to TYPE.
template<typename TYPE>
static TYPE scale_to_range(int v)
{
	return v;  // works for unsigned char, override for the rest.
}

template<>
inline float scale_to_range(int v)
{
	return (float) v / 0xff;
}

template<>
inline uint16_t scale_to_range(int v)
{
	return v << 8 | v;
}

static inline void rgb_to_yuv(YUV & yuv,
			      unsigned char   r, unsigned char   g, unsigned char   b,
			      unsigned char & y, unsigned char & u, unsigned char & v)
{
	yuv.rgb_to_yuv_8(r, g, b, y, u, v);
}

static inline void rgb_to_yuv(YUV & yuv,
			      float   r, float   g, float   b,
			      float & y, float & u, float & v)
{
	yuv.rgb_to_yuv_f(r, g, b, y, u, v);
}

static inline void rgb_to_yuv(YUV & yuv,
			      uint16_t   r, uint16_t   g, uint16_t   b,
			      uint16_t & y, uint16_t & u, uint16_t & v)
{
	yuv.rgb_to_yuv_16(r, g, b, y, u, v);
}

template<typename TYPE, int COMPONENTS, bool USE_YUV>
void ThresholdUnit::render_data(LoadPackage *package)
{
	const ThresholdPackage *pkg = (ThresholdPackage*)package;
	const ThresholdConfig *config = & server->plugin->config;
	VFrame *data = server->data;
	const int min = (int)(config->min * 0xffff);
	const int max = (int)(config->max * 0xffff);
	const int w = data->get_w();
	const int h = data->get_h();
	
	const TYPE r_low = scale_to_range<TYPE>(config->low_color.r);
	const TYPE g_low = scale_to_range<TYPE>(config->low_color.g);
	const TYPE b_low = scale_to_range<TYPE>(config->low_color.b);
	const TYPE a_low = scale_to_range<TYPE>(config->low_color.a);
	
	const TYPE r_mid = scale_to_range<TYPE>(config->mid_color.r);
	const TYPE g_mid = scale_to_range<TYPE>(config->mid_color.g);
	const TYPE b_mid = scale_to_range<TYPE>(config->mid_color.b);
	const TYPE a_mid = scale_to_range<TYPE>(config->mid_color.a);
	
	const TYPE r_high = scale_to_range<TYPE>(config->high_color.r);
	const TYPE g_high = scale_to_range<TYPE>(config->high_color.g);
	const TYPE b_high = scale_to_range<TYPE>(config->high_color.b);
	const TYPE a_high = scale_to_range<TYPE>(config->high_color.a);

	TYPE y_low,  u_low,  v_low;
	TYPE y_mid,  u_mid,  v_mid;
	TYPE y_high, u_high, v_high;

	if (USE_YUV)
	{
		rgb_to_yuv(*server->yuv, r_low,  g_low,  b_low,  y_low,  u_low,  v_low);
		rgb_to_yuv(*server->yuv, r_mid,  g_mid,  b_mid,  y_mid,  u_mid,  v_mid);
		rgb_to_yuv(*server->yuv, r_high, g_high, b_high, y_high, u_high, v_high);
	}

	for(int i = pkg->start; i < pkg->end; i++)
	{
		TYPE *in_row = (TYPE*)data->get_rows()[i];
		TYPE *out_row = in_row;
		for(int j = 0; j < w; j++)
		{
			if (USE_YUV)
			{
				const int y = get_component(in_row[0]);
				if (y < min)
				{
					*out_row++ = y_low;
					*out_row++ = u_low;
					*out_row++ = v_low;
					if(COMPONENTS == 4) *out_row++ = a_low;
				}
				else if (y < max)
				{
					*out_row++ = y_mid;
					*out_row++ = u_mid;
					*out_row++ = v_mid;
					if(COMPONENTS == 4) *out_row++ = a_mid;
				}
				else
				{
					*out_row++ = y_high;
					*out_row++ = u_high;
					*out_row++ = v_high;
					if(COMPONENTS == 4) *out_row++ = a_high;
				}
			}
			else
			{
				const int r = get_component(in_row[0]);
				const int g = get_component(in_row[1]);
				const int b = get_component(in_row[2]);
				const int y = (r * 76 + g * 150 + b * 29) >> 8;
				if (y < min)
				{
					*out_row++ = r_low;
					*out_row++ = g_low;
					*out_row++ = b_low;
					if(COMPONENTS == 4) *out_row++ = a_low;
				}
				else if (y < max)
				{
					*out_row++ = r_mid;
					*out_row++ = g_mid;
					*out_row++ = b_mid;
					if(COMPONENTS == 4) *out_row++ = a_mid;
				}
				else
				{
					*out_row++ = r_high;
					*out_row++ = g_high;
					*out_row++ = b_high;
					if(COMPONENTS == 4) *out_row++ = a_high;
				}
			}
			in_row += COMPONENTS;
		}
	}
}

void ThresholdUnit::process_package(LoadPackage *package)
{
	switch(server->data->get_color_model())
	{
		case BC_RGB888:
			render_data<unsigned char, 3, false>(package);
			break;

		case BC_RGB_FLOAT:
			render_data<float, 3, false>(package);
			break;

		case BC_RGBA8888:
			render_data<unsigned char, 4, false>(package);
			break;

		case BC_RGBA_FLOAT:
			render_data<float, 4, false>(package);
			break;

		case BC_YUV888:
			render_data<unsigned char, 3, true>(package);
			break;

		case BC_YUVA8888:
			render_data<unsigned char, 4, true>(package);
			break;

		case BC_YUV161616:
			render_data<uint16_t, 3, true>(package);
			break;

		case BC_YUVA16161616:
			render_data<uint16_t, 4, true>(package);
			break;
	}
}











ThresholdEngine::ThresholdEngine(ThresholdMain *plugin)
 : LoadServer(plugin->get_project_smp() + 1,
 	plugin->get_project_smp() + 1)
{
	this->plugin = plugin;
	yuv = new YUV;
}

ThresholdEngine::~ThresholdEngine()
{
	delete yuv;
}

void ThresholdEngine::process_packages(VFrame *data)
{
	this->data = data;
	LoadServer::process_packages();
}

void ThresholdEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		ThresholdPackage *package = (ThresholdPackage*)get_package(i);
		package->start = data->get_h() * i / get_total_packages();
		package->end = data->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* ThresholdEngine::new_client()
{
	return (LoadClient*)new ThresholdUnit(this);
}

LoadPackage* ThresholdEngine::new_package()
{
	return (LoadPackage*)new HistogramPackage;
}








RGBA::RGBA()
{
	r = g = b = a = 0;
}

RGBA::RGBA(int r, int g, int b, int a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

void RGBA::set(int r, int g, int b, int a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

void RGBA::set(int rgb, int alpha)
{
	r = (rgb & 0xff0000) >> 16;
	g = (rgb & 0xff00)   >>  8;
	b = (rgb & 0xff);
	a = alpha;
}

int RGBA::getRGB() const
{
	return r << 16 | g << 8 | b;
}

static void init_RGBA_keys(const char * prefix,
			   string & r_s,
			   string & g_s,
			   string & b_s,
			   string & a_s)
{
	r_s = prefix;
	g_s = prefix;
	b_s = prefix;
	a_s = prefix;

	r_s += "_R";
	g_s += "_G";
	b_s += "_B";
	a_s += "_A";
}

RGBA RGBA::load_default(BC_Hash * defaults, const char * prefix) const
{
	string r_s, g_s, b_s, a_s;
	init_RGBA_keys(prefix, r_s, g_s, b_s, a_s);

	return RGBA(defaults->get(const_cast<char *>(r_s.c_str()), r),
		    defaults->get(const_cast<char *>(g_s.c_str()), g),
		    defaults->get(const_cast<char *>(b_s.c_str()), b),
		    defaults->get(const_cast<char *>(a_s.c_str()), a));
}

void RGBA::save_defaults(BC_Hash * defaults, const char * prefix) const
{
	string r_s, g_s, b_s, a_s;
	init_RGBA_keys(prefix, r_s, g_s, b_s, a_s);

	defaults->update(const_cast<char *>(r_s.c_str()), r);
	defaults->update(const_cast<char *>(g_s.c_str()), g);
	defaults->update(const_cast<char *>(b_s.c_str()), b);
	defaults->update(const_cast<char *>(a_s.c_str()), a);
}

void RGBA::set_property(XMLTag & tag, const char * prefix) const
{
	string r_s, g_s, b_s, a_s;
	init_RGBA_keys(prefix, r_s, g_s, b_s, a_s);

	tag.set_property(const_cast<char *>(r_s.c_str()), r);
	tag.set_property(const_cast<char *>(g_s.c_str()), g);
	tag.set_property(const_cast<char *>(b_s.c_str()), b);
	tag.set_property(const_cast<char *>(a_s.c_str()), a);
}

RGBA RGBA::get_property(XMLTag & tag, const char * prefix) const
{
	string r_s, g_s, b_s, a_s;
	init_RGBA_keys(prefix, r_s, g_s, b_s, a_s);

	return RGBA(tag.get_property(const_cast<char *>(r_s.c_str()), r),
		    tag.get_property(const_cast<char *>(g_s.c_str()), g),
		    tag.get_property(const_cast<char *>(b_s.c_str()), b),
		    tag.get_property(const_cast<char *>(a_s.c_str()), a));
}

bool operator==(const RGBA & a, const RGBA & b)
{
	return  a.r == b.r &&
		a.g == b.g &&
		a.b == b.b &&
		a.a == b.a;
}

template<>
RGBA interpolate(const RGBA & prev_color, const double & prev_scale, const RGBA &next_color, const double & next_scale)
{
	return RGBA(interpolate(prev_color.r, prev_scale, next_color.r, next_scale),
		    interpolate(prev_color.g, prev_scale, next_color.g, next_scale),
		    interpolate(prev_color.b, prev_scale, next_color.b, next_scale),
		    interpolate(prev_color.a, prev_scale, next_color.a, next_scale));
}
