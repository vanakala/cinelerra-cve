#include "funcprotos.h"
#include "quicktime.h"


// Update during close:
// length
// samples per chunk


void quicktime_read_strh(quicktime_t *file, quicktime_atom_t *parent_atom)
{
// AVI translation:
// vids -> trak
// auds -> trak
	do
	{
		char data[4], codec[4];

		codec[0] = codec[1] = codec[2] = codec[3] = 0;
		quicktime_read_data(file, data, 4);

		if(quicktime_match_32(data, "vids"))
		{
			int denominator;
			int numerator;
			double frame_rate;
			int width = 0;
			int height = 0;
			int depth = 24;
			int frames = 0;
			quicktime_trak_t *trak = quicktime_add_trak(file);
			trak->tkhd.track_id = file->moov.mvhd.next_track_id;
			file->moov.mvhd.next_track_id++;


// Codec
			quicktime_read_data(file, 
				codec, 
				4);
// Blank
			quicktime_set_position(file, quicktime_position(file) + 12);
			denominator = quicktime_read_int32_le(file);
			numerator = quicktime_read_int32_le(file);
			if(denominator != 0)
				frame_rate = (double)numerator / denominator;
			else
				frame_rate = numerator;

// printf(__FUNCTION__ " 1 %d %d\n", numerator, denominator);
// Blank
			quicktime_set_position(file, quicktime_position(file) + 4);
			frames = quicktime_read_int32_le(file);

			quicktime_atom_skip(file, parent_atom);



// Get footer
			quicktime_atom_read_header(file, parent_atom);
			if(quicktime_atom_is(parent_atom, "strf"))
			{
				quicktime_set_position(file, quicktime_position(file) + 4);
				width = quicktime_read_int32_le(file);
				height = quicktime_read_int32_le(file);
				quicktime_read_int16_le(file);
				depth = quicktime_read_int16_le(file);
				quicktime_read_data(file, 
					codec, 
					4);
				quicktime_atom_skip(file, parent_atom);
			}



// Store in quicktime structures
			quicktime_trak_init_video(file, 
				trak, 
				width, 
				height, 
				frame_rate,
				codec);
			quicktime_mhvd_init_video(file, 
				&file->moov.mvhd, 
				frame_rate);
			trak->mdia.mdhd.duration = frames;
//			trak->mdia.mdhd.time_scale = 1;
			memcpy(trak->mdia.minf.stbl.stsd.table[0].format, codec, 4);
			trak->mdia.minf.stbl.stsd.table[0].depth = depth;
		}
		else
		if(quicktime_match_32(data, "auds"))
		{
			int bytes_per_sample;
			int sample_size = 16;
			int samples;
			int samples_per_chunk;
			int channels = 2;
			int sample_rate = 0;
			int audio_format = 0;
			int bytes_per_second;
			quicktime_trak_t *trak = quicktime_add_trak(file);

			trak->tkhd.track_id = file->moov.mvhd.next_track_id;
			file->moov.mvhd.next_track_id++;
			quicktime_read_data(file, 
				codec, 
				4);
			quicktime_set_position(file, quicktime_position(file) + 12);
			samples_per_chunk = quicktime_read_int32_le(file);
			bytes_per_second = quicktime_read_int32_le(file);
			quicktime_set_position(file, quicktime_position(file) + 4);
			samples = quicktime_read_int32_le(file);
			quicktime_set_position(file, quicktime_position(file) + 8);
// If this is 0 use constant samples_per_chunk to guess locations.
// If it isn't 0 synthesize samples per chunk table to get locations.
// Microsoft doesn't really obey this rule.
			bytes_per_sample = quicktime_read_int32_le(file);

//printf(__FUNCTION__ " 2 %d %d\n", samples_per_chunk, bytes_per_sample);
			quicktime_atom_skip(file, parent_atom);








// Get footer
			quicktime_atom_read_header(file, parent_atom);
			if(quicktime_atom_is(parent_atom, "strf"))
			{
				audio_format = quicktime_read_int16_le(file);
//printf(__FUNCTION__ " 1 %x\n", audio_format);
				quicktime_id_to_codec(codec, audio_format);
				channels = quicktime_read_int16_le(file);
				sample_rate = quicktime_read_int32_le(file);
				quicktime_set_position(file, quicktime_position(file) + 6);
				sample_size = quicktime_read_int16_le(file);
				quicktime_atom_skip(file, parent_atom);
			}



// Store in quicktime structures
			quicktime_trak_init_audio(file, 
						trak, 
						channels, 
						sample_rate, 
						sample_size, 
						codec);
// The idx1 parser should store a constant samples per chunk based on the 
// packet size if packet_size zero
// or calculate the samples per chunk based on the chunk size if packet_size 
// is nonzero.
			trak->mdia.minf.stbl.stsd.table[0].packet_size = bytes_per_sample;
			trak->mdia.minf.stbl.stsd.table[0].compression_id = audio_format;

// Synthesize stsc table for constant samples per chunk
			if(!bytes_per_sample)
			{
				trak->mdia.minf.stbl.stsc.table[0].samples = samples_per_chunk;
				trak->mdia.minf.stbl.stsc.total_entries = 1;
			}
		}
	}while(quicktime_position(file) < parent_atom->end);

}

