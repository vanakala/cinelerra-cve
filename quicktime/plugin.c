#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include "funcprotos.h"
#include "quicktime.h"


static int total_vcodecs = 0;
static int total_acodecs = 0;
static quicktime_extern_video_t *vcodecs = NULL; 
static quicktime_extern_audio_t *acodecs = NULL; 


#define CODEC_PREFIX "quicktime_codec_"



static int decode_audio_external(quicktime_t *file, 
				int16_t *output_i, 
				float *output_f, 
				long samples, 
				int track,
				int channel)
{
    return -1;
}

static int encode_audio_external(quicktime_t *file, 
				int16_t **input_i, 
				float **input_f, 
				int track, 
				long samples)
{
    return -1;
}

static int decode_video_external(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track)
{
	unsigned char *input;
	unsigned char *output = row_pointers[0]; 
	int	index = 0;
	int error = -1;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	unsigned int bytes;
	char *compressor;

	compressor = quicktime_video_compressor(file, track);
	index = quicktime_find_vcodec(compressor);

	if(index >= 0)
	{
    	quicktime_set_video_position(file, vtrack->current_position, track);
    	bytes = quicktime_frame_size(file, vtrack->current_position, track);

    	if(bytes <= 0) 
		{
    		fprintf(stderr, "decode_video_external: frame size equal %d\n", bytes);
    		return -1;
    	}

    	input = (unsigned char*)malloc(bytes);
    	if(input) 
		{
    	    if(quicktime_read_data(file, input, bytes)) 
				error = vcodecs[index].decode(file, track, bytes, input, output); 
    	  	else 
				fprintf(stderr, "decode_video_external: can't read data from file\n");

    	} 
		else 
			fprintf(stderr, "decode_video_external: Can't allocate decoding buffer");

    	free(input);
	}
	else 
	{
    	fprintf(stderr, 
	    	"decode_video_external: Can't find the corresponding codec: ",
	    	quicktime_video_compressor(file, track));
	}
    return error;
}




static int encode_video_external(quicktime_t *file, 
				 unsigned char **row_pointers, 
				 int track)
{
	unsigned char *input = row_pointers[0];
	unsigned char *output; 
	int index = 0;
	int error = -1;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	longest bytes;
	longest offset = quicktime_position(file);
	char *compressor;
	short width, height, depth;

	compressor = quicktime_video_compressor(file, track);
	index = quicktime_find_vcodec(compressor);
	if(index >= 0)
	{
/* Initializing some local variables */
    	width  = vtrack->track->tkhd.track_width;
    	height = vtrack->track->tkhd.track_height;
    	depth  = file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].depth;
    	bytes  = width * height * depth / 8;

    	output = (unsigned char*)malloc(bytes);
    	if(output) 
		{
    	    bytes = vcodecs[index].encode(file, track, input, output); 

    	    if(bytes > 0)
			{
				printf("Writing %d bytes\n", bytes);

				error = !quicktime_write_data(file, output, bytes);

				quicktime_update_tables(file,
					file->vtracks[track].track,
					offset,
					file->vtracks[track].current_chunk,
					file->vtracks[track].current_position,
					1,
					bytes);

				file->vtracks[track].current_chunk++;
      		} 
			else 
				fprintf(stderr, "encode_video_external: Error in external encoding function\n");

      		free(output);
    	} 
		else 
			fprintf(stderr, "encode_video_external: Can't allocate encoding buffer");
  	} 
	else 
    	fprintf(stderr, 
	    	"encode_video_external: Can't find the corresponding codec: ",
	    	quicktime_video_compressor(file, track) );
  
    return error;    
}



int quicktime_vcodec_size()
{
	return total_vcodecs;
}

int quicktime_acodec_size()
{
	return total_acodecs;
}


int quicktime_find_vcodec(char *fourcc)
{
  	int i;

  	for(i = 0; i < total_vcodecs; i++)
  	{
    	if(quicktime_match_32(fourcc, vcodecs[i].fourcc))
      		return i;
  	}
  	return -1;
}


