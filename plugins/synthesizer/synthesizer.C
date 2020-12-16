// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "clip.h"
#include "bchash.h"
#include "bcmenu.h"
#include "bcmenubar.h"
#include "bctitle.h"
#include "filexml.h"
#include "picon_png.h"
#include "synthesizer.h"

#include <string.h>

REGISTER_PLUGIN

Synth::Synth(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

Synth::~Synth()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void Synth::load_defaults()
{
	defaults = load_defaults_file("synthesizer.rc");

	config.wetness = defaults->get("WETNESS", 0);
	config.base_freq = defaults->get("BASEFREQ", 440);
	config.wavefunction = defaults->get("WAVEFUNCTION", 0);

	int total_oscillators = defaults->get("OSCILLATORS", TOTALOSCILLATORS);
	config.oscillator_config.remove_all_objects();
	for(int i = 0; i < total_oscillators; i++)
	{
		config.oscillator_config.append(new SynthOscillatorConfig(i));
		config.oscillator_config.values[i]->load_defaults(defaults);
	}
}

void Synth::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0, current_osc = 0, total_oscillators = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SYNTH"))
			{
				config.wetness = input.tag.get_property("WETNESS", config.wetness);
				config.base_freq = input.tag.get_property("BASEFREQ", config.base_freq);
				config.wavefunction = input.tag.get_property("WAVEFUNCTION", config.wavefunction);
				total_oscillators = input.tag.get_property("OSCILLATORS", 0);
			}
			else
			if(input.tag.title_is("OSCILLATOR"))
			{
				if(current_osc >= config.oscillator_config.total)
					config.oscillator_config.append(new SynthOscillatorConfig(current_osc));

				config.oscillator_config.values[current_osc]->read_data(&input);
				current_osc++;
			}
		}
	}

	while(config.oscillator_config.total > current_osc)
		config.oscillator_config.remove_object();
}

void Synth::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("SYNTH");
	output.tag.set_property("WETNESS", config.wetness);
	output.tag.set_property("BASEFREQ", config.base_freq);
	output.tag.set_property("WAVEFUNCTION", config.wavefunction);
	output.tag.set_property("OSCILLATORS", config.oscillator_config.total);
	output.append_tag();
	output.append_newline();

	for(int i = 0; i < config.oscillator_config.total; i++)
	{
		config.oscillator_config.values[i]->save_data(&output);
	}

	output.tag.set_title("/SYNTH");
	output.append_tag();
	keyframe->set_data(output.string);
}

void Synth::save_defaults()
{
	defaults->update("WETNESS", config.wetness);
	defaults->update("BASEFREQ", config.base_freq);
	defaults->update("WAVEFUNCTION", config.wavefunction);
	defaults->update("OSCILLATORS", config.oscillator_config.total);

	for(int i = 0; i < config.oscillator_config.total; i++)
	{
		config.oscillator_config.values[i]->save_defaults(defaults);
	}
	defaults->save();
}

void Synth::add_oscillator()
{
	if(config.oscillator_config.total > 20) return;

	config.oscillator_config.append(new SynthOscillatorConfig(config.oscillator_config.total - 1));
}

void Synth::delete_oscillator()
{
	if(config.oscillator_config.total)
	{
		config.oscillator_config.remove_object();
	}
}

double Synth::get_total_power()
{
	double result = 0;

	if(config.wavefunction == DC)
		return 1.0;

	for(int i = 0; i < config.oscillator_config.total; i++)
	{
		result += DB::fromdb(config.oscillator_config.values[i]->level);
	}

	if(result == 0)
		result = 1;  // prevent division by 0
	return result;
}

double Synth::get_point(double x, double normalize_constant)
{
	double result = 0;

	for(int i = 0; i < config.oscillator_config.total; i++)
		result += get_oscillator_point(x, normalize_constant, i);

	return result;
}

double Synth::get_oscillator_point(double x, double normalize_constant,
	int oscillator)
{
	SynthOscillatorConfig *config = this->config.oscillator_config.values[oscillator];
	double power = DB::fromdb(config->level) * normalize_constant;

	switch(this->config.wavefunction)
	{
	case DC:
		return power;

	case SINE:
		return sin((x + config->phase) * config->freq_factor * 2 * M_PI) * power;

	case SAWTOOTH:
		return function_sawtooth((x + config->phase) * config->freq_factor) * power;

	case SQUARE:
		return function_square((x + config->phase) * config->freq_factor) * power;

	case TRIANGLE:
		return function_triangle((x + config->phase) * config->freq_factor) * power;

	case PULSE:
		return function_pulse((x + config->phase) * config->freq_factor) * power;

	case NOISE:
		return function_noise() * power;
	}
	return 0;
}

