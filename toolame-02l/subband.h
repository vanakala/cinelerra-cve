

void  WindowFilterSubband( short *pBuffer, int ch, double s[SBLIMIT] );
void create_dct_matrix (double filter[16][32]);

#ifdef REFERENCECODE
void window_subband (short **buffer, double z[64], int k);
void filter_subband (double z[HAN_SIZE], double s[SBLIMIT]);
#endif
