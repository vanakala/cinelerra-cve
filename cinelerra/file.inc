#ifndef FILE_INC
#define FILE_INC

// Return values for open_file
#define FILE_OK 0
#define FILE_NOT_FOUND 1
#define FILE_UNRECOGNIZED_CODEC 2
#define FILE_IS_XML 3

#define FILE_UNKNOWN    		0
#define FILE_AIFF       		10
#define FILE_AMPEG      	  	16    // For encoding only
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
#define FILE_MPEG       	  	15    // For decoding only
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
#define FILE_VORBIS     	  	18
#define FILE_WAV  	    		2

// For formats supported by plugins, the format number is the plugin number in the 
// plugin list ORed with 0x8000.


#define AIFF_NAME "Apple/SGI AIFF"
#define AMPEG_NAME "MPEG Audio"    // For encoding only
#define AU_NAME "Sun/NeXT AU"
#define AVI_NAME "Microsoft AVI"
#define WAV_NAME "Microsoft WAV"
#define AVI_ARNE1_NAME "AVI Arne Type 1"
#define AVI_ARNE2_NAME "AVI DV Type 2"
#define AVI_AVIFILE_NAME "AVI Avifile"
#define AVI_LAVTOOLS_NAME "AVI Lavtools"
#define JPEG_LIST_NAME "JPEG Sequence"
#define JPEG_NAME "JPEG"
#define MOV_NAME "Quicktime for Linux"
#define MPEG_NAME "MPEG"           // For decoding only
#define PCM_NAME "Raw PCM"
#define PNG_LIST_NAME "PNG Sequence"
#define PNG_NAME "PNG"
#define SND_NAME "Unknown sound"
#define TGA_LIST_NAME "TGA Sequence"
#define TGA_NAME "TGA"
#define TIFF_NAME "TIFF"
#define TIFF_LIST_NAME "TIFF Sequence"
#define VMPEG_NAME "MPEG Video"    // For encoding only
#define VORBIS_NAME "OGG Vorbis"

#define BITSLINEAR8    8
#define BITSLINEAR16   16
#define BITSLINEAR24   24
#define BITSLINEAR32   32
#define BITS_ADPCM     252
#define BITSFLOAT      253
#define BITSULAW       254
#define BITSIMA4       255

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
