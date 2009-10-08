
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

#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H



#include "filexml.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"


class Synth;
class SynthWindow;

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

class SynthWindow : public BC_Window
{
public:
	SynthWindow(Synth *synth, int x, int y);
	~SynthWindow();

	int create_objects();
	int close_event();
	int resize_event(int w, int h);
	void update_gui();
	int waveform_to_text(char *text, int waveform);
	void update_scrollbar();
	void update_oscillators();


	Synth *synth;
	SynthCanvas *canvas;
	SynthWetness *wetness;
	SynthWaveForm *waveform;
	SynthBaseFreq *base_freq;
	SynthFreqPot *freqpot;
	SynthSubWindow *subwindow;
	SynthScroll *scroll;
	ArrayList<SynthOscGUI*> oscillators;
};


class SynthOscGUILevel;
class SynthOscGUIPhase;
class SynthOscGUIFreq;

class SynthOscGUI
{
public:
	SynthOscGUI(SynthWindow *window, int number);
	~SynthOscGUI();

	int create_objects(int view_y);

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
	~SynthOscGUILevel();

	int handle_event();

	Synth *synth;
	SynthOscGUI *gui;
};

class SynthOscGUIPhase : public BC_IPot
{
public:
	SynthOscGUIPhase(Synth *synth, SynthOscGUI *gui, int y);
	~SynthOscGUIPhase();

	int handle_event();

	Synth *synth;
	SynthOscGUI *gui;
};

class SynthOscGUIFreq : public BC_IPot
{
public:
	SynthOscGUIFreq(Synth *synth, SynthOscGUI *gui, int y);
	~SynthOscGUIFreq();

	int handle_event();

	Synth *synth;
	SynthOscGUI *gui;
};

class SynthScroll : public BC_ScrollBar
{
public:
	SynthScroll(Synth *synth, SynthWindow *window, int x, int y, int h);
	~SynthScroll();
	
	int handle_event();
	
	Synth *synth;
	SynthWindow *window;
};

class SynthAddOsc : public BC_GenericButton
{
public:
	SynthAddOsc(Synth *synth, SynthWindow *window, int x, int y);
	~SynthAddOsc();
	
	int handle_event();
	
	Synth *synth;
	SynthWindow *window;
};


class SynthDelOsc : public BC_GenericButton
{
public:
	SynthDelOsc(Synth *synth, SynthWindow *window, int x, int y);
	~SynthDelOsc();
	
	int handle_event();
	
	Synth *synth;
	SynthWindow *window;
};

class SynthSubWindow : public BC_SubWindow
{
public:
	SynthSubWindow(Synth *synth, int x, int y, int w, int h);
	~SynthSubWindow();

	Synth *synth;
};

class SynthClear : public BC_GenericButton
{
public:
	SynthClear(Synth *synth, int x, int y);
	~SynthClear();
	int handle_event();
	Synth *synth;
};

class SynthWaveForm : public BC_PopupMenu
{
public:
	SynthWaveForm(Synth *synth, int x, int y, char *text);
	~SynthWaveForm();

	int create_objects();
	Synth *synth;
};

class SynthWaveFormItem : public BC_MenuItem
{
public:
	SynthWaveFormItem(Synth *synth, char *text, int value);
	~SynthWaveFormItem();
	
	int handle_event();
	
	int value;
	Synth *synth;
};

class SynthBaseFreq : public BC_TextBox
{
public:
	SynthBaseFreq(Synth *synth, int x, int y);
	~SynthBaseFreq();
	int handle_event();
	Synth *synth;
	SynthFreqPot *freq_pot;
};

class SynthFreqPot : public BC_QPot
{
public:
	SynthFreqPot(Synth *synth, SynthWindow *window, int x, int y);
	~SynthFreqPot();
	int handle_event();
	SynthWindow *window;
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
	~SynthCanvas();

	int update();
	Synth *synth;
	SynthWindow *window;
};

// ======================= level calculations
class SynthLevelZero : public BC_MenuItem
{
public:
	SynthLevelZero(Synth *synth);
	~SynthLevelZero();
	int handle_event();
	Synth *synth;
};

class SynthLevelMax : public BC_MenuItem
{
public:
	SynthLevelMax(Synth *synth);
	~SynthLevelMax();
	int handle_event();
	Synth *synth;
};

class SynthLevelNormalize : public BC_MenuItem
{
public:
	SynthLevelNormalize(Synth *synth);
	~SynthLevelNormalize();
	int handle_event();
	Synth *synth;
};

class SynthLevelSlope : public BC_MenuItem
{
public:
	SynthLevelSlope(Synth *synth);
	~SynthLevelSlope();
	int handle_event();
	Synth *synth;
};

class SynthLevelRandom : public BC_MenuItem
{
public:
	SynthLevelRandom(Synth *synth);
	~SynthLevelRandom();
	int handle_event();
	Synth *synth;
};

