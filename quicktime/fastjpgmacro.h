#ifndef FASTJPGMACRO_H
#define FASTJPGMACRO_H


#define QUICKTIME_FASTJPG_HANDLE_RST(rst_int, rst_cnt) \
{ \
	if(((rst_int) && (rst_cnt == 0))) \
	{ \
		jpeg_info->jpg_h_bbuf = 0; \
		jpeg_info->jpg_h_bnum = 0; \
		if(jpeg_info->marker == 0) jpeg_info->marker = quicktime_fastjpg_check_for_marker(jpeg_info); \
		if(jpeg_info->marker) \
		{ \
			if(jpeg_info->marker == M_EOI) \
			{ \
				jpeg_info->jpg_saw_EOI = 1; \
				return 1; \
			} \
    		else \
			if(jpeg_info->marker == M_SOS) quicktime_fastjpg_readSOS(jpeg_info); \
    		else \
			if(!((jpeg_info->marker >= M_RST0) && (jpeg_info->marker <= M_RST7))) \
    		{ \
				printf("QUICKTIME_FASTJPG_HANDLE_RST: unexp marker(%x)\n", jpeg_info->marker); \
				/*return(0);*/ \
			} \
    		jpeg_info->marker = 0; \
  		} \
		jpeg_info->jpg_comps[0].dc = jpeg_info->jpg_comps[1].dc = jpeg_info->jpg_comps[2].dc = 0; \
		rst_cnt = rst_int; \
	} \
	else \
		rst_cnt--; \
}

#define QUICKTIME_FASTJPG_TEST_MARKER \
while(jpeg_info->marker) \
{ \
	if(jpeg_info->marker == M_EOI) \
	{ \
		jpeg_info->jpg_saw_EOI = 1; \
		jpeg_info->marker = 0; \
	} \
	else \
	if(jpeg_info->marker == M_SOS) \
	{ \
		quicktime_fastjpg_readSOS(jpeg_info); \
		jpeg_info->marker = 0; \
	} \
	else \
	if((jpeg_info->marker >= M_RST0) && (jpeg_info->marker <= M_RST7)) \
	{ \
  		jpeg_info->jpg_comps[0].dc = jpeg_info->jpg_comps[1].dc = jpeg_info->jpg_comps[2].dc = 0; \
		rst_skip = rst_count; \
		rst_count = jpeg_info->jpg_rst_interval; \
		jpeg_info->marker = 0; \
		jpeg_info->jpg_h_bbuf = 0; \
		jpeg_info->jpg_h_bnum = 0; \
	} \
	else /* Unknown or unexpected Marker */ \
	{ \
		printf("QUICKTIME_FASTJPG_TEST_MARKER: unexp marker(%x)\n", jpeg_info->marker); \
		jpeg_info->marker = quicktime_fastjpg_skip_to_next_rst(jpeg_info); /* hopefully a RST marker */ \
	} \
}

#define QUICKTIME_FASTJPG_HBBUF_FILL8_1(hbbuf, hbnum) \
{ \
	(hbbuf) <<= 8; \
	(hbnum) += 8; \
\
	if(jpeg_info->marker) tmp__ = 0x00; \
  	else \
	{ \
		tmp__ = *(jpeg_info->chunk++); \
		jpeg_info->chunk_size--; \
	} \
\
  	while(tmp__ == 0xff) \
  	{ \
		t1_ = *(jpeg_info->chunk++); \
		jpeg_info->chunk_size--; \
    	if(t1_ == 0x00) break; \
    	else if(t1_ == 0xff) continue; \
    	else \
		{ \
			jpeg_info->marker = t1_; \
			tmp__ = 0x00; \
			break; \
		} \
	} \
	hbbuf |= tmp__; \
}

#define QUICKTIME_FASTJPG_HUFF_DECODE(huff_hdr, htbl, hbnum, hbbuf, result)	\
{ \
	while(hbnum < 16) { QUICKTIME_FASTJPG_HBBUF_FILL8_1(hbbuf, hbnum); } \
	tmp_ = (hbbuf >> (hbnum - 8)) & 0xff; \
  	hcode_ = (htbl)[tmp_]; \
  	if(hcode_) \
	{ \
		hbnum -= (hcode_ >> 8); \
		(result) = hcode_ & 0xff; \
	} \
  	else \
  	{ \
		minbits_ = 9; \
    	tmp_ = (hbbuf >> (hbnum - 16)) & 0xffff; /* get 16 bits */	\
    	shift_ = 16 - minbits_;	\
		hcode_ = tmp_ >> shift_; \
\
    	while(hcode_ > huff_hdr->maxcode[minbits_]) \
		{ \
			minbits_++; \
			shift_--; \
			hcode_ = tmp_ >> shift_; \
		} \
\
    	if(minbits_ > 16) \
		{ \
			printf("QUICKTIME_FASTJPG_HUFF_DECODE error\n"); \
			return 1; \
		} \
   		else  \
    	{ \
			hbnum -= minbits_; \
			hcode_ -= huff_hdr->mincode[minbits_]; \
      		result = huff_hdr->vals[(huff_hdr->valptr[minbits_] + hcode_)]; \
		} \
  	} \
}


