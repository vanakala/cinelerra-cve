#include "graphics.h"

#include <string.h>

/* Graphics acceleration routines */

void quicktime_init_yuv(quicktime_yuv_t *yuv_table)
{
	int i;
	for(i = 0; i < 256; i++)
	{
/* compression */
		yuv_table->rtoy_tab[i] = (long)( 0.2990 * 65536 * i);
		yuv_table->rtou_tab[i] = (long)(-0.1687 * 65536 * i);
		yuv_table->rtov_tab[i] = (long)( 0.5000 * 65536 * i);

		yuv_table->gtoy_tab[i] = (long)( 0.5870 * 65536 * i);
		yuv_table->gtou_tab[i] = (long)(-0.3320 * 65536 * i);
		yuv_table->gtov_tab[i] = (long)(-0.4187 * 65536 * i);

		yuv_table->btoy_tab[i] = (long)( 0.1140 * 65536 * i);
		yuv_table->btou_tab[i] = (long)( 0.5000 * 65536 * i);
		yuv_table->btov_tab[i] = (long)(-0.0813 * 65536 * i);
	}

	yuv_table->vtor = &(yuv_table->vtor_tab[128]);
	yuv_table->vtog = &(yuv_table->vtog_tab[128]);
	yuv_table->utog = &(yuv_table->utog_tab[128]);
	yuv_table->utob = &(yuv_table->utob_tab[128]);
	for(i = -128; i < 128; i++)
	{
/* decompression */
		yuv_table->vtor[i] = (long)( 1.4020 * 65536 * i);
		yuv_table->vtog[i] = (long)(-0.7141 * 65536 * i);

		yuv_table->utog[i] = (long)(-0.3441 * 65536 * i);
		yuv_table->utob[i] = (long)( 1.7720 * 65536 * i);
	}
}

void quicktime_delete_yuv(quicktime_yuv_t *yuv_table)
{
}


quicktime_scaletable_t* quicktime_new_scaletable(int input_w, int input_h, int output_w, int output_h)
{
	quicktime_scaletable_t *result = (quicktime_scaletable_t*)malloc(sizeof(quicktime_scaletable_t));
	float i;
	float scalex = (float)input_w / output_w, scaley = (float)input_h / output_h;

	result->input_x = (int*)malloc(sizeof(int) * output_w);
	result->input_y = (int*)malloc(sizeof(int) * output_h);

	for(i = 0; i < output_w; i++)
	{
		result->input_x[(int)i] = (int)(scalex * i);
	}

	for(i = 0; i < output_h; i++)
	{
		result->input_y[(int)i] = (int)(scaley * i);
	}

	result->in_w = input_w;
	result->in_h = input_h;
	result->out_w = output_w;
	result->out_h = output_h;
	return result;
}

void quicktime_delete_scaletable(quicktime_scaletable_t *scaletable)
{
	free(scaletable->input_x);
	free(scaletable->input_y);
	free(scaletable);
}

/* Return 1 if dimensions are different from scaletable */
int quicktime_compare_scaletable(quicktime_scaletable_t *scaletable, 
	int in_w, 
	int in_h, 
	int out_w, 
	int out_h)
{
	if(scaletable->in_w != in_w ||
		scaletable->in_h != in_h ||
		scaletable->out_w != out_w ||
		scaletable->out_h != out_h)
		return 1;
	else
		return 0;
}

/* Return 1 if the scaletable is 1:1 */
int quicktime_identity_scaletable(quicktime_scaletable_t *scaletable)
{
	if(scaletable->in_w == scaletable->out_w &&
		scaletable->in_h == scaletable->out_h)
		return 1;
	else
		return 0;
}
