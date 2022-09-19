// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcwindowbase.inc"
#include "bcsignals.h"
#include "units.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

int* Freq::freqtable = 0;

DB::DB()
{
	db = 0;
}

double DB::fromdb(double db)
{
	return pow(10.0, db / 20.0);
}

// set db to the power given using a formula
double DB::todb(double power)
{
	double db;

	if(power == 0)
		db = -100;
	else 
	{
		db = 20.0 * log10(power);
		if(db < -100) db = -100;
	}
	return db;
}


Freq::Freq()
{
	init_table();
	freq = 0;
}

Freq::Freq(const Freq& oldfreq)
{
	this->freq = oldfreq.freq;
}

void Freq::init_table()
{
	if(!freqtable)
	{
		freqtable = new int[TOTALFREQS + 1];
// starting frequency
		double freq1 = 27.5, freq2 = 55;  
// Some number divisable by three.  This depends on the value of TOTALFREQS
		int scale = 105;

		freqtable[0] = 0;
		for(int i = 1, j = 0; i <= TOTALFREQS; i++, j++)
		{
			freqtable[i] = (int)(freq1 + (freq2 - freq1) / scale * j + 0.5);
			if(j >= scale)
			{
				freq1 = freq2;
				freq2 *= 2;
				j = 0;
			}
		}
	}
}

int Freq::fromfreq() 
{
	int i;

	for(i = 0; i < TOTALFREQS && freqtable[i] < freq; i++)
		;
	return(i);
};

int Freq::fromfreq(int index) 
{
	int i;

	init_table();
	for(i = 0; i < TOTALFREQS && freqtable[i] < index; i++)
		;
	return(i);
};

int Freq::tofreq(int index)
{ 
	init_table();
	int freq = freqtable[index]; 
	return freq; 
}

Freq& Freq::operator++() 
{
	if(freq < TOTALFREQS) freq++;
	return *this;
}

Freq& Freq::operator--()
{
	if(freq > 0) freq--;
	return *this;
}

int Freq::operator>(Freq &newfreq) { return freq > newfreq.freq; }
int Freq::operator<(Freq &newfreq) { return freq < newfreq.freq; }
Freq& Freq::operator=(const Freq &newfreq) { freq = newfreq.freq; return *this; }
int Freq::operator=(const int newfreq) { freq = newfreq; return newfreq; }
int Freq::operator!=(Freq &newfreq) { return freq != newfreq.freq; }
int Freq::operator==(Freq &newfreq) { return freq == newfreq.freq; }
int Freq::operator==(int newfreq) { return freq == newfreq; }

char* Units::totext(char *text, ptstime seconds,
	int time_format, int sample_rate,
	double frame_rate)    // give text representation as time
{
	int hour, minute, second, thousandths;
	int64_t frame, feet;

	switch(time_format)
	{
		case TIME_SECONDS:
			seconds = fabs(seconds);
			sprintf(text, "%04d.%03d", (int)seconds, (int)(seconds * 1000) % 1000);
			return text;

		case TIME_HMS:
			seconds = fabs(seconds);
			hour = (int)(seconds / 3600);
			minute = (int)(seconds / 60 - hour * 60);
			second = (int)seconds - (int64_t)hour * 3600 - (int64_t)minute * 60;
			thousandths = (int)(seconds * 1000) % 1000;
			sprintf(text, "%d:%02d:%02d.%03d", 
				hour, 
				minute, 
				second, 
				thousandths);
			return text;

		case TIME_HMS2:
		{
			double second;
			seconds = fabs(seconds);
			hour = (int)(seconds / 3600);
			minute = (int)(seconds / 60 - hour * 60);
			second = (double)seconds - (int64_t)hour * 3600 - (int64_t)minute * 60;
			sprintf(text, "%d:%02d:%02d", hour, minute, (int)second);
			return text;
		}

		case TIME_HMS3:
		{
			double second;
			seconds = fabs(seconds);
			hour = (int)(seconds / 3600);
			minute = (int)(seconds / 60 - hour * 60);
			second = (double)seconds - (int64_t)hour * 3600 - (int64_t)minute * 60;
			sprintf(text, "%02d:%02d:%02d", hour, minute, (int)second);
			return text;
		}

		case TIME_HMSF:
		{
			int second;
			seconds = fabs(seconds);
			hour = (int)(seconds / 3600);
			minute = (int)(seconds / 60 - hour * 60);
			second = (int)(seconds - hour * 3600 - minute * 60);
			frame = (int64_t)((double)frame_rate * 
					seconds + 
					0.0000001) - 
				(int64_t)((double)frame_rate * 
					(hour * 
					3600 + 
					minute * 
					60 + 
					second) + 
				0.0000001);   
			sprintf(text, "%01d:%02d:%02d:%02" PRId64, hour, minute, second, frame);
			return text;
		}

		case TIME_SAMPLES:
			sprintf(text, "%09" PRId64, (int64_t)round(seconds * sample_rate));
			break;

		case TIME_FRAMES:
			frame = round(seconds * frame_rate);
			sprintf(text, "%06" PRId64, frame);
			return text;
			break;
	}
	return text;
}

// give text representation as time
char* Units::totext(char *text, samplenum samples, int samplerate,
	int time_format, double frame_rate)
{
	return totext(text, (double)samples / samplerate, time_format, samplerate, frame_rate);
}

