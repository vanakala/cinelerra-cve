
/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */


#ifndef COLORMODELS_INC
#define COLORMODELS_INC

class ColorModels;

// Colormodels
// Extracted from Quicktime

#define BC_NOCOLOR      0
#define BC_COMPRESSED   1
#define BC_RGB8         2
#define BC_RGB565       3
#define BC_BGR565       4
#define BC_BGR888       5
#define BC_BGR8888      6
#define BC_TRANSPARENCY 8
// Working bitmaps are packed to simplify processing
#define BC_RGB888       9
#define BC_RGBA8888     10
#define BC_ARGB8888     20
#define BC_ABGR8888     21
#define BC_RGB161616    11
#define BC_RGBA16161616 12
#define BC_YUV888       13
#define BC_YUVA8888     14
#define BC_YUV161616    15
#define BC_YUVA16161616 16
#define BC_YUV422       19
#define BC_A8           22
#define BC_A16          23
#define BC_A_FLOAT      31
#define BC_RGB_FLOAT    29
#define BC_RGBA_FLOAT   30
#define BC_AYUV16161616 32
// Planar
#define BC_YUV420P      7
#define BC_YUV422P      17
#define BC_YUV444P      27
#define BC_YUV411P      18

// Interpolation types
#define NEAREST_NEIGHBOR 0        // Nearest for enlargement and reduction
#define CUBIC_CUBIC      1        // Bicubic enlargement and reduction
#define CUBIC_LINEAR     2        // Bicubic enlargement and linear reduction
#define LINEAR_LINEAR    3        // Not explicitely called but used for user
#define LANCZOS_LANCZOS  4        // Just because we can

#endif