int quicktime_find_acodec(char *fourcc)
{
	int i;

	for(i = 0; i < total_acodecs; i++)
	{
    	if(quicktime_match_32(fourcc, acodecs[i].fourcc))
    		return i;
	}
	return -1;
}





int quicktime_register_vcodec(char *fourcc, 
	void (*init_vcodec)(quicktime_video_map_t *))
{
	int index = quicktime_find_vcodec(fourcc);

	if(index == -1)
	{
    	total_vcodecs++;
    	vcodecs = (quicktime_extern_video_t *)realloc(vcodecs,
	    	total_vcodecs * sizeof(quicktime_extern_video_t));

    	vcodecs[total_vcodecs - 1].init = init_vcodec;
    	quicktime_copy_char32(vcodecs[total_vcodecs - 1].fourcc, fourcc);
    	return total_vcodecs - 1;
	}
	return index;
}

int quicktime_register_acodec(char *fourcc, 
	void (*init_acodec)(quicktime_audio_map_t *))
{
	int index = quicktime_find_acodec(fourcc);

	if(index == -1)
	{
    	total_acodecs++;
    	acodecs = (quicktime_extern_audio_t *)realloc(acodecs,
			total_acodecs * sizeof(quicktime_extern_audio_t));
    	acodecs[total_acodecs - 1].init = init_acodec;
    	quicktime_copy_char32(acodecs[total_acodecs - 1].fourcc, fourcc);
    	return total_acodecs - 1;
	}
	return index;
}

static int select_codec(const struct dirent *ptr)
{
	if(strncmp(ptr->d_name, CODEC_PREFIX, 15) == 0)
    	return 1;
	else 
    	return 0;
}

#include "qtvorbis.h"
#include "qtmp3.h"
void quicktime_register_internal_acodec()
{
	quicktime_register_acodec(QUICKTIME_TWOS, quicktime_init_codec_twos);
	quicktime_register_acodec(QUICKTIME_RAW,  quicktime_init_codec_rawaudio);
	quicktime_register_acodec(QUICKTIME_IMA4, quicktime_init_codec_ima4); 
	quicktime_register_acodec(QUICKTIME_ULAW, quicktime_init_codec_ulaw); 

	quicktime_register_acodec(QUICKTIME_VORBIS, quicktime_init_codec_vorbis);
	quicktime_register_acodec(QUICKTIME_MP3, quicktime_init_codec_mp3);
	quicktime_register_acodec(QUICKTIME_WMX2, quicktime_init_codec_wmx2);
}

#include "raw.h"
#include "div3.h"
#include "divx.h"
#include "dv.h"
void quicktime_register_internal_vcodec()
{

	quicktime_register_vcodec(QUICKTIME_RAW, quicktime_init_codec_raw);

	quicktime_register_vcodec(QUICKTIME_DIVX, quicktime_init_codec_divx); 
	quicktime_register_vcodec(QUICKTIME_HV60, quicktime_init_codec_hv60); 
	quicktime_register_vcodec(QUICKTIME_DIV3, quicktime_init_codec_div3); 
	quicktime_register_vcodec(QUICKTIME_DIV4, quicktime_init_codec_div4); 
	quicktime_register_vcodec(QUICKTIME_DV, quicktime_init_codec_dv); 



	quicktime_register_vcodec(QUICKTIME_JPEG, quicktime_init_codec_jpeg); 
	quicktime_register_vcodec(QUICKTIME_MJPA, quicktime_init_codec_jpeg); 
	quicktime_register_vcodec(QUICKTIME_PNG, quicktime_init_codec_png); 

	quicktime_register_vcodec(QUICKTIME_YUV422, quicktime_init_codec_yuv2); 
	quicktime_register_vcodec(QUICKTIME_YUV4, quicktime_init_codec_yuv4); 
	quicktime_register_vcodec(QUICKTIME_YUV420, quicktime_init_codec_yv12); 
	quicktime_register_vcodec(QUICKTIME_YUV444_10bit, quicktime_init_codec_v410); 
	quicktime_register_vcodec(QUICKTIME_YUV444, quicktime_init_codec_v308); 
	quicktime_register_vcodec(QUICKTIME_YUVA4444, quicktime_init_codec_v408); 
}


