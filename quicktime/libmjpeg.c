/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */
 
 #include <stdio.h>
#include <stdlib.h>
#include "colormodels.h"
#include "libmjpeg.h"

/* JPEG MARKERS */
#define   M_SOF0    0xc0
#define   M_SOF1    0xc1
#define   M_SOF2    0xc2
#define   M_SOF3    0xc3
#define   M_SOF5    0xc5
#define   M_SOF6    0xc6
#define   M_SOF7    0xc7
#define   M_JPG     0xc8
#define   M_SOF9    0xc9
#define   M_SOF10   0xca
#define   M_SOF11   0xcb
#define   M_SOF13   0xcd
#define   M_SOF14   0xce
#define   M_SOF15   0xcf
#define   M_DHT     0xc4
#define   M_DAC     0xcc
#define   M_RST0    0xd0
#define   M_RST1    0xd1
#define   M_RST2    0xd2
#define   M_RST3    0xd3
#define   M_RST4    0xd4
#define   M_RST5    0xd5
#define   M_RST6    0xd6
#define   M_RST7    0xd7
#define   M_SOI     0xd8
#define   M_EOI     0xd9
#define   M_SOS     0xda
#define   M_DQT     0xdb
#define   M_DNL     0xdc
#define   M_DRI     0xdd
#define   M_DHP     0xde
#define   M_EXP     0xdf
#define   M_APP0    0xe0
#define   M_APP1    0xe1
#define   M_APP2    0xe2
#define   M_APP3    0xe3
#define   M_APP4    0xe4
#define   M_APP5    0xe5
#define   M_APP6    0xe6
#define   M_APP7    0xe7
#define   M_APP8    0xe8
#define   M_APP9    0xe9
#define   M_APP10   0xea
#define   M_APP11   0xeb
#define   M_APP12   0xec
#define   M_APP13   0xed
#define   M_APP14   0xee
#define   M_APP15   0xef
#define   M_JPG0    0xf0
#define   M_JPG13   0xfd
#define   M_COM     0xfe
#define   M_TEM     0x01
#define   M_ERROR   0x100

#define QUICKTIME_MARKER_SIZE 0x2c
#define AVI_MARKER_SIZE 0x12
#define QUICKTIME_JPEG_TAG 0x6d6a7067
#define QUICKTIME_AVI_TAG 0x41564931


METHODDEF(void) mjpeg_error_exit (j_common_ptr cinfo)
{
/* cinfo->err really points to a mjpeg_error_mgr struct, so coerce pointer */
  	mjpeg_error_ptr mjpegerr = (mjpeg_error_ptr) cinfo->err;

/* Always display the message. */
/* We could postpone this until after returning, if we chose. */
  	(*cinfo->err->output_message) (cinfo);

/* Return control to the setjmp point */
  	longjmp(mjpegerr->setjmp_buffer, 1);
}

typedef struct 
{
	struct jpeg_destination_mgr pub; /* public fields */

	JOCTET *buffer;		/* Pointer to buffer */
	mjpeg_compressor *engine;
} mjpeg_destination_mgr;

typedef mjpeg_destination_mgr *mjpeg_dest_ptr;


/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

METHODDEF(void) init_destination(j_compress_ptr cinfo)
{
  	mjpeg_dest_ptr dest = (mjpeg_dest_ptr)cinfo->dest;

/* Set the pointer to the preallocated buffer */
	if(!dest->engine->output_buffer)
	{
		dest->engine->output_buffer = calloc(1, 65536);
		dest->engine->output_allocated = 65536;
	}
  	dest->buffer = dest->engine->output_buffer;
  	dest->pub.next_output_byte = dest->engine->output_buffer;
  	dest->pub.free_in_buffer = dest->engine->output_allocated;
}

/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).  The
 * application should resume compression after it has made more room in the
 * output buffer.  Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_output_byte & free_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */

METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo)
{
/* Allocate a bigger buffer. */
	mjpeg_dest_ptr dest = (mjpeg_dest_ptr)cinfo->dest;

	dest->engine->output_size = dest->engine->output_allocated;
	dest->engine->output_allocated *= 2;
	dest->engine->output_buffer = realloc(dest->engine->output_buffer, 
		dest->engine->output_allocated);
	dest->buffer = dest->engine->output_buffer;
	dest->pub.next_output_byte = dest->buffer + dest->engine->output_size;
	dest->pub.free_in_buffer = dest->engine->output_allocated - dest->engine->output_size;

	return TRUE;
}

/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
METHODDEF(void) term_destination(j_compress_ptr cinfo)
{
/* Just get the length */
	mjpeg_dest_ptr dest = (mjpeg_dest_ptr)cinfo->dest;
	dest->engine->output_size = dest->engine->output_allocated - dest->pub.free_in_buffer;
}

GLOBAL(void) jpeg_buffer_dest(j_compress_ptr cinfo, mjpeg_compressor *engine)
{
  	mjpeg_dest_ptr dest;

/* The destination object is made permanent so that multiple JPEG images
 * can be written to the same file without re-executing jpeg_stdio_dest.
 * This makes it dangerous to use this manager and a different destination
 * manager serially with the same JPEG object, because their private object
 * sizes may be different.  Caveat programmer.
 */
	if(cinfo->dest == NULL) 
	{	
/* first time for this JPEG object? */
      	cinfo->dest = (struct jpeg_destination_mgr *)
    		(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, 
				JPOOL_PERMANENT,
				sizeof(mjpeg_destination_mgr));
	}

	dest = (mjpeg_dest_ptr)cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->engine = engine;
}














typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */

	JOCTET * buffer;		/* start of buffer */
	int bytes;             /* total size of buffer */
} mjpeg_source_mgr;

typedef mjpeg_source_mgr* mjpeg_src_ptr;

METHODDEF(void) init_source(j_decompress_ptr cinfo)
{
    mjpeg_src_ptr src = (mjpeg_src_ptr) cinfo->src;
}

METHODDEF(boolean) fill_input_buffer(j_decompress_ptr cinfo)
{
	mjpeg_src_ptr src = (mjpeg_src_ptr) cinfo->src;

	src->buffer[0] = (JOCTET)0xFF;
	src->buffer[1] = (JOCTET)M_EOI;
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = 2;

	return TRUE;
}


METHODDEF(void) skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	mjpeg_src_ptr src = (mjpeg_src_ptr)cinfo->src;

	src->pub.next_input_byte += (size_t)num_bytes;
	src->pub.bytes_in_buffer -= (size_t)num_bytes;
}


METHODDEF(void) term_source(j_decompress_ptr cinfo)
{
}

GLOBAL(void) jpeg_buffer_src(j_decompress_ptr cinfo, unsigned char *buffer, long bytes)
{
	mjpeg_src_ptr src;

/* first time for this JPEG object? */
	if(cinfo->src == NULL)
	{	
      	cinfo->src = (struct jpeg_source_mgr*)
    		(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, 
            		JPOOL_PERMANENT,
					sizeof(mjpeg_source_mgr));
      	src = (mjpeg_src_ptr)cinfo->src;
	}

	src = (mjpeg_src_ptr)cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->pub.bytes_in_buffer = bytes;
	src->pub.next_input_byte = buffer;
	src->buffer = buffer;
	src->bytes = bytes;
}
















