
//void fft (FLOAT[BLKSIZE], FLOAT[BLKSIZE], FLOAT[BLKSIZE], FLOAT[BLKSIZE], int);

void psycho_2_fft (FLOAT * x_real, FLOAT * energy, FLOAT * phi);
void psycho_1_fft (FLOAT * x_real, FLOAT * energy, int N);


void atan_table_init(void);
INLINE FLOAT atan_table(FLOAT y, FLOAT x);
