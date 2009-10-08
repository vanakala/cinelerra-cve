
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

#ifndef WORKAROUNDS_H
#define WORKAROUNDS_H

#include <stdint.h>

class Workarounds
{
public:
	Workarounds() {};
	~Workarounds() {};

	static void copy_int(int &a, int &b);
	static void copy_double(double *a, double b);
	static double divide_double(double a, double b);
	static void clamp(int32_t &x, int32_t y, int32_t z);
	static void clamp(int64_t &x, int64_t y, int64_t z);
	static void clamp(float &x, float y, float z);
	static void clamp(double &x, double y, double z);
	static float pow(float x, float y);
};

#endif
