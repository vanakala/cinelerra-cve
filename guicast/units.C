// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcwindowbase.inc"
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
	double frame_rate, double frames_per_foot)    // give text representation as time
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

		case TIME_SAMPLES_HEX:
			sprintf(text, "%08" PRIx64, (int64_t)round(seconds * sample_rate));
			break;

		case TIME_FRAMES:
			frame = round(seconds * frame_rate);
			sprintf(text, "%06" PRId64, frame);
			return text;
			break;

		case TIME_FEET_FRAMES:
			frame = round(seconds * frame_rate);
			feet = (int64_t)(frame / frames_per_foot);
			sprintf(text, "%05" PRId64 "-%02" PRId64,
				feet, 
				(int64_t)(frame - feet * frames_per_foot));
			return text;
			break;
	}
	return text;
}

// give text representation as time
char* Units::totext(char *text, samplenum samples, int samplerate,
	int time_format, double frame_rate, double frames_per_foot)
{
	return totext(text, (double)samples / samplerate, time_format, samplerate, frame_rate, frames_per_foot);
}

samplenum Units::fromtext(const char *text, int samplerate,
	int time_format, double frame_rate,
	double frames_per_foot)
{
	int64_t hours, minutes, frames, total_samples, i, j;
	int64_t feet;
	double seconds;
	char string[BCTEXTLEN];

	switch(time_format)
	{
		case TIME_SECONDS:
			seconds = atof(text);
			return seconds * samplerate;

		case TIME_HMS:
		case TIME_HMS2:
		case TIME_HMS3:
// get hours
			i = 0;
			j = 0;
			while(text[i] >= '0' && text[i] <= '9' && j < 10)
				string[j++] = text[i++];
			string[j] = 0;
			hours = atol(string);
// get minutes
			j = 0;
// skip separator
			while((text[i] < '0' || text[i] > '9') && text[i] != 0) i++;
			while(text[i] >= '0' && text[i] <= '9' && j < 10)
				string[j++] = text[i++];
			string[j] = 0;
			minutes = atol(string);

// get seconds
			j = 0;
// skip separator
			while((text[i] < '0' || text[i] > '9') && text[i] != 0) i++;
			while((text[i] == '.' || (text[i] >= '0' && text[i] <= '9')) && j < 10)
				string[j++] = text[i++];
			string[j] = 0;
			seconds = atof(string);

			return ((double)seconds + minutes * 60 + hours * 3600) *
					samplerate;

		case TIME_HMSF:
// get hours
			i = 0;
			j = 0;
			while(text[i] >= '0' && text[i] <= '9' && j < 10)
				string[j++] = text[i++];
			string[j] = 0;
			hours = atol(string);

// get minutes
			j = 0;
// skip separator
			while((text[i] < '0' || text[i] > '9') && text[i] != 0) i++;
			while(text[i] >= '0' && text[i] <= '9' && j < 10)
				string[j++] = text[i++];
			string[j] = 0;
			minutes = atol(string);

// get seconds
			j = 0;
// skip separator
			while((text[i] < '0' || text[i] > '9') && text[i] != 0) i++;
			while(text[i] >= '0' && text[i] <= '9' && j < 10)
				string[j++] = text[i++];
			string[j] = 0;
			seconds = atof(string);

// skip separator
			while((text[i] < '0' || text[i] > '9') && text[i] != 0) i++;
// get frames
			j = 0;
			while(text[i] >= '0' && text[i] <= '9' && j < 10)
				string[j++] = text[i++];
			string[j] = 0;
			frames = atol(string);

			return ((double)frames / frame_rate + seconds +
				minutes * 60 + hours * 3600) * samplerate;

		case TIME_SAMPLES:
			return atol(text);

		case TIME_SAMPLES_HEX:
			sscanf(text, "%" PRId64, &total_samples);
			return total_samples;

		case TIME_FRAMES:
			return (ptstime)(atof(text) / frame_rate * samplerate);

		case TIME_FEET_FRAMES:
// Get feet
			i = 0;
			j = 0;
			while(text[i] >= '0' && text[i] <= '9' && text[i] != 0 && j < 10)
				string[j++] = text[i++];
			string[j] = 0;
			feet = atol(string);

// skip separator
			while((text[i] < '0' || text[i] > '9') && text[i] != 0) i++;

// Get frames
			j = 0;
			while(text[i] >= '0' && text[i] <= '9' && text[i] != 0 && j < 10)
				string[j++] = text[i++];
			string[j] = 0;
			frames = atol(string);
			return ((double)feet * frames_per_foot + frames) / frame_rate * samplerate;
	}
	return 0;
}

ptstime Units::text_to_seconds(const char *text, int samplerate,
	int time_format, double frame_rate, double frames_per_foot)
{
	return (ptstime)fromtext(text, samplerate, time_format,
		frame_rate, frames_per_foot) / samplerate;
}

int Units::timeformat_totype(const char *tcf) 
{
	if(!strcmp(tcf,TIME_SECONDS__STR)) return(TIME_SECONDS);
	if(!strcmp(tcf,TIME_HMS__STR)) return(TIME_HMS);
	if(!strcmp(tcf,TIME_HMS2__STR)) return(TIME_HMS2);
	if(!strcmp(tcf,TIME_HMS3__STR)) return(TIME_HMS3);
	if(!strcmp(tcf,TIME_HMSF__STR)) return(TIME_HMSF);
	if(!strcmp(tcf,TIME_SAMPLES__STR)) return(TIME_SAMPLES);
	if(!strcmp(tcf,TIME_SAMPLES_HEX__STR)) return(TIME_SAMPLES_HEX);
	if(!strcmp(tcf,TIME_FRAMES__STR)) return(TIME_FRAMES);
	if(!strcmp(tcf,TIME_FEET_FRAMES__STR)) return(TIME_FEET_FRAMES);
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
		case 0: sprintf(string, "Hours:Minutes:Seconds.xxx"); break;
		case 1: sprintf(string, "Hours:Minutes:Seconds:Frames"); break;
		case 2: sprintf(string, "Samples"); break;
		case 3: sprintf(string, "Hex Samples"); break;
		case 4: sprintf(string, "Frames"); break;
		case 5: sprintf(string, "Feet-frames"); break;
		case 8: sprintf(string, "Seconds"); break;
	}
	return string;
}

const char* Units::format_to_separators(int time_format)
{
	switch(time_format)
	{
		case TIME_SECONDS:     return "0000.000";
		case TIME_HMS:         return "0:00:00.000";
		case TIME_HMS2:        return "0:00:00";
		case TIME_HMS3:        return "00:00:00";
		case TIME_HMSF:        return "0:00:00:00";
		case TIME_SAMPLES:     return 0;
		case TIME_SAMPLES_HEX: return 0;
		case TIME_FRAMES:      return 0;
		case TIME_FEET_FRAMES: return "00000-00";
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
