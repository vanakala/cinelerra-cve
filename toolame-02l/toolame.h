#include <pthread.h>



void global_init (void);
void proginfo (void);
void short_usage (void);

void obtain_parameters (frame_info *, int *, unsigned long *,
			       char[MAX_NAME_SIZE], char[MAX_NAME_SIZE]);
void parse_args (int, char **, frame_info *, int *, unsigned long *,
			char[MAX_NAME_SIZE], char[MAX_NAME_SIZE]);
void print_config (frame_info *, int *,
			  char[MAX_NAME_SIZE], char[MAX_NAME_SIZE]);
void usage (void);



void smr_dump(double smr[2][SBLIMIT], int nch);





// Input buffer management private structures.
// This is not reentrant but neither is toolame.

extern pthread_mutex_t toolame_input_lock;
extern pthread_mutex_t toolame_output_lock;
extern pthread_mutex_t toolame_copy_lock;
extern char *toolame_buffer;
extern int toolame_buffer_bytes;
extern int toolame_error;
extern int toolame_eof;
// Bigger than the biggest fragment
#define TOOLAME_BUFFER_BYTES 0x200000
// replaces fread
int toolame_buffer_read(char *dst, int size, int n);
