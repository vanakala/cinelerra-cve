void psycho_4 (short int *, short int[1056], int, double *smr, double sfeq, options *glopts);
void psycho_4_init (double sfreq, options *glopts);
FLOAT8 psycho_4_spreading_function(FLOAT8 bark);
void psycho_4_allocmem(void);

void psycho_4_trigtable_init(void);
INLINE FLOAT psycho_4_cos(FLOAT phi);
INLINE FLOAT psycho_4_sin(FLOAT phi);
