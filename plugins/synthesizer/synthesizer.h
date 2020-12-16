// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

#define PLUGIN_TITLE N_("Synthesizer")

#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_CLASS Synth
#define PLUGIN_CONFIG_CLASS SynthConfig
#define PLUGIN_THREAD_CLASS SynthThread
#define PLUGIN_GUI_CLASS SynthWindow

#include "pluginmacros.h"

#include "bcbutton.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bcpot.h"
#include "bcscrollbar.h"
#include "bctextbox.h"
#include "language.h"
#include "pluginaclient.h"
#include "pluginwindow.h"

#define TOTALOSCILLATORS 1
#define OSCILLATORHEIGHT 40

#define SINE 0
#define SAWTOOTH 1
#define SQUARE 2
#define TRIANGLE 3
#define PULSE 4
#define NOISE 5
#define DC    6


class SynthCanvas;
class SynthWaveForm;
class SynthBaseFreq;
class SynthFreqPot;
class SynthOscGUI;
class SynthScroll;
class SynthSubWindow;
class SynthWetness;

class SynthWindow : public PluginWindow
{
public:
	SynthWindow(Synth *plugin, int x, int y);

	void update();
	const char *waveform_to_text(int waveform);
	void update_scrollbar();
	void update_oscillators();

	SynthCanvas *canvas;
	SynthWetness *wetness;
	SynthWaveForm *waveform;
	SynthBaseFreq *base_freq;
	SynthFreqPot *freqpot;
	SynthSubWindow *subwindow;
	SynthScroll *scroll;
	ArrayList<SynthOscGUI*> oscillators;
	PLUGIN_GUI_CLASS_MEMBERS
};


class SynthOscGUILevel;
class SynthOscGUIPhase;
class SynthOscGUIFreq;

class SynthOscGUI
{
public:
	SynthOscGUI(SynthWindow *window, int number, int y);
	~SynthOscGUI();

	SynthOscGUILevel *level;
	SynthOscGUIPhase *phase;
	SynthOscGUIFreq *freq;
	BC_Title *title;

	int number;
	SynthWindow *window;
};

class SynthOscGUILevel : public BC_FPot
{
public:
	SynthOscGUILevel(Synth *synth, SynthOscGUI *gui, int y);

	int handle_event();

	Synth *synth;
	SynthOscGUI *gui;
};

class SynthOscGUIPhase : public BC_IPot
{
public:
	SynthOscGUIPhase(Synth *synth, SynthOscGUI *gui, int y);

	int handle_event();

	Synth *synth;
	SynthOscGUI *gui;
};

class SynthOscGUIFreq : public BC_IPot
{
public:
	SynthOscGUIFreq(Synth *synth, SynthOscGUI *gui, int y);

	int handle_event();

	Synth *synth;
	SynthOscGUI *gui;
};

class SynthScroll : public BC_ScrollBar
{
public:
	SynthScroll(Synth *synth, SynthWindow *window, int x, int y, int h);

	int handle_event();

	Synth *synth;
	SynthWindow *window;
};

class SynthAddOsc : public BC_GenericButton
{
public:
	SynthAddOsc(Synth *synth, SynthWindow *window, int x, int y);

	int handle_event();

	Synth *synth;
	SynthWindow *window;
};


class SynthDelOsc : public BC_GenericButton
{
public:
	SynthDelOsc(Synth *synth, SynthWindow *window, int x, int y);

	int handle_event();

	Synth *synth;
	SynthWindow *window;
};

class SynthSubWindow : public BC_SubWindow
{
public:
	SynthSubWindow(Synth *synth, int x, int y, int w, int h);

	Synth *synth;
};

class SynthClear : public BC_GenericButton
{
public:
	SynthClear(Synth *synth, int x, int y);

	int handle_event();
	Synth *synth;
};

class SynthWaveForm : public BC_PopupMenu
{
public:
	SynthWaveForm(Synth *synth, int x, int y, const char *text);

	Synth *synth;
};

class SynthWaveFormItem : public BC_MenuItem
{
public:
	SynthWaveFormItem(Synth *synth, SynthWaveForm *form, 
		const char *text, int value);

	int handle_event();

	int value;
	SynthWaveForm *form;
	Synth *synth;
};