double Synth::function_square(double x)
{
	x -= trunc(x); // only fraction counts
	return (x < .5) ? -1 : 1;
}

double Synth::function_pulse(double x)
{
	x -= trunc(x); // only fraction counts
	return (x < .5) ? 0 : 1;
}

double Synth::function_noise()
{
	return (double)(rand() % 65536 - 32768) / 32768;
}

double Synth::function_sawtooth(double x)
{
	x -= trunc(x);
	return 1 - x * 2;
}

double Synth::function_triangle(double x)
{
	x -= trunc(x);
	return (x < .5) ? 1 - x * 4 : -3 + x * 4;
}

AFrame *Synth::process_tmpframe(AFrame *input)
{
	int size = input->get_length();
	int fragment_len;
	double wetness;

	if(load_configuration())
		update_gui();

	if(config.wetness < (INFINITYGAIN + EPSILON)) 
		wetness = 0;
	else
		wetness = DB::fromdb(config.wetness);

	for(int j = 0; j < size; j++)
		input->buffer[j] *= wetness;

	overlay_synth(input);

	return input;
}

void Synth::overlay_synth(AFrame *frame)
{
	int size = frame->get_length();
	double period = frame->get_samplerate() / config.base_freq;

	for(int i = 0; i < config.oscillator_config.total; i++)
	{
		SynthOscillatorConfig *osc = config.oscillator_config.values[i];

		if(osc->level <= INFINITYGAIN)
			continue;

		double normalize_constant = 1.0 / get_total_power();
		double power = DB::fromdb(osc->level) * normalize_constant;
		double osc_period = period / osc->freq_factor;
		double x = ((frame->get_pts() - get_start()) * frame->get_samplerate()) +
			osc->phase * period;
		x = fmod(x, osc_period);

		switch(config.wavefunction)
		{
		case DC:
			for(int j = 0; j < size; j++)
				frame->buffer[j] += power;
			break;
		case SINE:
			for(int j = 0; j < size; j++)
				frame->buffer[j] += sin(x++ / osc_period * 2 * M_PI) * power;
			break;
		case SAWTOOTH:
			for(int j = 0; j < size; j++)
				frame->buffer[j] += function_sawtooth(x++ / osc_period) * power;
			break;
		case SQUARE:
			for(int j = 0; j < size; j++)
				frame->buffer[j] += function_square(x++ / osc_period) * power;
			break;
		case TRIANGLE:
			for(int j = 0; j < size; j++)
				frame->buffer[j] += function_triangle(x++ / period) * power;
			break;
		case PULSE:
			for(int j = 0; j < size; j++)
				frame->buffer[j] += function_pulse(x++ / period) * power;
			break;
		case NOISE:
			for(int j = 0; j < size; j++)
				frame->buffer[j] +=  function_noise() * power;
			break;
		}
	}
}

PLUGIN_THREAD_METHODS