ptstime Units::text_to_seconds(const char *text, int samplerate,
	int time_format, double frame_rate)
{
	char *nptr;
	long hour, min, sec, mil;
	posnum pos;

	hour = min = sec = mil = 0;

	switch(time_format)
	{
	case TIME_HMSF:
	case TIME_HMS:
	case TIME_HMS2:
	case TIME_HMS3:
		hour = strtol(text, &nptr, 10);
		if(*nptr)
		{
			min = strtol(nptr + 1, &nptr, 10);
			if(*nptr)
			{
				sec = strtol(nptr + 1, &nptr, 10);
				if(*nptr)
					mil = strtol(nptr + 1, &nptr, 10);
			}
		}
		if(time_format == TIME_HMSF)
			return (ptstime)hour * 3600 + min * 60 + sec + mil / frame_rate;
		return (ptstime)hour * 3600 + min * 60 + sec + mil / 1000.0;
	case TIME_SECONDS:
		return atof(text);

	case TIME_SAMPLES:
		return atof(text) / samplerate;

	case TIME_FRAMES:
		return atof(text) / frame_rate;
	}
	return 0;
}

int Units::timeformat_totype(const char *tcf) 
{
	if(!strcmp(tcf,TIME_SECONDS__STR)) return(TIME_SECONDS);
	if(!strcmp(tcf,TIME_HMS__STR)) return(TIME_HMS);
	if(!strcmp(tcf,TIME_HMS2__STR)) return(TIME_HMS2);
	if(!strcmp(tcf,TIME_HMS3__STR)) return(TIME_HMS3);
	if(!strcmp(tcf,TIME_HMSF__STR)) return(TIME_HMSF);
	if(!strcmp(tcf,TIME_SAMPLES__STR)) return(TIME_SAMPLES);
	if(!strcmp(tcf,TIME_FRAMES__STR)) return(TIME_FRAMES);
	return(-1);
}

double Units::fix_framerate(double value)
{
	if(value > 29.5 && value < 30) 
		value = (double)30000 / (double)1001;
	else
	if(value > 59.5 && value < 60) 
		value = (double)60000 / (double)1001;
	else
	if(value > 23.5 && value < 24) 
		value = (double)24000 / (double)1001;

	return value;
}

double Units::atoframerate(const char *text)
{
	double result = atof(text);
	return fix_framerate(result);
}

samplenum Units::tosamples(double frames, int sample_rate, double framerate)
{
	return ceil(frames / framerate * sample_rate);
}

double Units::xy_to_polar(int x, int y)
{
	double angle;

	if(x > 0 && y <= 0)
	{
		angle = atan((double)-y / x) / (2 * M_PI) * 360;
	}
	else
	if(x < 0 && y <= 0)
	{
		angle = 180 - atan((double)-y / -x) / (2 * M_PI) * 360;
	}
	else
	if(x < 0 && y > 0)
	{
		angle = 180 - atan((double)-y / -x) / (2 * M_PI) * 360;
	}
	else
	if(x > 0 && y > 0)
	{
		angle = 360 + atan((double)-y / x) / (2 * M_PI) * 360;
	}
	else
	if(x == 0 && y < 0)
	{
		angle = 90;
	}
	else
	if(x == 0 && y > 0)
	{
		angle = 270;
	}
	else
	if(x == 0 && y == 0)
	{
		angle = 0;
	}
	return angle;
}

void Units::polar_to_xy(double angle, int radius, int &x, int &y)
{
	while(angle < 0)
		angle += 360;

	x = round(cos(angle / 360 * (2 * M_PI)) * radius);
	y = round(-sin(angle / 360 * (2 * M_PI)) * radius);
}

double Units::quantize10(double value)
{
	return round(value * 10) / 10;
}

double Units::quantize(double value, double precision)
{
	return round(value / precision) * precision;
}

char* Units::print_time_format(int time_format, char *string)
{

	switch(time_format)
	{
	case TIME_HMS:
		strcpy(string, "Hours:Minutes:Seconds.xxx");
		break;
	case TIME_HMSF:
		strcpy(string, "Hours:Minutes:Seconds:Frames");
		break;
	case TIME_SAMPLES:
		strcpy(string, "Samples");
		break;
	case TIME_FRAMES:
		strcpy(string, "Frames");
		break;
	case TIME_HMS2:
	case TIME_HMS3:
		strcpy(string, "Hours:Minutes:Seconds");
		break;
	case TIME_SECONDS:
		strcpy(string, "Seconds");
		break;
	default:
		*string = 0;
	}
	return string;
}

const char* Units::format_to_separators(int time_format)
{
	switch(time_format)
	{
	case TIME_SECONDS:
		return "0000.000";
	case TIME_HMS:
		return "0:00:00.000";
	case TIME_HMS2:
		return "0:00:00";
	case TIME_HMS3:
		return "00:00:00";
	case TIME_HMSF:
		return "0:00:00:00";
	case TIME_SAMPLES:
	case TIME_FRAMES:
		break;
	}
	return 0;
}

void Units::punctuate(char *string)
{
	int len = strlen(string);
	int commas = (len - 1) / 3;
	for(int i = len + commas, j = len, k; j >= 0 && i >= 0; i--, j--)
	{
		k = (len - j - 1) / 3;
		if(k * 3 == len - j - 1 && j != len - 1 && string[j] != 0)
		{
			string[i--] = ',';
		}

		string[i] = string[j];
	}
}
