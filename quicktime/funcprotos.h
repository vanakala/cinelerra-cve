#ifndef FUNCPROTOS_H
#define FUNCPROTOS_H

#include "graphics.h"
#include "qtprivate.h"

/* atom handling routines */
int quicktime_atom_write_header(quicktime_t *file, quicktime_atom_t *atom, char *text);
int quicktime_atom_write_header64(quicktime_t *file, quicktime_atom_t *atom, char *text);
int64_t quicktime_byte_position(quicktime_t *file);
// Used internally to replace ftello values
int64_t quicktime_ftell(quicktime_t *file);

quicktime_trak_t* quicktime_add_track(quicktime_t *file);



int quicktime_file_open(quicktime_t *file, char *path, int rd, int wr);
int quicktime_file_close(quicktime_t *file);
int64_t quicktime_get_file_length(char *path);

/* Initialize audio_map_t and video_map_t objects after loading headers */
/* Called by quicktime_read_info */
void quicktime_init_maps(quicktime_t *file);

int quicktime_read_char(quicktime_t *file);
float quicktime_read_fixed32(quicktime_t *file);
float quicktime_read_fixed16(quicktime_t *file);
int64_t quicktime_read_int64(quicktime_t *file);
int64_t quicktime_read_int64_le(quicktime_t *file);
unsigned long quicktime_read_uint32(quicktime_t *file);
long quicktime_read_int32(quicktime_t *file);
long quicktime_read_int32_le(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int64_t quicktime_position(quicktime_t *file);
int quicktime_set_position(quicktime_t *file, int64_t position);
int quicktime_write_fixed32(quicktime_t *file, float number);
int quicktime_write_char(quicktime_t *file, char x);
int quicktime_write_int16(quicktime_t *file, int number);
int quicktime_write_int16_le(quicktime_t *file, int number);
int quicktime_write_int24(quicktime_t *file, long number);
int quicktime_write_int32(quicktime_t *file, long value);
int quicktime_write_int32_le(quicktime_t *file, long value);
int quicktime_write_int64(quicktime_t *file, int64_t value);
int quicktime_write_int64_le(quicktime_t *file, int64_t value);
int quicktime_write_char32(quicktime_t *file, char *string);
int quicktime_write_fixed16(quicktime_t *file, float number);

/* Returns number of bytes written */
int quicktime_write_data(quicktime_t *file, char *data, int size);
/* Returns 1 if equal or 0 if different */
int quicktime_match_32(char *input, char *output);
int quicktime_match_24(char *input, char *output);
void quicktime_write_pascal(quicktime_t *file, char *data);
int quicktime_read_data(quicktime_t *file, char *data, int64_t size);

/* Most codecs don't specify the actual number of bits on disk in the stbl. */
/* Convert the samples to the number of bytes for reading depending on the codec. */
int64_t quicktime_samples_to_bytes(quicktime_trak_t *track, long samples);

/* Convert Microsoft audio id to codec */
void quicktime_id_to_codec(char *result, int id);

int quicktime_find_vcodec(quicktime_video_map_t *vtrack);
int quicktime_find_acodec(quicktime_audio_map_t *atrack);







/* Graphics */
quicktime_scaletable_t* quicktime_new_scaletable(int input_w, int input_h, int output_w, int output_h);


/* chunks always start on 1 */
/* samples start on 0 */

/* update the position pointers in all the tracks after a set_position */
int quicktime_update_positions(quicktime_t *file);







/* AVI section */
void quicktime_read_strh(quicktime_t *file, quicktime_atom_t *parent_atom);
void quicktime_write_strh(quicktime_t *file, int track);




/* Create new strl object for reading or writing */
quicktime_strl_t* quicktime_new_strl();
/* Initialize new strl object for writing */
void quicktime_init_strl(quicktime_t *file, 
	quicktime_audio_map_t *atrack,
	quicktime_video_map_t *vtrack,
	quicktime_trak_t *trak,
	quicktime_strl_t *strl);
void quicktime_delete_strl(quicktime_strl_t *strl);
/* Read strl object */
void quicktime_read_strl(quicktime_t *file, 
	quicktime_strl_t *strl, 
	quicktime_atom_t *parent_atom);


void quicktime_delete_indx(quicktime_indx_t *indx);
void quicktime_init_indx(quicktime_t *file, 
	quicktime_indx_t *indx, 
	quicktime_strl_t *strl);
void quicktime_update_indx(quicktime_t *file, 
	quicktime_indx_t *indx, 
	quicktime_ix_t *ix);
void quicktime_finalize_indx(quicktime_t *file);
/* Called by quicktime_read_strl */
void quicktime_read_indx(quicktime_t *file, 
	quicktime_strl_t *strl, 
	quicktime_atom_t *parent_atom);



void quicktime_delete_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_read_hdrl(quicktime_t *file, 
	quicktime_hdrl_t *hdrl,
	quicktime_atom_t *parent_atom);
void quicktime_init_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_finalize_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_read_esds(quicktime_t *file, 
	quicktime_atom_t *parent_atom, 
	quicktime_stsd_table_t *table);






/* Read movi and create objects */
void quicktime_delete_movi(quicktime_t *file, quicktime_movi_t *movi);
void quicktime_read_movi(quicktime_t *file, 
	quicktime_atom_t *parent_atom,
	quicktime_movi_t *movi);
/* Write preliminary movi data and create objects */
void quicktime_init_movi(quicktime_t *file, quicktime_riff_t *riff);
void quicktime_finalize_movi(quicktime_t *file, quicktime_movi_t *movi);






void quicktime_delete_idx1(quicktime_idx1_t *idx1);
void quicktime_read_idx1(quicktime_t *file, 
	quicktime_riff_t *riff, 
	quicktime_atom_t *parent_atom);
void quicktime_write_idx1(quicktime_t *file, 
	quicktime_idx1_t *idx1);
/* Set the keyframe flag in the right IDX1 entry.  new_keyframes starts on 0 */
/* Used by quicktime_insert_keyframe */
void quicktime_set_idx1_keyframe(quicktime_t *file, 
	quicktime_trak_t *trak,
	int new_keyframe);
/* Create new IDX1 entry.  Used by quicktime_write_chunk_footer */
void quicktime_update_idx1table(quicktime_t *file, 
	quicktime_trak_t *trak, 
	int offset,
	int size);




void quicktime_read_odml(quicktime_t *file, quicktime_atom_t *parent_atom);
void quicktime_finalize_odml(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_init_odml(quicktime_t *file, quicktime_hdrl_t *hdrl);








/* Create new riff and put on riff table */
quicktime_riff_t* quicktime_new_riff(quicktime_t *file);
void quicktime_delete_riff(quicktime_t *file, quicktime_riff_t *riff);
void quicktime_read_riff(quicktime_t *file, quicktime_atom_t *parent_atom);

/* Write riff header and initialize structures */
/* Only the first riff has a hdrl object */
void quicktime_init_riff(quicktime_t *file);
void quicktime_finalize_riff(quicktime_t *file, quicktime_riff_t *riff);


/* Create ix object for writing only */
quicktime_ix_t* quicktime_new_ix(quicktime_t *file, 
	quicktime_trak_t *trak,
	quicktime_strl_t *strl);
void quicktime_delete_ix(quicktime_ix_t *ix);
void quicktime_update_ixtable(quicktime_t *file, 
	quicktime_trak_t *trak, 
	int64_t offset,
	int size);
void quicktime_write_ix(quicktime_t *file,
	quicktime_ix_t *ix,
	int track);
/* Read entire ix atom from current position in file */
/* Called by quicktime_read_indx. */
void quicktime_read_ix(quicktime_t *file,
	quicktime_ix_t *ix);



/* compression_id is for AVI reading */
int quicktime_trak_init_audio(quicktime_t *file, 
							quicktime_trak_t *trak, 
							int channels, 
							int sample_rate, 
							int bits, 
							char *compressor);
int quicktime_trak_init_video(quicktime_t *file, 
							quicktime_trak_t *trak, 
							int frame_w, 
							int frame_h, 
							float frame_rate,
							char *compressor);
int quicktime_trak_delete(quicktime_trak_t *trak);
int quicktime_trak_dump(quicktime_trak_t *trak);
int quicktime_delete_trak(quicktime_moov_t *moov);
int quicktime_read_trak(quicktime_t *file, quicktime_trak_t *trak, quicktime_atom_t *trak_atom);
int quicktime_write_trak(quicktime_t *file, quicktime_trak_t *trak, long moov_time_scale);
int64_t quicktime_track_end(quicktime_trak_t *trak);


/* the total number of samples in the track depending on the access mode */
long quicktime_track_samples(quicktime_t *file, quicktime_trak_t *trak);


/* queries for every atom */
/* the starting sample in the given chunk */
long quicktime_sample_of_chunk(quicktime_trak_t *trak, long chunk);


/* the byte offset from mdat start of the chunk.  Chunk is numbered from 1 */
int64_t quicktime_chunk_to_offset(quicktime_t *file, quicktime_trak_t *trak, long chunk);


/* the chunk of any offset from mdat start */
long quicktime_offset_to_chunk(int64_t *chunk_offset, 
	quicktime_trak_t *trak, 
	int64_t offset);

/* Bytes in the chunk.  Chunk is numbered from 1 */
/* Only available in AVI. */
/* Reads the chunk header to get the size. */
/* Stores the chunk offset in chunk_offset. */
/* Returns the bytes in the chunk less the 8 byte header size */
int quicktime_chunk_bytes(quicktime_t *file, 
	int64_t *chunk_offset,
	int chunk, 
	quicktime_trak_t *trak);


/* total bytes between the two samples */
int64_t quicktime_sample_range_size(quicktime_trak_t *trak, long chunk_sample, long sample);


int quicktime_chunk_of_sample(int64_t *chunk_sample, 
	int64_t *chunk, 
	quicktime_trak_t *trak, 
	long sample);



/* converting between mdat offsets to samples */
int64_t quicktime_sample_to_offset(quicktime_t *file, quicktime_trak_t *trak, long sample);
long quicktime_offset_to_sample(quicktime_trak_t *trak, int64_t offset);
quicktime_trak_t* quicktime_add_trak(quicktime_t *file);


int quicktime_time_to_sample(quicktime_stts_t *stts,
	int64_t *start_time);


void quicktime_write_chunk_header(quicktime_t *file, 
	quicktime_trak_t *trak, 
	quicktime_atom_t *chunk);
void quicktime_write_chunk_footer(quicktime_t *file, 
	quicktime_trak_t *trak,
	int current_chunk,
	quicktime_atom_t *chunk, 
	int samples);
/* update all the tables after writing a buffer */
/* set sample_size to 0 if no sample size should be set */
/*
 * int quicktime_update_tables(quicktime_t *file, 
 * 							quicktime_trak_t *trak, 
 * 							int64_t offset, 
 * 							int64_t chunk, 
 * 							int64_t sample, 
 * 							int64_t samples, 
 * 							int64_t sample_size);
 */
void quicktime_update_stco(quicktime_stco_t *stco, long chunk, int64_t offset);
void quicktime_update_stsz(quicktime_stsz_t *stsz, long sample, long sample_size);
int quicktime_update_stsc(quicktime_stsc_t *stsc, long chunk, long samples);
int quicktime_trak_duration(quicktime_trak_t *trak, long *duration, long *timescale);
int quicktime_trak_fix_counts(quicktime_t *file, quicktime_trak_t *trak);
int quicktime_sample_size(quicktime_trak_t *trak, int sample);

/* number of samples in the chunk */
long quicktime_chunk_samples(quicktime_trak_t *trak, long chunk);
int quicktime_trak_shift_offsets(quicktime_trak_t *trak, int64_t offset);
void quicktime_mhvd_init_video(quicktime_t *file, 
	quicktime_mvhd_t *mvhd, 
	double frame_rate);


void quicktime_stsd_init_video(quicktime_t *file, 
								quicktime_stsd_t *stsd, 
								int frame_w,
								int frame_h, 
								float frame_rate,
								char *compression);
void quicktime_stsd_init_audio(quicktime_t *file, 
							quicktime_stsd_t *stsd, 
							int channels,
							int sample_rate, 
							int bits, 
							char *compressor);
void quicktime_stts_init_video(quicktime_t *file, 
							quicktime_stts_t *stts, 
							int time_scale, 
							float frame_rate);
void quicktime_stbl_init_video(quicktime_t *file, 
								quicktime_stbl_t *stbl, 
								int frame_w,
								int frame_h, 
								int time_scale, 
								float frame_rate,
								char *compressor);
void quicktime_stbl_init_audio(quicktime_t *file, 
							quicktime_stbl_t *stbl, 
							int channels, 
							int sample_rate, 
							int bits, 
							char *compressor);
void quicktime_minf_init_video(quicktime_t *file, 
								quicktime_minf_t *minf, 
								int frame_w,
								int frame_h, 
								int time_scale, 
								float frame_rate,
								char *compressor);
void quicktime_mdhd_init_video(quicktime_t *file, 
								quicktime_mdhd_t *mdhd, 
								int frame_w,
								int frame_h, 
								float frame_rate);
void quicktime_mdhd_init_audio(quicktime_mdhd_t *mdhd, 
							int sample_rate);
void quicktime_minf_init_audio(quicktime_t *file, 
							quicktime_minf_t *minf, 
							int channels, 
							int sample_rate, 
							int bits, 
							char *compressor);
void quicktime_mdia_init_video(quicktime_t *file, 
								quicktime_mdia_t *mdia,
								int frame_w,
								int frame_h, 
								float frame_rate,
								char *compressor);
void quicktime_mdia_init_audio(quicktime_t *file, 
							quicktime_mdia_t *mdia, 
							int channels,
							int sample_rate, 
							int bits, 
							char *compressor);
void quicktime_tkhd_init_video(quicktime_t *file, 
								quicktime_tkhd_t *tkhd, 
								int frame_w, 
								int frame_h);
int quicktime_get_timescale(double frame_rate);

unsigned long quicktime_current_time(void);





// Utility functions for vbr audio codecs

// Delete buffers from the vbr structure when finished
void quicktime_clear_vbr(quicktime_vbr_t  *ptr);
void quicktime_vbr_set_channels(quicktime_vbr_t *ptr, int channels);
void quicktime_init_vbr(quicktime_vbr_t *ptr, int channels);

// Shift the output buffer and fix pointers for the given range.
// Return 1 if not possible to handle the sample count.
int quicktime_align_vbr(quicktime_audio_map_t *atrack, 
	int samples);
// The current endpoint of the buffer
int64_t quicktime_vbr_end(quicktime_vbr_t  *ptr);
unsigned char* quicktime_vbr_input(quicktime_vbr_t *ptr);
int quicktime_vbr_input_size(quicktime_vbr_t *ptr);
// Read the next sample/frame of compressed data and advance the counters.
int quicktime_read_vbr(quicktime_t *file,
	quicktime_audio_map_t *atrack);
// Put the next sample/frame of uncompressed data in the buffer and advance the
// counters
void quicktime_store_vbr_float(quicktime_audio_map_t *atrack,
	float *samples,
	int sample_count);

void quicktime_copy_vbr_float(quicktime_vbr_t *vbr,
	int64_t start_position, 
	int samples,
	float *output, 
	int channel);


#endif
