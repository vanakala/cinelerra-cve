/* AIFF Definitions */

#define IFF_ID_FORM 0x464f524d	/* "FORM" */
#define IFF_ID_AIFF 0x41494646	/* "AIFF" */
#define IFF_ID_COMM 0x434f4d4d	/* "COMM" */
#define IFF_ID_SSND 0x53534e44	/* "SSND" */
#define IFF_ID_MPEG 0x4d504547	/* "MPEG" */

#define AIFF_FORM_HEADER_SIZE 12
#define AIFF_SSND_HEADER_SIZE 16


typedef struct blockAlign_struct
{
  unsigned long offset;
  unsigned long blockSize;
}
blockAlign;

typedef struct IFF_AIFF_struct
{
  short numChannels;
  unsigned long numSampleFrames;
  short sampleSize;
  double sampleRate;
  unsigned long sampleType;
  blockAlign blkAlgn;
}
IFF_AIFF;

void parse_input_file (FILE *musicin, char *, frame_header *header, unsigned long *num_samples);
void aiff_check (char *file_name, IFF_AIFF * pcm_aiff_data, int *version);

int aiff_read_headers (FILE *, IFF_AIFF *);
int aiff_seek_to_sound_data (FILE *);
enum byte_order DetermineByteOrder (void);
void SwapBytesInWords (short *loc, int words);
 unsigned long read_samples (FILE *, short[2304], unsigned long,
				   unsigned long);
 unsigned long get_audio (FILE *, short[2][1152], unsigned long,
				int, frame_header *header);