int quicktime_register_external_vcodec(const char *codec_name)
{
	void *handle;
	int (*quicktime_codec_register)(quicktime_extern_video_t*);
	char path[1024];
	char *error;

	sprintf(path, "%s%s.so", CODEC_PREFIX, codec_name);
	fprintf(stderr, "Trying to load external codec %s\n", path);

	handle = dlopen(path, RTLD_NOW);
	if(!handle)
	{
    	fprintf(stderr, "Can't load the codec\n");
    	fprintf(stderr, "%s\n", dlerror());
    	return -1;
	}
	fprintf(stderr, "External codec %s loaded\n", path);

	quicktime_codec_register = dlsym(handle, "quicktime_codec_register");
	if((error = dlerror()) != NULL)  
	{
    	fprintf(stderr, "%s\n",error);
    	return -1;
	}

	total_vcodecs++;
	vcodecs = (quicktime_extern_video_t *)realloc(vcodecs,
		total_vcodecs*sizeof(quicktime_extern_video_t));

	if((*quicktime_codec_register)(&(vcodecs[total_vcodecs - 1]))) 
	{
    	printf("adding intermediate functions\n");

    	vcodecs[total_vcodecs - 1].codec.delete_vcodec = vcodecs[total_vcodecs - 1].delete_codec;
    	vcodecs[total_vcodecs - 1].codec.decode_video = decode_video_external;
    	vcodecs[total_vcodecs - 1].codec.encode_video = encode_video_external;

    	return total_vcodecs - 1;
	} 
	else
        return -1;
}



int quicktime_register_external_acodec(const char *codec_name)
{
	void *handle;
	int (*quicktime_codec_register)(quicktime_extern_audio_t*);
	char path[1024];
	char *error;

	sprintf(path, "%s%s.so", CODEC_PREFIX, codec_name);
	fprintf(stderr, "Trying to load external codec %s\n", path);
	handle = dlopen(path, RTLD_NOW);
	if(!handle)
	{
    	fprintf(stderr, "Can't load the codec\n");
    	fprintf(stderr, "%s\n", dlerror());
    	return -1;
	}
	fprintf(stderr, "External codec %s loaded\n", path);

	quicktime_codec_register = dlsym(handle, "quicktime_codec_register");
	if((error = dlerror()) != NULL)  
	{
    	fprintf(stderr, "%s\n",error);
    	return -1;
	}

	total_acodecs++;
	acodecs = (quicktime_extern_audio_t *)realloc(acodecs,
		total_acodecs*sizeof(quicktime_extern_audio_t));

	if((*quicktime_codec_register)(&(acodecs[total_acodecs-1])))
	{
    	printf("adding intermediate functions\n");

    	acodecs[total_acodecs - 1].codec.delete_acodec = acodecs[total_acodecs - 1].delete_codec;
    	acodecs[total_acodecs - 1].codec.decode_audio = decode_audio_external;
    	acodecs[total_acodecs - 1].codec.encode_audio = encode_audio_external;

    	return total_vcodecs - 1;
	} 
	else
    	return -1;
}


int quicktime_init_vcodec_core(int index, quicktime_video_map_t *vtrack)
{
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = vcodecs[total_vcodecs - 1].codec.delete_vcodec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = vcodecs[total_vcodecs - 1].codec.decode_video;
	((quicktime_codec_t*)vtrack->codec)->encode_video = vcodecs[total_vcodecs - 1].codec.encode_video;

	vcodecs[index].init(vtrack);
	return 0;
}

int quicktime_init_acodec_core(int index, quicktime_audio_map_t *atrack)
{
	((quicktime_codec_t*)atrack->codec)->delete_acodec = acodecs[total_acodecs - 1].codec.delete_acodec;
	((quicktime_codec_t*)atrack->codec)->decode_audio = acodecs[total_acodecs - 1].codec.decode_audio;
	((quicktime_codec_t*)atrack->codec)->encode_audio = acodecs[total_acodecs - 1].codec.encode_audio;

	acodecs[index].init(atrack);
	return 0;
}




