/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. This software is an   *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// mp4_header.c //

#include <stdlib.h>
#include <math.h>

#include "mp4_vars.h"

#include "getbits.h"
#include "debug.h"
#include "mp4_header.h"

/**
 *
**/

void next_start_code();

/***/

int getvolhdr()
{
	if (showbits(27) == VO_START_CODE)
	{
		getbits(27); // start_code
		getbits(5); // vo_id

		if (getbits(28) != VOL_START_CODE)
		{
			exit(101);
		}
		mp4_state->hdr.ident = getbits(4); // vol_id
		mp4_state->hdr.random_accessible_vol = getbits(1);
		mp4_state->hdr.type_indication = getbits(8); 
		mp4_state->hdr.is_object_layer_identifier = getbits(1);

		if (mp4_state->hdr.is_object_layer_identifier) {
			mp4_state->hdr.visual_object_layer_verid = getbits(4);
			mp4_state->hdr.visual_object_layer_priority = getbits(3);
		} 
		else {
			mp4_state->hdr.visual_object_layer_verid = 1;
			mp4_state->hdr.visual_object_layer_priority = 1;
		}
		mp4_state->hdr.aspect_ratio_info = getbits(4);
		mp4_state->hdr.vol_control_parameters = getbits(1);
		if (mp4_state->hdr.vol_control_parameters) {
			mp4_state->hdr.chroma_format = getbits(2);
			mp4_state->hdr.low_delay = getbits(1);
			mp4_state->hdr.vbv_parameters = getbits(1);
			if (mp4_state->hdr.vbv_parameters) {
				mp4_state->hdr.first_half_bit_rate = getbits(15);
				getbits1(); // marker
				mp4_state->hdr.latter_half_bit_rate = getbits(15);
				getbits1(); // marker
				mp4_state->hdr.first_half_vbv_buffer_size = getbits(15);
				getbits1(); // marker
				mp4_state->hdr.latter_half_vbv_buffer_size = getbits(3);
				mp4_state->hdr.first_half_vbv_occupancy = getbits(11);
				getbits1(); // marker
				mp4_state->hdr.latter_half_vbv_occupancy = getbits(15);
				getbits1(); // marker
			}
		}
		mp4_state->hdr.shape = getbits(2);
		getbits1(); // marker
		mp4_state->hdr.time_increment_resolution = getbits(16);
		getbits1(); // marker
		mp4_state->hdr.fixed_vop_rate = getbits(1);

		if (mp4_state->hdr.fixed_vop_rate) {
			int bits = (int) ceil(log((double)mp4_state->hdr.time_increment_resolution)/log(2.0));
			if (bits < 1) 
				bits = 1;
			mp4_state->hdr.fixed_vop_time_increment = getbits(bits);
		}
		
		if (mp4_state->hdr.shape != BINARY_SHAPE_ONLY)  
		{
			if(mp4_state->hdr.shape == 0)
			{
				getbits1(); // marker
				mp4_state->hdr.width = getbits(13);
				getbits1(); // marker
				mp4_state->hdr.height = getbits(13);
				getbits1(); // marker
			}

			mp4_state->hdr.interlaced = getbits(1);
			mp4_state->hdr.obmc_disable = getbits(1);
			
			if (mp4_state->hdr.visual_object_layer_verid == 1) {
				mp4_state->hdr.sprite_usage = getbits(1);
			} 
			else {
				mp4_state->hdr.sprite_usage = getbits(2);
			}
			
			mp4_state->hdr.not_8_bit = getbits(1);
			if (mp4_state->hdr.not_8_bit) 
			{
				mp4_state->hdr.quant_precision = getbits(4);
				mp4_state->hdr.bits_per_pixel = getbits(4);
			}
			else 
			{
				mp4_state->hdr.quant_precision = 5;
				mp4_state->hdr.bits_per_pixel = 8;
			}

			if (mp4_state->hdr.shape == GRAY_SCALE) {
				exit(102);
			}

			mp4_state->hdr.quant_type = getbits(1); // quant type

			if (mp4_state->hdr.quant_type) 
			{
				mp4_state->hdr.load_intra_quant_matrix = getbits(1);
				if (mp4_state->hdr.load_intra_quant_matrix) {
					// load intra quant matrix
					unsigned int val;
					int i, k = 0;
					do {
						k++;
						val = getbits(8);
						mp4_tables->intra_quant_matrix[mp4_tables->zig_zag_scan[k]] = val;
					} while ((k < 64) && (val != 0));
					for (i = k; i < 64; i++) {
						mp4_tables->intra_quant_matrix[mp4_tables->zig_zag_scan[i]] =
							mp4_tables->intra_quant_matrix[mp4_tables->zig_zag_scan[k-1]];
					}
				}
				mp4_state->hdr.load_nonintra_quant_matrix = getbits(1);
				if (mp4_state->hdr.load_nonintra_quant_matrix) {
					// load nonintra quant matrix
					unsigned int val;
					int i, k = 0;
					do {
						k++;
						val = getbits(8);
						mp4_tables->nonintra_quant_matrix[mp4_tables->zig_zag_scan[k]] = val;
					} while ((k < 64) && (val != 0));
					for (i = k; i < 64; i++) {
						mp4_tables->nonintra_quant_matrix[mp4_tables->zig_zag_scan[i]] =
							mp4_tables->nonintra_quant_matrix[mp4_tables->zig_zag_scan[k-1]];
					}
				}
			}

			if (mp4_state->hdr.visual_object_layer_verid/*ident*/ != 1) {
				mp4_state->hdr.quarter_pixel = getbits(1);
			} else {
				mp4_state->hdr.quarter_pixel = 0;
			}

			mp4_state->hdr.complexity_estimation_disable = getbits(1);
			mp4_state->hdr.error_res_disable = getbits(1);
			mp4_state->hdr.data_partitioning = getbits(1);
			if (mp4_state->hdr.data_partitioning) {
				exit(102);
			}	  
			else {
				mp4_state->hdr.error_res_disable = 1;
			}
			
			mp4_state->hdr.intra_acdc_pred_disable = 0;
			mp4_state->hdr.scalability = getbits(1);

			if (mp4_state->hdr.scalability)	{
				exit(103);
			}
			
			// next_start_code();

			if (showbits(32) == USER_DATA_START_CODE) {
				exit(104);
			}
    } 

		return 1;
  }
  
  return 0; // no VO start code
}

