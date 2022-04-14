// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef UNITS_H
#define UNITS_H

#include "datatype.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define INFINITYGAIN -96
#define MAXGAIN 50
#define TOTALFREQS 1024

// h:mm:ss.sss
#define TIME_HMS 0
// h:mm:ss
#define TIME_HMS2 6
// hh:mm:ss
#define TIME_HMS3 7
// h:mm:ss:ff
#define TIME_SECONDS 8
#define TIME_HMSF 1
#define TIME_SAMPLES 2
#define TIME_SAMPLES_HEX 3
#define TIME_FRAMES 4
// fffff-ff
#define TIME_FEET_FRAMES 5
#define TIME_SECONDS__STR      "ssss.sss"
#define TIME_HMS__STR          "h:mm:ss.sss"
#define TIME_HMS2__STR         "h:mm:ss"
#define TIME_HMS3__STR         "hh:mm:ss"
#define TIME_HMSF__STR         "h:mm:ss:ff"
#define TIME_SAMPLES__STR      "audio samples"
#define TIME_SAMPLES_HEX__STR  "audio samples (hex)"
#define TIME_FRAMES__STR       "video frames"
#define TIME_FEET_FRAMES__STR  "video frames (feet)"

class DB
{
public:
	DB();
	virtual ~DB() {};
	static double fromdb(double db);

// convert db to power using a formula
	static double todb(double power);

	inline DB& operator++() { if(db < MAXGAIN) db += 0.1; return *this; };
	inline DB& operator--() { if(db > INFINITYGAIN) db -= 0.1; return *this; };
	inline DB& operator=(DB &newdb) { db = newdb.db; return *this; };
	inline DB& operator=(int newdb) { db = newdb; return *this; };
	inline int operator==(DB &newdb) { return db == newdb.db; };
	inline int operator==(int newdb) { return db == newdb; };

	double db;
};

// Third octave frequency table
class Freq
{
public:
	Freq();
	Freq(const Freq& oldfreq);
	virtual ~Freq() {};

	static void init_table();

// set freq to index given
	static int tofreq(int index);

// return index of frequency
	int fromfreq();
	static int fromfreq(int index);

// increment frequency by one
	Freq& operator++();
	Freq& operator--();

	int operator>(Freq &newfreq);
	int operator<(Freq &newfreq);
	Freq& operator=(const Freq &newfreq);
	int operator=(const int newfreq);
	int operator!=(Freq &newfreq);
	int operator==(Freq &newfreq);
	int operator==(int newfreq);

	static int *freqtable;
	int freq;
};


class Units
{
public:
	Units() {};

	static int timeformat_totype(const char *tcf);

	static double fix_framerate(double value);
	static double atoframerate(const char *text);

// Punctuate with commas
	static void punctuate(char *string);

// separator strings for BC_TextBox::set_separators
// Returns 0 if the format has no separators.
	static const char* format_to_separators(int time_format);

	static samplenum tosamples(double frames, int sample_rate, double framerate);
// give text representation as time
	static char* totext(char *text, samplenum samples,
		int time_format, int samplerate,
		double frame_rate = 0, double frames_per_foot = 0);
// give text representation as time
	static char* totext(char *text, ptstime seconds,
		int time_format, int sample_rate = 0,
		double frame_rate = 0, double frames_per_foot = 0);
// convert time to samples
	static samplenum fromtext(const char *text,
		int samplerate, int time_format,
		double frame_rate, double frames_per_foot);
// Convert text to seconds
	static double text_to_seconds(const char *text, int samplerate,
		int time_format, double frame_rate,
		double frames_per_foot);

	static char* print_time_format(int time_format, char *string);

	static double xy_to_polar(int x, int y);
	static void polar_to_xy(double angle, int radius, int &x, int &y);
// Numbers < 0 round down if next digit is < 5
// Numbers > 0 round up if next digit is > 5
	static int64_t round(double result);

	static double quantize10(double value);
	static double quantize(double value, double precision);
};

#endif