static void reset_buffer(unsigned char **buffer, long *size, long *allocated)
{
	*size = 0;
}

static void delete_buffer(unsigned char **buffer, long *size, long *allocated)
{
	if(*buffer)
	{
		free(*buffer);
		*size = 0;
		*allocated = 0;
	}
}

static void append_buffer(unsigned char **buffer, 
	long *size, 
	long *allocated,
	unsigned char *data,
	long data_size)
{
	if(!*buffer)
	{
		*buffer = calloc(1, 65536);
		*size = 0;
		*allocated = 65536;
	}

	if(*size + data_size + 0x100 > *allocated)
	{
		*allocated = *size + data_size + 0x100;
		*buffer = realloc(*buffer, *allocated);
	}

	memcpy(*buffer + *size, data, data_size);
	*size += data_size;
}

static void allocate_temps(mjpeg_t *mjpeg)
{
	int i;

	if(!mjpeg->temp_data)
    {
        switch(mjpeg->jpeg_color_model)
        {
            case BC_YUV422P:
	            mjpeg->temp_data = calloc(1, mjpeg->coded_w * mjpeg->coded_h * 2);
	            mjpeg->temp_rows[0] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h);
	            mjpeg->temp_rows[1] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h);
	            mjpeg->temp_rows[2] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h);
	            for(i = 0; i < mjpeg->coded_h; i++)
	            {
		            mjpeg->temp_rows[0][i] = mjpeg->temp_data + i * mjpeg->coded_w;
		            mjpeg->temp_rows[1][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + i * mjpeg->coded_w / 2;
		            mjpeg->temp_rows[2][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + mjpeg->coded_w / 2 * mjpeg->coded_h + i * mjpeg->coded_w / 2;
	            }
    	        break;

            case BC_YUV444P:
	            mjpeg->temp_data = calloc(1, mjpeg->coded_w * mjpeg->coded_h * 3);
	            mjpeg->temp_rows[0] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h);
	            mjpeg->temp_rows[1] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h);
	            mjpeg->temp_rows[2] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h);
	            for(i = 0; i < mjpeg->coded_h; i++)
	            {
		            mjpeg->temp_rows[0][i] = mjpeg->temp_data + i * mjpeg->coded_w;
		            mjpeg->temp_rows[1][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + i * mjpeg->coded_w;
		            mjpeg->temp_rows[2][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + mjpeg->coded_w * mjpeg->coded_h + i * mjpeg->coded_w;
	            }
    	        break;

            case BC_YUV420P:
	            mjpeg->temp_data = calloc(1, mjpeg->coded_w * mjpeg->coded_h + mjpeg->coded_w * mjpeg->coded_h / 2);
	            mjpeg->temp_rows[0] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h);
	            mjpeg->temp_rows[1] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h / 2);
	            mjpeg->temp_rows[2] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h / 2);
	            for(i = 0; i < mjpeg->coded_h; i++)
	            {
		            mjpeg->temp_rows[0][i] = mjpeg->temp_data + i * mjpeg->coded_w;
		            if(i < mjpeg->coded_h / 2)
		            {
			            mjpeg->temp_rows[1][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + i * (mjpeg->coded_w / 2);
			            mjpeg->temp_rows[2][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + (mjpeg->coded_h / 2) * (mjpeg->coded_w / 2) + i * (mjpeg->coded_w / 2);
		            }
	            }
                break;
        }
    }
}

static int get_input_row(mjpeg_t *mjpeg, mjpeg_compressor *compressor, int i)
{
	int input_row;
	if(mjpeg->fields > 1) 
		input_row = i * 2 + compressor->instance;
	else
		input_row = i;
	if(input_row >= mjpeg->coded_h) input_row = mjpeg->coded_h - 1;
	return input_row;
}

// Get pointers to rows for the JPEG compressor
static void get_rows(mjpeg_t *mjpeg, mjpeg_compressor *compressor)
{
	int i;
    switch(mjpeg->jpeg_color_model)
    {
		case BC_YUV444P:
		{
			if(!compressor->rows[0])
			{
				compressor->rows[0] = calloc(1, sizeof(unsigned char*) * compressor->coded_field_h);
				compressor->rows[1] = calloc(1, sizeof(unsigned char*) * compressor->coded_field_h);
				compressor->rows[2] = calloc(1, sizeof(unsigned char*) * compressor->coded_field_h);
			}

// User colormodel matches jpeg colormodel
			if(mjpeg->color_model == BC_YUV444P &&
				mjpeg->output_w == mjpeg->coded_w &&
				mjpeg->output_h == mjpeg->coded_h)
			{
				for(i = 0; i < compressor->coded_field_h; i++)
				{
					int input_row = get_input_row(mjpeg, compressor, i);
					compressor->rows[0][i] = mjpeg->y_argument + 
						mjpeg->coded_w * input_row;
					compressor->rows[1][i] = mjpeg->u_argument + 
						mjpeg->coded_w * input_row;
					compressor->rows[2][i] = mjpeg->v_argument + 
						mjpeg->coded_w * input_row;
				}
			}
			else
			{
				for(i = 0; i < compressor->coded_field_h; i++)
				{
					int input_row = get_input_row(mjpeg, compressor, i);
					compressor->rows[0][i] = mjpeg->temp_rows[0][input_row];
					compressor->rows[1][i] = mjpeg->temp_rows[1][input_row];
					compressor->rows[2][i] = mjpeg->temp_rows[2][input_row];
				}
			}
			break;
		}

		case BC_YUV422P:
		{
			if(!compressor->rows[0])
			{
				compressor->rows[0] = calloc(1, sizeof(unsigned char*) * compressor->coded_field_h);
				compressor->rows[1] = calloc(1, sizeof(unsigned char*) * compressor->coded_field_h);
				compressor->rows[2] = calloc(1, sizeof(unsigned char*) * compressor->coded_field_h);
			}

	// User colormodel matches jpeg colormodel
			if(mjpeg->color_model == BC_YUV422P &&
				mjpeg->output_w == mjpeg->coded_w &&
				mjpeg->output_h == mjpeg->coded_h)
			{
				for(i = 0; i < compressor->coded_field_h; i++)
				{
					int input_row = get_input_row(mjpeg, compressor, i);
					compressor->rows[0][i] = mjpeg->y_argument + 
						mjpeg->coded_w * input_row;
					compressor->rows[1][i] = mjpeg->u_argument + 
						(mjpeg->coded_w / 2) * input_row;
					compressor->rows[2][i] = mjpeg->v_argument + 
						(mjpeg->coded_w / 2) * input_row;
				}
			}
			else
			{
				for(i = 0; i < compressor->coded_field_h; i++)
				{
					int input_row = get_input_row(mjpeg, compressor, i);
					compressor->rows[0][i] = mjpeg->temp_rows[0][input_row];
					compressor->rows[1][i] = mjpeg->temp_rows[1][input_row];
					compressor->rows[2][i] = mjpeg->temp_rows[2][input_row];
				}
			}
			break;
		}

		case BC_YUV420P:
		{
			if(!compressor->rows[0])
			{
				compressor->rows[0] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h);
				compressor->rows[1] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h / 2);
				compressor->rows[2] = calloc(1, sizeof(unsigned char*) * mjpeg->coded_h / 2);
			}

// User colormodel matches jpeg colormodel
			if(mjpeg->color_model == BC_YUV420P &&
				mjpeg->output_w == mjpeg->coded_w &&
				mjpeg->output_h == mjpeg->coded_h)
			{
				for(i = 0; i < compressor->coded_field_h; i++)
				{
					int input_row = get_input_row(mjpeg, compressor, i);
					compressor->rows[0][i] = mjpeg->y_argument + 
						mjpeg->coded_w * input_row;
                	if(i < compressor->coded_field_h / 2)
                	{
				    	compressor->rows[1][i] = mjpeg->u_argument + 
					    	(mjpeg->coded_w / 2) * input_row;
				    	compressor->rows[2][i] = mjpeg->v_argument + 
					    	(mjpeg->coded_w / 2) * input_row;
                	}
				}
			}
			else
			{
				for(i = 0; i < compressor->coded_field_h; i++)
				{
					int input_row = get_input_row(mjpeg, compressor, i);
					compressor->rows[0][i] = mjpeg->temp_rows[0][input_row];
                	if(i < compressor->coded_field_h / 2)
                	{
				    	compressor->rows[1][i] = mjpeg->temp_rows[1][input_row];
				    	compressor->rows[2][i] = mjpeg->temp_rows[2][input_row];
                	}
				}
			}
			break;
		}
	}
}

