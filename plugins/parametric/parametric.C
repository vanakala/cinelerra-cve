#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "filexml.h"
#include "parametric.h"
#include "picon_png.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>








PluginClient* new_plugin(PluginServer *server)
{
	return new ParametricEQ(server);
}








ParametricBand::ParametricBand()
{
	freq = 440;
	quality = 1;
	magnitude = 0;
	mode = NONE;
}


int ParametricBand::equivalent(ParametricBand &that)
{
	if(freq == that.freq && 
		EQUIV(quality, that.quality) && 
		EQUIV(magnitude, that.magnitude) &&
		mode == that.mode)
	{
		return 1;
	}
	else
		return 0;
}


void ParametricBand::copy_from(ParametricBand &that)
{
	freq = that.freq;
	quality = that.quality;
	magnitude = that.magnitude;
	mode = that.mode;
}

void ParametricBand::interpolate(ParametricBand &prev, 
		ParametricBand &next, 
		double prev_scale, 
		double next_scale)
{
	freq = (int)(prev.freq * prev_scale + next.freq * next_scale + 0.5);
	quality = prev.quality * prev_scale + next.quality * next_scale;
	magnitude = prev.magnitude * prev_scale + next.magnitude * next_scale;
	mode = prev.mode;
}





ParametricConfig::ParametricConfig()
{
	wetness = INFINITYGAIN;
}


int ParametricConfig::equivalent(ParametricConfig &that)
{
	for(int i = 0; i < BANDS; i++)
		if(!band[i].equivalent(that.band[i])) return 0;

	if(!EQUIV(wetness, that.wetness)) return 0;
	return 1;
}

void ParametricConfig::copy_from(ParametricConfig &that)
{
	wetness = that.wetness;
	for(int i = 0; i < BANDS; i++)
		band[i].copy_from(that.band[i]);
}

void ParametricConfig::interpolate(ParametricConfig &prev, 
		ParametricConfig &next, 
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	wetness = prev.wetness;
	for(int i = 0; i < BANDS; i++)
	{
		band[i].interpolate(prev.band[i], next.band[i], prev_scale, next_scale);
	}
}



















ParametricFreq::ParametricFreq(ParametricEQ *plugin, int x, int y, int band)
 : BC_QPot(x, y, plugin->config.band[band].freq)
{
	this->plugin = plugin;
	this->band = band;
}

int ParametricFreq::handle_event()
{
	plugin->config.band[band].freq = get_value();
	plugin->send_configure_change();
	plugin->thread->window->update_canvas();
	return 1;
}








ParametricQuality::ParametricQuality(ParametricEQ *plugin, int x, int y, int band)
 : BC_FPot(x, y, plugin->config.band[band].quality, 0, 1)
{
	this->plugin = plugin;
	this->band = band;
	set_precision(0.01);
}

int ParametricQuality::handle_event()
{
	plugin->config.band[band].quality = get_value();
	plugin->send_configure_change();
	plugin->thread->window->update_canvas();
	return 1;
}











ParametricMagnitude::ParametricMagnitude(ParametricEQ *plugin, int x, int y, int band)
 : BC_FPot(x, y, plugin->config.band[band].magnitude, -MAXMAGNITUDE, MAXMAGNITUDE)
{
	this->plugin = plugin;
	this->band = band;
}

int ParametricMagnitude::handle_event()
{
	plugin->config.band[band].magnitude = get_value();
	plugin->send_configure_change();
	plugin->thread->window->update_canvas();
	return 1;
}









ParametricMode::ParametricMode(ParametricEQ *plugin, int x, int y, int band)
 : BC_PopupMenu(x, 
		y, 
		150, 
		mode_to_text(plugin->config.band[band].mode))
{
//printf("ParametricMode::ParametricMode %d %d\n", band, plugin->config.band[band].mode);
	this->plugin = plugin;
	this->band = band;
}

void ParametricMode::create_objects()
{
	add_item(new BC_MenuItem(mode_to_text(ParametricBand::LOWPASS)));
	add_item(new BC_MenuItem(mode_to_text(ParametricBand::HIGHPASS)));
	add_item(new BC_MenuItem(mode_to_text(ParametricBand::BANDPASS)));
	add_item(new BC_MenuItem(mode_to_text(ParametricBand::NONE)));
}


int ParametricMode::handle_event()
{
	plugin->config.band[band].mode = text_to_mode(get_text());
	plugin->send_configure_change();
	plugin->thread->window->update_canvas();
	return 1;
}