SynthWindow::SynthWindow(Synth *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	440,
	400)
{
	BC_MenuBar *menu;
	int i;

	add_subwindow(menu = new BC_MenuBar(0, 0, get_w()));

	BC_Menu *levelmenu, *phasemenu, *harmonicmenu;
	menu->add_menu(levelmenu = new BC_Menu(_("Level")));
	menu->add_menu(phasemenu = new BC_Menu(_("Phase")));
	menu->add_menu(harmonicmenu = new BC_Menu(_("Harmonic")));

	levelmenu->add_item(new SynthLevelInvert(plugin));
	levelmenu->add_item(new SynthLevelMax(plugin));
	levelmenu->add_item(new SynthLevelRandom(plugin));
	levelmenu->add_item(new SynthLevelSine(plugin));
	levelmenu->add_item(new SynthLevelSlope(plugin));
	levelmenu->add_item(new SynthLevelZero(plugin));

	phasemenu->add_item(new SynthPhaseInvert(plugin));
	phasemenu->add_item(new SynthPhaseRandom(plugin));
	phasemenu->add_item(new SynthPhaseSine(plugin));
	phasemenu->add_item(new SynthPhaseZero(plugin));

	harmonicmenu->add_item(new SynthFreqEnum(plugin));
	harmonicmenu->add_item(new SynthFreqEven(plugin));
	harmonicmenu->add_item(new SynthFreqFibonacci(plugin));
	harmonicmenu->add_item(new SynthFreqOdd(plugin));
	harmonicmenu->add_item(new SynthFreqPrime(plugin));

	x = 10;
	y = 30;
	add_subwindow(new BC_Title(x, y, _("Waveform")));
	x += 240;
	add_subwindow(new BC_Title(x, y, _("Wave Function")));
	y += 20;
	x = 10;
	add_subwindow(canvas = new SynthCanvas(plugin, this, x, y, 230, 160));
	canvas->update();

	x += 240;

	add_subwindow(waveform = new SynthWaveForm(plugin, x, y, waveform_to_text(plugin->config.wavefunction)));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Base Frequency:")));
	y += 30;
	add_subwindow(base_freq = new SynthBaseFreq(plugin, x, y));
	x += 80;
	add_subwindow(freqpot = new SynthFreqPot(plugin, x, y - 10));
	base_freq->freq_pot = freqpot;
	freqpot->freq_text = base_freq;
	x -= 80;
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Wetness:")));
	add_subwindow(wetness = new SynthWetness(plugin, x + 70, y - 10));

	y += 40;
	add_subwindow(new SynthClear(plugin, x, y));

	x = 50;
	y = 220;
	add_subwindow(new BC_Title(x, y, _("Level"))); 
	x += 75;
	add_subwindow(new BC_Title(x, y, _("Phase"))); 
	x += 75;
	add_subwindow(new BC_Title(x, y, _("Harmonic")));

	y += 20; x = 10;
	add_subwindow(subwindow = new SynthSubWindow(plugin, x, y, 265, get_h() - y));
	x += 265;
	add_subwindow(scroll = new SynthScroll(plugin, this, x, y, get_h() - y));

	x += 20;
	add_subwindow(new SynthAddOsc(plugin, this, x, y));
	y += 30;
	add_subwindow(new SynthDelOsc(plugin, this, x, y));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_scrollbar();
	update_oscillators();
}

void SynthWindow::update()
{
	freqpot->update(plugin->config.base_freq);
	base_freq->update(plugin->config.base_freq);
	wetness->update(plugin->config.wetness);
	waveform->set_text(waveform_to_text(plugin->config.wavefunction));

	update_scrollbar();
	update_oscillators();
	canvas->update();
}

void SynthWindow::update_scrollbar()
{
	scroll->update_length(plugin->config.oscillator_config.total * OSCILLATORHEIGHT, 
		scroll->get_position(), subwindow->get_h());
}

void SynthWindow::update_oscillators()
{
	int i, y = -scroll->get_position();

// Add new oscillators
	for(i = 0; i < plugin->config.oscillator_config.total; i++)
	{
		SynthOscGUI *gui;
		SynthOscillatorConfig *config = plugin->config.oscillator_config.values[i];

		if(oscillators.total <= i)
		{
			oscillators.append(gui = new SynthOscGUI(this, i, y));
		}
		else
		{
			gui = oscillators.values[i];

			gui->title->reposition_window(gui->title->get_x(), y + 15);

			gui->level->reposition_window(gui->level->get_x(), y);
			gui->level->update(config->level);

			gui->phase->reposition_window(gui->phase->get_x(), y);
			gui->phase->update((int64_t)(config->phase * 360));

			gui->freq->reposition_window(gui->freq->get_x(), y);
			gui->freq->update((int64_t)(config->freq_factor));
		}
		y += OSCILLATORHEIGHT;
	}

// Delete old oscillators
	for(; i < oscillators.total; i++)
		oscillators.remove_object();
}

const char* SynthWindow::waveform_to_text(int waveform)
{
	switch(waveform)
	{
	case DC:
		return _("DC");
	case SINE:
		return _("Sine");
	case SAWTOOTH:
		return _("Sawtooth");
	case SQUARE:
		return _("Square");
	case TRIANGLE:
		return _("Triangle");
	case PULSE:
		return _("Pulse");
	case NOISE:
		return _("Noise");
	}
	return "";
}


