
void create_ana_filter (double[SBLIMIT][64]);
void encode_info (frame_info *, Bit_stream_struc *);
void combine_LR (double[2][3][SCALE_BLOCK][SBLIMIT],
			double[3][SCALE_BLOCK][SBLIMIT], int);
void scale_factor_calc (double[][3][SCALE_BLOCK][SBLIMIT],
			       unsigned int[][3][SBLIMIT], int, int);
void pick_scale (unsigned int[2][3][SBLIMIT], frame_info *,
			double[2][SBLIMIT]);
void put_scale (unsigned int[2][3][SBLIMIT], frame_info *,
		       double[2][SBLIMIT]);
void transmission_pattern (unsigned int[2][3][SBLIMIT],
				  unsigned int[2][SBLIMIT], frame_info *);
void encode_scale (unsigned int[2][SBLIMIT],
			  unsigned int[2][SBLIMIT],
			  unsigned int[2][3][SBLIMIT], frame_info *,
			  Bit_stream_struc *);
int bits_for_nonoise (double[2][SBLIMIT], unsigned int[2][SBLIMIT],
			     frame_info *);
void main_bit_allocation (double[2][SBLIMIT],
				 unsigned int[2][SBLIMIT],
				 unsigned int[2][SBLIMIT], int *,
				 frame_info *, options *);

int a_bit_allocation (double[2][SBLIMIT], unsigned int[2][SBLIMIT],
			     unsigned int[2][SBLIMIT], int *, frame_info *);
void subband_quantization (unsigned int[2][3][SBLIMIT],
				  double[2][3][SCALE_BLOCK][SBLIMIT],
				  unsigned int[3][SBLIMIT],
				  double[3][SCALE_BLOCK][SBLIMIT],
				  unsigned int[2][SBLIMIT],
				  unsigned int[2][3][SCALE_BLOCK][SBLIMIT],
				  frame_info *);
void encode_bit_alloc (unsigned int[2][SBLIMIT], frame_info *,
			      Bit_stream_struc *);
void sample_encoding (unsigned int[2][3][SCALE_BLOCK][SBLIMIT],
			     unsigned int[2][SBLIMIT], frame_info *,
			     Bit_stream_struc *);
void encode_CRC (unsigned int, Bit_stream_struc *);

void maxmnr (double mnr[2][SBLIMIT], char used[2][SBLIMIT], int sblimit, int stereo, int *min_sb, int *min_ch);

int VBR_bits_for_nonoise (double perm_smr[2][SBLIMIT], unsigned int scfsi[2][SBLIMIT], frame_info * frame, int vbrlevel);
void VBR_maxmnr (double mnr[2][SBLIMIT], char used[2][SBLIMIT], int sblimit, int stereo, int *min_sb, int *min_ch, options * glopts);
int VBR_bit_allocation (double perm_smr[2][SBLIMIT],
			unsigned int scfsi[2][SBLIMIT], unsigned int bit_alloc[2][SBLIMIT], int *adb, frame_info * frame, options * glopts);