static void delete_rows(mjpeg_compressor *compressor)
{
	if(compressor->rows[0])
	{
		free(compressor->rows[0]);
		free(compressor->rows[1]);
		free(compressor->rows[2]);
	}
}


static void new_jpeg_objects(mjpeg_compressor *engine)
{
	engine->jpeg_decompress.err = jpeg_std_error(&(engine->jpeg_error.pub));
	engine->jpeg_error.pub.error_exit = mjpeg_error_exit;
/* Ideally the error handler would be set here but it must be called in a thread */
	jpeg_create_decompress(&(engine->jpeg_decompress));
	engine->jpeg_decompress.raw_data_out = TRUE;
	engine->jpeg_decompress.dct_method = JDCT_IFAST;
}

static void delete_jpeg_objects(mjpeg_compressor *engine)
{
	jpeg_destroy_decompress(&(engine->jpeg_decompress));
}



static void unlock_compress_loop(mjpeg_compressor *engine)
{
	pthread_mutex_unlock(&(engine->input_lock));
}

static void lock_compress_loop(mjpeg_compressor *engine)
{
	pthread_mutex_lock(&(engine->output_lock));
}

// Make temp rows for compressor
static void get_mcu_rows(mjpeg_t *mjpeg, 
	mjpeg_compressor *engine,
	int start_row)
{
	int i, j, scanline;
	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 16; j++)
		{
			if(i > 0 && j >= 8 && mjpeg->jpeg_color_model == BC_YUV420P) break;

			scanline = start_row;
			if(i > 0 && mjpeg->jpeg_color_model == BC_YUV420P) scanline /= 2;
			scanline += j;
			if(scanline >= engine->coded_field_h) scanline = engine->coded_field_h - 1;
			engine->mcu_rows[i][j] = engine->rows[i][scanline];
		}
	}
}


static void decompress_field(mjpeg_compressor *engine)
{
	mjpeg_t *mjpeg = engine->mjpeg;
	long buffer_offset = engine->instance * mjpeg->input_field2;
	unsigned char *buffer = mjpeg->input_data + buffer_offset;
	long buffer_size;
	int i, j;

//printf("decompress_field %02x%02x %d\n", buffer[0], buffer[1], engine->instance * mjpeg->input_field2);
	if(engine->instance == 0 && mjpeg->fields > 1)
		buffer_size = mjpeg->input_field2 - buffer_offset;
	else
		buffer_size = mjpeg->input_size - buffer_offset;

	mjpeg->error = 0;

	if(setjmp(engine->jpeg_error.setjmp_buffer))
	{
/* If we get here, the JPEG code has signaled an error. */
		delete_jpeg_objects(engine);
		new_jpeg_objects(engine);
		mjpeg->error = 1;
//printf("decompress_field 1\n");
		goto finish;
	}

//printf("decompress_field 2\n");
	jpeg_buffer_src(&engine->jpeg_decompress, 
		buffer, 
		buffer_size);
	jpeg_read_header(&engine->jpeg_decompress, TRUE);

// Reset by jpeg_read_header
	engine->jpeg_decompress.raw_data_out = TRUE;
	jpeg_start_decompress(&engine->jpeg_decompress);

// Generate colormodel from jpeg sampling
	if(engine->jpeg_decompress.comp_info[0].v_samp_factor == 2 &&
		engine->jpeg_decompress.comp_info[0].h_samp_factor == 2)
    	mjpeg->jpeg_color_model = BC_YUV420P;
    else
	if(engine->jpeg_decompress.comp_info[0].v_samp_factor == 1 &&
		engine->jpeg_decompress.comp_info[0].h_samp_factor == 2)
    	mjpeg->jpeg_color_model = BC_YUV422P;
	else
		mjpeg->jpeg_color_model = BC_YUV444P;

// Must be here because the color model isn't known until now
	pthread_mutex_lock(&(mjpeg->decompress_init));
	allocate_temps(mjpeg);
	pthread_mutex_unlock(&(mjpeg->decompress_init));
	get_rows(mjpeg, engine);

//printf("decompress_field 3 %d\n", engine->jpeg_decompress.output_scanline);

	while(engine->jpeg_decompress.output_scanline < engine->jpeg_decompress.output_height)
	{
		get_mcu_rows(mjpeg, engine, engine->jpeg_decompress.output_scanline);
		jpeg_read_raw_data(&engine->jpeg_decompress, 
			engine->mcu_rows, 
			engine->coded_field_h);
	}
	jpeg_finish_decompress(&engine->jpeg_decompress);


finish:
	;
}

void mjpeg_decompress_loop(mjpeg_compressor *engine)
{
	while(!engine->done)
	{
		pthread_mutex_lock(&engine->input_lock);
		if(!engine->done)
		{
			decompress_field(engine);
		}
		pthread_mutex_unlock(&(engine->output_lock));
	}
}


static void compress_field(mjpeg_compressor *engine)
{
	int i, j;
	mjpeg_t *mjpeg = engine->mjpeg;

//printf("compress_field 1\n");
	get_rows(engine->mjpeg, engine);
	reset_buffer(&engine->output_buffer, &engine->output_size, &engine->output_allocated);
	jpeg_buffer_dest(&engine->jpeg_compress, engine);


	engine->jpeg_compress.raw_data_in = TRUE;
	jpeg_start_compress(&engine->jpeg_compress, TRUE);

	while(engine->jpeg_compress.next_scanline < engine->jpeg_compress.image_height)
	{
		get_mcu_rows(mjpeg, engine, engine->jpeg_compress.next_scanline);

		jpeg_write_raw_data(&engine->jpeg_compress, 
			engine->mcu_rows, 
			engine->coded_field_h);
	}
	jpeg_finish_compress(&engine->jpeg_compress);
//printf("compress_field 2\n");
}


