
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

#ifndef HISTOGRAM_INC
#define HISTOGRAM_INC





// modes
#define HISTOGRAM_MODES 4
#define HISTOGRAM_RED 0
#define HISTOGRAM_GREEN 1
#define HISTOGRAM_BLUE 2
#define HISTOGRAM_VALUE 3

// Number of divisions in histogram.  
// 65536 + min and max range to speed up the tabulation
#define HISTOGRAM_SLOTS 0x13333
#define FLOAT_RANGE 1.2
// Minimum value in percentage
#define HISTOGRAM_MIN -10
#define MIN_INPUT -0.1
// Maximum value in percentage
#define HISTOGRAM_MAX 110
#define MAX_INPUT 1.1

#define PRECISION 0.001
#define DIGITS 3
#define THRESHOLD_SCALE 1000

#define BOX_SIZE 10


class HistogramEngine;
class HistogramMain;


#endif