int ParametricMode::text_to_mode(char *text)
{
	if(!strcmp(mode_to_text(ParametricBand::LOWPASS), text)) return ParametricBand::LOWPASS;
	if(!strcmp(mode_to_text(ParametricBand::HIGHPASS), text)) return ParametricBand::HIGHPASS;
	if(!strcmp(mode_to_text(ParametricBand::BANDPASS), text)) return ParametricBand::BANDPASS;
	if(!strcmp(mode_to_text(ParametricBand::NONE), text)) return ParametricBand::NONE;
	return ParametricBand::BANDPASS;
}



char* ParametricMode::mode_to_text(int mode)
{
	switch(mode)
	{
		case ParametricBand::LOWPASS:
			return "Lowpass";
			break;
		case ParametricBand::HIGHPASS:
			return "Highpass";
			break;
		case ParametricBand::BANDPASS:
			return "Bandpass";
			break;
		case ParametricBand::NONE:
			return "None";
			break;
	}
	return "";
}











ParametricBandGUI::ParametricBandGUI(ParametricEQ *plugin, ParametricWindow *window, int x, int y, int band)
{
	this->plugin = plugin;
	this->band = band;
	this->window = window;
	this->x = x;
	this->y = y;
}

ParametricBandGUI::~ParametricBandGUI()
{
}


#define X1 10
#define X2 60
#define X3 110
#define X4 160

	
void ParametricBandGUI::create_objects()
{
	window->add_subwindow(freq = new ParametricFreq(plugin, X1, y, band));
	window->add_subwindow(quality = new ParametricQuality(plugin, X2, y, band));
	window->add_subwindow(magnitude = new ParametricMagnitude(plugin, X3, y, band));
	window->add_subwindow(mode = new ParametricMode(plugin, X4, y, band));
	mode->create_objects();
}

void ParametricBandGUI::update_gui()
{
	freq->update(plugin->config.band[band].freq);
	quality->update(plugin->config.band[band].quality);
	magnitude->update(plugin->config.band[band].magnitude);
}






ParametricWetness::ParametricWetness(ParametricEQ *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.wetness, INFINITYGAIN, 0)
{
	this->plugin = plugin;
}

int ParametricWetness::handle_event()
{
	plugin->config.wetness = get_value();
	plugin->send_configure_change();
	plugin->thread->window->update_canvas();
	return 1;
}






ParametricWindow::ParametricWindow(ParametricEQ *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	320, 
	400, 
	320, 
	400,
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

ParametricWindow::~ParametricWindow()
{
	for(int i = 0; i < BANDS; i++)
		delete bands[i];
}

void ParametricWindow::create_objects()
{
	int y = 35;
	
	
	add_subwindow(new BC_Title(X1, 10, "Freq"));
	add_subwindow(new BC_Title(X2, 10, "Qual"));
	add_subwindow(new BC_Title(X3, 10, "Level"));
	add_subwindow(new BC_Title(X4, 10, "Mode"));
	for(int i = 0; i < BANDS; i++)
	{
		bands[i] = new ParametricBandGUI(plugin, this, 10, y, i);
		bands[i]->create_objects();
		y += 50;
	}

	add_subwindow(new BC_Title(10, y + 10, "Wetness:"));
	add_subwindow(wetness = new ParametricWetness(plugin, 80, y));
	y += 50;
	add_subwindow(canvas = new BC_SubWindow(10, y, get_w() - 20, get_h() - y - 10, WHITE));
	
	
	update_canvas();
	show_window();
	flush();
}

int ParametricWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}

void ParametricWindow::update_gui()
{
	for(int i = 0; i < BANDS; i++)
		bands[i]->update_gui();
	wetness->update(plugin->config.wetness);
	update_canvas();
}


void ParametricWindow::update_canvas()
{
//printf("ParametricWindow::update_canvas 1\n");
	double scale = 1;
	int y1 = canvas->get_h() / 2;
	double normalize = DB::fromdb(MAXMAGNITUDE);
	int niquist = plugin->PluginAClient::project_sample_rate / 2;

	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	canvas->set_color(GREEN);
	canvas->draw_line(0, 
		canvas->get_h() - (int)((double)canvas->get_h() / normalize), 
		canvas->get_w(), 
		canvas->get_h() - (int)((double)canvas->get_h() / normalize));

	canvas->set_color(BLACK);

	plugin->calculate_envelope();
	for(int i = 0; i < canvas->get_w(); i++)
	{
		int freq = Freq::tofreq((int)((float)i / canvas->get_w() * TOTALFREQS));
		int index = (int)((float)freq / niquist * WINDOW_SIZE / 2);
		double magnitude = plugin->envelope[index];
		int y2 = canvas->get_h() - 
			(int)((double)canvas->get_h() / normalize * magnitude);
		if(i > 0) canvas->draw_line(i - 1, y1, i, y2);
		y1 = y2;
	}

	canvas->flash();
	flush();
}