void mjpeg_compress_loop(mjpeg_compressor *engine)
{
	while(!engine->done)
	{
		pthread_mutex_lock(&engine->input_lock);
		if(!engine->done)
		{
			compress_field(engine);
		}
		pthread_mutex_unlock(&engine->output_lock);
	}
}

static void delete_temps(mjpeg_t *mjpeg)
{
	if(mjpeg->temp_data)
    {
	    free(mjpeg->temp_data);
	    free(mjpeg->temp_rows[0]);
	    free(mjpeg->temp_rows[1]);
	    free(mjpeg->temp_rows[2]);
	}
}

mjpeg_compressor* mjpeg_new_decompressor(mjpeg_t *mjpeg, int instance)
{
	mjpeg_compressor *result = calloc(1, sizeof(mjpeg_compressor));
	pthread_attr_t  attr;
	struct sched_param param;
	pthread_mutexattr_t mutex_attr;
	int i;

	result->mjpeg = mjpeg;
	result->instance = instance;
	new_jpeg_objects(result);
	result->field_h = mjpeg->output_h / mjpeg->fields;
	result->coded_field_h = (result->field_h % 16) ? 
		result->field_h + (16 - (result->field_h % 16)) : result->field_h;

	result->mcu_rows[0] = malloc(16 * sizeof(unsigned char*));
	result->mcu_rows[1] = malloc(16 * sizeof(unsigned char*));
	result->mcu_rows[2] = malloc(16 * sizeof(unsigned char*));

	pthread_mutexattr_init(&mutex_attr);
//	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	pthread_mutex_init(&(result->input_lock), &mutex_attr);
	pthread_mutex_lock(&(result->input_lock));
	pthread_mutex_init(&(result->output_lock), &mutex_attr);
	pthread_mutex_lock(&(result->output_lock));

	pthread_attr_init(&attr);
	pthread_create(&(result->tid), &attr, (void*)mjpeg_decompress_loop, result);

	return result;
}

void mjpeg_delete_decompressor(mjpeg_compressor *engine)
{
	engine->done = 1;
	pthread_mutex_unlock(&(engine->input_lock));
	pthread_join(engine->tid, 0);
	pthread_mutex_destroy(&(engine->input_lock));
	pthread_mutex_destroy(&(engine->output_lock));
	jpeg_destroy_decompress(&(engine->jpeg_decompress));
	delete_rows(engine);
	free(engine->mcu_rows[0]);
	free(engine->mcu_rows[1]);
	free(engine->mcu_rows[2]);
	free(engine);
}

mjpeg_compressor* mjpeg_new_compressor(mjpeg_t *mjpeg, int instance)
{
	pthread_attr_t  attr;
	struct sched_param param;
	pthread_mutexattr_t mutex_attr;
	mjpeg_compressor *result = calloc(1, sizeof(mjpeg_compressor));

	result->field_h = mjpeg->output_h / mjpeg->fields;
	result->coded_field_h = (result->field_h % 16) ? 
		result->field_h + (16 - (result->field_h % 16)) : result->field_h;
	result->mjpeg = mjpeg;
	result->instance = instance;
	result->jpeg_compress.err = jpeg_std_error(&(result->jpeg_error.pub));
	jpeg_create_compress(&(result->jpeg_compress));
	result->jpeg_compress.image_width = mjpeg->output_w;
	result->jpeg_compress.image_height = result->field_h;
	result->jpeg_compress.input_components = 3;
	result->jpeg_compress.in_color_space = JCS_RGB;
	jpeg_set_defaults(&(result->jpeg_compress));
	result->jpeg_compress.input_components = 3;
	result->jpeg_compress.in_color_space = JCS_RGB;
	jpeg_set_quality(&(result->jpeg_compress), mjpeg->quality, 0);

	if(mjpeg->use_float) 
		result->jpeg_compress.dct_method = JDCT_FLOAT;
	else
		result->jpeg_compress.dct_method = JDCT_IFAST;
//		result->jpeg_compress.dct_method = JDCT_ISLOW;

/* Fix sampling */
	switch(mjpeg->fields)
    {
    	case 1:
        	mjpeg->jpeg_color_model = BC_YUV420P;
		    result->jpeg_compress.comp_info[0].h_samp_factor = 2;
		    result->jpeg_compress.comp_info[0].v_samp_factor = 2;
		    result->jpeg_compress.comp_info[1].h_samp_factor = 1;
		    result->jpeg_compress.comp_info[1].v_samp_factor = 1;
		    result->jpeg_compress.comp_info[2].h_samp_factor = 1;
		    result->jpeg_compress.comp_info[2].v_samp_factor = 1;
        	break;
        case 2:
        	mjpeg->jpeg_color_model = BC_YUV422P;
		    result->jpeg_compress.comp_info[0].h_samp_factor = 2;
		    result->jpeg_compress.comp_info[0].v_samp_factor = 1;
		    result->jpeg_compress.comp_info[1].h_samp_factor = 1;
		    result->jpeg_compress.comp_info[1].v_samp_factor = 1;
		    result->jpeg_compress.comp_info[2].h_samp_factor = 1;
		    result->jpeg_compress.comp_info[2].v_samp_factor = 1;
        	break;
    }
    allocate_temps(mjpeg);

	result->mcu_rows[0] = malloc(16 * sizeof(unsigned char*));
	result->mcu_rows[1] = malloc(16 * sizeof(unsigned char*));
	result->mcu_rows[2] = malloc(16 * sizeof(unsigned char*));

	pthread_mutexattr_init(&mutex_attr);
//	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	pthread_mutex_init(&(result->input_lock), &mutex_attr);
	pthread_mutex_lock(&(result->input_lock));
	pthread_mutex_init(&(result->output_lock), &mutex_attr);
	pthread_mutex_lock(&(result->output_lock));

	pthread_attr_init(&attr);
	pthread_create(&(result->tid), &attr, (void*)mjpeg_compress_loop, result);
	return result;
}


void mjpeg_delete_compressor(mjpeg_compressor *engine)
{
	engine->done = 1;
	pthread_mutex_unlock(&(engine->input_lock));
	pthread_join(engine->tid, 0);
	pthread_mutex_destroy(&(engine->input_lock));
	pthread_mutex_destroy(&(engine->output_lock));
	jpeg_destroy((j_common_ptr)&(engine->jpeg_compress));
	if(engine->output_buffer) free(engine->output_buffer);
	delete_rows(engine);
	free(engine->mcu_rows[0]);
	free(engine->mcu_rows[1]);
	free(engine->mcu_rows[2]);
	free(engine);
}

unsigned char* mjpeg_output_buffer(mjpeg_t *mjpeg)
{
	return mjpeg->output_data;
}

