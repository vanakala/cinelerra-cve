#ifndef MPEGVIDEO_H
#define MPEGVIDEO_H

/* Function prototypes */
int mpeg3video_read_frame_backend(mpeg3video_t *, int);

/* getpicture.c */
int mpeg3video_get_cbp(mpeg3_slice_t *);
int mpeg3video_get_macroblocks(mpeg3video_t *);
int mpeg3video_getinterblock(mpeg3_slice_t *, mpeg3video_t *, int comp);
int mpeg3video_getintrablock(mpeg3_slice_t *, mpeg3video_t *, int, int []);
int mpeg3video_getmpg2intrablock(mpeg3_slice_t *, mpeg3video_t *, int, int []);
int mpeg3video_getmpg2interblock(mpeg3_slice_t *, mpeg3video_t *, int);
int mpeg3video_getpicture(mpeg3video_t *);
int mpeg3video_get_macroblocks(mpeg3video_t *);
#endif
