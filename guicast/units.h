#ifndef UNITS_H
#define UNITS_H

#include "sizes.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>


#define INFINITYGAIN -40
#define MAXGAIN 50
#define TOTALFREQS 1024


#define TIME_HMS 0
#define TIME_HMS2 6
#define TIME_HMSF 1
#define TIME_SAMPLES 2
#define TIME_SAMPLES_HEX 3
#define TIME_FRAMES 4
#define TIME_FEET_FRAMES 5


class DB
{
public:
	DB(float infinitygain = INFINITYGAIN);
	virtual ~DB() {};
	
// return power of db using a table
	float fromdb_table();
	float fromdb_table(float db);
// return power from db using log10
	float fromdb();
	static float fromdb(float db);

// convert db to power using a formula
	static float todb(float power);

	inline DB& operator++() { if(db < MAXGAIN) db += 0.1; return *this; };
	inline DB& operator--() { if(db > INFINITYGAIN) db -= 0.1; return *this; };
	inline DB& operator=(DB &newdb) { db = newdb.db; return *this; };
	inline DB& operator=(int newdb) { db = newdb; return *this; };
	inline int operator==(DB &newdb) { return db == newdb.db; };
	inline int operator==(int newdb) { return db == newdb; };

	static float *topower;
	float db;
	float infinitygain;
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

	// No rounding.
	static float toframes(int64_t samples, int sample_rate, float framerate);
	// Round up if > .5
	static int64_t toframes_round(int64_t samples, int sample_rate, float framerate);
	static double fix_framerate(double value);
	static double atoframerate(char *text);


	static int64_t tosamples(float frames, int sample_rate, float framerate);
	static char* totext(char *text, 
				int64_t samples, 
				int time_format, 
				int samplerate, 
				float frame_rate = 0, 
				float frames_per_foot = 0);    // give text representation as time
	static char* totext(char *text, 
				double seconds, 
				int time_format, 
				int sample_rate = 0,
				float frame_rate = 0, 
				float frames_per_foot = 0);    // give text representation as time
	static int64_t fromtext(char *text, 
				int samplerate, 
				int time_format, 
				float frame_rate, 
				float frames_per_foot);    // convert time to samples
	static double text_to_seconds(char *text, 
				int samplerate, 
				int time_format, 
				float frame_rate, 
				float frames_per_foot);   // Convert text to seconds

	static char* print_time_format(int time_format, char *string);

	static float xy_to_polar(int x, int y);
	static void polar_to_xy(float angle, int radius, int &x, int &y);

// Numbers < 0 round down if next digit is < 5
// Numbers > 0 round up if next digit is > 5
	static int64_t round(double result);

// Flooring type converter rounded to nearest .001
	static int64_t to_int64(double result);

	static float quantize10(float value);
	static float quantize(float value, float precision);

	static void* int64_to_ptr(uint64_t value);
	static uint64_t ptr_to_int64(void *ptr);
};

#endif