long mjpeg_output_field2(mjpeg_t *mjpeg)
{
	return mjpeg->output_field2;
}

long mjpeg_output_size(mjpeg_t *mjpeg)
{
	return mjpeg->output_size;
}

long mjpeg_output_allocated(mjpeg_t *mjpeg)
{
	return mjpeg->output_allocated;
}

void mjpeg_set_output_size(mjpeg_t *mjpeg, long output_size)
{
	mjpeg->output_size = output_size;
}


int mjpeg_compress(mjpeg_t *mjpeg, 
	unsigned char **row_pointers, 
	unsigned char *y_plane, 
	unsigned char *u_plane, 
	unsigned char *v_plane,
	int color_model,
	int cpus)
{
	int i, result = 0;
	int corrected_fields = mjpeg->fields;
	mjpeg->color_model = color_model;
	mjpeg->cpus = cpus;

//printf("mjpeg_compress 1 %d\n", color_model);
/* Reset output buffer */
	reset_buffer(&mjpeg->output_data, 
		&mjpeg->output_size, 
		&mjpeg->output_allocated);

/* Create compression engines as needed */
	for(i = 0; i < mjpeg->fields; i++)
	{
		if(!mjpeg->compressors[i])
		{
			mjpeg->compressors[i] = mjpeg_new_compressor(mjpeg, i);
		}
	}

/* Arm YUV buffers */
	mjpeg->row_argument = row_pointers;
	mjpeg->y_argument = y_plane;
	mjpeg->u_argument = u_plane;
	mjpeg->v_argument = v_plane;
// User colormodel doesn't match encoder colormodel
// Copy to interlacing buffer first
	if(mjpeg->color_model != mjpeg->jpeg_color_model || 
		mjpeg->output_w != mjpeg->coded_w ||
		mjpeg->output_h != mjpeg->coded_h)
	{
/*
 * printf("mjpeg_compress %d %d %d %d\n", 
 * mjpeg->output_w, mjpeg->output_h, mjpeg->coded_w, mjpeg->coded_h);
 */
		cmodel_transfer(0, 
			row_pointers,
			mjpeg->temp_rows[0][0],
			mjpeg->temp_rows[1][0],
			mjpeg->temp_rows[2][0],
			y_plane,
			u_plane,
			v_plane,
			0, 
			0, 
			mjpeg->output_w, 
			mjpeg->output_h,
			0, 
			0, 
			mjpeg->output_w, 
			mjpeg->output_h,
			mjpeg->color_model, 
			mjpeg->jpeg_color_model,
			0,
			mjpeg->output_w,
			mjpeg->coded_w);
	}

/* Start the compressors on the image fields */
	if(mjpeg->deinterlace) corrected_fields = 1;
	for(i = 0; i < corrected_fields && !result; i++)
	{
		unlock_compress_loop(mjpeg->compressors[i]);

		if(mjpeg->cpus < 2 && i < corrected_fields - 1)
		{
			lock_compress_loop(mjpeg->compressors[i]);
		}
	}

/* Wait for the compressors and store in master output */
	for(i = 0; i < corrected_fields && !result; i++)
	{
		if(mjpeg->cpus > 1 || i == corrected_fields - 1)
		{
			lock_compress_loop(mjpeg->compressors[i]);
		}

		append_buffer(&mjpeg->output_data, 
			&mjpeg->output_size, 
			&mjpeg->output_allocated,
			mjpeg->compressors[i]->output_buffer, 
			mjpeg->compressors[i]->output_size);
		if(i == 0) mjpeg->output_field2 = mjpeg->output_size;
	}

	if(corrected_fields < mjpeg->fields)
	{
		append_buffer(&mjpeg->output_data, 
			&mjpeg->output_size, 
			&mjpeg->output_allocated,
			mjpeg->compressors[0]->output_buffer, 
			mjpeg->compressors[0]->output_size);
	}
//printf("mjpeg_compress 2\n");
	return 0;
}



int mjpeg_decompress(mjpeg_t *mjpeg, 
	unsigned char *buffer, 
	long buffer_len,
	long input_field2,  
	unsigned char **row_pointers, 
	unsigned char *y_plane, 
	unsigned char *u_plane, 
	unsigned char *v_plane,
	int color_model,
	int cpus)
{
	int i, result = 0;
	int got_first_thread = 0;

//printf("mjpeg_decompress 1 %d %d\n", buffer_len, input_field2);
	if(buffer_len == 0) return 1;
	if(input_field2 == 0 && mjpeg->fields > 1) return 1;

//printf("mjpeg_decompress 2\n");
/* Create decompression engines as needed */
	for(i = 0; i < mjpeg->fields; i++)
	{
		if(!mjpeg->decompressors[i])
		{
			mjpeg->decompressors[i] = mjpeg_new_decompressor(mjpeg, i);
		}
	}

//printf("mjpeg_decompress 3\n");
/* Arm YUV buffers */
	mjpeg->row_argument = row_pointers;
	mjpeg->y_argument = y_plane;
	mjpeg->u_argument = u_plane;
	mjpeg->v_argument = v_plane;
	mjpeg->input_data = buffer;
	mjpeg->input_size = buffer_len;
	mjpeg->input_field2 = input_field2;
	mjpeg->color_model = color_model;
	mjpeg->cpus = cpus;

//printf("mjpeg_decompress 4 %02x %02x %d %02x %02x\n", buffer[0], buffer[1], input_field2, buffer[input_field2], buffer[input_field2 + 1]);
/* Start decompressors */
	for(i = 0; i < mjpeg->fields && !result; i++)
	{
		unlock_compress_loop(mjpeg->decompressors[i]);

// For dual CPUs, don't want second thread to start until temp data is allocated by the first.
// For single CPUs, don't want two threads running simultaneously
		if(mjpeg->cpus < 2 || !mjpeg->temp_data
			/* && i < mjpeg->fields - 1 && !mjpeg->temp_data */)
		{
			lock_compress_loop(mjpeg->decompressors[i]);
			if(i == 0) got_first_thread = 1;
		}
	}

//printf("mjpeg_decompress 5\n");
/* Wait for decompressors */
	for(i = 0; i < mjpeg->fields && !result; i++)
	{
		if(mjpeg->cpus > 1)
		{
			if(i > 0 || !got_first_thread)
				lock_compress_loop(mjpeg->decompressors[i]);
		}
	}
//printf("mjpeg_decompress 6\n");

/* Convert colormodel */
// User colormodel didn't match decompressor
/*
 * 	if(!mjpeg->error &&
 * 		(mjpeg->jpeg_color_model != mjpeg->color_model ||
 * 		mjpeg->coded_w != mjpeg->output_w ||
 * 		mjpeg->coded_h != mjpeg->output_h))
 */

 	if((mjpeg->jpeg_color_model != mjpeg->color_model ||
 		mjpeg->coded_w != mjpeg->output_w ||
 		mjpeg->coded_h != mjpeg->output_h) 
		&&
		(mjpeg->temp_data || 
		!mjpeg->error))
	{
		cmodel_transfer(row_pointers, 
			0,
			y_plane,
			u_plane,
			v_plane,
			mjpeg->temp_rows[0][0],
			mjpeg->temp_rows[1][0],
			mjpeg->temp_rows[2][0],
			0, 
			0, 
			mjpeg->output_w, 
			mjpeg->output_h,
			0, 
			0, 
			mjpeg->output_w, 
			mjpeg->output_h,
			mjpeg->jpeg_color_model,
			mjpeg->color_model, 
			0,
			mjpeg->coded_w,
			mjpeg->rowspan ? mjpeg->rowspan : mjpeg->output_w);
	}
//printf("mjpeg_decompress 100\n");
	return 0;
}


