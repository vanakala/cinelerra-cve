#include "funcprotos.h"
#include "quicktime.h"






void quicktime_read_hdrl(quicktime_t *file, quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);

		if(file->use_avi && quicktime_atom_is(&leaf_atom, "LIST"))
		{
// No size here.
			quicktime_set_position(file, 
				quicktime_position(file) + 4);
		}
		else
		if(file->use_avi && quicktime_atom_is(&leaf_atom, "strh"))
		{
			quicktime_read_strh(file, &leaf_atom);
		}
		else
			quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < parent_atom->end);

	quicktime_atom_skip(file, &leaf_atom);
}

void quicktime_write_hdrl(quicktime_t *file)
{
	int i;
	quicktime_atom_t list_atom, avih_atom;
	quicktime_atom_write_header(file, &list_atom, "LIST");
	quicktime_write_char32(file, "hdrl");
	quicktime_atom_write_header(file, &avih_atom, "avih");

	if(file->total_vtracks)
   		quicktime_write_int32_le(file, 
			(uint32_t)(1000000 / 
			quicktime_frame_rate(file, 0)));
    else
		quicktime_write_int32_le(file, 0);

	file->bitrate_offset = quicktime_position(file);
	quicktime_write_int32_le(file, 0); /* bitrate in bytes */
    quicktime_write_int32_le(file, 0); /* padding */
    quicktime_write_int32_le(file, 
		AVI_TRUSTCKTYPE | 
		AVI_HASINDEX | 
		AVI_MUSTUSEINDEX | 
		AVI_ISINTERLEAVED); /* flags */
	file->frames_offset = quicktime_position(file);
    quicktime_write_int32_le(file, 0); /* nb frames, filled later */
    quicktime_write_int32_le(file, 0); /* initial frame */
    quicktime_write_int32_le(file, file->moov.total_tracks); /* nb streams */
    quicktime_write_int32_le(file, 0); /* suggested buffer size */

	if(file->total_vtracks)
    {
		quicktime_write_int32_le(file, file->vtracks[0].track->tkhd.track_width);
    	quicktime_write_int32_le(file, file->vtracks[0].track->tkhd.track_height);
	}
	else
    {
		quicktime_write_int32_le(file, 0);
    	quicktime_write_int32_le(file, 0);
	}
    quicktime_write_int32_le(file, 0); /* reserved */
    quicktime_write_int32_le(file, 0); /* reserved */
    quicktime_write_int32_le(file, 0); /* reserved */
    quicktime_write_int32_le(file, 0); /* reserved */

	quicktime_atom_write_footer(file, &avih_atom);
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_write_strh(file, i);
	}

	quicktime_atom_write_footer(file, &list_atom);
}


void quicktime_finalize_hdrl(quicktime_t *file)
{
	int i;
	longest position = quicktime_position(file);
	longest total_frames = 0;
	double frame_rate = 0;

	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_trak_t *trak = file->moov.trak[i];
		if(trak->mdia.minf.is_video)
		{
			int length;
			quicktime_set_position(file, trak->length_offset);
			total_frames = length = quicktime_track_samples(file, trak);
			quicktime_write_int32_le(file, length);
			frame_rate = (double)trak->mdia.mdhd.time_scale /
				trak->mdia.minf.stbl.stts.table[0].sample_duration;
		}
		else
		if(trak->mdia.minf.is_audio)
		{
			int length, samples_per_chunk;
			quicktime_set_position(file, trak->length_offset);
			length = quicktime_track_samples(file, trak);
			quicktime_write_int32_le(file, length);
			quicktime_set_position(file, trak->samples_per_chunk_offset);

			samples_per_chunk = quicktime_avg_chunk_samples(file, trak);
			quicktime_write_int32_le(file, 
				samples_per_chunk);
			quicktime_write_int32_le(file, 
				samples_per_chunk *
				trak->mdia.minf.stbl.stsd.table[0].sample_rate);
		}
	}

	if(total_frames)
	{
		quicktime_set_position(file, file->bitrate_offset);
		quicktime_write_int32_le(file, 
			file->total_length / (total_frames / frame_rate));
//printf("%lld %lld %f\n", file->total_length, total_frames, frame_rate);
		quicktime_set_position(file, file->frames_offset);
		quicktime_write_int32_le(file, total_frames);
	}

	quicktime_set_position(file, position);
}







