#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include "funcprotos.h"
#include "quicktime.h"


static int total_vcodecs = 0;
static int total_acodecs = 0;
static quicktime_codectable_t *vcodecs = NULL; 
static quicktime_codectable_t *acodecs = NULL; 




static int register_vcodec(void (*init_vcodec)(quicktime_video_map_t *))
{
    total_vcodecs++;
    vcodecs = (quicktime_codectable_t *)realloc(vcodecs,
	    total_vcodecs * sizeof(quicktime_codectable_t));

    vcodecs[total_vcodecs - 1].init_vcodec = init_vcodec;
    return total_vcodecs - 1;
}

static int register_acodec(void (*init_acodec)(quicktime_audio_map_t *))
{
    total_acodecs++;
    acodecs = (quicktime_codectable_t *)realloc(acodecs,
		total_acodecs * sizeof(quicktime_codectable_t));
    acodecs[total_acodecs - 1].init_acodec = init_acodec;
    return total_acodecs - 1;
}




#include "ima4.h"
#include "mp4a.h"
#include "qtvorbis.h"
#include "qtmp3.h"
#include "rawaudio.h"
#include "twos.h"
#include "ulaw.h"
#include "wma.h"
#include "wmx2.h"

static void register_acodecs()
{
	register_acodec(quicktime_init_codec_twos);
	register_acodec(quicktime_init_codec_rawaudio);
	register_acodec(quicktime_init_codec_ima4);
	register_acodec(quicktime_init_codec_mp4a);
	register_acodec(quicktime_init_codec_ulaw);

	register_acodec(quicktime_init_codec_vorbis);
	register_acodec(quicktime_init_codec_mp3);
	register_acodec(quicktime_init_codec_wmx2);
	register_acodec(quicktime_init_codec_wmav1);
	register_acodec(quicktime_init_codec_wmav2);
}





#include "qth264.h"
#include "raw.h"
#include "qtdv.h"
#include "jpeg.h"
#include "mpeg4.h"
#include "qtpng.h"
#include "rle.h"
#include "v308.h"
#include "v408.h"
#include "v410.h"
#include "yuv2.h"
#include "yuv4.h"
#include "yv12.h"

static void register_vcodecs()
{

	register_vcodec(quicktime_init_codec_raw);

	register_vcodec(quicktime_init_codec_h264);
	register_vcodec(quicktime_init_codec_hv64);
	register_vcodec(quicktime_init_codec_divx);
	register_vcodec(quicktime_init_codec_hv60);
	register_vcodec(quicktime_init_codec_div5);
	register_vcodec(quicktime_init_codec_div3);
	register_vcodec(quicktime_init_codec_div3v2);
	register_vcodec(quicktime_init_codec_div3lower);
	register_vcodec(quicktime_init_codec_mp4v);
	register_vcodec(quicktime_init_codec_xvid);
	register_vcodec(quicktime_init_codec_dnxhd);
	register_vcodec(quicktime_init_codec_svq1);
	register_vcodec(quicktime_init_codec_svq3);
	register_vcodec(quicktime_init_codec_h263);
	register_vcodec(quicktime_init_codec_dv);
	register_vcodec(quicktime_init_codec_dvsd);
	register_vcodec(quicktime_init_codec_dvcp);

	register_vcodec(quicktime_init_codec_jpeg);
	register_vcodec(quicktime_init_codec_mjpa);
	register_vcodec(quicktime_init_codec_mjpg);
	register_vcodec(quicktime_init_codec_png);
	register_vcodec(quicktime_init_codec_rle);

	register_vcodec(quicktime_init_codec_yuv2);
	register_vcodec(quicktime_init_codec_2vuy);
	register_vcodec(quicktime_init_codec_yuv4);
	register_vcodec(quicktime_init_codec_yv12);
	register_vcodec(quicktime_init_codec_v410);
	register_vcodec(quicktime_init_codec_v308);
	register_vcodec(quicktime_init_codec_v408);
}




int quicktime_find_vcodec(quicktime_video_map_t *vtrack)
{
  	int i;
	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	if(!total_vcodecs) register_vcodecs();

  	for(i = 0; i < total_vcodecs; i++)
  	{
		quicktime_codectable_t *table = &vcodecs[i];
		table->init_vcodec(vtrack);
		if(quicktime_match_32(compressor, codec_base->fourcc))
		{
			return 0;
		}
		else
		{
			codec_base->delete_vcodec(vtrack);
			codec_base->priv = 0;
		}
  	}

  	return -1;
}


int quicktime_find_acodec(quicktime_audio_map_t *atrack)
{
	int i;
	char *compressor = atrack->track->mdia.minf.stbl.stsd.table[0].format;
	int compression_id = atrack->track->mdia.minf.stbl.stsd.table[0].compression_id;
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	int32_t compressor_int = *(int32_t*)compressor;
	if(!total_acodecs) register_acodecs();

	for(i = 0; i < total_acodecs; i++)
	{
		quicktime_codectable_t *table = &acodecs[i];
		table->init_acodec(atrack);

// For writing and reading Quicktime
		if(quicktime_match_32(compressor, codec_base->fourcc))
			return 0;
		else
// For reading AVI, sometimes the fourcc is 0 and the compression_id is used instead.
// Sometimes the compression_id is the fourcc.
		if((compressor[0] == 0 || compressor_int == codec_base->wav_id) && 
			codec_base->wav_id == compression_id)
			return 0;
		else
		{
			codec_base->delete_acodec(atrack);
			codec_base->priv = 0;
		}
	}

	return -1;
}


char* quicktime_acodec_title(char *fourcc)
{
	int i;
	char *result = 0;
	quicktime_audio_map_t *atrack = 
		(quicktime_audio_map_t*)calloc(1, sizeof(quicktime_audio_map_t));
	quicktime_codec_t *codec_base = 
		atrack->codec = 
		(quicktime_codec_t*)calloc(1, sizeof(quicktime_codec_t));
	int done = 0;
	if(!total_acodecs) register_acodecs();
	for(i = 0; i < total_acodecs && !done; i++)
	{
//printf("quicktime_acodec_title 1\n");
		quicktime_codectable_t *table = &acodecs[i];
//printf("quicktime_acodec_title 2\n");
		table->init_acodec(atrack);
//printf("quicktime_acodec_title 3\n");
		if(quicktime_match_32(fourcc, codec_base->fourcc))
		{
			result = codec_base->title;
			done = 1;
		}
//printf("quicktime_acodec_title 4\n");
		codec_base->delete_acodec(atrack);
//printf("quicktime_acodec_title 5\n");
	}
	free(codec_base);
	free(atrack);
	
	if(!result)
		return fourcc;
	else
		return result;
}

char* quicktime_vcodec_title(char *fourcc)
{
	int i;
	char *result = 0;

	quicktime_video_map_t *vtrack = 
		(quicktime_video_map_t*)calloc(1, sizeof(quicktime_video_map_t));
	quicktime_codec_t *codec_base = 
		vtrack->codec = 
		(quicktime_codec_t*)calloc(1, sizeof(quicktime_codec_t));
	int done = 0;


	if(!total_vcodecs) register_vcodecs();
	for(i = 0; i < total_vcodecs && !done; i++)
	{
		quicktime_codectable_t *table = &vcodecs[i];
		table->init_vcodec(vtrack);
		if(quicktime_match_32(fourcc, codec_base->fourcc))
		{
			result = codec_base->title;
			done = 1;
		}
		codec_base->delete_vcodec(vtrack);
	}



	free(codec_base);
	free(vtrack);

	if(!result)
		return fourcc;
	else
		return result;
}