void mjpeg_set_deinterlace(mjpeg_t *mjpeg, int value)
{
	mjpeg->deinterlace = value;
}

void mjpeg_set_quality(mjpeg_t *mjpeg, int quality)
{
	mjpeg->quality = quality;
}

void mjpeg_set_float(mjpeg_t *mjpeg, int use_float)
{
	mjpeg->use_float = use_float;
}

void mjpeg_set_cpus(mjpeg_t *mjpeg, int cpus)
{
	mjpeg->cpus = cpus;
}

void mjpeg_set_rowspan(mjpeg_t *mjpeg, int rowspan)
{
	mjpeg->rowspan = rowspan;
}

int mjpeg_get_fields(mjpeg_t *mjpeg)
{
	return mjpeg->fields;
}


mjpeg_t* mjpeg_new(int w, 
	int h, 
	int fields)
{
	mjpeg_t *result = calloc(1, sizeof(mjpeg_t));
	pthread_mutexattr_t mutex_attr;
	int i;

	result->output_w = w;
	result->output_h = h;
	result->fields = fields;
	result->color_model = BC_RGB888;
	result->cpus = 1;
	result->quality = 80;
	result->use_float = 0;

	pthread_mutexattr_init(&mutex_attr);
//	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	pthread_mutex_init(&(result->decompress_init), &mutex_attr);
	

// Calculate coded dimensions
// An interlaced frame with 4:2:0 sampling must be a multiple of 32

	result->coded_w = (w % 16) ? w + (16 - (w % 16)) : w;

	if(fields == 1)
		result->coded_h = (h % 16) ? h + (16 - (h % 16)) : h;
	else
		result->coded_h = (h % 32) ? h + (32 - (h % 32)) : h;

	

//printf("mjpeg_new %d %d %d %d\n", result->output_w, result->output_h, result->coded_w, result->coded_h);
	return result;
}




void mjpeg_delete(mjpeg_t *mjpeg)
{
	int i;
//printf("mjpeg_delete 1\n");
	for(i = 0; i < mjpeg->fields; i++)
	{
//printf("mjpeg_delete 2\n");
		if(mjpeg->compressors[i]) mjpeg_delete_compressor(mjpeg->compressors[i]);
//printf("mjpeg_delete 3\n");
		if(mjpeg->decompressors[i]) mjpeg_delete_decompressor(mjpeg->decompressors[i]);
//printf("mjpeg_delete 4\n");
	}
//printf("mjpeg_delete 5\n");
	delete_temps(mjpeg);
//printf("mjpeg_delete 6\n");
	delete_buffer(&mjpeg->output_data, &mjpeg->output_size, &mjpeg->output_allocated);
//printf("mjpeg_delete 7\n");
	free(mjpeg);
//printf("mjpeg_delete 2\n");
}


/* Open up a space to insert a marker */
static void insert_space(unsigned char **buffer, 
	long *buffer_size, 
	long *buffer_allocated,
	long space_start,
	long space_len)
{
	int in, out;
// Make sure enough space is available
	if(*buffer_allocated - *buffer_size < space_len)
	{
		*buffer_allocated += space_len;
		*buffer = realloc(*buffer, *buffer_allocated);
	}

// Shift data back
	for(in = *buffer_size - 1, out = *buffer_size - 1 + space_len;
		in >= space_start;
		in--, out--)
	{
		(*buffer)[out] = (*buffer)[in];
	}
	*buffer_size += space_len;
}


static inline int nextbyte(unsigned char *data, long *offset, long length)
{
	if(length - *offset < 1) return 0;
	*offset += 1;
	return (unsigned char)data[*offset - 1];
}

static inline int read_int32(unsigned char *data, long *offset, long length)
{
	if(length - *offset < 4)
	{
		*offset = length;
		return 0;
	}
	*offset += 4;
	return ((((unsigned int)data[*offset - 4]) << 24) | 
		(((unsigned int)data[*offset - 3]) << 16) | 
		(((unsigned int)data[*offset - 2]) << 8) | 
		(((unsigned int)data[*offset - 1])));
}

static inline int read_int16(unsigned char *data, long *offset, long length)
{
	if(length - *offset < 2)	
	{
		*offset = length;
		return 0;
	}

	*offset += 2;
	return ((((unsigned int)data[*offset - 2]) << 8) | 
		(((unsigned int)data[*offset - 1])));
}

static inline unsigned char read_char(unsigned char *data, long *offset, long length)
{
	if(length - *offset < 1)	
	{
		*offset = length;
		return 0;
	}

	*offset += 1;
	return (unsigned char)data[*offset - 1];
}

static inline int next_int16(unsigned char *data, long *offset, long length)
{
	if(length - *offset < 2)	
	{
		return 0;
	}

	return ((((unsigned int)data[*offset]) << 8) | 
		(((unsigned int)data[*offset + 1])));
}

static inline void write_int32(unsigned char *data, long *offset, long length, unsigned int value)
{
	if(length - *offset < 4)
	{
		*offset = length;
		return;
	}


	data[(*offset)++] = (unsigned int)(value & 0xff000000) >> 24;
	data[(*offset)++] = (unsigned int)(value & 0xff0000) >> 16;
	data[(*offset)++] = (unsigned int)(value & 0xff00) >> 8;
	data[(*offset)++] = (unsigned char)(value & 0xff);
	return;
}

static inline void write_char(unsigned char *data, long *offset, long length, unsigned char value)
{
	if(length - *offset < 1)
	{
		*offset = length;
		return;
	}

	data[(*offset)++] = value;
	return;
}

static int next_marker(unsigned char *buffer, long *offset, long buffer_size)
{
	int c, done = 0;  /* 1 - completion    2 - error */

	while(*offset < buffer_size - 1)
	{
		if(buffer[*offset] == 0xff && buffer[*offset + 1] != 0xff)
		{
			(*offset) += 2;
			return buffer[*offset - 1];
		}
		
		(*offset)++;
	}

	return 0;

}

/* Find the next marker after offset and return 0 on success */
static int find_marker(unsigned char *buffer, 
	long *offset, 
	long buffer_size,
	unsigned long marker_type)
{
	long result = 0;
	long marker_len;

	while(!result && *offset < buffer_size - 1)
	{
		int marker = next_marker(buffer, offset, buffer_size);
		if(marker == (marker_type & 0xff)) result = 1;
	}

	return !result;
}


typedef struct
{
	int field_size;
	int padded_field_size;
	int next_offset;
	int quant_offset;
	int huffman_offset;
	int image_offset;
	int scan_offset;
	int data_offset;
} qt_hdr_t;