class SynthLevelInvert : public BC_MenuItem
{
public:
	SynthLevelInvert(Synth *synth);
	~SynthLevelInvert();
	int handle_event();
	Synth *synth;
};

class SynthLevelSine : public BC_MenuItem
{
public:
	SynthLevelSine(Synth *synth);
	~SynthLevelSine();
	int handle_event();
	Synth *synth;
};

// ============================ phase calculations

class SynthPhaseInvert : public BC_MenuItem
{
public:
	SynthPhaseInvert(Synth *synth);
	~SynthPhaseInvert();
	int handle_event();
	Synth *synth;
};

class SynthPhaseZero : public BC_MenuItem
{
public:
	SynthPhaseZero(Synth *synth);
	~SynthPhaseZero();
	int handle_event();
	Synth *synth;
};

class SynthPhaseSine : public BC_MenuItem
{
public:
	SynthPhaseSine(Synth *synth);
	~SynthPhaseSine();
	int handle_event();
	Synth *synth;
};

class SynthPhaseRandom : public BC_MenuItem
{
public:
	SynthPhaseRandom(Synth *synth);
	~SynthPhaseRandom();
	int handle_event();
	Synth *synth;
};


// ============================ freq calculations

class SynthFreqRandom : public BC_MenuItem
{
public:
	SynthFreqRandom(Synth *synth);
	~SynthFreqRandom();
	int handle_event();
	Synth *synth;
};

class SynthFreqEnum : public BC_MenuItem
{
public:
	SynthFreqEnum(Synth *synth);
	~SynthFreqEnum();
	int handle_event();
	Synth *synth;
};

class SynthFreqEven : public BC_MenuItem
{
public:
	SynthFreqEven(Synth *synth);
	~SynthFreqEven();
	int handle_event();
	Synth *synth;
};

class SynthFreqOdd : public BC_MenuItem
{
public:
	SynthFreqOdd(Synth *synth);
	~SynthFreqOdd();
	int handle_event();
	Synth *synth;
};

class SynthFreqFibonacci : public BC_MenuItem
{
public:
	SynthFreqFibonacci(Synth *synth);
	~SynthFreqFibonacci();
	int handle_event();
	Synth *synth;
};

class SynthFreqPrime : public BC_MenuItem
{
public:
	SynthFreqPrime(Synth *synth);
	~SynthFreqPrime();
	int handle_event();
	Synth *synth;
private:
	float get_next_prime(float number);
};


class SynthThread : public Thread
{
public:
	SynthThread(Synth *synth);
	~SynthThread();

	void run();

	Mutex completion;
	Synth *synth;
	SynthWindow *window;
};

class SynthOscillatorConfig
{
public:
	SynthOscillatorConfig(int number);
	~SynthOscillatorConfig();
	
	int equivalent(SynthOscillatorConfig &that);
	void copy_from(SynthOscillatorConfig& that);
	void reset();
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
	void read_data(FileXML *file);
	void save_data(FileXML *file);
	int is_realtime();

	float level;
	float phase;
	float freq_factor;
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
		int64_t prev_frame, 
		int64_t next_frame, 
		int64_t current_frame);
	void reset();
	
	float wetness;
	int64_t base_freq;         // base frequency for oscillators
	int wavefunction;        // SINE, SAWTOOTH, etc
	ArrayList<SynthOscillatorConfig*> oscillator_config;
};


class Synth : public PluginAClient
{
public:
	Synth(PluginServer *server);
	~Synth();



	int is_realtime();
	int is_synthesis();
	int load_configuration();
	int load_defaults();
	VFrame* new_picon();
	char* plugin_title();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int save_defaults();
	int show_gui();
	void raise_window();
	int set_string();
	int process_realtime(int64_t size, double *input_ptr, double *output_ptr);







	void add_oscillator();
	void delete_oscillator();
	double get_total_power();
	double get_oscillator_point(float x, 
		double normalize_constant, 
		int oscillator);
	double solve_eqn(double *output, 
		double x1, 
		double x2, 
		double normalize_constant,
		int oscillator);
	double get_point(float x, double normalize_constant);
	double function_square(double x);
	double function_pulse(double x);
	double function_noise();
	double function_sawtooth(double x);
	double function_triangle(double x);
	void reconfigure();
	int overlay_synth(int64_t start, int64_t length, double *input, double *output);
	void update_gui();
	void reset();



	double *dsp_buffer;
	int need_reconfigure;
	BC_Hash *defaults;
	SynthThread *thread;
	SynthConfig config;
	int w, h;
	DB db;
	int64_t waveform_length;           // length of loop buffer
	int64_t waveform_sample;           // current sample in waveform of loop
	int64_t samples_rendered;          // samples of the dsp_buffer rendered since last buffer redo
	float period;            // number of samples in a period for this frequency
};








#endif
