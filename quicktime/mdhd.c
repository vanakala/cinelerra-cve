#include "funcprotos.h"
#include "quicktime.h"

void quicktime_mdhd_init(quicktime_mdhd_t *mdhd)
{
	mdhd->version = 0;
	mdhd->flags = 0;
	mdhd->creation_time = quicktime_current_time();
	mdhd->modification_time = quicktime_current_time();
	mdhd->time_scale = 0;
	mdhd->duration = 0;
	mdhd->language = 0;
	mdhd->quality = 100;
}

void quicktime_mdhd_init_video(quicktime_t *file, 
								quicktime_mdhd_t *mdhd, 
								int frame_w,
								int frame_h, 
								float frame_rate)
{
	mdhd->time_scale = quicktime_get_timescale(frame_rate);
//printf("quicktime_mdhd_init_video %d %f\n", mdhd->time_scale, frame_rate);
	mdhd->duration = 0;      /* set this when closing */
}

void quicktime_mdhd_init_audio(quicktime_mdhd_t *mdhd, 
							int sample_rate)
{
	mdhd->time_scale = sample_rate;
	mdhd->duration = 0;      /* set this when closing */
}

void quicktime_mdhd_delete(quicktime_mdhd_t *mdhd)
{
}

void quicktime_read_mdhd(quicktime_t *file, quicktime_mdhd_t *mdhd)
{
	mdhd->version = quicktime_read_char(file);
	mdhd->flags = quicktime_read_int24(file);
	mdhd->creation_time = quicktime_read_int32(file);
	mdhd->modification_time = quicktime_read_int32(file);
	mdhd->time_scale = quicktime_read_int32(file);
	mdhd->duration = quicktime_read_int32(file);
	mdhd->language = quicktime_read_int16(file);
	mdhd->quality = quicktime_read_int16(file);
}

void quicktime_mdhd_dump(quicktime_mdhd_t *mdhd)
{
	printf("   media header\n");
	printf("    version %d\n", mdhd->version);
	printf("    flags %d\n", mdhd->flags);
	printf("    creation_time %u\n", mdhd->creation_time);
	printf("    modification_time %u\n", mdhd->modification_time);
	printf("    time_scale %d\n", mdhd->time_scale);
	printf("    duration %d\n", mdhd->duration);
	printf("    language %d\n", mdhd->language);
	printf("    quality %d\n", mdhd->quality);
}

void quicktime_write_mdhd(quicktime_t *file, quicktime_mdhd_t *mdhd)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "mdhd");

	quicktime_write_char(file, mdhd->version);
	quicktime_write_int24(file, mdhd->flags);
	quicktime_write_int32(file, mdhd->creation_time);
	quicktime_write_int32(file, mdhd->modification_time);
	quicktime_write_int32(file, mdhd->time_scale);
	quicktime_write_int32(file, mdhd->duration);
	quicktime_write_int16(file, mdhd->language);
	quicktime_write_int16(file, mdhd->quality);	

	quicktime_atom_write_footer(file, &atom);
}