typedef struct
{
	int field_number;
	int field_size;
	int unpadded_field_size;
} avi_hdr_t;

#define LML_MARKER_SIZE 0x2c
#define LML_MARKER_TAG 0xffe3
void insert_lml33_markers(unsigned char **buffer, 
	long *field2_offset, 
	long *buffer_size, 
	long *buffer_allocated)
{
	long marker_offset = -1;
	int marker_exists;

/* Search for existing marker to replace */
//	marker_offset = find_marker(*buffer, *buffer_size, LML_MARKER_TAG);

/* Insert new marker */
	if(marker_offset < 0)
	{
		marker_offset = 2;
		insert_space(buffer, 
			buffer_size, 
			buffer_allocated,
			2,
			LML_MARKER_SIZE);
	}
}

static int qt_table_offsets(unsigned char *buffer, 
	long buffer_size, 
	qt_hdr_t *header)
{
	int done = 0;
	long offset = 0;
	int marker = 0;
	int field = 0;
	int len;
	int result = 0;

	bzero(header, sizeof(qt_hdr_t) * 2);

// Read every marker to get the offsets for the headers
	for(field = 0; field < 2; field++)
	{
		done = 0;

		while(!done)
		{
			marker = next_marker(buffer, 
				&offset, 
				buffer_size);

			len = 0;

			switch(marker)
			{
				case M_SOI:
// The first field may be padded
					if(field > 0) 
					{
						header[0].next_offset = 
							header[0].padded_field_size = 
							offset - 2;
					}
					len = 0;
					break;

				case M_APP1:
// Quicktime marker already exists.  Abort.
					if(buffer[offset + 6] == 'm' &&
						buffer[offset + 7] == 'j' &&
						buffer[offset + 8] == 'p' &&
						buffer[offset + 9] == 'a')
					{
						result = 1;
						done = 1;
					}
					break;

				case M_DQT:
					if(!header[field].quant_offset)
					{
						header[field].quant_offset = offset - 2;
						if(field > 0)
							header[field].quant_offset -= header[0].next_offset;
					}
					len = read_int16(buffer, &offset, buffer_size);
					len -= 2;
					break;

				case M_DHT:
					if(!header[field].huffman_offset)
					{
						header[field].huffman_offset = offset - 2;
						if(field > 0)
							header[field].huffman_offset -= header[0].next_offset;
					}
					len = read_int16(buffer, &offset, buffer_size);
					len -= 2;
					break;

				case M_SOF0:
					if(!header[field].image_offset)
					{
						header[field].image_offset = offset - 2;
						if(field > 0)
							header[field].image_offset -= header[0].next_offset;
					}
					len = read_int16(buffer, &offset, buffer_size);
					len -= 2;
					break;

				case M_SOS:
					header[field].scan_offset = offset - 2;
					if(field > 0)
						header[field].scan_offset -= header[0].next_offset;
					len = read_int16(buffer, &offset, buffer_size);
					len -= 2;
					header[field].data_offset = offset + len;
					if(field > 0)
						header[field].data_offset -= header[0].next_offset;
					break;

//				case 0:
				case M_EOI:
					if(field > 0) 
					{
						header[field].field_size = 
							header[field].padded_field_size = 
							offset - header[0].next_offset;
						header[field].next_offset = 0;
					}
					else
					{
// Often misses second SOI but gets first EOI
//						header[0].next_offset = 
//							header[0].padded_field_size = 
//							offset;
					}
//printf("table_offsets M_EOI %d %x\n", field, offset);
					done = 1;
					len = 0;
					break;

				default:
// Junk appears between fields
					len = 0;
//					len = read_int16(buffer, &offset, buffer_size);
//					len -= 2;
					break;
			}

			if(!done) offset += len;
			if(offset >= buffer_size) done = 1;
		}
//printf("qt_table_offsets 10 %d\n", field);
	}

	return result;
}

static void insert_quicktime_marker(unsigned char *buffer, 
	long buffer_size, 
	long offset, 
	qt_hdr_t *header)
{
	write_int32(buffer, &offset, buffer_size, 0xff000000 | 
			((unsigned long)M_APP1 << 16) | 
			(QUICKTIME_MARKER_SIZE - 2));
	write_int32(buffer, &offset, buffer_size, 0);
	write_int32(buffer, &offset, buffer_size, QUICKTIME_JPEG_TAG);
	write_int32(buffer, &offset, buffer_size, header->field_size);
	write_int32(buffer, &offset, buffer_size, header->padded_field_size);
	write_int32(buffer, &offset, buffer_size, header->next_offset);
	write_int32(buffer, &offset, buffer_size, header->quant_offset);
	write_int32(buffer, &offset, buffer_size, header->huffman_offset);
	write_int32(buffer, &offset, buffer_size, header->image_offset);
	write_int32(buffer, &offset, buffer_size, header->scan_offset);
	write_int32(buffer, &offset, buffer_size, header->data_offset);
}


void mjpeg_insert_quicktime_markers(unsigned char **buffer, 
	long *buffer_size, 
	long *buffer_allocated,
	int fields,
	long *field2_offset)
{
	qt_hdr_t header[2];
	long offset = 0;
	int exists = 0;
	*field2_offset = -1;

	if(fields < 2) return;


// Get offsets for tables in both fields
	exists = qt_table_offsets(*buffer, *buffer_size, header);

// APP1 for quicktime already exists
	if(exists) return;

//printf("mjpeg_insert_quicktime_markers %x %02x %02x\n", 
//	header[0].next_offset, (*buffer)[*field2_offset], (*buffer)[*field2_offset + 1]);
//if(*field2_offset == 0)
//	fwrite(*buffer, *buffer_size, 1, stdout);



	header[0].field_size += QUICKTIME_MARKER_SIZE;
	header[0].padded_field_size += QUICKTIME_MARKER_SIZE;
	header[0].next_offset += QUICKTIME_MARKER_SIZE;
	header[0].quant_offset += QUICKTIME_MARKER_SIZE;
	header[0].huffman_offset += QUICKTIME_MARKER_SIZE;
	header[0].image_offset += QUICKTIME_MARKER_SIZE;
	header[0].scan_offset += QUICKTIME_MARKER_SIZE;
	header[0].data_offset += QUICKTIME_MARKER_SIZE;
	header[1].field_size += QUICKTIME_MARKER_SIZE;
	header[1].padded_field_size += QUICKTIME_MARKER_SIZE;
	header[1].quant_offset += QUICKTIME_MARKER_SIZE;
	header[1].huffman_offset += QUICKTIME_MARKER_SIZE;
	header[1].image_offset += QUICKTIME_MARKER_SIZE;
	header[1].scan_offset += QUICKTIME_MARKER_SIZE;
	header[1].data_offset += QUICKTIME_MARKER_SIZE;
	*field2_offset = header[0].next_offset;



// Insert APP1 marker
	insert_space(buffer, 
		buffer_size, 
		buffer_allocated,
		2,
		QUICKTIME_MARKER_SIZE);

	insert_quicktime_marker(*buffer, 
		*buffer_size, 
		2, 
		&header[0]);

	insert_space(buffer, 
		buffer_size, 
		buffer_allocated,
		header[0].next_offset + 2,
		QUICKTIME_MARKER_SIZE);

	header[1].next_offset = 0;
	insert_quicktime_marker(*buffer, 
		*buffer_size, 
		header[0].next_offset + 2, 
		&header[1]);
}


