




void psycho_1_read_cbound (int lay, int freq);
void psycho_1_read_freq_band (g_ptr *, int, int);
void psycho_1_init_add_db (void);
INLINE double add_db (double a, double b);
void psycho_1_make_map (mask[HAN_SIZE], g_thres *);

void psycho_1_hann_fft_pickmax (double sample[FFT_SIZE], mask power[HAN_SIZE], double spike[SBLIMIT], FLOAT energy[FFT_SIZE]);
void psycho_1_tonal_label (mask power[HAN_SIZE], int *tone);
void psycho_1_noise_label (mask *power, int *noise, g_thres *, FLOAT[FFT_SIZE]);
void psycho_1_subsampling (mask[HAN_SIZE], g_thres *, int *, int *);
void psycho_1_threshold (mask power[HAN_SIZE], g_thres *, int *, int *, int);
void psycho_1_minimum_mask (g_thres *, double[SBLIMIT], int);
void psycho_1_smr (double[SBLIMIT], double[SBLIMIT], double[SBLIMIT], int);



void psycho_1_dump(mask power[HAN_SIZE], int *tone, int *noise);