PLUGIN_THREAD_OBJECT(ParametricEQ, ParametricThread, ParametricWindow)







ParametricFFT::ParametricFFT(ParametricEQ *plugin)
 : CrossfadeFFT()
{
	this->plugin = plugin;
}

ParametricFFT::~ParametricFFT()
{
}


int ParametricFFT::signal_process()
{
//printf("ParametricFFT::signal_process 1\n");
	for(int i = 0; i < window_size / 2; i++)
	{
		double result = plugin->envelope[i] * sqrt(freq_real[i] * freq_real[i] + freq_imag[i] * freq_imag[i]);
		double angle = atan2(freq_imag[i], freq_real[i]);
		freq_real[i] = result * cos(angle);
		freq_imag[i] = result * sin(angle);
	}
//printf("ParametricFFT::signal_process 2\n");

	symmetry(window_size, freq_real, freq_imag);
//printf("ParametricFFT::signal_process 3\n");
	return 0;
}










ParametricEQ::ParametricEQ(PluginServer *server)
 : PluginAClient(server)
{
//printf("ParametricEQ::ParametricEQ 1\n");
	reset();
	load_defaults();
//printf("ParametricEQ::ParametricEQ 2\n");
}

ParametricEQ::~ParametricEQ()
{
	if(thread)
	{
		thread->window->set_done(0);
		thread->completion.lock();
		delete thread;
	}
	
	save_defaults();
	delete defaults;

	if(fft) delete fft;
}

NEW_PICON_MACRO(ParametricEQ)

SHOW_GUI_MACRO(ParametricEQ, ParametricThread)

RAISE_WINDOW_MACRO(ParametricEQ)

SET_STRING_MACRO(ParametricEQ)

LOAD_CONFIGURATION_MACRO(ParametricEQ, ParametricConfig)


char* ParametricEQ::plugin_title()
{
	return "EQ Parametric";
}

int ParametricEQ::is_realtime()
{
	return 1;
}

void ParametricEQ::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("PARAMETRICEQ"))
			{
				config.wetness = input.tag.get_property("WETNESS", config.wetness);
			}
			else
			if(input.tag.title_is("BAND"))
			{
				int band = input.tag.get_property("NUMBER", 0);
				config.band[band].freq = input.tag.get_property("FREQ", config.band[band].freq);
				config.band[band].quality = input.tag.get_property("QUALITY", config.band[band].quality);
				config.band[band].magnitude = input.tag.get_property("MAGNITUDE", config.band[band].magnitude);
				config.band[band].mode = input.tag.get_property("MODE", config.band[band].mode);
			}
		}
	}
}

void ParametricEQ::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("PARAMETRICEQ");
	output.tag.set_property("WETNESS", config.wetness);
	output.append_tag();
	output.append_newline();

	for(int i = 0; i < BANDS; i++)
	{
		output.tag.set_title("BAND");
		output.tag.set_property("NUMBER", i);
		output.tag.set_property("FREQ", config.band[i].freq);
		output.tag.set_property("QUALITY", config.band[i].quality);
		output.tag.set_property("MAGNITUDE", config.band[i].magnitude);
		output.tag.set_property("MODE", config.band[i].mode);
		output.append_tag();
		output.append_newline();
	}

	output.terminate_string();
}

void ParametricEQ::reconfigure()
{
	if(!fft)
	{
		fft = new ParametricFFT(this);
		fft->initialize(WINDOW_SIZE);
	}

// Reset envelope

//printf("ParametricEQ::reconfigure %f\n", wetness);
	calculate_envelope();

	for(int i = 0; i < WINDOW_SIZE / 2; i++)
	{
		if(envelope[i] < 0) envelope[i] = 0;
	}

	need_reconfigure = 0;
}