static int avi_table_offsets(unsigned char *buffer, 
	long buffer_size, 
	avi_hdr_t *header)
{
	int field2 = mjpeg_get_field2(buffer, buffer_size);

	header[0].field_number = 1;
	header[0].field_size = field2;
	header[0].unpadded_field_size = field2;

	header[1].field_number = 2;
	header[1].field_size = buffer_size - field2;
	header[1].unpadded_field_size = buffer_size - field2;
	return 0;
}

static void insert_avi_marker(unsigned char *buffer, 
	long buffer_size, 
	long offset, 
	avi_hdr_t *header)
{
	write_int32(buffer, &offset, buffer_size, 0xff000000 | 
			((unsigned long)M_APP0 << 16) | 
			(AVI_MARKER_SIZE - 2));
	write_int32(buffer, &offset, buffer_size, QUICKTIME_AVI_TAG);

// One version of McRoweSoft only allows field polarity while
// another version allows field size.
	write_char(buffer, &offset, buffer_size, header->field_number);
	write_char(buffer, &offset, buffer_size, 0);
	write_int32(buffer, &offset, buffer_size, header->field_size);
	write_int32(buffer, &offset, buffer_size, header->unpadded_field_size);
}

void mjpeg_insert_avi_markers(unsigned char **buffer, 
	long *buffer_size, 
	long *buffer_allocated,
	int fields,
	long *field2_offset)
{
	avi_hdr_t header[2];
	long offset = 0;
	*field2_offset = -1;


// Test for existing marker
	if(!find_marker(*buffer, &offset, *buffer_size, M_APP0))
	{
		if((*buffer)[offset + 2] == 'A' &&
			(*buffer)[offset + 3] == 'V' &&
			(*buffer)[offset + 4] == 'I' &&
			(*buffer)[offset + 5] == '1')
			return;
	}


	avi_table_offsets(*buffer, *buffer_size, header);

	header[0].field_size += AVI_MARKER_SIZE;
	header[0].unpadded_field_size += AVI_MARKER_SIZE;
	header[1].field_size += AVI_MARKER_SIZE;
	header[1].unpadded_field_size += AVI_MARKER_SIZE;
	*field2_offset = header[0].field_size;

// Insert APP0 marker into field 1
	insert_space(buffer, 
		buffer_size, 
		buffer_allocated,
		2,
		AVI_MARKER_SIZE);
	insert_avi_marker(*buffer, 
		*buffer_size, 
		2, 
		&header[0]);

	insert_space(buffer, 
		buffer_size, 
		buffer_allocated,
		*field2_offset + 2,
		AVI_MARKER_SIZE);
	insert_avi_marker(*buffer, 
		*buffer_size, 
		*field2_offset + 2, 
		&header[1]);



}


static void read_avi_markers(unsigned char *buffer,
	long buffer_size,
	avi_hdr_t *header)
{
	long offset = 0;
	int marker_count = 0;
	int result = 0;
	int marker_size = 0;
	while(marker_count < 2 && offset < buffer_size && !result)
	{
		result = find_marker(buffer, 
			&offset, 
			buffer_size,
			M_APP0);
		marker_size = ((unsigned char)buffer[offset] << 8) | (unsigned char)buffer[offset];

		
		if(!result && marker_size >= 16)
		{
// Marker size, AVI1
			offset += 6;
// field polarity
			header[marker_count].field_number = read_char(buffer, &offset, buffer_size);
			read_char(buffer, &offset, buffer_size);
			header[marker_count].field_size = read_int32(buffer, &offset, buffer_size);
			header[marker_count].unpadded_field_size = read_int32(buffer, &offset, buffer_size);
			marker_count++;
		}
	}
}


static void read_quicktime_markers(unsigned char *buffer, 
	long buffer_size, 
	qt_hdr_t *header)
{
	long offset = 0;
	int marker_count = 0;
	int result = 0;

	while(marker_count < 2 && offset < buffer_size && !result)
	{
		result = find_marker(buffer, 
			&offset, 
			buffer_size,
			M_APP1);

		if(!result)
		{
// Marker size
			read_int16(buffer, &offset, buffer_size);
// Zero
			read_int32(buffer, &offset, buffer_size);
// MJPA
			read_int32(buffer, &offset, buffer_size);
// Information
			header[marker_count].field_size = read_int32(buffer, &offset, buffer_size);
			header[marker_count].padded_field_size = read_int32(buffer, &offset, buffer_size);
			header[marker_count].next_offset = read_int32(buffer, &offset, buffer_size);
			header[marker_count].quant_offset = read_int32(buffer, &offset, buffer_size);
			header[marker_count].huffman_offset = read_int32(buffer, &offset, buffer_size);
			header[marker_count].image_offset = read_int32(buffer, &offset, buffer_size);
			header[marker_count].scan_offset = read_int32(buffer, &offset, buffer_size);
			header[marker_count].data_offset = read_int32(buffer, &offset, buffer_size);
			marker_count++;
		}
	}
//printf("read_quicktime_markers 1 %d\n", marker_count);
}

long mjpeg_get_quicktime_field2(unsigned char *buffer, long buffer_size)
{
	qt_hdr_t header[2];
	bzero(&header, sizeof(qt_hdr_t) * 2);

	read_quicktime_markers(buffer, buffer_size, header);
	return header[0].next_offset;
}

long mjpeg_get_avi_field2(unsigned char *buffer, 
	long buffer_size, 
	int *field_dominance)
{
	avi_hdr_t header[2];
	bzero(&header, sizeof(avi_hdr_t) * 2);
	read_avi_markers(buffer, buffer_size, header);

	*field_dominance = (header[0].field_number == 1) ? 1 : 2;

// One version of McRoweSoft only allows field polarity while
// another version allows field size.
	if(header[0].field_size)
	{
		return header[0].field_size;
	}
	else
	{
		return mjpeg_get_field2(buffer, buffer_size);
	}
	return 0;
}

long mjpeg_get_field2(unsigned char *buffer, long buffer_size)
{
	long result = 0;
	int total_fields = 0;
	long offset = 0;
	long field2_offset = 0;
	int i;

	for(i = 0; i < buffer_size; i++)
	{
		if(buffer[i] == 0xff && buffer[i + 1] == M_SOI)
		{
			total_fields++;
			field2_offset = i;
			if(total_fields == 2) break;
		}
	}
	

	return field2_offset;
}

void mjpeg_video_size(unsigned char *data, long data_size, int *w, int *h)
{
	  long offset = 0;
	  find_marker(data, 
			  &offset, 
			  data_size,
			  M_SOF0);
	  *h = (data[offset + 3] << 8) | (data[offset + 4]);
	  *w = (data[offset + 5] << 8) | (data[offset + 6]);
}






