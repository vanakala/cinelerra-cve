#include "funcprotos.h"
#include "quicktime.h"


// Update during close:
// length
// samples per chunk
#define JUNK_SIZE 0x1018



quicktime_strl_t* quicktime_new_strl()
{
	quicktime_strl_t *strl = calloc(1, sizeof(quicktime_strl_t));
	return strl;
}


void quicktime_init_strl(quicktime_t *file, 
	quicktime_audio_map_t *atrack,
	quicktime_video_map_t *vtrack,
	quicktime_trak_t *trak,
	quicktime_strl_t *strl)
{
	quicktime_atom_t list_atom, strh_atom, strf_atom;
	quicktime_atom_t junk_atom;
	int i;

/* Construct tag */
	if(vtrack)
	{
		strl->tag[0] = '0' + (trak->tkhd.track_id - 1) / 10;
		strl->tag[1] = '0' + (trak->tkhd.track_id - 1) % 10;
		strl->tag[2] = 'd';
		strl->tag[3] = 'c';
	}
	else
	if(atrack)
	{
		strl->tag[0] = '0' + (trak->tkhd.track_id - 1) / 10;
		strl->tag[1] = '0' + (trak->tkhd.track_id - 1) % 10;
		strl->tag[2] = 'w';
		strl->tag[3] = 'b';
	}


/* LIST 'strl' */
	quicktime_atom_write_header(file, &list_atom, "LIST");
	quicktime_write_char32(file, "strl");

/* 'strh' */
	quicktime_atom_write_header(file, &strh_atom, "strh");



/* vids */
	if(vtrack)
	{
		quicktime_write_char32(file, "vids");
		quicktime_write_char32(file, 
			trak->mdia.minf.stbl.stsd.table[0].format);
/* flags */
        quicktime_write_int32_le(file, 0);
/* priority */
        quicktime_write_int16_le(file, 0);
/* language */
        quicktime_write_int16_le(file, 0);
/* initial frame */
        quicktime_write_int32_le(file, 0);

/* framerate denominator */
        quicktime_write_int32_le(file, 
			trak->mdia.minf.stbl.stts.table[0].sample_duration);
/* framerate numerator */
        quicktime_write_int32_le(file, 
			trak->mdia.mdhd.time_scale);

/* start */
        quicktime_write_int32_le(file, 0);
		strl->length_offset = quicktime_position(file);
/* length: fill later */
        quicktime_write_int32_le(file, 0); 
/* suggested buffer size */
        quicktime_write_int32_le(file, 0); 
/* quality */
        quicktime_write_int32_le(file, -1); 
/* sample size */
        quicktime_write_int32_le(file, 0); 
        quicktime_write_int16_le(file, 0);
        quicktime_write_int16_le(file, 0);
        quicktime_write_int16_le(file, trak->tkhd.track_width);
        quicktime_write_int16_le(file, trak->tkhd.track_height);
	}
	else
/* auds */
	if(atrack)
	{
        quicktime_write_char32(file, "auds");
        quicktime_write_int32_le(file, 0);
/* flags */
        quicktime_write_int32_le(file, 0);
/* priority */
        quicktime_write_int16_le(file, 0);
/* language */
        quicktime_write_int16_le(file, 0);
/* initial frame */
        quicktime_write_int32_le(file, 0);
		strl->samples_per_chunk_offset = quicktime_position(file);
/* samples per chunk */
        quicktime_write_int32_le(file, 0);
/* sample rate * samples per chunk  if uncompressed */
/* sample rate if compressed */
        quicktime_write_int32_le(file, 0);
/* start */
        quicktime_write_int32_le(file, 0);
		strl->length_offset = quicktime_position(file);
/* length, XXX: filled later */
        quicktime_write_int32_le(file, 0);
/* suggested buffer size */
        quicktime_write_int32_le(file, 0);
/* quality */
        quicktime_write_int32_le(file, -1);
/* sample size: 0 for compressed and number of bytes for uncompressed */
		strl->sample_size_offset = quicktime_position(file);
        quicktime_write_int32_le(file, 0);
        quicktime_write_int32_le(file, 0);
        quicktime_write_int32_le(file, 0);
	}
	quicktime_atom_write_footer(file, &strh_atom);







/* strf */
	quicktime_atom_write_header(file, &strf_atom, "strf");

	if(vtrack)
	{
/* atom size repeated */
		quicktime_write_int32_le(file, 40);  
		quicktime_write_int32_le(file, trak->tkhd.track_width);
		quicktime_write_int32_le(file, trak->tkhd.track_height);
/* planes */
		quicktime_write_int16_le(file, 1);  
/* depth */
		quicktime_write_int16_le(file, 24); 
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
	if(atrack)
	{
/* By now the codec is instantiated so the WAV ID is available. */
		int wav_id = 0x0;
		quicktime_codec_t *codec_base = atrack->codec;

		if(codec_base->wav_id)
			wav_id = codec_base->wav_id;
		quicktime_write_int16_le(file, 
			wav_id);
		quicktime_write_int16_le(file, 
			trak->mdia.minf.stbl.stsd.table[0].channels);
		quicktime_write_int32_le(file, 
			trak->mdia.minf.stbl.stsd.table[0].sample_rate);
/* bitrate in bytes */
		quicktime_write_int32_le(file, 256000 / 8); 
/* block align */
		quicktime_write_int16_le(file, 1); 
/* bits per sample */
		quicktime_write_int16_le(file, 
			trak->mdia.minf.stbl.stsd.table[0].sample_size); 
		quicktime_write_int16_le(file, 0);
	}

	quicktime_atom_write_footer(file, &strf_atom);




/* Junk is required in Windows. */
/* In Heroine Kernel it's padding for the super index */
	strl->indx_offset = quicktime_position(file);
	strl->padding_size = JUNK_SIZE;



	quicktime_atom_write_header(file, &junk_atom, "JUNK");
	for(i = 0; i < JUNK_SIZE; i += 4)
		quicktime_write_int32_le(file, 0);
	quicktime_atom_write_footer(file, &junk_atom);


/* Initialize super index */
	quicktime_init_indx(file, &strl->indx, strl);


	quicktime_atom_write_footer(file, &list_atom);
}



void quicktime_delete_strl(quicktime_strl_t *strl)
{
	quicktime_delete_indx(&strl->indx);
	free(strl);
}

void quicktime_read_strl(quicktime_t *file, 
	quicktime_strl_t *strl, 
	quicktime_atom_t *parent_atom)
{
// These are 0 if no track is currently being processed.
// Set to 1 if audio or video track is being processed.
	char data[4], codec[4];
	int denominator;
	int numerator;
	double frame_rate;
	int width;
	int height;
	int depth;
	int frames;
	int bytes_per_sample;
	int sample_size;
	int samples;
	int samples_per_chunk;
	int channels;
	int sample_rate;
	int compression_id;
	int bytes_per_second;
	quicktime_trak_t *trak = 0;
	quicktime_riff_t *first_riff = file->riff[0];


	codec[0] = codec[1] = codec[2] = codec[3] = 0;

/* AVI translation: */
/* vids -> trak */
/* auds -> trak */
/* Only one track is in each strl object */
	do
	{
		quicktime_atom_t leaf_atom;
		quicktime_atom_read_header(file, &leaf_atom);

// strh
		if(quicktime_atom_is(&leaf_atom, "strh"))
		{
// stream type
			quicktime_read_data(file, data, 4);

			if(quicktime_match_32(data, "vids"))
			{
				trak = quicktime_add_trak(file);
				width = 0;
				height = 0;
				depth = 24;
				frames = 0;
				strl->is_video = 1;


				trak->tkhd.track_id = file->moov.mvhd.next_track_id;
				file->moov.mvhd.next_track_id++;


/* Codec */
				quicktime_read_data(file, 
					codec, 
					4);
//printf("quicktime_read_strl 1 %c%c%c%c\n", codec[0], codec[1], codec[2], codec[3]);
/* Blank */
				quicktime_set_position(file, quicktime_position(file) + 12);
				denominator = quicktime_read_int32_le(file);
				numerator = quicktime_read_int32_le(file);
				if(denominator != 0)
					frame_rate = (double)numerator / denominator;
				else
					frame_rate = numerator;

/* Blank */
				quicktime_set_position(file, quicktime_position(file) + 4);
				frames = quicktime_read_int32_le(file);
			}
			else
			if(quicktime_match_32(data, "auds"))
			{
				trak = quicktime_add_trak(file);
				sample_size = 16;
				channels = 2;
				sample_rate = 0;
				compression_id = 0;
				strl->is_audio = 1;

				trak->tkhd.track_id = file->moov.mvhd.next_track_id;
				file->moov.mvhd.next_track_id++;
				quicktime_read_data(file, 
					codec, 
					4);
//printf("quicktime_read_strl 2 %c%c%c%c\n", codec[0], codec[1], codec[2], codec[3]);
				quicktime_set_position(file, quicktime_position(file) + 12);
				samples_per_chunk = quicktime_read_int32_le(file);
				bytes_per_second = quicktime_read_int32_le(file);
/* start */
				quicktime_set_position(file, quicktime_position(file) + 4);
/* length of track */
				samples = quicktime_read_int32_le(file);
/* suggested buffer size, quality */
				quicktime_set_position(file, quicktime_position(file) + 8);

// If this is 0 use constant samples_per_chunk to guess locations.
// If it isn't 0 synthesize samples per chunk table to get locations.
// McRowesoft doesn't really obey this rule so we may need to base it on codec ID.
				bytes_per_sample = quicktime_read_int32_le(file);
//printf("quicktime_read_strl 20 %d\n", samples_per_chunk);
			}
		}
// strf
		else
		if(quicktime_atom_is(&leaf_atom, "strf"))
		{
			if(strl->is_video)
			{
/* atom size repeated */
				quicktime_read_int32_le(file);
				width = quicktime_read_int32_le(file);
				height = quicktime_read_int32_le(file);
/* Panes */
				quicktime_read_int16_le(file);
/* Depth in bits */
				depth = quicktime_read_int16_le(file);
				quicktime_read_data(file, 
					codec, 
					4);
			}
			else
			if(strl->is_audio)
			{
				compression_id = quicktime_read_int16_le(file);
				channels = quicktime_read_int16_le(file);
				sample_rate = quicktime_read_int32_le(file);
				quicktime_set_position(file, quicktime_position(file) + 6);
				sample_size = quicktime_read_int16_le(file);
//printf("quicktime_read_strl 40 %d %d %d\n", channels, sample_rate, sample_size);
			}
		}
		else
// Super index.
// Read the super index + all the partial indexes now
		if(quicktime_atom_is(&leaf_atom, "indx"))
		{
//printf("quicktime_read_strl 50\n");
			quicktime_read_indx(file, strl, &leaf_atom);
			strl->have_indx = 1;
		}

//printf("quicktime_read_strl 60\n");


// Next object
		quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < parent_atom->end);
//printf("quicktime_read_strl 70 %d %d\n", strl->is_audio, strl->is_video);


	if(strl->is_video)
	{
/* Generate quicktime structures */
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
	if(strl->is_audio)
	{
/* Generate quicktime structures */
//printf("quicktime_read_strl 70 %d\n", sample_size);
		quicktime_trak_init_audio(file, 
					trak, 
					channels, 
					sample_rate, 
					sample_size, 
					codec);


// We store a constant samples per chunk based on the 
// packet size if sample_size zero
// and calculate the samples per chunk based on the chunk size if sample_size 
// is nonzero.
//		trak->mdia.minf.stbl.stsd.table[0].sample_size = bytes_per_sample;
		trak->mdia.minf.stbl.stsd.table[0].compression_id = compression_id;

/* Synthesize stsc table for constant samples per chunk */
		if(!bytes_per_sample)
		{
/* Should be enough entries allocated in quicktime_stsc_init_table */
			trak->mdia.minf.stbl.stsc.table[0].samples = samples_per_chunk;
			trak->mdia.minf.stbl.stsc.total_entries = 1;
		}
	}


//printf("quicktime_read_strl 100\n");
}