void quicktime_write_strh(quicktime_t *file, int track)
{
	quicktime_atom_t list_atom, strh_atom, strf_atom;
	quicktime_trak_t *trak = file->moov.trak[track];
	quicktime_atom_t junk_atom;
	int i;

	quicktime_atom_write_header(file, &list_atom, "LIST");
	quicktime_write_char32(file, "strl");
	quicktime_atom_write_header(file, &strh_atom, "strh");


	if(trak->mdia.minf.is_video)
	{
		quicktime_write_char32(file, "vids");
		quicktime_write_char32(file, trak->mdia.minf.stbl.stsd.table[0].format);
        quicktime_write_int32_le(file, 0); /* flags */
        quicktime_write_int16_le(file, 0); /* priority */
        quicktime_write_int16_le(file, 0); /* language */
        quicktime_write_int32_le(file, 0); /* initial frame */

        quicktime_write_int32_le(file, 
			trak->mdia.minf.stbl.stts.table[0].sample_duration); /* framerate denominator */
        quicktime_write_int32_le(file, 
			trak->mdia.mdhd.time_scale); /* framerate numerator */

        quicktime_write_int32_le(file, 0); /* start */
		trak->length_offset = quicktime_position(file);
        quicktime_write_int32_le(file, 0); /* length: fill later */
        quicktime_write_int32_le(file, 0); /* suggested buffer size */
        quicktime_write_int32_le(file, -1); /* quality */
        quicktime_write_int32_le(file, 0); /* sample size */
        quicktime_write_int16_le(file, 0);
        quicktime_write_int16_le(file, 0);
        quicktime_write_int16_le(file, trak->tkhd.track_width);
        quicktime_write_int16_le(file, trak->tkhd.track_height);
	}
	else
	if(trak->mdia.minf.is_audio)
	{
        quicktime_write_char32(file, "auds");
        quicktime_write_int32_le(file, 0);
        quicktime_write_int32_le(file, 0); /* flags */
        quicktime_write_int16_le(file, 0); /* priority */
        quicktime_write_int16_le(file, 0); /* language */
        quicktime_write_int32_le(file, 0); /* initial frame */
		trak->samples_per_chunk_offset = quicktime_position(file);
        quicktime_write_int32_le(file, 0); /* samples per chunk */
/* sample rate * samples per chunk  if uncompressed */
/* sample rate if compressed */
        quicktime_write_int32_le(file, 0); 
        quicktime_write_int32_le(file, 0); /* start */
		trak->length_offset = quicktime_position(file);
        quicktime_write_int32_le(file, 0); /* length, XXX: filled later */
        quicktime_write_int32_le(file, 0); /* suggested buffer size */
        quicktime_write_int32_le(file, -1); /* quality */
/* sample size: 0 for compressed and number of bytes for uncompressed */
        quicktime_write_int32_le(file, 0); 
        quicktime_write_int32_le(file, 0);
        quicktime_write_int32_le(file, 0);
	}
	quicktime_atom_write_footer(file, &strh_atom);


	quicktime_atom_write_header(file, &strf_atom, "strf");

	if(trak->mdia.minf.is_video)
	{
		quicktime_write_int32_le(file, 40);  // atom size
		quicktime_write_int32_le(file, trak->tkhd.track_width);
		quicktime_write_int32_le(file, trak->tkhd.track_height);
		quicktime_write_int16_le(file, 1);  /* planes */
		quicktime_write_int16_le(file, 24); /* depth */
		quicktime_write_char32(file, 
			trak->mdia.minf.stbl.stsd.table[0].format);
		quicktime_write_int32_le(file, 
			trak->tkhd.track_width * trak->tkhd.track_height * 3);
		quicktime_write_int32_le(file, 0);
		quicktime_write_int32_le(file, 0);
		quicktime_write_int32_le(file, 0);
		quicktime_write_int32_le(file, 0);
	}
	else
	if(trak->mdia.minf.is_audio)
	{
		quicktime_write_int16_le(file, 
			quicktime_codec_to_id(trak->mdia.minf.stbl.stsd.table[0].format));
		quicktime_write_int16_le(file, 
			trak->mdia.minf.stbl.stsd.table[0].channels);
		quicktime_write_int32_le(file, 
			trak->mdia.minf.stbl.stsd.table[0].sample_rate);
		quicktime_write_int32_le(file, 256000 / 8); // bitrate in bytes
		quicktime_write_int16_le(file, 1); // block align
		quicktime_write_int16_le(file, 
			trak->mdia.minf.stbl.stsd.table[0].sample_size); // bits per sample
		quicktime_write_int16_le(file, 0);
	}

	quicktime_atom_write_footer(file, &strf_atom);

// Junk is required in Windows
	quicktime_atom_write_header(file, &junk_atom, "JUNK");
	for(i = 0; i < 0x406; i++)
		quicktime_write_int32_le(file, 0);
	quicktime_atom_write_footer(file, &junk_atom);

	quicktime_atom_write_footer(file, &list_atom);
}






