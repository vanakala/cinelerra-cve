
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

#include "assets.h"
#include "byteorder.h"
#include "file.h"
#include "filebase.h"
#include "sizes.h"

// ======================================= ulaw codecs

float FileBase::ulawtofloat(char ulaw)
{
//printf("%f\n", ulawtofloat_ptr[ulaw]);
	return ulawtofloat_ptr[(unsigned char)ulaw];
}

char FileBase::floattoulaw(float value)
{
	return floattoulaw_ptr[(int)(value * 32767)];
}

// turn off the trap as per the MIL-STD
#undef ZEROTRAP
// define the add-in bias for 16 bit samples
#define uBIAS 0x84
#define uCLIP 32635

int FileBase::generate_ulaw_tables()
{
	int i;
	float value;

	if(!ulawtofloat_table)
	{
    	static int exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
    	int sign, exponent, mantissa, sample;
		unsigned char ulawbyte;

		ulawtofloat_table = new float[256];
		ulawtofloat_ptr = ulawtofloat_table;
		for(i = 0; i < 256; i++)
		{
			ulawbyte = (unsigned char)i;
    		ulawbyte = ~ ulawbyte;
    		sign = ( ulawbyte & 0x80 );
    		exponent = ( ulawbyte >> 4 ) & 0x07;
    		mantissa = ulawbyte & 0x0F;
    		sample = exp_lut[exponent] + ( mantissa << ( exponent + 3 ) );
    		if ( sign != 0 ) sample = -sample;

			ulawtofloat_ptr[(int)i] = (float)sample / 32768;
		}
	}

	if(!floattoulaw_table)
	{
    	int sign, exponent, mantissa;
    	unsigned char ulawbyte;
		int sample;
    	int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                               4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};

 		floattoulaw_table = new unsigned char[65536];
		floattoulaw_ptr = floattoulaw_table + 32768;

		for(i = -32768; i < 32768; i++)
		{
			sample = i;
// Get the sample into sign-magnitude.
    		sign = (sample >> 8) & 0x80;		// set aside the sign
    		if ( sign != 0 ) sample = -sample;		// get magnitude
    		if ( sample > uCLIP ) sample = uCLIP;		// clip the magnitude

// Convert from 16 bit linear to ulaw.
    		sample = sample + uBIAS;
		    exponent = exp_lut[( sample >> 7 ) & 0xFF];
		    mantissa = ( sample >> ( exponent + 3 ) ) & 0x0F;
		    ulawbyte = ~ ( sign | ( exponent << 4 ) | mantissa );
#ifdef ZEROTRAP
		    if ( ulawbyte == 0 ) ulawbyte = 0x02;	/* optional CCITT trap */
#endif

		    floattoulaw_ptr[i] = ulawbyte;
		}
	}
	return 0;
}

int FileBase::delete_ulaw_tables()
{
	if(floattoulaw_table) delete [] floattoulaw_table;
	if(ulawtofloat_table) delete [] ulawtofloat_table;
	floattoulaw_table = 0;
	ulawtofloat_table = 0;
	return 0;
}
