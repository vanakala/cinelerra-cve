#ifndef FUNCPROTOS_H
#define FUNCPROTOS_H

#include "graphics.h"
#include "qtprivate.h"

/* atom handling routines */
int quicktime_atom_write_header(quicktime_t *file, quicktime_atom_t *atom, char *text);
int quicktime_atom_write_header64(quicktime_t *file, quicktime_atom_t *atom, char *text);
longest quicktime_byte_position(quicktime_t *file);
// Used internally to replace ftello values
longest quicktime_ftell(quicktime_t *file);

quicktime_trak_t* quicktime_add_track(quicktime_t *file);



int quicktime_file_open(quicktime_t *file, char *path, int rd, int wr);
int quicktime_file_close(quicktime_t *file);
longest quicktime_get_file_length(quicktime_t *file);

int quicktime_read_char(quicktime_t *file);
float quicktime_read_fixed32(quicktime_t *file);
float quicktime_read_fixed16(quicktime_t *file);
longest quicktime_read_int64(quicktime_t *file);
unsigned long quicktime_read_uint32(quicktime_t *file);
long quicktime_read_int32(quicktime_t *file);
long quicktime_read_int32_le(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
longest quicktime_position(quicktime_t *file);
int quicktime_set_position(quicktime_t *file, longest position);
int quicktime_write_fixed32(quicktime_t *file, float number);
int quicktime_write_char(quicktime_t *file, char x);
int quicktime_write_int16(quicktime_t *file, int number);
int quicktime_write_int16_le(quicktime_t *file, int number);
int quicktime_write_int24(quicktime_t *file, long number);
int quicktime_write_int32(quicktime_t *file, long value);
int quicktime_write_int32_le(quicktime_t *file, long value);
int quicktime_write_int64(quicktime_t *file, longest value);
int quicktime_write_char32(quicktime_t *file, char *string);
int quicktime_write_fixed16(quicktime_t *file, float number);
int quicktime_write_data(quicktime_t *file, char *data, int size);
int quicktime_match_32(char *input, char *output);
int quicktime_match_24(char *input, char *output);
void quicktime_write_pascal(quicktime_t *file, char *data);
int quicktime_read_data(quicktime_t *file, char *data, longest size);

/* Most codecs don't specify the actual number of bits on disk in the stbl. */
/* Convert the samples to the number of bytes for reading depending on the codec. */
longest quicktime_samples_to_bytes(quicktime_trak_t *track, long samples);

/* Convert Microsoft audio id to codec */
void quicktime_id_to_codec(char *result, int id);
int quicktime_codec_to_id(char *codec);
extern void quicktime_init_codec_jpeg(quicktime_video_map_t *);
extern void quicktime_init_codec_png(quicktime_video_map_t *);
extern void quicktime_init_codec_v308(quicktime_video_map_t *);
extern void quicktime_init_codec_v408(quicktime_video_map_t *);
extern void quicktime_init_codec_v410(quicktime_video_map_t *);
extern void quicktime_init_codec_yuv2(quicktime_video_map_t *);
extern void quicktime_init_codec_yuv4(quicktime_video_map_t *);
extern void quicktime_init_codec_yv12(quicktime_video_map_t *);

extern void quicktime_init_codec_twos(quicktime_audio_map_t *);
extern void quicktime_init_codec_rawaudio(quicktime_audio_map_t *);
extern void quicktime_init_codec_ima4(quicktime_audio_map_t *); 
extern void quicktime_init_codec_ulaw(quicktime_audio_map_t *); 

/* Graphics */
quicktime_scaletable_t* quicktime_new_scaletable(int input_w, int input_h, int output_w, int output_h);


/* chunks always start on 1 */
/* samples start on 0 */

/* update the position pointers in all the tracks after a set_position */
int quicktime_update_positions(quicktime_t *file);


void quicktime_read_strh(quicktime_t *file, quicktime_atom_t *parent_atom);
void quicktime_read_hdrl(quicktime_t *file, quicktime_atom_t *parent_atom);
void quicktime_read_idx1(quicktime_t *file, quicktime_atom_t *parent_atom);
void quicktime_read_movi(quicktime_t *file, quicktime_atom_t *parent_atom);
void quicktime_write_idx1(quicktime_t *file);
void quicktime_write_movi(quicktime_t *file);
void quicktime_write_hdrl(quicktime_t *file);
void quicktime_write_strh(quicktime_t *file, int track);



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
longest quicktime_track_end(quicktime_trak_t *trak);


/* the total number of samples in the track depending on the access mode */
long quicktime_track_samples(quicktime_t *file, quicktime_trak_t *trak);


/* queries for every atom */
/* the starting sample in the given chunk */
long quicktime_sample_of_chunk(quicktime_trak_t *trak, long chunk);


/* the byte offset from mdat start of the chunk */
longest quicktime_chunk_to_offset(quicktime_t *file, quicktime_trak_t *trak, long chunk);


/* the chunk of any offset from mdat start */
long quicktime_offset_to_chunk(longest *chunk_offset, 
	quicktime_trak_t *trak, 
	longest offset);



/* total bytes between the two samples */
longest quicktime_sample_range_size(quicktime_trak_t *trak, long chunk_sample, long sample);


int quicktime_chunk_of_sample(longest *chunk_sample, 
	longest *chunk, 
	quicktime_trak_t *trak, 
	long sample);



/* converting between mdat offsets to samples */
longest quicktime_sample_to_offset(quicktime_t *file, quicktime_trak_t *trak, long sample);
long quicktime_offset_to_sample(quicktime_trak_t *trak, longest offset);
quicktime_trak_t* quicktime_add_trak(quicktime_t *file);


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
int quicktime_update_tables(quicktime_t *file, 
							quicktime_trak_t *trak, 
							longest offset, 
							longest chunk, 
							longest sample, 
							longest samples, 
							longest sample_size);
void quicktime_update_stco(quicktime_stco_t *stco, long chunk, longest offset);
void quicktime_update_stsz(quicktime_stsz_t *stsz, long sample, long sample_size);
int quicktime_update_stsc(quicktime_stsc_t *stsc, long chunk, long samples);
int quicktime_trak_duration(quicktime_trak_t *trak, long *duration, long *timescale);
int quicktime_trak_fix_counts(quicktime_t *file, quicktime_trak_t *trak);
/* number of samples in the chunk */
long quicktime_chunk_samples(quicktime_trak_t *trak, long chunk);
int quicktime_trak_shift_offsets(quicktime_trak_t *trak, longest offset);
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
int quicktime_delete_trak(quicktime_moov_t *moov);
int quicktime_get_timescale(double frame_rate);

unsigned long quicktime_current_time(void);

#endif