SynthOscGUI::SynthOscGUI(SynthWindow *window, int number, int y)
{
	char text[BCTEXTLEN];

	this->window = window;
	this->number = number;

	sprintf(text, "%d:", number + 1);
	window->subwindow->add_subwindow(title = new BC_Title(10, y + 15, text));

	window->subwindow->add_subwindow(level = new SynthOscGUILevel(window->plugin, this, y));
	window->subwindow->add_subwindow(phase = new SynthOscGUIPhase(window->plugin, this, y));
	window->subwindow->add_subwindow(freq = new SynthOscGUIFreq(window->plugin, this, y));
}

SynthOscGUI::~SynthOscGUI()
{
	delete title;
	delete level;
	delete phase;
	delete freq;
}


SynthOscGUILevel::SynthOscGUILevel(Synth *synth, SynthOscGUI *gui, int y)
 : BC_FPot(50, 
	y, 
	synth->config.oscillator_config.values[gui->number]->level,
	INFINITYGAIN, 
	0)
{
	this->synth = synth;
	this->gui = gui;
}

int SynthOscGUILevel::handle_event()
{
	SynthOscillatorConfig *config = synth->config.oscillator_config.values[gui->number];
	config->level = get_value();
	synth->send_configure_change();
	return 1;
}


SynthOscGUIPhase::SynthOscGUIPhase(Synth *synth, SynthOscGUI *gui, int y)
 : BC_IPot(125, 
	y,
	synth->config.oscillator_config.values[gui->number]->phase * 360,
	0, 
	360)
{
	this->synth = synth;
	this->gui = gui;
}

int SynthOscGUIPhase::handle_event()
{
	SynthOscillatorConfig *config = synth->config.oscillator_config.values[gui->number];
	config->phase = (double)get_value() / 360;
	synth->send_configure_change();
	return 1;
}


SynthOscGUIFreq::SynthOscGUIFreq(Synth *synth, SynthOscGUI *gui, int y)
 : BC_IPot(200, 
	y,
	synth->config.oscillator_config.values[gui->number]->freq_factor,
	1, 
	100)
{
	this->synth = synth;
	this->gui = gui;
}

int SynthOscGUIFreq::handle_event()
{
	SynthOscillatorConfig *config = synth->config.oscillator_config.values[gui->number];

	config->freq_factor = get_value();
	synth->send_configure_change();
	return 1;
}


