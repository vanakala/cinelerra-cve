#include "funcprotos.h"
#include "quicktime.h"


void quicktime_mdia_init(quicktime_mdia_t *mdia)
{
	quicktime_mdhd_init(&(mdia->mdhd));
	quicktime_hdlr_init(&(mdia->hdlr));
	quicktime_minf_init(&(mdia->minf));
}

void quicktime_mdia_init_video(quicktime_t *file, 
								quicktime_mdia_t *mdia,
								int frame_w,
								int frame_h, 
								float frame_rate,
								char *compressor)
{
//printf("quicktime_mdia_init_video 1\n");
	quicktime_mdhd_init_video(file, &(mdia->mdhd), frame_w, frame_h, frame_rate);
//printf("quicktime_mdia_init_video 1 %d %f\n", mdia->mdhd.time_scale, frame_rate);
	quicktime_minf_init_video(file, &(mdia->minf), frame_w, frame_h, mdia->mdhd.time_scale, frame_rate, compressor);
//printf("quicktime_mdia_init_video 1\n");
	quicktime_hdlr_init_video(&(mdia->hdlr));
//printf("quicktime_mdia_init_video 2\n");
}

void quicktime_mdia_init_audio(quicktime_t *file, 
							quicktime_mdia_t *mdia, 
							int channels,
							int sample_rate, 
							int bits, 
							char *compressor)
{
	quicktime_mdhd_init_audio(&(mdia->mdhd), 
		sample_rate);
	quicktime_minf_init_audio(file, 
		&(mdia->minf), 
		channels, 
		sample_rate, 
		bits, 
		compressor);
	quicktime_hdlr_init_audio(&(mdia->hdlr));
}

void quicktime_mdia_delete(quicktime_mdia_t *mdia)
{
	quicktime_mdhd_delete(&(mdia->mdhd));
	quicktime_hdlr_delete(&(mdia->hdlr));
	quicktime_minf_delete(&(mdia->minf));
}

void quicktime_mdia_dump(quicktime_mdia_t *mdia)
{
	printf("  media\n");
	quicktime_mdhd_dump(&(mdia->mdhd));
	quicktime_hdlr_dump(&(mdia->hdlr));
	quicktime_minf_dump(&(mdia->minf));
}

int quicktime_read_mdia(quicktime_t *file, quicktime_mdia_t *mdia, quicktime_atom_t *trak_atom)
{
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);
//printf("quicktime_read_mdia %llx\n", quicktime_position(file));

/* mandatory */
		if(quicktime_atom_is(&leaf_atom, "mdhd"))
			{ quicktime_read_mdhd(file, &(mdia->mdhd)); }
		else
		if(quicktime_atom_is(&leaf_atom, "hdlr"))
		{
			quicktime_read_hdlr(file, &(mdia->hdlr)); 
/* Main Actor doesn't write component name */
			quicktime_atom_skip(file, &leaf_atom);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "minf"))
			{ quicktime_read_minf(file, &(mdia->minf), &leaf_atom); }
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < trak_atom->end);

	return 0;
}

void quicktime_write_mdia(quicktime_t *file, quicktime_mdia_t *mdia)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "mdia");

	quicktime_write_mdhd(file, &(mdia->mdhd));
	quicktime_write_hdlr(file, &(mdia->hdlr));
	quicktime_write_minf(file, &(mdia->minf));

	quicktime_atom_write_footer(file, &atom);
}
