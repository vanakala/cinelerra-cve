/**************************************************************************
 *                                                                        *
 * This code has been developed by Eugene Kuznetsov. This software is an  *
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
 * Eugene Kuznetsov
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
#ifndef _DECORE_TIMER_H
#define _DECORE_TIMER_H

#if (defined(LINUX) && defined(PROFILING))

#include "portab.h"
struct timer
{	
    int64_t dct_times;
    int64_t vld_times;
    int64_t iquant_times;
    int64_t edge_times;
    int64_t display_times;
    int64_t recon_times;
    int64_t transfer_times;
    int64_t block_times;
    int64_t current;
};
extern struct timer tim;
#ifdef __cplusplus
extern "C" {
#endif
extern void start_timer();
extern void stop_dct_timer();
extern void stop_vld_timer();
extern void stop_iquant_timer();
extern void stop_edge_timer();
extern void stop_display_timer();
extern void stop_recon_timer();
extern void stop_transfer_timer();
extern void stop_block_timer();
extern void clear_timer();
extern void write_timer();
#ifdef __cplusplus
};
#endif

#else
static __inline void start_timer(){}
static __inline void stop_dct_timer(){}
static __inline void stop_vld_timer(){}
static __inline void stop_iquant_timer(){}
static __inline void stop_edge_timer(){}
static __inline void stop_display_timer(){}
static __inline void stop_recon_timer(){}
static __inline void stop_block_timer(){}
static __inline void clear_timer(){}
static __inline void write_timer(){}
#endif /* LINUX */

#endif /* _DECORE_TIMER_H */