SynthAddOsc::SynthAddOsc(Synth *synth, SynthWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Add"))
{
	this->synth = synth;
	this->window = window;
}

int SynthAddOsc::handle_event()
{
	synth->add_oscillator();
	synth->send_configure_change();
	window->update();
	return 1;
}


SynthDelOsc::SynthDelOsc(Synth *synth, SynthWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->synth = synth;
	this->window = window;
}

int SynthDelOsc::handle_event()
{
	synth->delete_oscillator();
	synth->send_configure_change();
	window->update();
	return 1;
}

SynthScroll::SynthScroll(Synth *synth, 
	SynthWindow *window, 
	int x, 
	int y, 
	int h)
 : BC_ScrollBar(x, 
	y,
	SCROLL_VERT,
	h, 
	synth->config.oscillator_config.total * OSCILLATORHEIGHT, 
	0, 
	window->subwindow->get_h())
{
	this->synth = synth;
	this->window = window;
}

int SynthScroll::handle_event()
{
	window->update();
	return 1;
}


SynthSubWindow::SynthSubWindow(Synth *synth, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h)
{
	this->synth = synth;
}


SynthClear::SynthClear(Synth *synth, int x, int y)
 : BC_GenericButton(x, y, _("Clear"))
{
	this->synth = synth;
}

int SynthClear::handle_event()
{
	synth->config.reset();
	synth->send_configure_change();
	synth->update_gui();
	return 1;
}


SynthWaveForm::SynthWaveForm(Synth *synth, int x, int y, const char *text)
 : BC_PopupMenu(x, y, 120, text)
{
	this->synth = synth;
	add_item(new SynthWaveFormItem(synth, this, _("Sine"), SINE));
	add_item(new SynthWaveFormItem(synth, this, _("Sawtooth"), SAWTOOTH));
	add_item(new SynthWaveFormItem(synth, this, _("Square"), SQUARE));
	add_item(new SynthWaveFormItem(synth, this, _("Triangle"), TRIANGLE));
	add_item(new SynthWaveFormItem(synth, this, _("Pulse"), PULSE));
	add_item(new SynthWaveFormItem(synth, this, _("Noise"), NOISE));
}

SynthWaveFormItem::SynthWaveFormItem(Synth *synth, SynthWaveForm *form,
	const char *text, int value)
 : BC_MenuItem(text)
{
	this->synth = synth;
	this->form = form;
	this->value = value;
}

int SynthWaveFormItem::handle_event()
{
	synth->config.wavefunction = value;
	synth->thread->window->canvas->update();
	form->set_text(get_text());
	synth->send_configure_change();
	return 1;
}


SynthWetness::SynthWetness(Synth *synth, int x, int y)
 : BC_FPot(x, y, synth->config.wetness, INFINITYGAIN, 0)
{
	this->synth = synth;
}

int SynthWetness::handle_event()
{
	synth->config.wetness = get_value();
	synth->send_configure_change();
	return 1;
}


SynthFreqPot::SynthFreqPot(Synth *synth, int x, int y)
 : BC_QPot(x, y, synth->config.base_freq)
{
	this->synth = synth;
}

int SynthFreqPot::handle_event()
{
	if(get_value() > 0 && get_value() < 30000)
	{
		synth->config.base_freq = get_value();
		freq_text->update((int)get_value());
		synth->send_configure_change();
	}
	return 1;
}


SynthBaseFreq::SynthBaseFreq(Synth *synth, int x, int y)
 : BC_TextBox(x, y, 70, 1, (int)synth->config.base_freq)
{
	this->synth = synth;
}

int SynthBaseFreq::handle_event()
{
	int new_value = atol(get_text());

	if(new_value > 0 && new_value < 30000)
	{
		synth->config.base_freq = new_value;
		freq_pot->update(synth->config.base_freq);
		synth->send_configure_change();
	}
	return 1;
}


SynthCanvas::SynthCanvas(Synth *synth, 
	SynthWindow *window, 
	int x, 
	int y, 
	int w, 
	int h)
 : BC_SubWindow(x, 
	y,
	w, 
	h, 
	BLACK)
{
	this->synth = synth;
	this->window = window;
}

void SynthCanvas::update()
{
	int y1, y2, y = 0;

	clear_box(0, 0, get_w(), get_h());
	set_color(RED);

	draw_line(0, get_h() / 2 + y, get_w(), get_h() / 2 + y);

	set_color(GREEN);

	double normalize_constant = (double)1 / synth->get_total_power();
	y1 = synth->get_point(0, normalize_constant) * get_h() / 2;

	for(int i = 1; i < get_w(); i++)
	{
		y2 = synth->get_point((double)i / get_w(), normalize_constant) * get_h() / 2;
		draw_line(i - 1, get_h() / 2 - y1, i, get_h() / 2 - y2);
		y1 = y2;
	}
	flash();
}


// ======================= level calculations
SynthLevelZero::SynthLevelZero(Synth *synth)
 : BC_MenuItem(_("Zero"))
{ 
	this->synth = synth; 
}

int SynthLevelZero::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = INFINITYGAIN;
	}

	synth->thread->window->update();
	synth->send_configure_change();
}

SynthLevelMax::SynthLevelMax(Synth *synth)
 : BC_MenuItem(_("Maximum"))
{ 
	this->synth = synth; 
}

int SynthLevelMax::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = 0;
	}
	synth->send_configure_change();
	return 1;
}

SynthLevelNormalize::SynthLevelNormalize(Synth *synth)
 : BC_MenuItem(_("Normalize"))
{
	this->synth = synth;
}

int SynthLevelNormalize::handle_event()
{
// get total power
	double total = 0;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		total += DB::fromdb(synth->config.oscillator_config.values[i]->level);
	}

	double scale = 1 / total;
	double new_value;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		new_value = DB::fromdb(synth->config.oscillator_config.values[i]->level);
		new_value *= scale;
		new_value = DB::todb(new_value);

		synth->config.oscillator_config.values[i]->level = new_value;
	}
	synth->send_configure_change();
	return 1;
}

SynthLevelSlope::SynthLevelSlope(Synth *synth)
 : BC_MenuItem(_("Slope"))
{
	this->synth = synth;
}

int SynthLevelSlope::handle_event()
{
	double slope = (double)INFINITYGAIN / synth->config.oscillator_config.total;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = i * slope;
	}

	synth->send_configure_change();
	return 1;
}

