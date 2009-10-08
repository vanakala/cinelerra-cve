
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

#ifndef FILE_INC
#define FILE_INC

#include "language.h"

// Return values for open_file
#define FILE_OK 0
#define FILE_NOT_FOUND 1
#define FILE_UNRECOGNIZED_CODEC 2
#define FILE_IS_XML 3

#define FILE_UNKNOWN    		0
#define FILE_AIFF       		10
#define FILE_AMPEG      	  	16    // For encoding only
#define FILE_AC3                25    // AC3 encoding
#define FILE_AU         		9
#define FILE_AVI_ARNE1          20    // DV type 1
#define FILE_AVI                24    // Heroine Virtual
#define FILE_AVI_ARNE2          19    // DV type 2
#define FILE_AVI_AVIFILE        21
#define FILE_AVI_LAVTOOLS       12    // MJPEG type 1
#define FILE_GIF  	    		7
#define FILE_JPEG 	    		5
#define FILE_JPEG_LIST  		8
#define FILE_MOV  	    		3
// Libmpeg3 decoding and transport stream capture only
#define FILE_MPEG       	  	15    
#define FILE_PCM  	    		1
#define FILE_PLUGIN     	  	0x8000
#define FILE_PNG  	    		4
#define FILE_PNG_LIST   		23
#define FILE_SND        		11
#define FILE_TGA        	  	14
#define FILE_TGA_LIST   	  	13
#define FILE_TIFF 	    		6
#define FILE_TIFF_LIST  		22
#define FILE_VMPEG      	  	17    // For encoding only
#define FILE_OGG		        30
#define FILE_WAV  	    		2
#define FILE_EXR                26
#define FILE_EXR_LIST           27
#define FILE_CR2                28
#define FILE_YUV		29	// mjpegtools YUV4MPEG (aka YUV4MPEG2)
#define FILE_VORBIS             31
#define FILE_RAWDV		32

// For formats supported by plugins, the format number is the plugin number in the 
// plugin list ORed with 0x8000.

#define FILE_FORMAT_PREFIX(format) ( \
	format == FILE_AIFF    ? "AIFF" :		\
	format == FILE_AMPEG   ? "MPEG" :		\
	format == FILE_AC3     ? "AC3" :		\
	format == FILE_AU      ? "AU" :			\
	format == FILE_AVI_ARNE1 ? "AVI_ARNE1" :	\
	format == FILE_AVI     ? "AVI" :		\
	format == FILE_AVI_ARNE2 ? "AVI_ARNE2" :	\
	format == FILE_AVI_AVIFILE ? "AVIFILE" :	\
	format == FILE_AVI_LAVTOOLS ? "AVI_LAVTOOLS" :	\
	format == FILE_GIF     ? "GIF" :		\
	format == FILE_JPEG    ? "JPEG" :		\
	format == FILE_JPEG_LIST ? "JPEG_LIST" :	\
	format == FILE_MOV     ? "MOV" :		\
	format == FILE_MPEG    ? "MPEG" :		\
	format == FILE_PCM     ? "PCM" :		\
	format == FILE_PLUGIN  ? "PLUGIN" :		\
	format == FILE_PNG     ? "PNG" :		\
	format == FILE_PNG_LIST ? "PNG_LIST" :		\
	format == FILE_SND     ? "SND" :		\
	format == FILE_TGA     ? "TGA" :		\
	format == FILE_TGA_LIST ? "TGA_LIST" :		\
	format == FILE_TIFF    ? "TIFF" :		\
	format == FILE_TIFF_LIST ? "TIFF_LIST" :	\
	format == FILE_VMPEG   ? "VMPEG" :		\
	format == FILE_VORBIS  ? "VORBIS" :		\
	format == FILE_WAV     ? "WAV" :		\
	format == FILE_EXR     ? "EXR" :		\
	format == FILE_EXR_LIST ? "EXR_LIST" :		\
	format == FILE_RAWDV   ? "RAWDV" :		\
	format == FILE_OGG     ? "OGG" :		\
	format == FILE_YUV     ? "YUV" :		\
	"UNKNOWN")

