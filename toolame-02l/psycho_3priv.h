void psycho_3_fft(FLOAT *sample, FLOAT *energy);
void psycho_3_powerdensityspectrum(FLOAT *energy, FLOAT *power);

void psycho_3_tonal_label (FLOAT *power, int *tonelabel, FLOAT *Xtm);
void psycho_3_tonal_label_range(FLOAT *power, int *type, int *maxima, FLOAT *Xtm, int start, int end, int srange) ;


void psycho_3_init_add_db (void);
INLINE double psycho_3_add_db (double a, double b);

void psycho_3_noise_label (FLOAT *power, FLOAT *energy, int *tonelabel, int *noiselabel, FLOAT *Xnm);
void psycho_3_decimation(FLOAT *ath, int *tonelabel, FLOAT *Xtm, int *noiselabel, FLOAT *Xnm, FLOAT *bark);

void psycho_3_threshold(FLOAT *LTg, int *tonelabel, FLOAT *Xtm, int *noiselabel, FLOAT *Xnm, FLOAT *bark, FLOAT *ath, int bit_rate, int *freq_subset);

void psycho_3_minimummasking(FLOAT *LTg, double *LTmin, int *freq_subset);

void psycho_3_spl(double *Lsb, FLOAT *power, double *scale);

void psycho_3_smr(double *LTmin, double *Lsb);

#ifdef OLDTHRESH
void psycho_3_threshold_old(FLOAT *LTg, int *tonelabel, FLOAT *Xtm, int *noiselabel, FLOAT *Xnm, FLOAT *bark, FLOAT *ath, int bit_rate);
void psycho_3_minimummasking_old(FLOAT *LTg, double *LTmin);
#endif

void psycho_3_dump(int *tonelabel, FLOAT *Xtm, int *noiselabel, FLOAT *Xnm);
void psycho_3_threshold_new(FLOAT *LTg, int *tonelabel, FLOAT *Xtm, int *noiselabel, FLOAT *Xnm, FLOAT *bark, FLOAT *ath, int bit_rate, int *freq_subset);