SynthLevelRandom::SynthLevelRandom(Synth *synth)
 : BC_MenuItem(_("Random"))
{ 
	this->synth = synth; 
}

int SynthLevelRandom::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = -(rand() % -INFINITYGAIN);
	}

	synth->send_configure_change();
	return 1;
}

SynthLevelInvert::SynthLevelInvert(Synth *synth)
 : BC_MenuItem(_("Invert"))
{
	this->synth = synth;
}

int SynthLevelInvert::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = 
			INFINITYGAIN - synth->config.oscillator_config.values[i]->level;
	}

	synth->send_configure_change();
	return 1;
}

SynthLevelSine::SynthLevelSine(Synth *synth)
 : BC_MenuItem(_("Sine"))
{
	this->synth = synth;
}

int SynthLevelSine::handle_event()
{
	double new_value;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		new_value = (double)i / synth->config.oscillator_config.total * 2 * M_PI;
		new_value = sin(new_value) * INFINITYGAIN / 2 + INFINITYGAIN / 2;
		synth->config.oscillator_config.values[i]->level = new_value;
	}

	synth->send_configure_change();
	return 1;
}

// ============================ phase calculations

SynthPhaseInvert::SynthPhaseInvert(Synth *synth)
 : BC_MenuItem(_("Invert"))
{
	this->synth = synth;
}

int SynthPhaseInvert::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->phase = 
			1 - synth->config.oscillator_config.values[i]->phase;
	}

	synth->send_configure_change();
	return 1;
}

SynthPhaseZero::SynthPhaseZero(Synth *synth)
 : BC_MenuItem(_("Zero"))
{
	this->synth = synth;
}

int SynthPhaseZero::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->phase = 0;
	}

	synth->send_configure_change();
	return 1;
}


SynthPhaseSine::SynthPhaseSine(Synth *synth)
 : BC_MenuItem(_("Sine"))
{
	this->synth = synth;
}

int SynthPhaseSine::handle_event()
{
	double new_value;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		new_value = (double)i / synth->config.oscillator_config.total * 2 * M_PI;
		new_value = sin(new_value) / 2 + .5;
		synth->config.oscillator_config.values[i]->phase = new_value;
	}

	synth->send_configure_change();
	return 1;
}

SynthPhaseRandom::SynthPhaseRandom(Synth *synth)
 : BC_MenuItem(_("Random"))
{
	this->synth = synth;
}

int SynthPhaseRandom::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->phase = 
			(double)(rand() % 360) / 360;
	}

	synth->send_configure_change();
	return 1;
}


// ============================ freq calculations

SynthFreqRandom::SynthFreqRandom(Synth *synth)
 : BC_MenuItem(_("Random"))
{
	this->synth = synth;
}

int SynthFreqRandom::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = rand() % 100;
	}

	synth->send_configure_change();
	return 1;
}

SynthFreqEnum::SynthFreqEnum(Synth *synth)
 : BC_MenuItem(_("Enumerate"))
{
	this->synth = synth;
}

int SynthFreqEnum::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = i + 1;
	}

	synth->send_configure_change();
	return 1;
}

SynthFreqEven::SynthFreqEven(Synth *synth)
 : BC_MenuItem(_("Even"))
{
	this->synth = synth;
}

int SynthFreqEven::handle_event()
{
	if(synth->config.oscillator_config.total)
		synth->config.oscillator_config.values[0]->freq_factor = 1;

	for(int i = 1; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = i * 2;
	}

	synth->send_configure_change();
	return 1;
}

SynthFreqOdd::SynthFreqOdd(Synth *synth)
 : BC_MenuItem(_("Odd"))
{
	this->synth = synth;
}

int SynthFreqOdd::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = 1 + i * 2;
	}

	synth->send_configure_change();
	return 1;
}


SynthFreqFibonacci::SynthFreqFibonacci(Synth *synth)
 : BC_MenuItem(_("Fibonnacci"))
{ 
	this->synth = synth; 
}

int SynthFreqFibonacci::handle_event()
{
	double last_value1 = 0, last_value2 = 1;
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = last_value1 + last_value2;
		if(synth->config.oscillator_config.values[i]->freq_factor > 100)
			synth->config.oscillator_config.values[i]->freq_factor = 100;
		last_value1 = last_value2;
		last_value2 = synth->config.oscillator_config.values[i]->freq_factor;
	}

	synth->send_configure_change();
	return 1;
}