/***/

int getgophdr()
{
	if (nextbits(32) == GOP_START_CODE) // [Ag][Review] possible bug, it's not possible to read 32 bits
	{
		getbits(32); 

		mp4_state->hdr.time_code = getbits(18);
		mp4_state->hdr.closed_gov = getbits(1);
		mp4_state->hdr.broken_link = getbits(1);
	}

	return 1;
}

/***/

int getvophdr()
{
	next_start_code();
	if(getbits(32) != (int) VOP_START_CODE)
  {
		_Print("Vop start_code NOT found\n");
		return 0;
  }

	mp4_state->hdr.prediction_type = getbits(2);

	while (getbits(1) == 1) // temporal time base
  {
		mp4_state->hdr.time_base++;
  }
	getbits1(); // marker bit
	{
		int bits = (int) ceil(log(mp4_state->hdr.time_increment_resolution)/log(2.0));
		if (bits < 1) bits = 1;
		
		mp4_state->hdr.time_inc = getbits(bits); // vop_time_increment (1-16 bits)
	}

	getbits1(); // marker bit
	mp4_state->hdr.vop_coded = getbits(1);
	if (mp4_state->hdr.vop_coded == 0) 
	{
		next_start_code();
		return 1;
	}  

	if ((mp4_state->hdr.shape != BINARY_SHAPE_ONLY) &&
		(mp4_state->hdr.prediction_type == P_VOP)) 
	{
		mp4_state->hdr.rounding_type = getbits(1);
	} else {
		mp4_state->hdr.rounding_type = 0;
	}
	
	if (mp4_state->hdr.shape != RECTANGULAR)
	{
		if (! (mp4_state->hdr.sprite_usage == STATIC_SPRITE && 
			mp4_state->hdr.prediction_type==I_VOP) )
		{
			mp4_state->hdr.width = getbits(13);
			getbits1();
			mp4_state->hdr.height = getbits(13);
			getbits1();
			mp4_state->hdr.hor_spat_ref = getbits(13);
			getbits1();
			mp4_state->hdr.ver_spat_ref = getbits(13);
			getbits1(); // corr
		}
		
		mp4_state->hdr.change_CR_disable = getbits(1);
		
		mp4_state->hdr.constant_alpha = getbits(1);
		if (mp4_state->hdr.constant_alpha) {
			mp4_state->hdr.constant_alpha_value = getbits(8);
		}
  }

	if (! (mp4_state->hdr.complexity_estimation_disable)) {
		exit(108);
	}

	if (mp4_state->hdr.shape != BINARY_SHAPE_ONLY)  
  { 
		mp4_state->hdr.intra_dc_vlc_thr = getbits(3);
		if (mp4_state->hdr.interlaced) {
			exit(109);
		}
  }
	
	if (mp4_state->hdr.shape != BINARY_SHAPE_ONLY) 
  { 
		mp4_state->hdr.quantizer = getbits(mp4_state->hdr.quant_precision); // vop quant

		if (mp4_state->hdr.prediction_type != I_VOP) 
		{
			mp4_state->hdr.fcode_for = getbits(3); 
		}
		
		if (! mp4_state->hdr.scalability) {
			if (mp4_state->hdr.shape && mp4_state->hdr.prediction_type!=I_VOP)
				mp4_state->hdr.shape_coding_type = getbits(1); // vop shape coding type

			/* motion_shape_texture() */
		}
	} 
	
	return 1;
}














/***/

void __inline next_start_code()
{
	if (mp4_state->juice_flag)
	{
//		juice_flag = 0; // [Review][Ag] before juice needed this changed only first time
		if (! bytealigned(0))
		{
			getbits(1);

			// bytealign
			while (! bytealigned(0)) {
				flushbits(1);
			}
		}
	}
	else
	{
		getbits(1);

		// bytealign
		while (! bytealigned(0)) {
			flushbits(1);
		}
	}
}



