class SynthBaseFreq : public BC_TextBox
{
public:
	SynthBaseFreq(Synth *synth, int x, int y);

	int handle_event();
	Synth *synth;
	SynthFreqPot *freq_pot;
};

class SynthFreqPot : public BC_QPot
{
public:
	SynthFreqPot(Synth *synth, int x, int y);

	int handle_event();

	Synth *synth;
	SynthBaseFreq *freq_text;
};

class SynthWetness : public BC_FPot
{
public:
	SynthWetness(Synth *synth, int x, int y);
	int handle_event();
	Synth *synth;
};


class SynthCanvas : public BC_SubWindow
{
public:
	SynthCanvas(Synth *synth, 
		SynthWindow *window, 
		int x, 
		int y, 
		int w, 
		int h);

	void update();
	Synth *synth;
	SynthWindow *window;
};

// ======================= level calculations
class SynthLevelZero : public BC_MenuItem
{
public:
	SynthLevelZero(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthLevelMax : public BC_MenuItem
{
public:
	SynthLevelMax(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthLevelNormalize : public BC_MenuItem
{
public:
	SynthLevelNormalize(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthLevelSlope : public BC_MenuItem
{
public:
	SynthLevelSlope(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthLevelRandom : public BC_MenuItem
{
public:
	SynthLevelRandom(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthLevelInvert : public BC_MenuItem
{
public:
	SynthLevelInvert(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthLevelSine : public BC_MenuItem
{
public:
	SynthLevelSine(Synth *synth);

	int handle_event();
	Synth *synth;
};

// ============================ phase calculations

class SynthPhaseInvert : public BC_MenuItem
{
public:
	SynthPhaseInvert(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthPhaseZero : public BC_MenuItem
{
public:
	SynthPhaseZero(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthPhaseSine : public BC_MenuItem
{
public:
	SynthPhaseSine(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthPhaseRandom : public BC_MenuItem
{
public:
	SynthPhaseRandom(Synth *synth);

	int handle_event();
	Synth *synth;
};


// ============================ freq calculations

class SynthFreqRandom : public BC_MenuItem
{
public:
	SynthFreqRandom(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthFreqEnum : public BC_MenuItem
{
public:
	SynthFreqEnum(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthFreqEven : public BC_MenuItem
{
public:
	SynthFreqEven(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthFreqOdd : public BC_MenuItem
{
public:
	SynthFreqOdd(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthFreqFibonacci : public BC_MenuItem
{
public:
	SynthFreqFibonacci(Synth *synth);

	int handle_event();
	Synth *synth;
};

class SynthFreqPrime : public BC_MenuItem
{
public:
	SynthFreqPrime(Synth *synth);

	int handle_event();
	Synth *synth;
private:
	double get_next_prime(double number);
};

PLUGIN_THREAD_HEADER

class SynthOscillatorConfig
{
public:
	SynthOscillatorConfig(int number);

	int equivalent(SynthOscillatorConfig &that);
	void copy_from(SynthOscillatorConfig& that);
	void reset();
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
	void read_data(FileXML *file);
	void save_data(FileXML *file);

	double level;
	double phase;
	double freq_factor;
	int number;
};

class SynthConfig
{
public:
	SynthConfig();
	~SynthConfig();

	int equivalent(SynthConfig &that);
	void copy_from(SynthConfig &that);
	void interpolate(SynthConfig &prev, 
		SynthConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_ptstime);
	void reset();

	double wetness;
	int base_freq;           // base frequency for oscillators
	int wavefunction;        // SINE, SAWTOOTH, etc
	ArrayList<SynthOscillatorConfig*> oscillator_config;
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class Synth : public PluginAClient
{
public:
	Synth(PluginServer *server);
	~Synth();

	PLUGIN_CLASS_MEMBERS

	void load_defaults();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void save_defaults();
	AFrame *process_tmpframe(AFrame *input);

	void add_oscillator();
	void delete_oscillator();
	double get_total_power();
	double get_oscillator_point(double x,
		double normalize_constant, 
		int oscillator);
	double get_point(double x, double normalize_constant);
	double function_square(double x);
	double function_pulse(double x);
	double function_noise();
	double function_sawtooth(double x);
	double function_triangle(double x);
	void overlay_synth(AFrame *frame);
};

#endif