SynthFreqPrime::SynthFreqPrime(Synth *synth)
 : BC_MenuItem(_("Prime"))
{ 
	this->synth = synth; 
}

int SynthFreqPrime::handle_event()
{
	double number = 1;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = number;
		number = get_next_prime(number);
	}

	synth->send_configure_change();
	return 1;
}

double SynthFreqPrime::get_next_prime(double number)
{
	int result = 1;

	while(result)
	{
		result = 0;
		number++;

		for(double i = number - 1; i > 1 && !result; i--)
		{
			if((number / i) - (int)(number / i) == 0)
				result = 1;
		}
	}
	return number;
}


SynthOscillatorConfig::SynthOscillatorConfig(int number)
{
	reset();
	this->number = number;
}

void SynthOscillatorConfig::reset()
{
	level = 0;
	phase = 0;
	freq_factor = 1;
}

void SynthOscillatorConfig::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	sprintf(string, "LEVEL%d", number);
	level = defaults->get(string, level);
	sprintf(string, "PHASE%d", number);
	phase = defaults->get(string, phase);
	sprintf(string, "FREQFACTOR%d", number);
	freq_factor = defaults->get(string, freq_factor);
}

void SynthOscillatorConfig::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	sprintf(string, "LEVEL%d", number);
	defaults->update(string, level);
	sprintf(string, "PHASE%d", number);
	defaults->update(string, phase);
	sprintf(string, "FREQFACTOR%d", number);
	defaults->update(string, freq_factor);
}

void SynthOscillatorConfig::read_data(FileXML *file)
{
	level = file->tag.get_property("LEVEL", level);
	phase = file->tag.get_property("PHASE", phase);
	freq_factor = file->tag.get_property("FREQFACTOR", freq_factor);
}

void SynthOscillatorConfig::save_data(FileXML *file)
{
	file->tag.set_title("OSCILLATOR");
	file->tag.set_property("LEVEL", level);
	file->tag.set_property("PHASE", phase);
	file->tag.set_property("FREQFACTOR", freq_factor);
	file->append_tag();
	file->append_newline();
}

int SynthOscillatorConfig::equivalent(SynthOscillatorConfig &that)
{
	return EQUIV(level, that.level) &&
		EQUIV(phase, that.phase) &&
		EQUIV(freq_factor, that.freq_factor);
}

void SynthOscillatorConfig::copy_from(SynthOscillatorConfig& that)
{
	level = that.level;
	phase = that.phase;
	freq_factor = that.freq_factor;
}


SynthConfig::SynthConfig()
{
	reset();
}

SynthConfig::~SynthConfig()
{
	oscillator_config.remove_all_objects();
}

void SynthConfig::reset()
{
	wetness = 0;
	base_freq = 440;
	wavefunction = SINE;
	for(int i = 0; i < oscillator_config.total; i++)
		oscillator_config.values[i]->reset();
}

int SynthConfig::equivalent(SynthConfig &that)
{
	if(base_freq != that.base_freq ||
		wavefunction != that.wavefunction ||
		oscillator_config.total != that.oscillator_config.total) return 0;

	for(int i = 0; i < oscillator_config.total; i++)
	{
		if(!oscillator_config.values[i]->equivalent(*that.oscillator_config.values[i]))
			return 0;
	}

	return 1;
}

void SynthConfig::copy_from(SynthConfig& that)
{
	wetness = that.wetness;
	base_freq = that.base_freq;
	wavefunction = that.wavefunction;

	int i;

	for(i = 0; i < oscillator_config.total && i < that.oscillator_config.total;
		i++)
	{
		oscillator_config.values[i]->copy_from(*that.oscillator_config.values[i]);
	}

	for(; i < that.oscillator_config.total; i++)
	{
		oscillator_config.append(new SynthOscillatorConfig(i));
		oscillator_config.values[i]->copy_from(*that.oscillator_config.values[i]);
	}

	for(; i < oscillator_config.total; i++)
	{
		oscillator_config.remove_object();
	}
}

void SynthConfig::interpolate(SynthConfig &prev, 
	SynthConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	copy_from(prev);
	wetness = prev.wetness * prev_scale + next.wetness * next_scale;
	base_freq = prev.base_freq * prev_scale + next.base_freq * next_scale;
}
