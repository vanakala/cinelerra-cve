/* bitstream stuff */
#define BUFFER_SIZE   4096
#define TRUE 1
#define FALSE 0

typedef long long bitcount_t;

struct _bitstream {
  unsigned char *bfr;
  unsigned char outbyte;
  int byteidx;
  int bitidx;
  int bufcount;
  fpos_t actpos;
  bitcount_t totbits;
  FILE *bitfile;
  int eobs;
  int fileOutError;
};

typedef struct _bitstream bitstream;
typedef struct _bitstream Bit_stream_struc;

/* bits */
int init_putbits(bitstream *bs, char *bs_filename);
void finish_putbits(bitstream *bs);
int init_getbits(bitstream *bs, char *bs_filename);
void finish_getbits(bitstream *bs);
unsigned int get1bit(bitstream *bs);
unsigned int getbits(bitstream *bs, int N);
void putbits(bitstream *bs, int val, int n);
void put1bit(bitstream *bs, int val);
void alignbits(bitstream *bs);
void prepareundo(bitstream *bs, bitstream *undo);
void undochanges(bitstream *bs, bitstream *old);
bitcount_t bitcount(bitstream *bs);
int end_bs(bitstream *bs);
int seek_sync(bitstream *bs, unsigned int sync, int N);