double ParametricEQ::calculate_envelope()
{
	double wetness = DB::fromdb(config.wetness);
	int niquist = PluginAClient::project_sample_rate / 2;
//printf("ParametricEQ::calculate_envelope %d %d\n", niquist, PluginAClient::project_sample_rate);


	for(int i = 0; i < WINDOW_SIZE / 2; i++)
	{
		envelope[i] = wetness;
	}

	for(int pass = 0; pass < 2; pass++)
	{
		for(int band = 0; band < BANDS; band++)
		{
			switch(config.band[band].mode)
			{
				case ParametricBand::LOWPASS:
					if(pass == 1)
					{
						double magnitude = DB::fromdb(config.band[band].magnitude);
						int cutoff = (int)((float)config.band[band].freq / niquist * WINDOW_SIZE / 2);
						for(int i = 0; i < WINDOW_SIZE / 2; i++)
						{
							if(i < cutoff) 
								envelope[i] += magnitude;
						}
					}
					break;

				case ParametricBand::HIGHPASS:
					if(pass == 1)
					{
						double magnitude = DB::fromdb(config.band[band].magnitude);
						int cutoff = (int)((float)config.band[band].freq / niquist * WINDOW_SIZE / 2);
						for(int i = 0; i < WINDOW_SIZE / 2; i++)
						{
							if(i > cutoff) 
								envelope[i] += magnitude;
						}
					}
					break;

				case ParametricBand::BANDPASS:
					if(pass == 0)
					{
						double magnitude = (config.band[band].magnitude > 0) ? 
							(DB::fromdb(config.band[band].magnitude) - 1) : 
							(-1 + DB::fromdb(config.band[band].magnitude));
						double sigma = (config.band[band].quality < 1) ?
							(1.0 - config.band[band].quality) :
							0.01;
						sigma /= 4;
						double a = (double)config.band[band].freq / niquist;
						double normalize = gauss(sigma, 0, 0);
						if(config.band[band].magnitude <= -MAXMAGNITUDE) 
							magnitude = -1;

						for(int i = 0; i < WINDOW_SIZE / 2; i++)
							envelope[i] += magnitude * 
								gauss(sigma, a, (double)i / (WINDOW_SIZE / 2)) / 
								normalize;
					}
					break;
			}
		}
	}
	return 0;
}

double ParametricEQ::gauss(double sigma, double a, double x)
{
	if(EQUIV(sigma, 0)) sigma = 0.01;

	return 1 / 
		sqrt(2 * M_PI * sigma * sigma) * 
		exp(-(x - a) * (x - a) / 
			(2 * sigma * sigma));
}

int ParametricEQ::process_realtime(int64_t size, 
	double *input_ptr, 
	double *output_ptr)
{
	need_reconfigure |= load_configuration();
	if(need_reconfigure) reconfigure();
	
	
	fft->process_fifo(size, input_ptr, output_ptr);
}









int ParametricEQ::load_defaults()
{
	char directory[BCTEXTLEN], string[BCTEXTLEN];
	sprintf(directory, "%sparametriceq.rc", BCASTDIR);
	defaults = new Defaults(directory);
	defaults->load();
	
	config.wetness = defaults->get("WETNESS", config.wetness);
	for(int i = 0; i < BANDS; i++)
	{
		sprintf(string, "FREQ_%d", i);
		config.band[i].freq = defaults->get(string, config.band[i].freq);
		sprintf(string, "QUALITY_%d", i);
		config.band[i].quality = defaults->get(string, config.band[i].quality);
		sprintf(string, "MAGNITUDE_%d", i);
		config.band[i].magnitude = defaults->get(string, config.band[i].magnitude);
		sprintf(string, "MODE_%d", i);
		config.band[i].mode = defaults->get(string, config.band[i].mode);
	}
	return 0;
}

int ParametricEQ::save_defaults()
{
	char string[BCTEXTLEN];

	defaults->update("WETNESS", config.wetness);


	for(int i = 0; i < BANDS; i++)
	{
		sprintf(string, "FREQ_%d", i);
		defaults->update(string, config.band[i].freq);
		sprintf(string, "QUALITY_%d", i);
		defaults->update(string, config.band[i].quality);
		sprintf(string, "MAGNITUDE_%d", i);
		defaults->update(string, config.band[i].magnitude);
		sprintf(string, "MODE_%d", i);
		defaults->update(string, config.band[i].mode);
	}


	defaults->save();

	return 0;
}


void ParametricEQ::reset()
{
	need_reconfigure = 1;
	thread = 0;
	fft = 0;
}

void ParametricEQ::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->update_gui();
		thread->window->unlock_window();
	}
}