#define QUICKTIME_FASTJPG_HUFF_MASK(s) ((1 << (s)) - 1)

#define QUICKTIME_FASTJPG_GET_BITS(n, hbnum, hbbuf, result) \
{ \
	hbnum -= n; \
  	while(hbnum < 0) \
	{ \
		QUICKTIME_FASTJPG_HBBUF_FILL8_1(hbbuf, hbnum); \
	} \
  	(result) = ((hbbuf >> hbnum) & QUICKTIME_FASTJPG_HUFF_MASK(n)); \
}

#define QUICKTIME_FASTJPG_HUFF_EXTEND(val, sz) \
	((val) < (1 << ((sz) - 1)) ? (val) + (((-1) << (sz)) + 1) : (val))

#define QUICKTIME_MCU_ARGS \
	quicktime_jpeg_t *jpeg_info,  \
	unsigned char **row_pointers, \
	long frame_width, \
	long frame_height, \
	unsigned long mcu_row_size, \
	unsigned long ip_size, \
	quicktime_mjpa_buffs *yuvbufs, \
	int interlaced
	

#define QUICKTIME_MCU_VARS \
	unsigned long yi; \
	unsigned char *yptr, *uptr, *vptr; \
	long *YTab = jpeg_info->yuvtabs.YUV_Y_tab; \
	long *UBTab = jpeg_info->yuvtabs.YUV_UB_tab; \
	long *VRTab = jpeg_info->yuvtabs.YUV_VR_tab; \
	long *UGTab = jpeg_info->yuvtabs.YUV_UG_tab; \
	long *VGTab = jpeg_info->yuvtabs.YUV_VG_tab; \
	unsigned char *ybuf = yuvbufs->ybuf; \
	unsigned char *ubuf = yuvbufs->ubuf; \
	unsigned char *vbuf = yuvbufs->vbuf;

#define QUICKTIME_MCU111111_MID_VARS \
	unsigned char *ip; \
	unsigned char *yp, *up, *vp; \
	long xi, skip;

#define QUICKTIME_MCU111111_MID_DECL \
	ip = *row_pointers; \
	yp = yptr; \
	up = uptr; \
	vp = vptr; \
	xi = frame_width; \
	skip = 0;

#define QUICKTIME_MCU_INNER_VARS \
	unsigned long u0; \
	unsigned long v0; \
	long cr; \
	long cb; \
	long cg; \
	long y_long;


#define QUICKTIME_MCU_INNER_INIT \
	u0 = (unsigned long)*up++; \
	v0 = (unsigned long)*vp++; \
	cr = VRTab[v0]; \
	cb = UBTab[u0]; \
	cg = UGTab[u0] + VGTab[v0];

#define QUICKTIME_MCU4H_INNER_TAIL(inc1, inc2) \
	skip++; \
	if(skip >= 8) \
	{ \
		skip = 0; \
		yp += inc2; \
		up += inc1; \
		vp += inc1; \
	} \
	else \
	if(!(skip & 1)) /* 2 4 6 */	\
		yp += inc1;

#define QUICKTIME_MCU_LIMITRANGE(x) \
	(((x) < 0) ? 0 : (((x) > 255) ? 255 : (x)));

#define QUICKTIME_MCU_YUV_TO_RGB(y, cr, cg, cb, ip) \
	y_long = (long)y; \
	*ip++ = (unsigned char)QUICKTIME_MCU_LIMITRANGE((y_long + cr) >> 6); \
	*ip++ = (unsigned char)QUICKTIME_MCU_LIMITRANGE((y_long + cg) >> 6); \
	*ip++ = (unsigned char)QUICKTIME_MCU_LIMITRANGE((y_long + cb) >> 6);


#define QUICKTIME_MCU221111_MID_VARS \
	unsigned char *ip0, *ip1; \
	unsigned char *yp, *up, *vp; \
	long xi, skip;


#define QUICKTIME_MCU221111_MID_DECL \
	if(frame_height <= 0) return 0; \
	if(yi == 4) yptr += 64; \
	ip0 = *row_pointers; \
	row_pointers += interlaced ? 2 : 1; \
	ip1 = *row_pointers; \
	row_pointers += interlaced ? 2 : 1; \
	yp = yptr; \
	up = uptr; \
	vp = vptr; \
	xi = frame_width; \
	skip = 0;

#define QUICKTIME_MCU2H_INNER_TAIL(inc1, inc2) \
	skip++; \
	if(skip == 4) yp += inc1; \
	else \
	if(skip >= 8) \
	{ \
		skip = 0; \
		yp += inc2; \
		up += inc1; \
		vp += inc1; \
	}

#define QUICKTIME_MCU1H_INNER_TAIL(inc) \
	skip++; \
	if(skip >= 8) \
	{ \
		skip = 0; \
		yp += inc; \
		up += inc; \
		vp += inc; \
	}


#endif
