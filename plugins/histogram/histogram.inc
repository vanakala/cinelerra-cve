// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef HISTOGRAM_INC
#define HISTOGRAM_INC

// modes
#define HISTOGRAM_MODES 4
#define HISTOGRAM_RED   0
#define HISTOGRAM_GREEN 1
#define HISTOGRAM_BLUE  2
#define HISTOGRAM_VALUE 3

// Number of divisions in histogram.  
// 65536 + min and max range to speed up the tabulation
#define HISTOGRAM_SLOTS         0x13333
#define HISTOGRAM_FLOAT_RANGE   1.2
// Minimum value in percentage
#define HISTOGRAM_MIN          -10
#define HISTOGRAM_MIN_INPUT    -0.1
// Maximum value in percentage
#define HISTOGRAM_MAX           110
#define HISTOGRAM_MAX_INPUT     1.1

#define HISTOGRAM_PRECISION 0.001
#define HISTOGRAM_DIGITS 3

#define HISTOGRAM_BOX_SIZE 10

class HistogramEngine;

#endif