#if 0
N_("AC3")
N_("Apple/SGI AIFF")
N_("MPEG Audio")    // For encoding only
N_("Sun/NeXT AU")
N_("Microsoft AVI")
N_("Microsoft WAV")
N_("AVI Arne Type 1")
N_("AVI DV Type 2")
N_("AVI Avifile")
N_("AVI Lavtools")
N_("Raw DV")
N_("JPEG Sequence")
N_("JPEG")
N_("Quicktime for Linux")
N_("MPEG")          // For decoding only
N_("Raw PCM")
N_("PNG Sequence")
N_("PNG")
N_("Unknown sound")
N_("TGA Sequence")
N_("TGA")
N_("TIFF")
N_("TIFF Sequence")
N_("MPEG Video")    // For encoding only
N_("OGG Theora/Vorbis")
N_("OGG Vorbis")    // For decoding only
N_("EXR")
N_("EXR Sequence")
N_("OGG Theora/Vorbis")
#endif

#define AC3_NAME "AC3"
#define AIFF_NAME "Apple/SGI AIFF"
#define AMPEG_NAME "MPEG Audio"    // For encoding only
#define AU_NAME "Sun/NeXT AU"
#define AVI_NAME "Microsoft AVI"
#define WAV_NAME "Microsoft WAV"
#define AVI_ARNE1_NAME "AVI Arne Type 1"
#define AVI_ARNE2_NAME "AVI DV Type 2"
#define AVI_AVIFILE_NAME "AVI Avifile"
#define AVI_LAVTOOLS_NAME "AVI Lavtools"
#define RAWDV_NAME "Raw DV"
#define OGG_NAME "OGG Theora/Vorbis"
#define JPEG_LIST_NAME "JPEG Sequence"
#define JPEG_NAME "JPEG"
#define MOV_NAME "Quicktime for Linux"
#define MPEG_NAME "MPEG"           // For decoding only
#define OGG_NAME "OGG Theora/Vorbis"
#define VORBIS_NAME "OGG Vorbis"
#define PCM_NAME "Raw PCM"
#define PNG_LIST_NAME "PNG Sequence"
#define PNG_NAME "PNG"
#define SND_NAME "Unknown sound"
#define TGA_LIST_NAME "TGA Sequence"
#define TGA_NAME "TGA"
#define TIFF_NAME "TIFF"
#define TIFF_LIST_NAME "TIFF Sequence"
#define VMPEG_NAME "MPEG Video"    // For encoding only
#define EXR_NAME "EXR"
#define CR2_NAME "Raw Camera"
#define EXR_LIST_NAME "EXR Sequence"
#define YUV_NAME "YUV4MPEG Stream"


#define BITSLINEAR8    8
#define BITSLINEAR16   16
#define BITSLINEAR24   24
#define BITSLINEAR32   32
#define BITS_ADPCM     252
#define BITSFLOAT      253
#define BITSULAW       254
#define BITSIMA4       255

#if 0
N_("8 Bit Linear")
N_("16 Bit Linear")
N_("24 Bit Linear")
N_("32 Bit Linear")
N_("u Law")
N_("IMA 4")
N_("ADPCM")
N_("Float")

// Video formats not part of Quicktime
N_("RGB ALPHA")
N_("PNG ALPHA")
#endif

#define NAME_8BIT "8 Bit Linear"
#define NAME_16BIT "16 Bit Linear"
#define NAME_24BIT "24 Bit Linear"
#define NAME_32BIT "32 Bit Linear"
#define NAME_ULAW "u Law"
#define NAME_IMA4 "IMA 4"
#define NAME_ADPCM "ADPCM"
#define NAME_FLOAT "Float"

// Video formats not part of Quicktime
#define MOV_RGBA "RGB ALPHA"
#define MOV_PNGA "PNG ALPHA"


class File;

#endif
